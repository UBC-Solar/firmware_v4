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
 * @brief returns error status
 * @param pointer to usigned MCU input voltage in milivolts
 */
HAL_StatusTypeDef McuHealthReadVddMv(uint16_t* p_V_mv);

/**
 * @brief returns error status
 * @param pointer to returns signed MCU temperature in Celcius
 */
HAL_StatusTypeDef McuHealthReadTempC(int8_t* p_temp_c);

#endif