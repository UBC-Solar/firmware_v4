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
 * @brief returns unsigned input voltage to ADC in milivolts
 */
uint16_t McuHealthReadVddMv(void);

/**
 * @brief returns signed temperature in Celcius
 */
int16_t McuHealthReadTempC(void);

#endif