/**
 * @file fsm.h
 * @brief Public interface for the HVC finite state machine
 *
 * Lists everything fsm.c defines, in the same sections and order (what each section is for
 * is explained in fsm.c). Include it to use the HVC-wide variables (g_state, g_fault_flags,
 * g_shared, g_ticks) or the helpers, such as TimerElapsed().
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// HVC app
#include "hvc_types.h"

/*==============================================================================
 * === GLOBAL VARIABLES ========================================================
 * Defined in fsm.c.
 *============================================================================*/

extern volatile FsmState   g_state;
extern volatile FaultFlags g_fault_flags;
extern SharedData          g_shared;
extern Ticks               g_ticks;

/*==============================================================================
 * === PUBLIC API ==============================================================
 * Implemented in fsm.c. Called from hvc_main.c.
 *============================================================================*/

void FsmInit(void);
void FsmRun(void);

/*==============================================================================
 * === STATES ==================================================================
 * Implemented in fsm.c.
 *============================================================================*/

void Reset(void);
void TelStartup(void);
void MstHandshake(void);
void MstStatusCheck(void);
void FansPowerup(void);
void HvConnect(void);
void OpenDischargeRelay(void);
void MotorPrecharge(void);
void MpptPrecharge(void);
void CloseLlim(void);
void CloseHlim(void);
void LvPowerup(void);
void Monitoring(void);
void Fault(void);

/*==============================================================================
 * === INTERRUPT CALLBACKS =====================================================
 * Implemented in fsm.c.
 *============================================================================*/

void EstopCallback(void);
void ImdFaultCallback(void);
void MstFaultCallback(bool armed);
void HvCurrentAlertCallback(void);
void DcdcFaultCallback(bool armed);

/*==============================================================================
 * === HELPERS =================================================================
 * Implemented in fsm.c.
 *============================================================================*/

// --- Timing ---
bool TimerElapsed(uint32_t interval_ms, uint32_t* last_tick);

// --- Faults ---
bool AnyFaultActive(void);
void CheckStartupFaultPins(void);

// --- Heartbeats ---
void CheckHeartbeats(void);

// --- Supplemental battery ---
void CheckSuppVoltage(void);

// --- Contactors ---
void OpenAllContactors(void);
