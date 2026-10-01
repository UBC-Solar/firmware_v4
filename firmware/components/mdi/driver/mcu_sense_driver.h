/**
 * @file    mcu_sense_driver.h
 * @brief   Reads the STM32's own supply voltage and chip temperature using ADC1's built-in
 *          sensors
 */

#ifndef __MCU_SENSE_DRIVER_H__
#define __MCU_SENSE_DRIVER_H__

#include <stdint.h>

/**
 * @brief Calibrates ADC1
 */
void McuSenseDriverInit(void);

/**
 * @brief Returns the MCU supply voltage (3.3 V rail) in millivolts
 */
uint16_t McuSenseDriverReadVddMv(void);

/**
 * @brief Returns the MCU chip temperature in degrees Celsius
 */
int16_t McuSenseDriverReadTempC(void);

#endif /* __MCU_SENSE_DRIVER_H__ */
