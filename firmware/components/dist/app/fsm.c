/**
 * @file fsm.c
 * @brief DIST FSM core: init and run, states, interrupt callbacks, and helpers
 *
 * The FSM turns the LV boards' power on once the HVC asks for it, then watches for faults.
 * In state order, it: waits for the HVC's LV power-up request, turns every eFuse on, then
 * monitors ESTOP, the HVC fault message, and the eFuse FAULT pins.
 * Any fault goes to STATE_FAULT, which turns every eFuse off and stays there until a reset.
 *
 * Sections, in order:
 *   GLOBAL VARIABLES:    the DIST-wide variables (types in dist_types.h).
 *   PUBLIC API:          FSM_Init() and FSM_Run(), called from dist_main.c.
 *   STATES:              one function per FsmState, in state order.
 *   INTERRUPT CALLBACKS: called from the eFuse FAULT pin interrupts.
 *   HELPERS:             small functions used by the states and FSM_Run().
 */

#include "fsm.h"

// HAL and CubeMX
#include "stm32f1xx_hal.h"
#include "main.h" // Pin names

// Drivers
#include "gpio_driver.h"

// Runtime
#include "led_runtime.h"

// DIST app
#include "can_messages.h"
#include "debug_output.h"
#include "dist_defs.h"

// States, in state order
static void Startup(void);
static void EnableEfuses(void);
static void Normal(void);
static void Fault(void);

// Fault checks
static void CheckEstop(void);
static void CheckEfuseTrips(void);

// ==== Global variables ====
// The one variable of each DIST-wide type (types in dist_types.h).
// Any file that includes fsm.h can use them.
// g_fault_flags is volatile because the CAN interrupt writes it.
// g_state stays volatile to be safe, but only the main loop writes it.
// g_shared marks its interrupt-written fields volatile.

volatile FsmState   g_state;
volatile FaultFlags g_fault_flags;
SharedData          g_shared;
Ticks               g_ticks;

// ==== Public API ====
// Called from dist_main.c: FSM_Init() once at startup, FSM_Run() every pass.

/**
 * @brief Sets the starting FSM state. Call once from DIST_Init().
 *
 * Side effects: writes g_state and g_ticks.state_entry.
 */
void FSM_Init(void) {
    g_ticks.state_entry = HAL_GetTick();
    g_state = STATE_STARTUP;
}

/**
 * @brief Runs one pass of the FSM. Call from DIST_Main() every pass.
 *
 * In order: prints the debug output (DEBUG_Output_Run(), debug_output.c), checks ESTOP, switches to
 * STATE_FAULT if any fault flag is set, sends periodic CAN messages, then runs the current state's
 * function.
 *
 * A fault reaches STATE_FAULT one of two ways.
 * A state that finds a fault sets its flag and goes to STATE_FAULT itself, in the same pass.
 * The CAN interrupt only sets a flag; the FSM_AnyFaultActive() check here then moves to STATE_FAULT
 * on the next pass.
 *
 * Side effects: may set g_fault_flags.estop and change g_state.
 */
void FSM_Run(void) {
    DEBUG_Output_Run(); // First, so it sees this pass's state and flags

    CheckEstop();

    if (FSM_AnyFaultActive()) {
        g_state = STATE_FAULT;
    }

    CAN_Messages_SendAll();

    switch (g_state) {
        case STATE_STARTUP:
            Startup();
            break;

        case STATE_ENABLE_EFUSES:
            EnableEfuses();
            break;

        case STATE_NORMAL:
            Normal();
            break;

        case STATE_FAULT:
        default:
            Fault();
            break;
    }
}

// ==== States ====
// One function per FsmState, in state order.
// FSM_Run() calls the current state's function every pass.
// Each state's doc lists its exit conditions and the state each one goes to.

/**
 * @brief Waits for the HVC to ask for LV power, with every eFuse still off.
 *
 * Blinks DEBUG_LED every STARTUP_LED_BLINK_MS while waiting.
 * ESTOP and the HVC fault message are checked by FSM_Run(); the eFuse FAULT pins are not armed yet.
 *
 * Exit Condition: LV power-up request received from the HVC.
 * Exit State: STATE_ENABLE_EFUSES
 */
