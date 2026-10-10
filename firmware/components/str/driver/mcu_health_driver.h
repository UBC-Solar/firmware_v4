/**
 * @file    mcu_health_driver.h
 * @brief   MCU health diagnostics interface for UBC Solar STR board.
*/

#ifndef __MCU_SENSE_DRIVER_H__
#define __MCU_SENSE_DRIVER_H__

/* INCLUDES */
#include <stdint.h>
#include <stm32f103xe.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief Initializes the MCU health driver
 */
void McuHealthDriverInit(void);

/**
 * @brief returns unsigned MCU input voltage in milivolts
 */
uint16_t McuSenseReadVddMv(void);

/**
 * @brief returns signed MCU temperature in Celcius
 */
int8_t McuSenseReadTempC(void);

#endif