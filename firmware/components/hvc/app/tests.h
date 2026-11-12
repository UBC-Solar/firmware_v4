/**
 * @file tests.h
 * @brief HVC hardware tests (run instead of the FSM) and bench override code (run with the FSM).
 *
 * Only the entry points are public.
 * Every other function is static in tests.c, and most only compile when the switch they belong to
 * (HVC_TEST, FAKE_*, IGNORE_*) is on.
 */

#pragma once

// Standard library
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t

// Hardware tests: TESTS_RunSelected() from HVC_Main()
void TESTS_RunSelected(void);

// Bench overrides: TESTS_BenchRemoveFakedCanIds() and TESTS_BenchBlockFakedPinInterrupts() from
// HVC_Init(), TESTS_BenchFakeInputs() from HVC_Main(), TESTS_BenchPrintOverrides() from
// DEBUG_Output_Init()
size_t TESTS_BenchRemoveFakedCanIds(const uint16_t *ids, size_t count, uint16_t *kept_ids);
void TESTS_BenchBlockFakedPinInterrupts(void);
void TESTS_BenchFakeInputs(void);
void TESTS_BenchPrintOverrides(void);
