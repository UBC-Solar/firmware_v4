/**
 * @file    mcu_health_driver.h
 * @brief   Reads the STM32's own supply voltage and chip temperature using ADC1's built-in
 *          sensors
 */

#ifndef __MCU_HEALTH_DRIVER_H__
#define __MCU_HEALTH_DRIVER_H__

#include <stdint.h>

/**
 * @brief Calibrates ADC1
 */
void McuHealthDriverInit(void);

/**
 * @brief Returns the MCU supply voltage (3.3 V rail) in millivolts
 */
uint16_t McuHealthDriverReadVddMv(void);

/**
 * @brief Returns the MCU chip temperature in degrees Celsius
 */
int16_t McuHealthDriverReadTempC(void);

#endif /* __MCU_HEALTH_DRIVER_H__ */
