/**
 * @file dist_types.h
 * @brief DIST data types: FSM states, fault flags, shared data, timing ticks.
 *
 * Types only: fsm.c creates the one variable of each (g_state, g_fault_flags, g_shared, g_ticks),
 * and fsm.h shares them with the other files.
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// Runtime
#include "adc_runtime.h"  // ADC_Values
#include "gpio_runtime.h" // PinLevels

/**
 * @brief Every state of the DIST FSM.
 *
 * Listed in startup order: STATE_STARTUP runs first and STATE_NORMAL is the normal running state.
 * Any state can go to STATE_FAULT, which has no exit.
 */
typedef enum {
    STATE_STARTUP = 0,
    STATE_ENABLE_EFUSES,
    STATE_NORMAL,
    STATE_FAULT,

    STATE_UNKNOWN, // Not a real state: last printed state before the first pass (debug_output.c)
} FsmState;

/**
 * @brief Every condition that sends the DIST to STATE_FAULT, one bool each.
 *
 * FSM_Run() goes to STATE_FAULT on the first pass that finds any flag true.
 * Flags are never cleared, so a flag stays set until the DIST resets.
 * Every field must be a bool: FSM_AnyFaultActive() reads the struct as a list of bools, and
 * debug_output.c names them in this order.
 */
typedef struct {
    bool estop;           // ESTOP pressed (skipped with IGNORE_ESTOP)
    bool hvc_fault;       // HVC fault message received with any non-zero byte
    bool drd_fuse;        // DRD eFuse tripped
    bool mdi_fuse;        // MDI eFuse tripped
    bool spare_fuse;      // SPARE eFuse tripped
    bool spare_ctrl_fuse; // SPARE_CTRL eFuse tripped
} FaultFlags;

/**
 * @brief Values passed between the FSM and the interrupts.
 *
 * Holds what the HVC sends over CAN, the eFuse trips the pin interrupts record, and this pass's
 * ADC measurements and pin levels.
 */
typedef struct {
    // Received over CAN.
    // Set by the CAN receive interrupt; read by the main loop.
    volatile bool lv_powerup_received;     // HVC's LV power-up request, read by Startup()

    // eFuse trips.
    // Set by the eFuse FAULT pin interrupts once efuse_irq_armed is true; turned into fault flags
    // by Normal() once the eFuses have settled (EFUSE_SETTLE_MS).
    volatile bool drd_fuse_tripped;
    volatile bool mdi_fuse_tripped;
    volatile bool spare_fuse_tripped;
    volatile bool spare_ctrl_fuse_tripped;
    volatile bool efuse_irq_armed;         // Set by EnableEfuses(), read by the pin interrupt

    // Read once per pass by DIST_Main(), before the FSM.
    ADC_Values    adc;                     // This pass's currents, in mA
    PinLevels     pins;                    // This pass's polled input pin levels
} SharedData;

/**
 * @brief Start times (HAL_GetTick(), in ms) for every delay and periodic task.
 *
 * Each one holds when its timer last started.
 * FSM_TimerElapsed() compares it with the current time, and restarts it when the time is up.
 */
typedef struct {
    // FSM timers
    uint32_t state_entry;    // When the current state started (only STATE_NORMAL uses it)
    uint32_t debug_led;      // DEBUG_LED blink (STATE_STARTUP and STATE_FAULT)

    // Periodic tasks. Run every pass, in any state.
    uint32_t heartbeat;      // DIST heartbeat transmit
    uint32_t lv_powerup_ack; // LV power-up acknowledge transmit
    uint32_t currents_tx;    // Currents transmit
    uint32_t branch_number;  // Branch number and commit hash transmit
    uint32_t fault_tx;       // Fault message transmit (STATE_FAULT only)
    uint32_t report;         // Debug output reports ([MEAS], [PINS])
} Ticks;
