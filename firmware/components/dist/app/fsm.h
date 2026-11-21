/**
 * @file fsm.h
 * @brief Public interface for the DIST finite state machine
 *
 * Include it to use the DIST-wide variables (g_state, g_fault_flags, g_shared, g_ticks) or the
 * public FSM functions.
 * The states and internal checks are static in fsm.c.
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// DIST app
#include "dist_types.h"

// DIST-wide variables, defined in fsm.c

extern volatile FsmState   g_state;
extern volatile FaultFlags g_fault_flags;
extern SharedData          g_shared;
extern Ticks               g_ticks;

// Called from dist_main.c
void FSM_Init(void);
void FSM_Run(void);

// Called from the eFuse FAULT pin interrupts (gpio_driver.c)
void FSM_EfuseFaultCallback(uint16_t fuse_pin);

// Helpers used across the DIST app
bool FSM_TimerElapsed(uint32_t interval_ms, uint32_t *last_tick);
bool FSM_AnyFaultActive(void);
void FSM_EnableAllEfuses(void);
void FSM_DisableAllEfuses(void);
