/**
 * @file debug_output.c
 * @brief HVC debug output: startup lines, events, periodic reports, and the dump, over SWO.
 *
 * Every HVC app print goes through a PRINT_* macro (debug_output.h), one per kind of message.
 * Each kind has a SHOW_* switch in hvc_defs.h DEBUG OUTPUT, and every line starts with its tag:
 *   [INIT]               firmware version, reset cause, init done (DEBUG_Output_Init())
 *   [BENCH]              bench overrides on at startup, fake board events (tests.c)
 *   [TEST]               hardware test running, its results (tests.c)
 *   [STATE]              every state change (here)
 *   [FAULT]              each fault flag once, with its cause and value (here)
 *   [STEP]               steps inside each state (fsm.c)
 *   [CAN]                first heartbeat of each board (here), LV requests (can_messages.c)
 *   [MEAS] [PINS] [HB]   reports: values printed every REPORT_INTERVAL_MS (here)
 *   [DUMP]               every value the HVC holds, as a last resort (here)
 *
 * Rules that keep the output readable:
 *   Events (states, faults, steps) print once, when they happen.
 *   Values that keep changing only print in the reports and the dump.
 *   At most one report or dump line per pass: the dump prints one section per pass, and
 *   reports that are due wait their turn.
 *   Nothing prints from an interrupt: an interrupt sets a flag, and the main loop prints.
 *   Each line is one print call, so lines never mix. (Driver and runtime prints are not
 *   tagged. i2c_driver.c can print from its interrupt, so its line can land inside another.)
 *
 * Sections, in order:
 *   PUBLIC API: DEBUG_Output_Init() (from HVC_Init()) and DEBUG_Output_Run() (from FSM_Run()).
 *   HELPERS:    events, reports, dump, names.
 */

#include "debug_output.h"

// Standard library
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h> // snprintf

// HAL and CubeMX
#include "stm32f1xx_hal.h"

// Common
#include "git_version.h" // GIT_VERSION_HASH, GIT_VERSION_DIRTY

// Runtime
#include "shunt_runtime.h"

// HVC app
#include "fsm.h"
#include "hvc_defs.h"
#include "tests.h" // TESTS_BenchPrintOverrides()

#ifdef DEBUG
// Events
static void PrintStateChange(FsmState old_state, FsmState new_state);
static void PrintNewFaults(void);
static void PrintFaultLines(bool in_dump);
static uint32_t CountSetFlags(const volatile FaultFlags *flags);
static void PrintFirstHeartbeats(void);

// Reports
static void PrintNextReport(void);
static void PrintMeasurementsReport(void);
static void PrintPinsReport(void);
static void PrintHeartbeatsReport(void);
static void HeartbeatAgeText(char *text, size_t size, bool check_enabled, uint32_t last_tick);

// Dump
static void StartDump(void);
static bool PrintNextDumpSection(void);

// Names
static const char *StateToString(FsmState state);
#endif // DEBUG

// ==== Public API ====
// DEBUG_Output_Init() runs once, at the end of HVC_Init().
// DEBUG_Output_Run() runs first in every FSM_Run() pass (not during a hardware test).
// Without DEBUG both do nothing.

/**
 * @brief Prints the startup lines. Call once, at the end of HVC_Init().
 *
 * In order: firmware version, reset cause, the bench overrides that are on
 * (TESTS_BenchPrintOverrides(), tests.c), then "done".
 * Runs after FSM_Init(), which records a watchdog reset.
 * A hardware test prints its own name on its first pass (tests.c).
 */
void DEBUG_Output_Init(void) {
#ifdef DEBUG
    PRINT_INIT("firmware %08lX%s", (unsigned long)GIT_VERSION_HASH,
               GIT_VERSION_DIRTY ? "-dirty" : "");
    PRINT_INIT("reset cause: %s", g_fault_flags.watchdog_fired ? "watchdog" : "normal");
    TESTS_BenchPrintOverrides();
    PRINT_INIT("done");
#endif // DEBUG
}

