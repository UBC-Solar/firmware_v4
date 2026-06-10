/**
 * @file hvc_fsm_private.h
 * @brief Internal definitions shared between hvc_fsm.c and hvc_fsm_states.c
 */

#pragma once

#include "hvc_fsm.h"
#include "gpio_driver.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "main.h"
#include <stdbool.h>

/*============================================================================*/
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
