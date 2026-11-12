/**
 * @file hvc_defs.h
 * @brief HVC build configuration: every compile-time value you might tune.
 *
 * Sections, in order:
 *   Hardware tests, bench overrides, debug output.
 *   Board-wide: CAN, heartbeats, HV current shunt, supplemental battery,
 *   LV current sensor, status LEDs.
 *   FSM, in state order: state timeouts, fans, contactors, discharge relay,
 *   precharge, DCDC.
 */

#pragma once

// Standard library
#include <stdbool.h>       // true / false for the bench overrides

// HAL and CubeMX
#include "stm32f1xx_hal.h" // GPIO_PIN_SET / GPIO_PIN_RESET for the contactor levels

/*==============================================================================
 * === HARDWARE TESTS ==========================================================
 * Small programs that run INSTEAD of the FSM, to check one piece of hardware
 * (unit tests) or one link to another board (integration tests).
 * HVC_TEST picks one. TEST_NONE runs the normal FSM. Each test has its own
 * number. The test code is in tests.c (HARDWARE TESTS), named after its TEST_*
 * value (TEST_UNIT_SHUNT -> TestUnitShunt()).
 * HVC_TEST must be TEST_NONE before flashing for real.
 *============================================================================*/

// --- Pick the test ---
#define HVC_TEST                        TEST_NONE // Change only this line

// --- Available tests ---
#define TEST_NONE                       0 // Normal operation: runs the FSM

// Unit tests
#define TEST_UNIT_SHUNT                 1 // Reads the shunt voltage on a timer

// Integration tests
#define TEST_INT_CAN                    2 // Powers DIST, sends a debug CAN message on a timer

// --- Test settings ---
#define TEST_UNIT_SHUNT_INTERVAL_MS     100  // Time between shunt voltage reads
#define TEST_INT_CAN_INTERVAL_MS        2000 // Time between debug CAN messages

/*==============================================================================
 * === BENCH OVERRIDES =========================================================
 * Let the normal FSM run on the bench without the whole car, by faking or
 * ignoring inputs from parts that are missing. The FSM code does not change:
 * a faked part's real input is blocked, and a fake value is written in its
 * place. Any number can be on at once. They do nothing during a hardware test,
 * because the test replaces the FSM.
 * Grouped by board: other boards first, then the HVC itself. Each board lists,
 * in order: fake data, then ignored faults. The code is in tests.c (BENCH
 * OVERRIDES), in the same board sections.
 * All must be false before flashing for real.
 *============================================================================*/

// --- MST ---
// Each part can be faked on its own; the real MST's matching part is then ignored.
// Times are measured from when the fake MST starts (the FSM first reaching STATE_MST_HANDSHAKE).

// Fault pin
#define FAKE_MST_FAULT_PIN              false  // Fakes fault pin (startup hold + later fault)
#define FAKE_MST_INIT_HOLD_MS           2000   // Held high this long at startup, like the real MST
                                               // Must be < MST_HANDSHAKE_TIMEOUT_MS
#define FAKE_MST_FAULT_AFTER_MS         0      // High again this long after the hold (0 = never)

// Heartbeat
#define FAKE_MST_HEARTBEAT              false  // Fakes CAN (heartbeat)
#define FAKE_MST_HEARTBEAT_PERIOD_MS    1000   // Send period, like the real MST
#define FAKE_MST_UNHEALTHY              false  // Reports a problem (MST status check times out)
#define FAKE_MST_HEARTBEAT_STOP_AFTER_MS 0     // Stops this long after start (0 = never)

// Pack voltage
#define FAKE_MST_PACK_VOLTAGE           false  // Fakes CAN (pack voltage)
#define FAKE_MST_PACK_VOLTAGE_PERIOD_MS 1000   // Send period
#define FAKE_MST_PACK_VOLTAGE_MV        115000 // Pack voltage reported