static void Startup(void) {
    if (FSM_TimerElapsed(STARTUP_LED_BLINK_MS, &g_ticks.debug_led)) {
        GPIO_Toggle(DEBUG_GPIO_Port, DEBUG_Pin);
    }

    if (g_shared.lv_powerup_received) {
        PRINT_STEP("LV power-up request received");
        g_ticks.state_entry = HAL_GetTick();
        g_state = STATE_ENABLE_EFUSES;
        return;
    }
}

/**
 * @brief Turns every eFuse on, then arms the eFuse FAULT pin interrupts.
 *
 * Runs once, for one pass.
 * The interrupts are armed only after the eFuses are on: a FAULT pin has no defined level while
 * its eFuse is off, so an edge before then means nothing.
 * Every trip from here on is recorded, including power-up transients; Normal() acts on them once
 * the outputs have settled.
 * Also sends the LV power-up acknowledge straight away, so the HVC does not wait for the periodic
 * one, and turns DEBUG_LED on.
 *
 * Exit Condition: none (immediately).
 * Exit State: STATE_NORMAL
 */
static void EnableEfuses(void) {
    FSM_EnableAllEfuses();
    g_shared.efuse_irq_armed = true;

    GPIO_Write(DEBUG_GPIO_Port, DEBUG_Pin, GPIO_PIN_SET);
    CAN_Messages_SendLvPowerupAck();
    PRINT_STEP("eFuses on, eFuse fault check armed");

    g_ticks.state_entry = HAL_GetTick();
    g_state = STATE_NORMAL;
}

/**
 * @brief Normal running state: every eFuse is on. Watches for eFuse trips.
 *
 * eFuse trips are only acted on once EFUSE_SETTLE_MS have passed since entering this state, to let
 * the outputs settle; a trip recorded during that time still faults once it is over.
 * ESTOP and the HVC fault message are checked by FSM_Run().
 *
 * Exit Condition: an eFuse trip recorded, after EFUSE_SETTLE_MS.
 * Exit State: STATE_FAULT
 */
