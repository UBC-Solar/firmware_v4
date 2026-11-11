/**
 * @file tests.h
 * @brief HVC hardware tests (run instead of the FSM) and bench override code (run with the FSM).
 *
 * Lists every function in tests.c, grouped like it. Only the entry points are called from
 * outside tests.c: from hvc_main.c, and BenchPrintOverrides() from debug_output.c. The rest
 * are called inside tests.c.
 * A function under a switch that is off (HVC_TEST, FAKE_*, IGNORE_*) is listed here but not
 * compiled, so calling it does not link.
 */

#pragma once

// Standard library
#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t

// HAL and CubeMX
#include "stm32f1xx_hal.h" // GPIO_PinState

/*==============================================================================
 * === HARDWARE TESTS ==========================================================
 * Implemented in tests.c (see its HARDWARE TESTS banner). Entry point:
 * TestRunSelected() (from HVC_Main()).
 *============================================================================*/

// --- Entry point ---
void TestRunSelected(void);

// --- Helpers ---
void PrintTestStart(const char* test_name);

// --- Unit tests ---
void TestUnitShunt(void);

// --- Integration tests ---
void TestIntCan(void);
void SendDebugCanMessage(void);

/*==============================================================================
 * === BENCH OVERRIDES =========================================================
 * Implemented in tests.c (see its BENCH OVERRIDES banner). Entry points:
 * BenchRemoveFakedCanIds() and BenchBlockFakedPinInterrupts() (from
 * HVC_Init()), BenchFakeInputs() (from HVC_Main()), BenchPrintOverrides()
 * (from DebugOutputInit()).
 *============================================================================*/

// --- Entry points ---
size_t BenchRemoveFakedCanIds(const uint16_t* ids, size_t count, uint16_t* kept_ids);
void BenchBlockFakedPinInterrupts(void);
void BenchFakeInputs(void);
void BenchPrintOverrides(void);

// --- Helpers ---
uint32_t TimeSince(uint32_t start_tick);

// --- MST ---
// Fake parts
GPIO_PinState FakeMstFaultPin(void);
void FakeMstFaultCall(void);
void FakeMstHeartbeat(void);
void FakeMstPackVoltage(void);
// Hooks
bool IsFakedMstCanId(uint16_t id);
void BlockMstPinInterrupts(void);
void FakeMstInputs(void);
bool PrintMstOverrides(void);

// --- TEL ---
// TODO: IsFakedTelCanId(), FakeTelInputs(), PrintTelOverrides()

// --- DIST ---
// TODO: IsFakedDistCanId(), FakeDistInputs(), PrintDistOverrides()

// --- HVC ---
// Fake parts
void FakePrechargeVoltage(void);
// Hooks
void FakeHvcInputs(void);
bool PrintHvcOverrides(void);
