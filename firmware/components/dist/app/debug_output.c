/**
 * @file debug_output.c
 * @brief DIST debug output: startup lines, events, and periodic reports, over SWO.
 *
 * Every DIST app print goes through a PRINT_* macro (debug_output.h), one per kind of message.
 * Each kind has a SHOW_* switch in dist_defs.h DEBUG OUTPUT, and every line starts with its tag:
 *   [INIT]         firmware version, LED panel found or not (DEBUG_Output_Init())
 *   [BENCH]        bench overrides that are on (DEBUG_Output_Init())
 *   [STATE]        every state change (here)
 *   [FAULT]        each fault flag once, when it is first set (here)
 *   [STEP]         steps inside each state (fsm.c)
 *   [MEAS] [PINS]  reports: values printed every REPORT_INTERVAL_MS (here)
 *
 * Nothing prints from an interrupt: an interrupt sets a flag, and the main loop prints.
 *
 * Sections, in order:
 *   PUBLIC API: DEBUG_Output_Init() (from DIST_Init()) and DEBUG_Output_Run() (from FSM_Run()).
 *   HELPERS:    events, reports, names.
 */

#include "debug_output.h"

// Standard library
#include <stdbool.h>
#include <stdint.h>

// HAL and CubeMX
#include "stm32f1xx_hal.h"

// Common
#include "git_version.h" // GIT_VERSION_HASH, GIT_VERSION_DIRTY

// DIST app
#include "dist_defs.h"
#include "fsm.h"

#ifdef DEBUG
static void PrintStateChange(void);
static void PrintNewFaults(void);
static void PrintReports(void);
static const char *StateToString(FsmState state);
#endif // DEBUG

// ==== Public API ====
// DEBUG_Output_Init() runs once, at the end of DIST_Init().
// DEBUG_Output_Run() runs first in every FSM_Run() pass.
// Without DEBUG both do nothing.

/**
 * @brief Prints the startup lines: firmware version, whether the LED panel was found, and every
 *        bench override that is on.
 *
 * @param led_panel_found true if LED_Init() found the IS31FL3236 LED panel.
 */
void DEBUG_Output_Init(bool led_panel_found) {
#ifdef DEBUG
    PRINT_INIT("firmware %08lX%s", (unsigned long)GIT_VERSION_HASH,
               GIT_VERSION_DIRTY ? "-dirty" : "");
    PRINT_INIT("LED panel %s", led_panel_found ? "found" : "not found, running without it");

    if (IGNORE_ESTOP) {
        PRINT_BENCH("IGNORE_ESTOP on: ESTOP does not fault");
    }
    if (KEEP_SPARE_MUX_OFF) {
        PRINT_BENCH("KEEP_SPARE_MUX_OFF on: spare mux eFuse stays off");
    }
#else // DEBUG
    (void)led_panel_found;
#endif // DEBUG
}

/**
 * @brief Prints this pass's events (state change, new faults), then the reports if they are due.
 *
 * Call first in every FSM_Run() pass.
 *
 * Side effects: updates g_ticks.report.
 */
void DEBUG_Output_Run(void) {
#ifdef DEBUG
    PrintStateChange();
    PrintNewFaults();
    PrintReports();
#endif // DEBUG
}

// ==== Helpers ====
// Only compiled with DEBUG: nothing calls them otherwise.

#ifdef DEBUG

// --- Events ---

/**
 * @brief Prints the state change since the last pass, if there was one.
 */
static void PrintStateChange(void) {
    static FsmState printed_state = STATE_UNKNOWN;

    if (g_state != printed_state) {
        PRINT_STATE("%s -> %s", StateToString(printed_state), StateToString(g_state));
        printed_state = g_state;
    }
}

/**
 * @brief Prints each fault flag the first pass it is seen set.
 *
 * Fault flags are never cleared, so each one prints once.
 */
static void PrintNewFaults(void) {
    // Same order as FaultFlags (dist_types.h)
    static const char *const fault_names[] = {
        "ESTOP pressed",
        "HVC fault message received",
        "DRD eFuse tripped",
        "MDI eFuse tripped",
        "SPARE eFuse tripped",
        "SPARE_CTRL eFuse tripped",
    };
    _Static_assert(sizeof(fault_names) / sizeof(fault_names[0]) == sizeof(FaultFlags),
                   "fault_names must list every FaultFlags field, in order");

    static bool printed[sizeof(FaultFlags)] = {false};
    const volatile bool *flag = (const volatile bool*)&g_fault_flags;

    for (uint32_t i = 0U; i < sizeof(FaultFlags); i++) {
        if (flag[i] && !printed[i]) {
            PRINT_FAULT("%s", fault_names[i]);
            printed[i] = true;
        }
    }
}

// --- Reports ---

/**
 * @brief Prints the currents ([MEAS]) and input pin levels ([PINS]) every REPORT_INTERVAL_MS.
 *
 * Uses this pass's g_shared.adc and g_shared.pins.
 * Pin levels are printed as read: ESTOP and the eFuse FAULT pins are active-low (0 = asserted).
 */
static void PrintReports(void) {
    if (!FSM_TimerElapsed(REPORT_INTERVAL_MS, &g_ticks.report)) {
        return;
    }

    PRINT_MEAS("DRD=%ldmA MDI=%ldmA SPARE_CTRL=%ldmA SPARE_MUX=%ldmA SPARE=%ldmA TEL=%ldmA",
               (long)g_shared.adc.drd_ma,
               (long)g_shared.adc.mdi_ma,
               (long)g_shared.adc.spare_ctrl_ma,
               (long)g_shared.adc.spare_mux_ma,
               (long)g_shared.adc.spare_ma,
               (long)g_shared.adc.tel_ma);

    PRINT_PINS("ESTOP=%d MUX_STATUS=%d DRD_FUSE=%d MDI_FUSE=%d SPARE_FUSE=%d SPARE_CTRL_FUSE=%d",
               (int)g_shared.pins.estop,
               (int)g_shared.pins.mux_status,
               (int)g_shared.pins.drd_fuse,
               (int)g_shared.pins.mdi_fuse,
               (int)g_shared.pins.spare_fuse,
               (int)g_shared.pins.spare_ctrl_fuse);
}

// --- Names ---

/**
 * @brief Gives the printable name of a state.
 * @param state The state.
 * @return Its name, without the STATE_ prefix.
 */
static const char *StateToString(FsmState state) {
    switch (state) {
        case STATE_STARTUP:
            return "STARTUP";

        case STATE_ENABLE_EFUSES:
            return "ENABLE_EFUSES";

        case STATE_NORMAL:
            return "NORMAL";

        case STATE_FAULT:
            return "FAULT";

        case STATE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

#endif // DEBUG