/**
 * @brief Prints what happened since the last pass, then at most one report or dump line. Call
 *        first in FSM_Run(), every pass.
 *
 * Events first (state change, new faults, first heartbeats), so they are never delayed.
 * Runs before CheckHeartbeats() and the state functions, so it sees the _received flags before the
 * main loop clears them.
 * Then starts a dump if one is due, and prints the next dump section, or if no dump is running, the
 * next report that is due.
 */
void DEBUG_Output_Run(void) {
#ifdef DEBUG
    static FsmState last_state = STATE_UNKNOWN;

    FsmState state = g_state; // Read once, so every print this pass uses the same state
    bool fault_started = (state == STATE_FAULT) && (last_state != STATE_FAULT);

    // Events: each prints once, when it happens
    PrintStateChange(last_state, state);
    PrintNewFaults(); // After the state change, so the causes follow "-> FAULT"
    PrintFirstHeartbeats();
    last_state = state;

    // Dump: started by its switches, then one section per pass
    if (SHOW_DUMP_ON_FAULT && fault_started) {
        StartDump();
    }
    if (SHOW_DUMP_PERIODIC && FSM_TimerElapsed(DUMP_INTERVAL_MS, &g_ticks.dump)) {
        StartDump();
    }

    // At most one repeating line per pass: a running dump goes first, reports wait for it
    if (!PrintNextDumpSection()) {
        PrintNextReport();
    }
#endif // DEBUG
}

// ==== Helpers ====
// Used by the entry points above, grouped by what they print.
// Only compiled with DEBUG: nothing calls them otherwise.

#ifdef DEBUG

// --- Events ---

static FaultFlags printed_faults = {0}; // Flags whose [FAULT] line was already printed

/**
 * @brief Prints "old -> new" when the state has changed, or "start in new" on the first pass.
 *        Prints nothing if they match.
 *
 * @param old_state State last pass (STATE_UNKNOWN on the first pass).
 * @param new_state State this pass.
 */
static void PrintStateChange(FsmState old_state, FsmState new_state) {
    if (new_state == old_state) {
        return;
    }

    if (old_state == STATE_UNKNOWN) {
        PRINT_STATE("start in %s", StateToString(new_state));
    } else {
        PRINT_STATE("%s -> %s", StateToString(old_state), StateToString(new_state));
    }
}

/**
 * @brief Prints each fault flag that is set, once, with its cause and the value behind it.
 *
 * Waits until the FSM is in STATE_FAULT, so the causes always print right after "-> FAULT" (a flag
 * set by an interrupt reaches STATE_FAULT one pass later).
 * A flag set after that prints on the pass it is set.
 * Then warns once if a set flag has no line in PrintFaultLines(), so a new flag that was not given
 * a line does not go unnoticed.
 */
static void PrintNewFaults(void) {
    static bool missing_line_warned = false;

    if (g_state != STATE_FAULT) {
        return;
    }

    // Counted before printing: a flag an interrupt sets during PrintFaultLines() is not counted
    // yet, so it cannot look like a flag with no line
    uint32_t set_count = CountSetFlags(&g_fault_flags);

    PrintFaultLines(false);

    if (!missing_line_warned && (set_count > CountSetFlags(&printed_faults))) {
        missing_line_warned = true;
        PRINT_FAULT("a fault flag is set that has no line in PrintFaultLines()");
    }
}

