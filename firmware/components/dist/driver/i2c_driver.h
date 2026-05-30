#pragma once

#include "stm32f1xx_hal.h"

void IS31FL3236A_Init(void);

void IS31FL3236A_HLIM_Toggle(void);
void IS31FL3236A_LLIM_Toggle(void);
void IS31FL3236A_NEG_Toggle(void);
void IS31FL3236A_DIST_FAULT_Toggle(void);
void IS31FL3236A_ESTOP_Toggle(void);
void IS31FL3236A_CAN_FAULT_Toggle(void);
void IS31FL3236A_POS_Toggle(void);
void IS31FL3236A_MOTOR_PC_Toggle(void);
void IS31FL3236A_MPPT_PC_Toggle(void);
void IS31FL3236A_IMD_Toggle(void);
void IS31FL3236A_MPPT_Toggle(void);
void IS31FL3236A_DCH_ON_Toggle(void);
void IS31FL3236A_DCH_OFF_Toggle(void);
void IS31FL3236A_FANS_Toggle(void);
void IS31FL3236A_SUPP_ACTIVE_Toggle(void);
void IS31FL3236A_SUPP_LOW_Toggle(void);
void IS31FL3236A_DCDC_ACTIVE_Toggle(void);
void IS31FL3236A_CONTACTOR_Toggle(void);
