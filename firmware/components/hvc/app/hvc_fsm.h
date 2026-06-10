/**
 * @file hvc_fsm.h
 * @brief Public interface for the HVC finite state machine
 */

#pragma once

#include "gpio_driver.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*============================================================================*/
/* STATE ENUM */

typedef enum
{
    HVC_RESET = 0,
    MVP_LV_POWERUP,
    MST_READY,
    MST_CHECK,
    FANS_POWERUP,
    HV_CONNECT,
    MOTOR_DISCHARGE,
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

/* CONTACTOR ACTIVE LEVELS — TODO: verify polarity from schematic */
#define HVC_CONTACTOR_CLOSE         GPIO_PIN_SET
#define HVC_CONTACTOR_OPEN          GPIO_PIN_RESET
#define TEL_HEARTBEAT_ID            300U          
#define MVP_LV_POWERUP_TIMEOUT_MS   5000U    
#define MST_READY_TIMEOUT_MS        5000U    


/*============================================================================*/
/* INTERNAL TYPE DEFS */

typedef struct
{
    uint32_t generic;
    uint32_t fault_led;
    uint32_t neg_contactor;
    uint32_t pos_contactor;
} HVC_Ticks_t;

/*============================================================================*/
/* SHARED STATE — defined in hvc_fsm.c */

extern volatile HVC_State_t hvc_state;
extern HVC_Ticks_t ticks;
extern bool startup_complete;
extern bool tel_heartbeat_received;

/*============================================================================*/
/* INTERNAL HELPERS — implemented in hvc_fsm.c */

bool   timer_elapsed(uint32_t interval, uint32_t *last_tick);
void   open_all_contactors(void);
void   check_supp_voltage(void);

/*============================================================================*/
/* STATE FUNCTION PROTOTYPES — implemented in hvc_fsm_states.c */

void Reset(void);
void MvpLvPowerup(void);
void MSTready(void);
void MSTcheck(void);
void Fans_Powerup(void);
void HV_Connect(void);
void MotorPrecharge(void);
void MpptPrecharge(void);
void MotorDischarge(void);
void CloseLLIM(void);
void CloseHLIM(void);
void LvPowerup(void);
void Monitoring(void);
void Fault(void);

/*============================================================================*/
/* PUBLIC API */

void HVC_FSM_Init(void);
void HVC_FSM_Run(void);

void HVC_ESTOPCallback(void);
void HVC_IMDFaultCallback(void);
void HVC_MasterboardFaultCallback(void);
void HVC_HVCurrentAlertCallback(void);