// Ignored faults
#define IGNORE_MST_FAULT                false  // Fault pin going high after startup doesn't fault

// --- TEL ---
// Ideas, not implemented yet:
//   FAKE_TEL_HEARTBEAT     - fake CAN (heartbeat)

// --- DIST ---
// Ideas, not implemented yet:
//   FAKE_DIST_DATA         - fake CAN (heartbeat + LV power-up reply)
//   IGNORE_DIST_FAULT      - don't fault on the DIST fault CAN message

// --- HVC ---

// Precharge voltage
#define FAKE_PRECHARGE_VOLTAGE          false  // Fakes ADC (reads as charged; still waits min time)

// Ignored faults
// Ideas, not implemented yet:
//   IGNORE_ESTOP           - don't fault when ESTOP is pressed
//   IGNORE_IMD             - don't fault on the IMD fault input
//   IGNORE_DCDC_FAULT      - don't fault when DCDC_ACTIVE goes high after STATE_HV_CONNECT
//   IGNORE_SUPP_FAULT      - don't fault on SUPP_ACTIVE high at startup (debugger-powered bench)
//   IGNORE_HV_CURRENT      - don't fault on the shunt monitor's overcurrent / reverse current alert
//   IGNORE_DCDC_TEMP       - don't fault on DCDC over-temperature (limit is still a dummy value)

/*==============================================================================
 * === DEBUG OUTPUT ============================================================
 * Controls what is printed over SWO (the debugger's trace pin, through debug_io).
 * Does not change how the board behaves. Each SHOW_* switch turns on one kind of
 * message, and each line starts with its kind's tag (Ex. [FAULT]). Nothing is
 * printed in a build without DEBUG.
 * Events (states, faults, steps) print once, when they happen. Values that keep
 * changing only print in the reports and the dump, at most one report line or
 * dump section per pass, so the output never floods or mixes. The macros and
 * most messages are in debug_output.c; tests.c prints its own [TEST] and
 * [BENCH] lines.
 *============================================================================*/

// --- Always useful (on by default) ---
#define SHOW_INIT                       true  // [INIT]  Firmware version, reset cause, init done
#define SHOW_BENCH                      true  // [BENCH] Bench overrides on, fake board events
#define SHOW_TESTS                      true  // [TEST]  Hardware test running, its results
#define SHOW_STATES                     true  // [STATE] Every state change
#define SHOW_FAULTS                     true  // [FAULT] Each fault once, with its cause and value

// --- For debugging (off by default) ---
#define SHOW_STEPS                      false // [STEP]  Steps inside each state (NEG closed, ...)
#define SHOW_CAN                        false // [CAN]   First heartbeat of each board, LV requests
#define SHOW_MEASUREMENTS               false // [MEAS]  Report: ADC, pack voltage, shunt current
#define SHOW_PINS                       false // [PINS]  Report: input pin levels
#define SHOW_HEARTBEATS                 false // [HB]    Report: ms since each board's heartbeat
#define REPORT_INTERVAL_MS              1000U // Time between reports ([MEAS], [PINS], [HB])

// --- Last resort ---
// The dump prints every value the HVC holds ([DUMP]), one section per pass. Long: turn it on
// only when the messages above are not enough.
#define SHOW_DUMP_ON_FAULT              false // Dump once, when the HVC goes to STATE_FAULT
#define SHOW_DUMP_PERIODIC              false // Dump every DUMP_INTERVAL_MS
#define DUMP_INTERVAL_MS                5000U // Time between periodic dumps

/*==============================================================================
 * === CAN =====================================================================
 * How often the HVC sends its periodic CAN messages and resends the LV power-up
 * request. Message IDs are in can_messages.h.
 *============================================================================*/

#define CAN_TX_INTERVAL_MS              200U // State, faults, measurements, precharge
#define LV_POWERUP_RETRY_MS             500U // LV power-up request resend interval

