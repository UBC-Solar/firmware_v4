/*
 * external_lights_driver.c
 *
 *	@brief   HAL-level GPIO driver for external lights signals.
 *
 *  Created on: Feb 12, 2025
 *      Author: Martin Wu
 *
 *
 */

/*	Includes	*/
#include "external_lights_driver.h"

/*
 * @brief Drives external light GPIO outputs.
 *
 * @param flts  Front-left  turn signal (LTS_OUT)
 * @param frts  Front-right turn signal (RTS_OUT)
 * @param blts  Rear-left   turn signal (BL_LIGHTS)
 * @param brts  Rear-right  turn signal (BR_LIGHTS)
 * @param brk   Brake output - active whenever braking (BRK_OUT).
 *
 * Each parameter should be 1 (active) or 0 (inactive).
 */
void Set_ExternalLights(uint8_t flts, uint8_t frts, uint8_t blts, uint8_t brts, uint8_t brk)
{
    HAL_GPIO_WritePin(LTS_OUT_GPIO_Port, LTS_OUT_Pin, flts ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RTS_OUT_GPIO_Port, RTS_OUT_Pin, frts ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BL_LIGHTS_GPIO_Port, BL_LIGHTS_Pin, blts ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BR_LIGHTS_GPIO_Port, BR_LIGHTS_Pin, brts ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRK_OUT_GPIO_Port, BRK_OUT_Pin, brk ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
