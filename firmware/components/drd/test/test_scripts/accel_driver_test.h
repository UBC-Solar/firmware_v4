/**
 * @file    accel_driver.h
 * @brief   Accelerator test file
 *
 */

#ifndef __ACCEL_DRIVER_TEST_H__
#define __ACCEL_DRIVER_TEST_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef ADC_LOWEST_VALID
#define ADC_LOWEST_VALID 1000
#endif
#ifndef ADC_HIGHEST_VALID
#define ADC_HIGHEST_VALID 1950
#endif
#ifndef ADC_LOWER_DEADZONE
#define ADC_LOWER_DEADZONE 10
#endif
#ifndef ADC_UPPER_DEADZONE
#define ADC_UPPER_DEADZONE 4000
#endif
#ifndef ADC_MAX_DIFFERENCE
#define ADC_MAX_DIFFERENCE 99999
#endif
#ifndef ADC_NO_THROTTLE_MAX
#define ADC_NO_THROTTLE_MAX 630
#endif
#ifndef ADC_FULL_THROTTLE_MIN
#define ADC_FULL_THROTTLE_MIN 1350
#endif
#ifndef MC_DAC_MAX
#define MC_DAC_MAX 1023
#endif
#ifndef MC_DAC_MIN
#define MC_DAC_MIN 0
#endif

/* FUNCTION PROTOTYPES */

uint16_t AccelDriverReadThrottle(void);

uint16_t AccelNormalizeToDac(float accel, float min, float max);

uint16_t AccelCruiseNormalizeToDac(float accel);

/**
 * @brief Returns the last ADC error encountered by the accelerator driver.
 * @return The last ADC error.
 */
AdcError AccelDriverGetAdcError(void);

#endif /* __ACCEL_DRIVER_TEST_H__ */