/*==============================================================================
 * === HEARTBEATS ==============================================================
 * A heartbeat is a CAN message a board sends on a timer to show it is alive.
 * The HVC sends its own, and faults if TEL, DIST, or MST stop sending theirs
 * (checked once each board is up, see CheckHeartbeats() in fsm.c).
 *============================================================================*/

// HVC heartbeat and commit hash send period
#define HVC_HEARTBEAT_INTERVAL_MS       1000U
// Max gap between TEL / DIST / MST heartbeats before STATE_FAULT
#define HEARTBEAT_TIMEOUT_MS            3000U

/*==============================================================================
 * === HV CURRENT SHUNT ========================================================
 * Pack current is measured across a shunt resistor by a shunt monitor chip, read
 * over I2C. The chip raises HV_CURRENT_ALERT when the current passes one of the
 * fault limits below. Register map and ADC-range-dependent values are in
 * shunt_runtime.h.
 *============================================================================*/

#define SHUNT_MONITOR_INA228            1
#define SHUNT_MONITOR_INA237            2
#define SHUNT_MONITOR                   SHUNT_MONITOR_INA237 // Which shunt monitor chip is fitted

// ADC range selection
// 0 = +/-163.84 mV full scale, shunt LSB = 312.5 nV (INA228) / 5 uV (INA237)
// 1 = +/-40.96 mV full scale, shunt LSB = 78.125 nV (INA228) / 1.25 uV (INA237)
//     (higher resolution, lower range)
#define SHUNT_ADCRANGE                  1

// Shunt resistor value in nano-ohms (Thx Bryan Cady of Moment Energy!)
#define SHUNT_RESISTANCE_NOHMS          100496

// --- Fault limits (shunt monitor alert) ---
#define SHUNT_OVERCURRENT_A             60U // Positive overcurrent limit (A)
#define SHUNT_REVERSE_CURRENT_A         20U // Reverse current magnitude limit (A)

// ADC offset error, measured during ADC characterization: the ADC reads this many bits off
// from the true value (negative = reads low). adc_driver.c subtracts it from every averaged
// reading, so every ADC channel is corrected before it is converted to engineering units.
#define ADC_ERROR_BIAS                  -70 // ADC bits

/*==============================================================================
 * === SUPPLEMENTAL BATTERY ====================================================
 * The supp battery powers the LV side until HV_CONNECT, when the DCDC takes
 * over. Its voltage reaches the ADC through a resistor divider (scaled back up
 * in adc_runtime.c); SUPP_LOW_LED turns on when it reads low.
 *============================================================================*/

#define SUPP_SENSE_DIVIDER_NUM          5700U  // (R13.4 + R13.5)
#define SUPP_SENSE_DIVIDER_DEN          1000U  // R13.5
#define SUPP_LOW_THRESHOLD_MV           10500U // SUPP_LOW_LED on below this supp battery voltage

/*==============================================================================
 * === LV CURRENT SENSOR =======================================================
 * The LV current sensor outputs a voltage read on the ADC. adc_runtime.c removes
 * the sensor's offset and divides by its sensitivity to get mA (sent over CAN).
 *============================================================================*/

#define LV_CURRENT_SENSOR_OFFSET_MV     330 // VS x 0.1, VS = 3.3V, A3U variant
#define LV_CURRENT_SENSOR_MV_PER_A      200 // A3 sensitivity (both B/U variants)

/*==============================================================================
 * === STATUS LEDS =============================================================
 * FAULT_LED blinks in STATE_FAULT. (ESTOP_LED and SUPP_LOW_LED are only on or
 * off, so they have no settings.)
 *============================================================================*/

#define FAULT_LED_BLINK_MS              200U // FAULT_LED toggle period

/*==============================================================================
 * === STATE TIMEOUTS ==========================================================
 * Time each state gets to meet its exit condition before it goes to STATE_FAULT,
 * counted from when the state starts, in state order.
 *============================================================================*/