static void Normal(void) {
    if ((HAL_GetTick() - g_ticks.state_entry) < EFUSE_SETTLE_MS) {
        return;
    }

    CheckEfuseTrips();

    if (FSM_AnyFaultActive()) {
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Safe state. Turns every eFuse off and blinks DEBUG_LED and the panel fault LED.
 *
 * The DIST fault CAN message is sent from CAN_Messages_SendAll() while in this state.
 * Has no exit; the DIST stays here until it is reset.
 */
static void Fault(void) {
    FSM_DisableAllEfuses();

    if (FSM_TimerElapsed(FAULT_LED_BLINK_MS, &g_ticks.debug_led)) {
        GPIO_Toggle(DEBUG_GPIO_Port, DEBUG_Pin);
        LED_Toggle(LED_OUT_DIST_FAULT); // Does nothing if the LED panel was not found
    }
}

// ==== Interrupt callbacks ====
// Called from the eFuse FAULT pin interrupts (HAL_GPIO_EXTI_Callback() in gpio_driver.c).
// Only records the trip in g_shared: Normal() turns it into a fault flag once the eFuses have
// settled.
// Nothing prints here: the fault is printed from the main loop (debug_output.c).

/**
 * @brief Records an eFuse trip, if the eFuse FAULT interrupts are armed.
 *
 * The TPS259631 FAULT pin is open-drain and active-low: it falls when the eFuse shuts its output
 * off (overcurrent or overtemperature), which fires this interrupt.
 * Ignored until EnableEfuses() arms it (g_shared.efuse_irq_armed).
 *
 * Side effects: may set one of the g_shared *_fuse_tripped flags.
 *
 * @param fuse_pin FAULT pin that fell (DRD_FUSE_Pin, MDI_FUSE_Pin, SPARE_FUSE_Pin or
 *                 SPARE_CTRL_FUSE_Pin).
 */
void FSM_EfuseFaultCallback(uint16_t fuse_pin) {
    if (!g_shared.efuse_irq_armed) {
        return;
    }

    switch (fuse_pin) {
        case DRD_FUSE_Pin:
            g_shared.drd_fuse_tripped = true;
            break;

        case MDI_FUSE_Pin:
            g_shared.mdi_fuse_tripped = true;
            break;

        case SPARE_FUSE_Pin:
            g_shared.spare_fuse_tripped = true;
            break;

        case SPARE_CTRL_FUSE_Pin:
            g_shared.spare_ctrl_fuse_tripped = true;
            break;

        default:
            break;
    }
}

// ==== Helpers ====
// Small functions used by the states and FSM_Run().
// Grouped by topic: Timing, Faults, eFuses.
// Debug output helpers are in debug_output.c.

// --- Timing ---

/**
 * @brief Checks whether interval_ms have passed since *last_tick, and if so restarts the timer.
 *
 * In a condition with other checks, put it last: it restarts the timer when it returns true, so it
 * must only run once the other checks are already true.
 *
 * @param interval_ms Time to wait, in ms.
 * @param last_tick   Timestamp (HAL_GetTick()) the interval is measured from.
 *                    Set to now when it returns true.
 * @return true if the interval has elapsed.
 */
bool FSM_TimerElapsed(uint32_t interval_ms, uint32_t *last_tick) {
    if (HAL_GetTick() - *last_tick >= interval_ms) {
        *last_tick = HAL_GetTick();
        return true;
    }
    return false;
}

// --- Faults ---

/**
 * @brief Checks whether any fault flag is set.
 *
 * Reads g_fault_flags as an array of bools, so new flags are checked automatically.
 *
 * @return true if at least one fault flag is set.
 */
bool FSM_AnyFaultActive(void) {
    const volatile bool *flag = (const volatile bool*)&g_fault_flags;

    for (uint32_t i = 0U; i < sizeof(g_fault_flags); i++) {
        if (flag[i]) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Sets the ESTOP fault while ESTOP is pressed, unless IGNORE_ESTOP is on.
 *
 * ESTOP is active-low and polled every pass (g_shared.pins), in every state.
 *
 * Side effects: may set g_fault_flags.estop.
 */
static void CheckEstop(void) {
    if (!IGNORE_ESTOP && (g_shared.pins.estop == GPIO_PIN_RESET)) {
        g_fault_flags.estop = true;
    }
}

/**
 * @brief Turns each eFuse trip the pin interrupts recorded into its fault flag.
 *
 * Called by Normal() once the eFuses have settled.
 *
 * Side effects: may set g_fault_flags.drd_fuse, mdi_fuse, spare_fuse, or spare_ctrl_fuse.
 */
static void CheckEfuseTrips(void) {
    if (g_shared.drd_fuse_tripped) {
        g_fault_flags.drd_fuse = true;
    }
    if (g_shared.mdi_fuse_tripped) {
        g_fault_flags.mdi_fuse = true;
    }
    if (g_shared.spare_fuse_tripped) {
        g_fault_flags.spare_fuse = true;
    }
    if (g_shared.spare_ctrl_fuse_tripped) {
        g_fault_flags.spare_ctrl_fuse = true;
    }
}

// --- eFuses ---
// Each CTRL pin drives the EN pin of a TPS259631 eFuse: high turns its output on, low turns it off.

/**
 * @brief Turns every eFuse output on: SPARE_CTRL, MDI, DRD, and SPARE_MUX.
 *
 * SPARE_MUX stays off with KEEP_SPARE_MUX_OFF (dist_defs.h).
 * Used by EnableEfuses().
 */
void FSM_EnableAllEfuses(void) {
    GPIO_Write(SPARE_CTRL_GPIO_Port, SPARE_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(MDI_CTRL_GPIO_Port,   MDI_CTRL_Pin,   GPIO_PIN_SET);
    GPIO_Write(DRD_CTRL_GPIO_Port,   DRD_CTRL_Pin,   GPIO_PIN_SET);
    if (!KEEP_SPARE_MUX_OFF) {
        GPIO_Write(SPARE_MUX_CTRL_GPIO_Port, SPARE_MUX_CTRL_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief Turns every eFuse output off: SPARE_CTRL, MDI, DRD, and SPARE_MUX.
 *
 * Used by Fault().
 */
void FSM_DisableAllEfuses(void) {
    GPIO_Write(SPARE_CTRL_GPIO_Port,     SPARE_CTRL_Pin,     GPIO_PIN_RESET);
    GPIO_Write(MDI_CTRL_GPIO_Port,       MDI_CTRL_Pin,       GPIO_PIN_RESET);
    GPIO_Write(DRD_CTRL_GPIO_Port,       DRD_CTRL_Pin,       GPIO_PIN_RESET);
    GPIO_Write(SPARE_MUX_CTRL_GPIO_Port, SPARE_MUX_CTRL_Pin, GPIO_PIN_RESET);
}
