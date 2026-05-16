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

#define HVC_CONTACTOR_CLOSE  GPIO_PIN_SET
#define HVC_CONTACTOR_OPEN   GPIO_PIN_RESET

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

/*============================================================================*/
/* INTERNAL HELPERS — implemented in hvc_fsm.c */

bool   timer_elapsed(uint32_t interval, uint32_t *last_tick);
void   open_all_contactors(void);
void   check_supp_voltage(void);

/*============================================================================*/
/* STATE FUNCTION PROTOTYPES — implemented in hvc_fsm_states.c */

void HVC_State_Reset(void);
void HVC_State_HVConnect(void);
void HVC_State_MotorPrecharge(void);
void HVC_State_CloseMotorBus(void);
void HVC_State_MpptPrecharge(void);
void HVC_State_CloseMpptBus(void);
void HVC_State_Monitoring(void);
void HVC_State_Fault(void);
