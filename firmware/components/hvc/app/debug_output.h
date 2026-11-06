/**
 * @file debug_output.h
 * @brief HVC debug output: the PRINT_* macros and the functions that print over SWO.
 *
 * Include it to print from any HVC app file. Each PRINT_* macro prints one tagged line, only
 * if its SHOW_* switch (hvc_defs.h DEBUG OUTPUT) is true. Also lists every function
 * debug_output.c defines, in the same sections and order (what each section is for, and the
 * rules that keep the output readable, are explained in debug_output.c).
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdint.h>

// Common
#include "debug_io.h"

// HVC app
#include "hvc_defs.h"  // SHOW_* switches
#include "hvc_types.h" // FsmState, FaultFlags

/*==============================================================================
 * === PRINT MACROS ============================================================
 * One per kind of message. Use them like printf, without "\r\n":
 *   PRINT_STEP("NEG closed");
 *   PRINT_STEP("motor side %ld mV", (long)g_shared.adc.motor_precharge_mv);
 * Each prints its tag, the message, and "\r\n" in one call, so a line is never
 * split. A message whose SHOW_* switch is false is not printed, and without
 * DEBUG none are compiled in. Only call them from main-loop code, never from an
 * interrupt (an interrupt sets a flag, and the main loop prints it).
 * To add a kind: add its SHOW_* switch in hvc_defs.h and its PRINT_* macro here.
 *============================================================================*/

// Prints "tag fmt\r\n" if show is true. Used by the PRINT_* macros below.
#define PRINT_TAGGED(show, tag, fmt, ...)                         \
    do                                                            \
    {                                                             \
        if (show)                                                 \
        {                                                         \
            DEBUG_IO_PRINT(tag " " fmt "\r\n", ##__VA_ARGS__);    \
        }                                                         \
    } while (0)

// --- Always useful (on by default) ---
#define PRINT_INIT(fmt, ...)  PRINT_TAGGED(SHOW_INIT,         "[INIT]",  fmt, ##__VA_ARGS__)
#define PRINT_BENCH(fmt, ...) PRINT_TAGGED(SHOW_BENCH,        "[BENCH]", fmt, ##__VA_ARGS__)
#define PRINT_TEST(fmt, ...)  PRINT_TAGGED(SHOW_TESTS,        "[TEST]",  fmt, ##__VA_ARGS__)
#define PRINT_STATE(fmt, ...) PRINT_TAGGED(SHOW_STATES,       "[STATE]", fmt, ##__VA_ARGS__)
#define PRINT_FAULT(fmt, ...) PRINT_TAGGED(SHOW_FAULTS,       "[FAULT]", fmt, ##__VA_ARGS__)

// --- For debugging (off by default) ---
#define PRINT_STEP(fmt, ...)  PRINT_TAGGED(SHOW_STEPS,        "[STEP]",  fmt, ##__VA_ARGS__)
#define PRINT_CAN(fmt, ...)   PRINT_TAGGED(SHOW_CAN,          "[CAN]",   fmt, ##__VA_ARGS__)
#define PRINT_MEAS(fmt, ...)  PRINT_TAGGED(SHOW_MEASUREMENTS, "[MEAS]",  fmt, ##__VA_ARGS__)
#define PRINT_PINS(fmt, ...)  PRINT_TAGGED(SHOW_PINS,         "[PINS]",  fmt, ##__VA_ARGS__)
#define PRINT_HB(fmt, ...)    PRINT_TAGGED(SHOW_HEARTBEATS,   "[HB]",    fmt, ##__VA_ARGS__)

// --- Last resort ---
// Printed whenever a dump runs: SHOW_DUMP_ON_FAULT and SHOW_DUMP_PERIODIC decide when it runs
#define PRINT_DUMP(fmt, ...)  PRINT_TAGGED(true,              "[DUMP]",  fmt, ##__VA_ARGS__)

/*==============================================================================
 * === PUBLIC API ==============================================================
 * Implemented in debug_output.c. DebugOutputInit() is called from HvcMainInit(),
 * DebugOutputRun() from FsmRun().
 *============================================================================*/

void DebugOutputInit(void);
void DebugOutputRun(void);

/*==============================================================================
 * === HELPERS =================================================================
 * Implemented in debug_output.c.
 *============================================================================*/

// --- Events ---
void PrintStateChange(FsmState old_state, FsmState new_state);
void PrintNewFaults(void);
void PrintFaultLines(bool in_dump);
uint32_t CountSetFlags(const volatile FaultFlags* flags);
void PrintFirstHeartbeats(void);

// --- Reports ---
void PrintNextReport(void);
void PrintMeasurementsReport(void);
void PrintPinsReport(void);
void PrintHeartbeatsReport(void);
void HeartbeatAgeText(char* text, size_t size, bool check_enabled, uint32_t last_tick);

// --- Dump ---
void StartDump(void);
bool PrintNextDumpSection(void);

// --- Names ---
const char* StateToString(FsmState state);
