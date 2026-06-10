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
    HVC_RESET = 0,
    MVP_LV_POWERUP,
    BMS_READY,
    BMS_CHECK,
    FANS_POWERUP,
    HV_CONNECT,
    MOTOR_PRECHARGE,
    MPPT_PRECHARGE,
    CLOSE_LLIM,
    CLOSE_HLIM,
    LV_POWERUP,
    MONITORING,
    FAULT,
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
