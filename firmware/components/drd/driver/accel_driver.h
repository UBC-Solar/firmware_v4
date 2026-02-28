#ifndef __ACCEL_DRIVER_H__
#define __ACCEL_DRIVER_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* ADC CONFIGURATION DEFINES */
#define ADC_LOWEST_VALID 800
#define ADC_HIGHEST_VALID 1950
#define ADC_LOWER_DEADZONE 10
#define ADC_UPPER_DEADZONE 4000
#define ADC_MAX_DIFFERENCE 99999

/* Throttle range (pedal position mapping) */
#define ADC_NO_THROTTLE_MAX 630
#define ADC_FULL_THROTTLE_MIN 1350
#define MC_DAC_MAX 1023 // 433 for Cascadia
#define MC_DAC_MIN 0

/* ERROR FLAGS */
typedef enum
{
    ADC_FAULT_NONE = 0x00,
    ADC1_SENSOR_FAULT = 0x01,
    ADC2_SENSOR_FAULT = 0x02,
    ADC_ERROR_DISAGREEMENT = 0x04,
} AdcError;

/* FUNCTION PROTOTYPES */

/**
 * @brief Reads and returns the current throttle value from the accelerator.
 * @return Throttle value as a 16-bit unsigned integer.
 */
uint16_t AccelDriverReadThrottle(void);

/**
 * @brief Returns the last ADC error encountered by the accelerator driver.
 * @return The last ADC error.
 */
AdcError AccelDriverGetAdcError(void);

/**
 * @brief Returns the last ADC error encountered by the accelerator driver.
 * @return The last ADC error.
 */
AdcError AccelDriverGetAdcError(void);

#endif /* __ACCEL_DRIVER_H__ */