// Prints flag's line. In a dump: whenever it is set. Otherwise: only the first time it is set.
#define PRINT_FAULT_LINE(flag, fmt, ...)                                  \
    do {                                                                  \
        if (g_fault_flags.flag && (in_dump || !printed_faults.flag)) {    \
            if (in_dump) {                                                \
                PRINT_DUMP("fault " #flag ": " fmt, ##__VA_ARGS__);       \
            } else {                                                      \
                printed_faults.flag = true;                               \
                PRINT_FAULT(#flag ": " fmt, ##__VA_ARGS__);               \
            }                                                             \
        }                                                                 \
    } while (0)

/**
 * @brief The one line per fault flag: its name, cause, and the value behind it.
 *
 * Used by PrintNewFaults() ([FAULT], each flag once) and by the dump ([DUMP], every set flag).
 * Values are read on the pass the line prints.
 * In FaultFlags order.
 * To add a fault flag: add its line here.
 *
 * @param in_dump true to print every set flag as a dump line, false to print only new ones.
 */
static void PrintFaultLines(bool in_dump) {
    // General faults
    PRINT_FAULT_LINE(estop, "ESTOP pressed");
    PRINT_FAULT_LINE(imd_fault, "IMD reports an insulation fault");
    PRINT_FAULT_LINE(mst_fault, "MASTERBOARD_FAULT went high");
    PRINT_FAULT_LINE(mst_unhealthy, "MST heartbeat reports errors, faults, or warnings");
    PRINT_FAULT_LINE(overcurrent, "shunt %ld mA, limit %u A",
                     (long)SHUNT_GetShuntCurrent_mA(), SHUNT_OVERCURRENT_A);
    PRINT_FAULT_LINE(reverse_current, "shunt %ld mA, limit -%u A",
                     (long)SHUNT_GetShuntCurrent_mA(), SHUNT_REVERSE_CURRENT_A);
    PRINT_FAULT_LINE(dist_fault, "DIST sent a fault message");
    PRINT_FAULT_LINE(dcdc_fault, "DCDC_ACTIVE went high (DCDC stopped supplying LV)");
    PRINT_FAULT_LINE(supp_fault, "SUPP_ACTIVE high at startup (supp battery not in use)");
    PRINT_FAULT_LINE(watchdog_fired, "watchdog reset: a pass took too long");
    PRINT_FAULT_LINE(dcdc_overtemp, "DCDC %ld mC, limit %d mC",
                     (long)g_shared.adc.dcdc_temp_mc, (int)DCDC_TEMP_MAX_MC);

    // Timeout faults for startup states
    PRINT_FAULT_LINE(tel_startup_timeout, "no TEL heartbeat in %u ms (DIST powers TEL)",
                     TEL_STARTUP_TIMEOUT_MS);
    PRINT_FAULT_LINE(mst_handshake_timeout,
                     "MASTERBOARD_FAULT did not go high, then low, in %u ms",
                     MST_HANDSHAKE_TIMEOUT_MS);
    PRINT_FAULT_LINE(mst_status_check_timeout, "no healthy MST heartbeat in %u ms",
                     MST_STATUS_CHECK_TIMEOUT_MS);
    PRINT_FAULT_LINE(hv_connect_timeout, "DCDC_ACTIVE %d, SUPP_ACTIVE %d (needs 0, 1)",
                     (int)g_shared.pins.dcdc_active, (int)g_shared.pins.supp_active);
    PRINT_FAULT_LINE(open_discharge_relay_timeout, "motor side %ld mV, needs below %u mV",
                     (long)g_shared.adc.motor_precharge_mv, DISCHARGE_COMPLETE_MV);
    PRINT_FAULT_LINE(motor_precharge_timeout, "motor side %ld mV, needs %ld mV",
                     (long)g_shared.adc.motor_precharge_mv,
                     (long)((int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100));
    PRINT_FAULT_LINE(mppt_precharge_timeout, "MPPT side %ld mV, needs %ld mV",
                     (long)g_shared.adc.mppt_precharge_mv,
                     (long)((int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100));
    PRINT_FAULT_LINE(lv_powerup_timeout, "no LV power-up ack from DIST in %u ms",
                     LV_POWERUP_TIMEOUT_MS);

    // Heartbeat timeout faults
    PRINT_FAULT_LINE(tel_heartbeat_timeout, "no TEL heartbeat for %u ms",
                     HEARTBEAT_TIMEOUT_MS);
    PRINT_FAULT_LINE(mst_heartbeat_timeout, "no MST heartbeat for %u ms",
                     HEARTBEAT_TIMEOUT_MS);
    PRINT_FAULT_LINE(dist_heartbeat_timeout, "no DIST heartbeat for %u ms",
                     HEARTBEAT_TIMEOUT_MS);
}

/**
 * @brief Counts the flags set in a FaultFlags.
 *
 * Reads it as an array of bools, like FSM_AnyFaultActive(), so new flags are counted automatically.
 *
 * @param flags Flags to count.
 * @return Number of flags that are true.
 */
static uint32_t CountSetFlags(const volatile FaultFlags *flags) {
    const volatile bool *flag = (const volatile bool*)flags;
    uint32_t count = 0U;

    for (uint32_t i = 0U; i < sizeof(FaultFlags); i++) {
        if (flag[i]) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Prints the first heartbeat received from TEL, DIST, and MST, once each.
 *
 * Reads the _received flags set by the CAN receive interrupt, but doesn't clear them.
 * They belong to the FSM.
 * DEBUG_Output_Run() runs before the FSM clears them.
 */
static void PrintFirstHeartbeats(void) {
    static bool tel_printed = false;
    static bool dist_printed = false;
    static bool mst_printed = false;

    if (!tel_printed && g_shared.tel_heartbeat_received) {
        tel_printed = true;
        PRINT_CAN("first TEL heartbeat");
    }
    if (!dist_printed && g_shared.dist_heartbeat_received) {
        dist_printed = true;
        PRINT_CAN("first DIST heartbeat");
    }
    if (!mst_printed && g_shared.mst_heartbeat_received) {
        mst_printed = true;
        PRINT_CAN("first MST heartbeat (%s)",
                  g_shared.mst_healthy ? "healthy" : "reports a problem");
    }
}

// --- Reports ---

/**
 * @brief Every REPORT_INTERVAL_MS, marks each report switched on as due, then prints one due
 *        report per pass.
 *
 * One per pass, so the reports never print as one burst of lines.
 * To add a report: add its SHOW_* switch, its _due flag here, and its Print...Report().
 */
static void PrintNextReport(void) {
    static bool measurements_due = false;
    static bool pins_due = false;
    static bool heartbeats_due = false;

    if (FSM_TimerElapsed(REPORT_INTERVAL_MS, &g_ticks.report)) {
        measurements_due = SHOW_MEASUREMENTS;
        pins_due = SHOW_PINS;
        heartbeats_due = SHOW_HEARTBEATS;
    }

    if (measurements_due) {
        measurements_due = false;
        PrintMeasurementsReport();
    } else if (pins_due) {
        pins_due = false;
        PrintPinsReport();
    } else if (heartbeats_due) {
        heartbeats_due = false;
        PrintHeartbeatsReport();
    }
}

/**
 * @brief Prints this pass's measurements: ADC values, MST pack voltage, and shunt current.
 *
 * Starts with the time and state, so each line can be read on its own.
 */
static void PrintMeasurementsReport(void) {
    PRINT_MEAS("%lu ms %s | supp %ld mV | LV %ld mA | DCDC %ld mC | motor %ld mV | "
               "MPPT %ld mV | pack %ld mV | shunt %ld mA",
               (unsigned long)HAL_GetTick(), StateToString(g_state),
               (long)g_shared.adc.supp_mv, (long)g_shared.adc.lv_current_ma,
               (long)g_shared.adc.dcdc_temp_mc, (long)g_shared.adc.motor_precharge_mv,
               (long)g_shared.adc.mppt_precharge_mv, (long)g_shared.mst_pack_voltage_mv,
               (long)SHUNT_GetShuntCurrent_mA());
}

/**
 * @brief Prints this pass's input pin levels (1 = high), named after their pins.
 */
static void PrintPinsReport(void) {
    PRINT_PINS("ESTOP %d | IMD %d | HV_CURRENT_ALERT %d | MASTERBOARD_FAULT %d | "
               "DCDC_ACTIVE %d | SUPP_ACTIVE %d",
               (int)g_shared.pins.estop, (int)g_shared.pins.imd_fault,
               (int)g_shared.pins.hv_current_alert, (int)g_shared.pins.mst_fault,
               (int)g_shared.pins.dcdc_active, (int)g_shared.pins.supp_active);
}

/**
 * @brief Prints how long since each board's last heartbeat, or "check off" before its
 *        heartbeat check is enabled.
 */
static void PrintHeartbeatsReport(void) {
    char tel[16];
    char dist[16];
    char mst[16];

    HeartbeatAgeText(tel, sizeof(tel), g_shared.tel_dist_heartbeat_check_enabled,
                     g_ticks.tel_heartbeat);
    HeartbeatAgeText(dist, sizeof(dist), g_shared.tel_dist_heartbeat_check_enabled,
                     g_ticks.dist_heartbeat);
    HeartbeatAgeText(mst, sizeof(mst), g_shared.mst_heartbeat_check_enabled,
                     g_ticks.mst_heartbeat);
    PRINT_HB("since last heartbeat: TEL %s | DIST %s | MST %s", tel, dist, mst);
}

/**
 * @brief Writes "<ms> ms" since last_tick into text, or "check off" if check_enabled is false.
 *
 * @param text          Gets the text.
 * @param size          Size of text, in bytes.
 * @param check_enabled The board's heartbeat check is enabled.
 * @param last_tick     The board's heartbeat timer (g_ticks).
 */
static void HeartbeatAgeText(char *text, size_t size, bool check_enabled, uint32_t last_tick) {
    if (check_enabled) {
        snprintf(text, size, "%lu ms", (unsigned long)(HAL_GetTick() - last_tick));
    } else {
        snprintf(text, size, "check off");
    }
}

// --- Dump ---

// Dump sections, in print order. One prints per pass.
typedef enum {
    DUMP_BEGIN = 0,
    DUMP_FAULTS,
    DUMP_ADC,
    DUMP_CAN_RX,
    DUMP_PINS,
    DUMP_FSM_FLAGS,
    DUMP_TIMERS,
    DUMP_SHUNT,
    DUMP_END,

    DUMP_IDLE, // Not a section: no dump running
} DumpSection;

static DumpSection next_dump_section = DUMP_IDLE;

/**
 * @brief Starts a dump on the next pass. Does nothing if a dump is already running.
 */
static void StartDump(void) {
    if (next_dump_section == DUMP_IDLE) {
        next_dump_section = DUMP_BEGIN;
    }
}

/**
 * @brief Prints the next section of the running dump.
 *
 * Every value the HVC holds, as a last resort when the reports are not enough.
 * One section per pass, so a dump never holds up the main loop.
 * Values are this pass's.
 * To add a value: add it to the section it belongs to (or add a section to DumpSection).
 *
 * @return true if a section was printed, false if no dump is running.
 */
static bool PrintNextDumpSection(void) {
    uint32_t now_ms = HAL_GetTick();

    switch (next_dump_section) {
        case DUMP_BEGIN:
            PRINT_DUMP("begin: %lu ms, state %s, firmware %08lX%s", (unsigned long)now_ms,
                       StateToString(g_state), (unsigned long)GIT_VERSION_HASH,
                       GIT_VERSION_DIRTY ? "-dirty" : "");
            break;
        case DUMP_FAULTS:
            if (FSM_AnyFaultActive()) {
                PrintFaultLines(true); // The same lines as [FAULT], for every flag set now
            } else {
                PRINT_DUMP("faults: none");
            }
            break;
        case DUMP_ADC:
            PRINT_DUMP("adc: supp %ld mV, LV current %ld mA, DCDC %ld mC, motor side %ld mV, "
                       "MPPT side %ld mV",
                       (long)g_shared.adc.supp_mv, (long)g_shared.adc.lv_current_ma,
                       (long)g_shared.adc.dcdc_temp_mc, (long)g_shared.adc.motor_precharge_mv,
                       (long)g_shared.adc.mppt_precharge_mv);
            break;
        case DUMP_CAN_RX:
            PRINT_DUMP("can rx: MST pack %ld mV, mst_healthy %d, not handled yet: "
                       "tel_heartbeat %d, dist_heartbeat %d, mst_heartbeat %d, lv_powerup %d",
                       (long)g_shared.mst_pack_voltage_mv, (int)g_shared.mst_healthy,
                       (int)g_shared.tel_heartbeat_received,
                       (int)g_shared.dist_heartbeat_received,
                       (int)g_shared.mst_heartbeat_received, (int)g_shared.lv_powerup_received);
            break;
        case DUMP_PINS:
            PRINT_DUMP("pins: ESTOP %d, IMD %d, HV_CURRENT_ALERT %d, MASTERBOARD_FAULT %d, "
                       "DCDC_ACTIVE %d, SUPP_ACTIVE %d",
                       (int)g_shared.pins.estop, (int)g_shared.pins.imd_fault,
                       (int)g_shared.pins.hv_current_alert, (int)g_shared.pins.mst_fault,
                       (int)g_shared.pins.dcdc_active, (int)g_shared.pins.supp_active);
            break;
        case DUMP_FSM_FLAGS:
            PRINT_DUMP("fsm flags: mst_irq_armed %d, dcdc_irq_armed %d, "
                       "tel_dist_heartbeat_check_enabled %d, mst_heartbeat_check_enabled %d",
                       (int)g_shared.mst_irq_armed, (int)g_shared.dcdc_irq_armed,
                       (int)g_shared.tel_dist_heartbeat_check_enabled,
                       (int)g_shared.mst_heartbeat_check_enabled);
            break;
        case DUMP_TIMERS:
            PRINT_DUMP("timers (ms since each one last started): state_timeout %lu, "
                       "state_delay %lu, lv_powerup_retry %lu, fault_led %lu, "
                       "tel_heartbeat %lu, dist_heartbeat %lu, mst_heartbeat %lu, can_tx %lu, "
                       "hvc_heartbeat %lu",
                       (unsigned long)(now_ms - g_ticks.state_timeout),
                       (unsigned long)(now_ms - g_ticks.state_delay),
                       (unsigned long)(now_ms - g_ticks.lv_powerup_retry),
                       (unsigned long)(now_ms - g_ticks.fault_led),
                       (unsigned long)(now_ms - g_ticks.tel_heartbeat),
                       (unsigned long)(now_ms - g_ticks.dist_heartbeat),
                       (unsigned long)(now_ms - g_ticks.mst_heartbeat),
                       (unsigned long)(now_ms - g_ticks.can_tx),
                       (unsigned long)(now_ms - g_ticks.hvc_heartbeat));
            break;
        case DUMP_SHUNT:
            PRINT_DUMP("shunt (last read): %ld nV, %ld mA", (long)SHUNT_GetShuntVoltage_nV(),
                       (long)SHUNT_GetShuntCurrent_mA());
            break;
        case DUMP_END:
            PRINT_DUMP("end");
            break;
        case DUMP_IDLE:
        default:
            return false;
    }

    next_dump_section = (DumpSection)(next_dump_section + 1);
    return true;
}

// --- Names ---

/**
 * @brief Gets the name of an FSM state, for debug prints.
 *
 * The returned strings are literals stored in flash, so no memory is allocated.
 *
 * @param state State to name.
 * @return The state's name without the STATE_ prefix, or "UNKNOWN" for any value not in
 *         FsmState.
 */
static const char *StateToString(FsmState state) {
    switch (state) {
        case STATE_RESET:                return "RESET";
        case STATE_TEL_STARTUP:          return "TEL_STARTUP";
        case STATE_MST_HANDSHAKE:        return "MST_HANDSHAKE";
        case STATE_MST_STATUS_CHECK:     return "MST_STATUS_CHECK";
        case STATE_FANS_POWERUP:         return "FANS_POWERUP";
        case STATE_HV_CONNECT:           return "HV_CONNECT";
        case STATE_OPEN_DISCHARGE_RELAY: return "OPEN_DISCHARGE_RELAY";
        case STATE_MOTOR_PRECHARGE:      return "MOTOR_PRECHARGE";
        case STATE_MPPT_PRECHARGE:       return "MPPT_PRECHARGE";
        case STATE_CLOSE_LLIM:           return "CLOSE_LLIM";
        case STATE_CLOSE_HLIM:           return "CLOSE_HLIM";
        case STATE_LV_POWERUP:           return "LV_POWERUP";
        case STATE_MONITORING:           return "MONITORING";
        case STATE_FAULT:                return "FAULT";
        default:                         return "UNKNOWN";
    }
}

#endif // DEBUG
