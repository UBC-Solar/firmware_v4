#include "fsm.h"
#include "gpio_driver.h"
#include "i2c_driver.h"
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
    led_driver_ready       = led_driver_available;
    ticks.blink_tick       = HAL_GetTick();
    ticks.state_tick       = HAL_GetTick();
    FSM_state              = FSM_STATE_STARTUP;
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
        DEBUG_IO_PRINT("FSM -> %s\r\n", state_names[FSM_state]);
        prev_state = FSM_state;
    }

    FSM_state_table[FSM_state]();
}

/*============================================================================*/
/* STATE FUNCTION IMPLEMENTATIONS */

/**
 * @brief Startup state. Runs immediately after power-on before enabling any outputs.
 *        Provides a visible indication that the board is booting.
 *
 * Entry Condition: Power-on or reset.
 *
 * Exit Condition: CAN message 0x323 received with first bit = 1.
 * Exit Action:    None.
 * Exit State:     FSM_STATE_ACTIVATE_CTRL
 */
static void state_startup(void)
{
    if (timer_check(100, &ticks.blink_tick))
    {
        DEBUG_LED_Toggle();
    }

    if (CAN_Startup_Received())
    {
        DEBUG_IO_PRINT("CAN 0x323 received, exiting STARTUP\r\n");
        FSM_state = FSM_STATE_ACTIVATE_CTRL;
    }
}

/**
 * @brief Activate control state. Enables all eFuse control signals.
 *        Runs once on entry to normal operation, then immediately transitions.
 *
 * Entry Condition: Startup sequence complete (500ms elapsed).
 *
 * Exit Condition: Immediate (single-cycle action state).
 * Exit Action:    Enable all CTRL GPIO pins, turn DEBUG LED on solid.
 * Exit State:     FSM_STATE_NORMAL
 */
static void state_activate_ctrl(void)
{
    CTRL_Enable_All();
    DEBUG_LED_Set(1);
    FSM_state = FSM_STATE_NORMAL;
}

/**
 * @brief Normal monitoring state. All eFuses are enabled. Continuously
 *        monitors fuse fault signals for any tripped eFuse.
 *
 * Entry Condition: Startup complete and all eFuse CTRLs activated.
 *
 * Exit Condition: Any fuse fault signal pulled low.
 * Exit Action:    None.
 * Exit State:     FSM_STATE_FAULT
 */
static void state_normal(void)
{
    // Allow eFuses time to stabilise before monitoring for faults
    if (HAL_GetTick() - ticks.state_tick < 200)
    {
        return;
    }

    if (Any_Fuse_Fault())
    {
        if (DRD_FUSE_Read()        == GPIO_PIN_RESET) DEBUG_IO_PRINT("FAULT: DRD_FUSE tripped\r\n");
        if (SPARE_FUSE_Read()      == GPIO_PIN_RESET) DEBUG_IO_PRINT("FAULT: SPARE_FUSE tripped\r\n");
        if (SPARE_CTRL_FUSE_Read() == GPIO_PIN_RESET) DEBUG_IO_PRINT("FAULT: SPARE_CTRL_FUSE tripped\r\n");

        FSM_state = FSM_STATE_FAULT;
    }
}

/**
 * @brief Fault state. Entered when any eFuse trips. Disables all control
 *        outputs and signals the fault visually and over CAN. Latching — requires reset.
 *
 * Entry Condition: Any fuse fault signal pulled low during normal operation.
 *
 * Exit Condition: None (latching until board reset).
 * Exit Action:    None.
 * Exit State:     FSM_STATE_FAULT
 */
static void state_fault(void)
{
    CTRL_Disable_All();

    if (timer_check(200, &ticks.blink_tick))
    {
        DEBUG_LED_Toggle();
        CAN_Send_Fault_0x324();

        if (led_driver_ready)
        {
            IS31FL3236A_DIST_FAULT_Toggle();
        }
    }
}

/*============================================================================*/
/* HELPER FUNCTIONS */

/**
 * @brief Returns true if the given interval has elapsed since last_tick,
 *        and resets last_tick to the current time.
 */
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
