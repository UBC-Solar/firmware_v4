/**
 * @file debug_output.h
 * @brief DIST debug output: the PRINT_* macros and the functions that print over SWO.
 *
 * Include it to print from any DIST app file.
 * Each PRINT_* macro prints one tagged line, only if its SHOW_* switch (dist_defs.h, debug output
 * section) is true.
 */

#pragma once

// Standard library
#include <stdbool.h>

// Common
#include "debug_io.h"

// DIST app
#include "dist_defs.h" // SHOW_* switches

// ==== Print macros ====
// One per kind of message.
// Use them like printf, without "\r\n":
//   PRINT_STEP("eFuses on");
// Each prints its tag, the message, and "\r\n" in one call, so a line is never split.
// A message whose SHOW_* switch is false is not printed, and without DEBUG none are compiled in.
// Only call them from main-loop code, never from an interrupt (an interrupt sets a flag, and the
// main loop prints it).
// To add a kind: add its SHOW_* switch in dist_defs.h and its PRINT_* macro here.

// Prints "tag fmt\r\n" if show is true. Used by the PRINT_* macros below.
#define PRINT_TAGGED(show, tag, fmt, ...)                         \
    do {                                                          \
        if (show) {                                               \
            DEBUG_IO_PRINT(tag " " fmt "\r\n", ##__VA_ARGS__);    \
        }                                                         \
    } while (0)

// --- Always useful (on by default) ---
#define PRINT_INIT(fmt, ...)  PRINT_TAGGED(SHOW_INIT,         "[INIT]",  fmt, ##__VA_ARGS__)
#define PRINT_BENCH(fmt, ...) PRINT_TAGGED(SHOW_BENCH,        "[BENCH]", fmt, ##__VA_ARGS__)
#define PRINT_STATE(fmt, ...) PRINT_TAGGED(SHOW_STATES,       "[STATE]", fmt, ##__VA_ARGS__)
#define PRINT_FAULT(fmt, ...) PRINT_TAGGED(SHOW_FAULTS,       "[FAULT]", fmt, ##__VA_ARGS__)
#define PRINT_MEAS(fmt, ...)  PRINT_TAGGED(SHOW_MEASUREMENTS, "[MEAS]",  fmt, ##__VA_ARGS__)
#define PRINT_PINS(fmt, ...)  PRINT_TAGGED(SHOW_PINS,         "[PINS]",  fmt, ##__VA_ARGS__)

// --- For debugging (off by default) ---
#define PRINT_STEP(fmt, ...)  PRINT_TAGGED(SHOW_STEPS,        "[STEP]",  fmt, ##__VA_ARGS__)

// DEBUG_Output_Init() from DIST_Init(), DEBUG_Output_Run() from FSM_Run()
void DEBUG_Output_Init(bool led_panel_found);
void DEBUG_Output_Run(void);
