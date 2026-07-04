#pragma once

#include "is31fl3236_driver.h"

/**
 * @brief Probe the I2C bus for the IS31FL3236A LED driver and initialise it.
 *
 * Tries all four possible I2C addresses (determined by the AD pin) across up
 * to three peripheral-reinit attempts. Populates the hled handle on success.
 *
 * @return 1 if the device was found and initialised, 0 if no device responded.
 */
uint8_t LED_Driver_Init(void);

/**
 * @brief Initialise the IS31FL3236A after hled.Init has been populated.
 *        Called internally by LED_Driver_Init(); not typically called directly.
 */
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
