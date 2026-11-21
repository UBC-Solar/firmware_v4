/**
 * @file dist_defs.h
 * @brief DIST build configuration: every compile-time value you might tune.
 *
 * Sections, in order:
 *   Bench overrides, debug output.
 *   Board-wide: CAN, status LEDs, current sensing.
 *   FSM: eFuse settling.
 */

#pragma once

// Standard library
#include <stdbool.h> // true / false for the bench overrides

// ==== Bench overrides ====
// Let the DIST run on the bench without the whole car, by ignoring inputs from parts that are
// missing.
// All must be false before flashing for real.

// ESTOP is not checked: pressing (or not wiring) ESTOP does not fault.
// Was BENCHTOP_TESTING in the old dist_main.h, which was left on in main. Kept on here so this
// refactor does not change behaviour; turn it off before the car runs.
#define IGNORE_ESTOP                    true

// The spare mux eFuse stays off when the others turn on.
// Was SPARE_MUX_BENCHTOP in the old gpio_driver.c (never defined, so the spare mux turned on).
#define KEEP_SPARE_MUX_OFF              false

// ==== Debug output ====
// Controls what is printed over SWO (the debugger's trace pin, through debug_io).
// Does not change how the board behaves.
// Each SHOW_* switch turns on one kind of message; each line starts with its tag (Ex. [FAULT]).
// Nothing is printed in a build without DEBUG.
// Events (states, faults, steps) print once, when they happen; values that keep changing only
// print in the reports, every REPORT_INTERVAL_MS.
// The macros and most messages are in debug_output.c.

// --- Always useful (on by default) ---
#define SHOW_INIT                       true  // [INIT]  Firmware version, LED panel, init done
#define SHOW_BENCH                      true  // [BENCH] Bench overrides that are on
#define SHOW_STATES                     true  // [STATE] Every state change
#define SHOW_FAULTS                     true  // [FAULT] Each fault once, when it is first set
#define SHOW_MEASUREMENTS               true  // [MEAS]  Report: every eFuse and TEL current
#define SHOW_PINS                       true  // [PINS]  Report: ESTOP, MUX_STATUS, eFuse FAULT pins

// --- For debugging (off by default) ---
#define SHOW_STEPS                      false // [STEP]  Steps inside each state

#define REPORT_INTERVAL_MS              500U  // Time between reports ([MEAS], [PINS])

// ==== CAN ====
// How often the DIST sends each periodic CAN message.
// Message IDs and layouts are in can_messages.h / can_messages.c.

#define HEARTBEAT_INTERVAL_MS           500U  // DIST heartbeat
#define LV_POWERUP_ACK_INTERVAL_MS      500U  // LV power-up acknowledge, sent in every state
#define CURRENTS_TX_INTERVAL_MS         1000U // eFuse currents and TEL current
#define BRANCH_NUMBER_INTERVAL_MS       1000U // Firmware branch number and commit hash
#define FAULT_TX_INTERVAL_MS            200U  // DIST fault message, only sent in STATE_FAULT

// Firmware branch number sent in the BRANCH_NUMBER_ID message
#define DIST_BRANCH_NUMBER              1U

// ==== Status LEDs ====
// DEBUG_LED blinks fast while waiting in STATE_STARTUP, stays on in STATE_NORMAL, and blinks slower
// in STATE_FAULT, together with the DIST fault LED on the IS31FL3236 LED panel.

#define STARTUP_LED_BLINK_MS            100U // DEBUG_LED toggle period in STATE_STARTUP
#define FAULT_LED_BLINK_MS              200U // DEBUG_LED and panel fault LED toggle period

// ==== Current sensing ====
// Every eFuse (TPS259631) outputs a current proportional to its load current on its ILM pin, and
// a resistor RILM turns it into a voltage the ADC reads:
//   IOUT = VILM / (GIMON x RILM)          (datasheet Eq. 6)
// RILM also sets each eFuse's current limit.
// The TEL rail has its own current sensor (ACS70331, U3.2):
//   IOUT = (VIOUT - VZERO) / SENS          (datasheet p. 13)
// adc_runtime.c does the conversions.

// --- eFuses ---
#define EFUSE_GIMON                     653e-6f // ILM current per A of load (A/A); typ. 657e-6
#define EFUSE_RILM_DRD_OHMS             910.0f
#define EFUSE_RILM_MDI_OHMS             7500.0f
#define EFUSE_RILM_SPARE_CTRL_OHMS      910.0f
#define EFUSE_RILM_SPARE_MUX_OHMS       910.0f
#define EFUSE_RILM_SPARE_OHMS           910.0f

// Correction for the 910 ohm channels, measured on the bench (BMS testing / DIST / efuse, Monday):
//   measured = SLOPE x actual - OFFSET, so actual = (measured + OFFSET) / SLOPE
#define EFUSE_910_CORRECTION_SLOPE      0.961085f
#define EFUSE_910_CORRECTION_OFFSET_A   0.00328378f

// --- TEL current sensor ---
#define TEL_SENSE_V_PER_A               0.400f // Sensitivity (typ.)
#define TEL_SENSE_ZERO_V                0.250f // Output at 0 A (typ., 25 degC)

// ==== eFuse settling ====
// After STATE_ENABLE_EFUSES turns the eFuses on, their FAULT pins are recorded right away but only
// acted on once this time has passed in STATE_NORMAL, so the outputs can settle first.

#define EFUSE_SETTLE_MS                 200U