#define TEL_STARTUP_TIMEOUT_MS          3000U
#define MST_HANDSHAKE_TIMEOUT_MS        5000U // MST holds its fault pin high ~2 s at startup
#define MST_STATUS_CHECK_TIMEOUT_MS     3000U
#define HV_CONNECT_TIMEOUT_MS           (3U * CONTACTOR_SETTLE_MS + 3000U) // 3 contactor waits + 3 s
#define OPEN_DISCHARGE_RELAY_TIMEOUT_MS 3000U
// Motor and MPPT precharge: 3 s after the minimum precharge time
#define MOTOR_PRECHARGE_TIMEOUT_MS      (MOTOR_PRECHARGE_MIN_MS + 3000U)
#define MPPT_PRECHARGE_TIMEOUT_MS       (MPPT_PRECHARGE_MIN_MS + 3000U)
#define LV_POWERUP_TIMEOUT_MS           3000U

/*==============================================================================
 * === FANS ====================================================================
 * The fans are driven by PWM on TIM4 channel 3. STATE_FANS_POWERUP runs them at
 * full speed, then drops them to half speed for the rest of the run.
 *============================================================================*/

// TIM4 CH3 PWM compare values (TIM4 period = 65535)
#define FANS_FULL_SPEED                 65535U // 100% duty
#define FANS_HALF_SPEED                 32767U // ~50% duty
#define FANS_FULL_SPEED_DURATION_MS     1000U  // Time at full speed before dropping to half speed

/*==============================================================================
 * === CONTACTORS ==============================================================
 * Contactors are the high-voltage switches, driven by output pins. NEG and POS
 * connect the pack (STATE_HV_CONNECT); LLIM connects the motor side and HLIM the
 * MPPT side, each after its precharge. A contactor needs time to settle after
 * each switch before the next step.
 *============================================================================*/

#define CONTACTOR_CLOSE                 GPIO_PIN_SET
#define CONTACTOR_OPEN                  GPIO_PIN_RESET

// Settling time between contactor steps (POS/NEG + LLIM/HLIM)
#define CONTACTOR_SETTLE_MS             200U

/*==============================================================================
 * === DISCHARGE RELAY =========================================================
 * A latching relay that keeps the motor side discharged while the car is off.
 * STATE_OPEN_DISCHARGE_RELAY waits until the motor side reads discharged, then
 * pulses DISCHARGE_TOGGLE_OFF to open it, so the motor side can be precharged.
 *============================================================================*/

#define DISCHARGE_COMPLETE_MV           100000U // TODO: dummy value, replace once known.
#define DISCHARGE_RELAY_PULSE_MS        250U    // DISCHARGE_TOGGLE_OFF pulse that opens the relay

/*==============================================================================
 * === PRECHARGE ===============================================================
 * Before LLIM / HLIM close, the motor and MPPT sides are charged slowly through
 * a precharge resistor, so closing the contactor does not cause a current
 * spike. A side is done when its voltage reaches PRECHARGE_PCT_OF_PACK % of the
 * MST pack voltage and its minimum time has passed.
 *============================================================================*/

// --- Motor and MPPT ---
#define PRECHARGE_SENSE_SCALE           61  // HV mV = ADC mV x this (used by adc_runtime)
#define PRECHARGE_PCT_OF_PACK           85U // Precharge complete at this % of MST pack voltage

// --- Motor ---
#define MOTOR_PRECHARGE_MIN_MS          15000U // Minimum precharge time, even if the voltage is reached

// --- MPPT ---
#define MPPT_PRECHARGE_MIN_MS           15000U // Minimum precharge time, even if the voltage is reached

/*==============================================================================
 * === DCDC ====================================================================
 * The DCDC turns pack voltage into the LV supply once the pack is connected.
 * Its thermistor temperature is checked in STATE_MONITORING.
 *============================================================================*/

#define DCDC_TEMP_MAX_MC                80000
