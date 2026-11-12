/**
 * @file hvc_types.h
 * @brief HVC data types: FSM states, fault flags, shared data, timing ticks.
 *
 * Types only: fsm.c creates the one variable of each (g_state, g_fault_flags, g_shared, g_ticks),
 * and fsm.h shares them with the other files.
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// Runtime
#include "adc_runtime.h" // ADC_Values
#include "gpio_runtime.h" // PinLevels

/**
 * @brief Every state of the HVC FSM.
 *
 * Listed in startup order: STATE_RESET runs first and STATE_MONITORING is the normal running state
 * once startup is done.
 * Any state can go to STATE_FAULT.
 * SendState() sends the state's number, so adding or moving a state changes a CAN message
 * (see can_messages.h).
 */
typedef enum {
    STATE_RESET = 0,
    STATE_TEL_STARTUP,
    STATE_MST_HANDSHAKE,
    STATE_MST_STATUS_CHECK,
    STATE_FANS_POWERUP,
    STATE_HV_CONNECT,
    STATE_OPEN_DISCHARGE_RELAY,
    STATE_MOTOR_PRECHARGE,
    STATE_MPPT_PRECHARGE,
    STATE_CLOSE_LLIM,
    STATE_CLOSE_HLIM,
    STATE_LV_POWERUP,
    STATE_MONITORING,
    STATE_FAULT,

    STATE_UNKNOWN, // Not a real state: last printed state before the first pass (debug_output.c)
} FsmState;

/**
 * @brief Every condition that sends the HVC to STATE_FAULT, one bool each.
 *
 * FSM_Run() goes to STATE_FAULT on the first pass that finds any flag true.
 * Flags are set by the FSM, the GPIO fault callbacks, and the CAN receive interrupt, and are never
 * cleared, so a flag stays set until the HVC resets.
 * SendFaults() sends them over CAN, one bit each in this order: adding or moving a flag changes the
 * CAN layout (see can_messages.h).
 * Every field must be a bool: FSM_AnyFaultActive() reads the struct as a list of bools.
 */
typedef struct {
    // General faults
    bool estop;
    bool imd_fault;
    bool mst_fault;
    bool mst_unhealthy;
    bool overcurrent;
    bool reverse_current;
    bool dist_fault;
    bool dcdc_fault;
    bool supp_fault;
    bool watchdog_fired;
    bool dcdc_overtemp;

    // Timeout faults for startup states
    bool tel_startup_timeout;
    bool mst_handshake_timeout;
    bool mst_status_check_timeout;
    bool hv_connect_timeout;
    bool open_discharge_relay_timeout;
    bool motor_precharge_timeout;
    bool mppt_precharge_timeout;
    bool lv_powerup_timeout;

    // Heartbeat timeout faults
    bool tel_heartbeat_timeout;
    bool mst_heartbeat_timeout;
    bool dist_heartbeat_timeout;
} FaultFlags;

/**
 * @brief Values passed between the FSM and the interrupts.
 *
 * Holds what other boards send over CAN, this pass's ADC measurements and pin levels, and the flags
 * the FSM sets as startup progresses to turn checks on.
 */
typedef struct {
    // Received over CAN.
    // Set by the CAN receive interrupt (or by a faked board in tests.c, through the same
    // CAN_Messages_HandleRx()); read by the main loop, which also clears the _received flags.
    volatile bool     tel_heartbeat_received;           // Also read by TelStartup()
    volatile bool     dist_heartbeat_received;
    volatile bool     mst_heartbeat_received;
    volatile bool     lv_powerup_received;
    // MST heartbeat reports no errors, faults, or warnings
    volatile bool     mst_healthy;
    volatile int32_t  mst_pack_voltage_mv;              // From MST_VOLT_SUMMARY

    // Read once per pass by HVC_Main(), before the FSM.
    // Bench overrides (tests.c) may write fake values over them before the FSM reads them.
    ADC_Values        adc;                              // This pass's ADC readings, in real units
    PinLevels         pins;                             // This pass's polled input pin levels

    // FSM bookkeeping. Written by the main loop.
    volatile bool     mst_irq_armed;                    // Read by the GPIO interrupt
    volatile bool     dcdc_irq_armed;                   // Read by the GPIO interrupt
    // TEL + DIST heartbeat checks, enabled after STATE_TEL_STARTUP
    bool              tel_dist_heartbeat_check_enabled;
    // MST heartbeat check, enabled after STATE_MST_STATUS_CHECK
    bool              mst_heartbeat_check_enabled;
} SharedData;

/**
 * @brief Start times (HAL_GetTick(), in ms) for every delay, timeout, and periodic task.
 *
 * Each one holds when its timer last started.
 * FSM_TimerElapsed() compares it with the current time, and restarts it when the time is up.
 */
typedef struct {
    // FSM timers. state_timeout and state_delay are both restarted on every state change.
    uint32_t state_timeout;    // State entry time, only for the state's timeout
    uint32_t state_delay;      // Start of the current wait inside a state (fans, contactors, ...)
    uint32_t lv_powerup_retry; // LV power-up request resend
    uint32_t fault_led;        // FAULT_LED blink

    // Heartbeat timeouts.
    // Started when the board's check is enabled, then restarted by each heartbeat received, in
    // CheckHeartbeats().
    uint32_t tel_heartbeat;    // Last TEL heartbeat
    uint32_t dist_heartbeat;   // Last DIST heartbeat
    uint32_t mst_heartbeat;    // Last MST heartbeat

    // Periodic tasks. Run every pass, in any state.
    uint32_t can_tx;           // Periodic CAN transmit
    uint32_t hvc_heartbeat;    // HVC heartbeat and commit hash transmit
    uint32_t report;           // Debug output reports ([MEAS], [PINS], [HB])
    uint32_t dump;             // Periodic debug output dump
} Ticks;
