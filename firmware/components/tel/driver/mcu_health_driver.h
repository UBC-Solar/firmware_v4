#ifndef __MCU_HEALTH_DRIVER_H__
#define __MCU_HEALTH_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

#define MAX_TEMP_C      85
#define MIN_VOLT_MV     2000

/**
 * @brief Inititates the ADC by calibrating it.
 */
void McuHealthDriverInit(void);

/**
 * @brief Reads built-in voltage sensor in ADC.
 * @return Voltage of microcontroller in mV.
 */
int16_t ReadVdd_mV(void);

/**
 * @brief Reads built-in temperature sensor in ADC.
 * @return Temperature of microcontroller in C.
 */
int16_t ReadTempC(void);

#endif