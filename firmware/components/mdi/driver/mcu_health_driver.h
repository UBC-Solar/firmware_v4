/**
 * @file    mcu_health_driver.h
 * @brief   Reads the STM32's own supply voltage and chip temperature using ADC1's built-in
 *          sensors
 *
 * Accuracy (datasheet typical-value constants, no factory calibration on the F1):
 *  - VDD: about +/-3% (VREFINT spread).
 *  - Temperature: up to about +/-30 degC worst case, uncalibrated (V25 spread plus the VDD
 *    error). Good for trends and catching overheating, not exact readings. Set
 *    MCU_TEMP_OFFSET_C in mcu_health_driver.c to calibrate a board.
 */

#ifndef __MCU_HEALTH_DRIVER_H__
#define __MCU_HEALTH_DRIVER_H__

#include <stdint.h>

#include "stm32f1xx_hal.h"

/**
 * @brief Calibrates ADC1
 */
void McuHealthDriverInit(void);

/**
 * @brief Reads the MCU supply voltage (3.3 V rail)
 * @param vdd_mv Output supply voltage in millivolts. Only written on HAL_OK.
 * @retval HAL_OK on success; HAL_ERROR/HAL_TIMEOUT if the ADC read failed or returned 0.
 */
HAL_StatusTypeDef McuHealthDriverReadVddMv(uint16_t* vdd_mv);

/**
 * @brief Reads the MCU chip temperature.
 * @param vdd_mv Supply voltage from McuHealthDriverReadVddMv(), used to scale the reading.
 * @param temp_c Output chip temperature in degrees Celsius. Only written on HAL_OK.
 * @retval HAL_OK on success; HAL_ERROR/HAL_TIMEOUT if the ADC read failed.
 */
HAL_StatusTypeDef McuHealthDriverReadTempC(uint16_t vdd_mv, int16_t* temp_c);

#endif /* __MCU_HEALTH_DRIVER_H__ */
