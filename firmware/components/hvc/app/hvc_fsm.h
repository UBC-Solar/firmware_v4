/**
 * @file hvc_fsm.h
 * @brief Public interface for the HVC finite state machine
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/*============================================================================*/
/* STATE ENUM */

typedef enum
{
    HVC_STATE_RESET = 0,
    HVC_STATE_HV_CONNECT,
    HVC_STATE_MOTOR_PRECHARGE,
    HVC_STATE_CLOSE_MOTOR_BUS,
    HVC_STATE_MPPT_PRECHARGE,
    HVC_STATE_CLOSE_MPPT_BUS,
    HVC_STATE_MONITORING,
    HVC_STATE_FAULT,
} HVC_State_t;

/*============================================================================*/
/* CONSTANTS */

#define HVC_CONTACTOR_DELAY_MS        200
#define HVC_MOTOR_PC_TIMEOUT_MS      2000
#define HVC_MPPT_PC_TIMEOUT_MS       2000
#define HVC_CAN_TX_INTERVAL_MS        200
#define HVC_FAULT_LED_BLINK_MS        200

#define HVC_SUPP_LOW_THRESHOLD_MV   10500
#define HVC_PC_COMPLETE_RATIO          90   // % of HV bus voltage for precharge complete

/*============================================================================*/
/* PUBLIC API */

void HVC_FSM_Init(void);
void HVC_FSM_Run(void);

void HVC_ESTOPCallback(void);
void HVC_IMDFaultCallback(void);
void HVC_MasterboardFaultCallback(void);
void HVC_HVCurrentAlertCallback(void);
