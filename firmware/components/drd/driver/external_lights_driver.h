/*
 * external_lights_driver.h
 *
 *  @brief  HAL-level GPIO driver for external lights outputs.
 *
 *  Created on: Feb 12, 2026
 *      Author: Martin Wu
 */

#ifndef INC_EXTERNAL_LIGHTS_DRIVER_H_
#define INC_EXTERNAL_LIGHTS_DRIVER_H_

/*	Includes	*/
#include "main.h"
#include <stdint.h>

/*	Function Prototypes	*/

/*
 * @brief Drives external light GPIO outputs.
 *
 * @param flts  Front-left  turn signal  (LTS_OUT)
 * @param frts  Front-right turn signal  (RTS_OUT)
 * @param blts  Rear-left   turn signal  (BL_LIGHTS)
 * @param brts  Rear-right  turn signal  (BR_LIGHTS)
 * @param brk   Brake output - active whenever braking (BRK_OUT).
 *
 * Each parameter should be 1 (active) or 0 (inactive).
 */
void Set_ExternalLights(uint8_t flts, uint8_t frts, uint8_t blts, uint8_t brts, uint8_t brk);

#endif /* INC_EXTERNAL_LIGHTS_DRIVER_H_ */
