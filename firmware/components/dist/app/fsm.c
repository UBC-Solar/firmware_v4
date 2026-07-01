#include "fsm.h"
#include "faulting_runtime.h"
#include "gpio_driver.h"
#include "led_runtime.h"
#include "adc_driver.h"
#include "can_driver.h"
#include "stm32f1xx_hal.h"
#include "debug_io.h"
#include <stdbool.h>

/*============================================================================*/
/* PRIVATE TYPE DEFS */

typedef struct {
    uint32_t blink_tick;
    uint32_t state_tick;
} dist_ticks_t;

/*============================================================================*/
/* PRIVATE VARIABLES */

static volatile FsmState_t FSM_state;
static dist_ticks_t ticks;
static uint8_t led_driver_ready;

/*============================================================================*/
/* PRIVATE FUNCTION PROTOTYPES */

static bool timer_check(uint32_t interval, uint32_t *last_tick);
static void send_heartbeat_if_due(void);
static void send_currents_if_due(void);
static bool check_critical_faults(FaultSource_t faults);
static bool check_efuse_faults(FaultSource_t faults);
static void print_currents(void);

static void state_startup(void);
static void state_activate_ctrl(void);
static void state_normal(void);
static void state_fault(void);

/*============================================================================*/
/* STATE TABLE */

static void (*FSM_state_table[])(void) = {
    state_startup,
    state_activate_ctrl,
    state_normal,
    state_fault,
};

/*============================================================================*/
/* PUBLIC FUNCTION IMPLEMENTATIONS */

void FSM_Init(uint8_t led_driver_available)
{
    led_driver_ready = led_driver_available;
    ticks.blink_tick = HAL_GetTick();
    ticks.state_tick = HAL_GetTick();
    FSM_state        = FSM_STATE_STARTUP;
}

void FSM_Run(void)
{
    static FsmState_t prev_state = FSM_STATE_STARTUP;

    if (FSM_state != prev_state)
    {
        static const char *state_names[] = {
            "STARTUP",
            "ACTIVATE_CTRL",
            "NORMAL",
            "FAULT",
        };
        ticks.state_tick = HAL_GetTick();
        DEBUG_IO_PRINT("FSM -> %s\r\n", state_names[FSM_state]);
        prev_state = FSM_state;
    }

    FSM_state_table[FSM_state]();

    send_heartbeat_if_due();
    send_currents_if_due();
}

/*============================================================================*/
/* STATE FUNCTION IMPLEMENTATIONS */

/**
 * @brief Startup state. Waits for CAN authorisation before enabling outputs.
 *        ESTOP is monitored throughout; eFuse FAULT pins are not yet active.
 *
 * Exit: CAN 0x303 bit 0 → ACTIVATE_CTRL. ESTOP asserted → FAULT.
 */
static void state_startup(void)
{
    if (timer_check(100, &ticks.blink_tick))
    {
        DEBUG_LED_Toggle();
    }

    if (Fault_Get() & FAULT_ESTOP)
    {
        DEBUG_IO_PRINT("FAULT: ESTOP asserted during STARTUP\r\n");
        FSM_state = FSM_STATE_FAULT;
    }
    else if (CAN_Startup_Received())
    {
        DEBUG_IO_PRINT("CAN 0x303 received, exiting STARTUP\r\n");
        FSM_state = FSM_STATE_ACTIVATE_CTRL;
    }
}

/**
 * @brief Activate control state. Enables all eFuse CTRL signals.
 *        Single-cycle action state — transitions immediately to NORMAL.
 *
 * Exit: Immediate → NORMAL.
 */
static void state_activate_ctrl(void)
{
    CTRL_Enable_All();
    DEBUG_LED_Set(1);
    DEBUG_IO_PRINT("CTRL signals enabled, exiting ACTIVATE_CTRL\r\n");
    FSM_state = FSM_STATE_NORMAL;
}

/**
 * @brief Normal monitoring state. All eFuses are enabled. ESTOP and CAN ext
 *        faults are checked every cycle; eFuse FAULT lines are not acted on
 *        until 200 ms after entry to allow outputs to stabilise.
 *
 * Exit: ESTOP, 0x304 non-zero, or any eFuse FAULT asserted → FAULT.
 */
