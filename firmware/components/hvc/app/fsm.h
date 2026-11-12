/**
 * @file fsm.h
 * @brief Public interface for the HVC finite state machine
 *
 * Include it to use the HVC-wide variables (g_state, g_fault_flags, g_shared, g_ticks) or the
 * public FSM functions.
 * The states and internal checks are static in fsm.c.
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// HVC app
#include "hvc_types.h"

// HVC-wide variables, defined in fsm.c

extern volatile FsmState   g_state;
extern volatile FaultFlags g_fault_flags;
extern SharedData          g_shared;
extern Ticks               g_ticks;

// Called from hvc_main.c
void FSM_Init(void);
void FSM_Run(void);

// Called from the fault pin interrupts (gpio_driver.c)
void FSM_EstopCallback(void);
void FSM_ImdFaultCallback(void);
void FSM_MstFaultCallback(bool armed);
void FSM_HvCurrentAlertCallback(void);
void FSM_DcdcFaultCallback(bool armed);

// Helpers used across the HVC app
bool FSM_TimerElapsed(uint32_t interval_ms, uint32_t *last_tick);
bool FSM_AnyFaultActive(void);
void FSM_OpenAllContactors(void);