static void state_normal(void)
{
    CAN_Send_LV_ON_0x303();
    FaultSource_t faults = Fault_Get();

    if (check_critical_faults(faults))
    {
        FSM_state = FSM_STATE_FAULT;
        return;
    }

    // Allow eFuses time to stabilise before monitoring their FAULT lines
    if (HAL_GetTick() - ticks.state_tick < 200)
    {
        return;
    }

    if (timer_check(500, &ticks.blink_tick))
    {
        DEBUG_IO_PRINT("MUX_STATUS: %d\r\n", MUX_STATUS_Read());
        print_currents();
    }

    if (check_efuse_faults(faults))
    {
        FSM_state = FSM_STATE_FAULT;
    }
}

/**
 * @brief Fault state. Disables all eFuse outputs and broadcasts a fault CAN frame.
 *        Latching — requires a board reset to clear.
 *
 * Exit: None.
 */
static void state_fault(void)
{
    CTRL_Disable_All();

    if (timer_check(200, &ticks.blink_tick))
    {
        DEBUG_LED_Toggle();
        CAN_Send_Fault();
        DEBUG_IO_PRINT("MUX_STATUS: %d\r\n", MUX_STATUS_Read());

        if (led_driver_ready)
        {
            IS31FL3236A_DIST_FAULT_Toggle();
        }
    }
}

/*============================================================================*/
/* HELPER FUNCTIONS */

static void send_heartbeat_if_due(void)
{
    static uint32_t heartbeat_tick = 0U;
    if (timer_check(500, &heartbeat_tick))
    {
        CAN_Send_Heartbeat();
    }
}

static void send_currents_if_due(void)
{
    static uint32_t currents_tick = 0U;
    if (timer_check(1000, &currents_tick))
    {
        AdcCurrentReadings c = ADC_Read_Currents();
#define TO_MA_U8(x) ((uint8_t)(((x) * 1000.0f / 5.0f) > 255.0f ? 255U : (unsigned int)((x) * 1000.0f / 5.0f)))
        CAN_Send_Currents(TO_MA_U8(c.drd), TO_MA_U8(c.mdi), TO_MA_U8(c.spare_ctrl),
                          TO_MA_U8(c.spare_mux), TO_MA_U8(c.spare));
#undef TO_MA_U8
    }
}

// Returns true and prints the fault if ESTOP is asserted or 0x304 is non-zero.
static bool check_critical_faults(FaultSource_t faults)
{
    if (faults & FAULT_ESTOP)
    {
        DEBUG_IO_PRINT("FAULT: ESTOP asserted\r\n");
        return true;
    }
    if (CAN_ExtFault_Received())
    {
        DEBUG_IO_PRINT("FAULT: non-zero 0x304 received\r\n");
        return true;
    }
    return false;
}

// Returns true and prints each tripped eFuse FAULT line.
static bool check_efuse_faults(FaultSource_t faults)
{
    if (!(faults & (FAULT_DRD_FUSE | FAULT_MDI_FUSE | FAULT_SPARE_FUSE | FAULT_SPARE_CTRL_FUSE)))
    {
        return false;
    }
    if (faults & FAULT_DRD_FUSE)        DEBUG_IO_PRINT("FAULT: DRD_FUSE tripped\r\n");
    if (faults & FAULT_MDI_FUSE)        DEBUG_IO_PRINT("FAULT: MDI_FUSE tripped\r\n");
    if (faults & FAULT_SPARE_FUSE)      DEBUG_IO_PRINT("FAULT: SPARE_FUSE tripped\r\n");
    if (faults & FAULT_SPARE_CTRL_FUSE) DEBUG_IO_PRINT("FAULT: SPARE_CTRL_FUSE tripped\r\n");
    return true;
}

static void print_currents(void)
{
    AdcCurrentReadings currents = ADC_Read_Currents();
    DEBUG_IO_PRINT("CURRENTS: DRD=%umA MDI=%umA SPARE_CTRL=%umA SPARE_MUX=%umA SPARE=%umA\r\n",
                   (unsigned int)(currents.drd        * 1000.0f),
                   (unsigned int)(currents.mdi        * 1000.0f),
                   (unsigned int)(currents.spare_ctrl * 1000.0f),
                   (unsigned int)(currents.spare_mux  * 1000.0f),
                   (unsigned int)(currents.spare      * 1000.0f));
}

static bool timer_check(uint32_t interval, uint32_t *last_tick)
{
    uint32_t now = HAL_GetTick();
    if (now - *last_tick >= interval)
    {
        *last_tick = now;
        return true;
    }
    return false;
}
