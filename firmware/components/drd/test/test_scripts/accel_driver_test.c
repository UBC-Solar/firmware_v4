/**
 * @file    accel_driver.c
 * @brief   Accelerator test file
 */

/* INCLUDES */
#include "cruise_control_test.h"
#include "accel_driver_test.h"
#include <stdbool.h>

/* DEFINES */
#define ADC_2_ACTIVE 0 // activate if adc2 is used

/* GLOBAL VARIABLES */
static AdcError g_last_error = ADC_FAULT_NONE;

/* PRIVATE FUNCTION DECLARATIONS */
/**
 * @brief Reads a value from the specified ADC peripheral.
 * @param hadc Pointer to the ADC handle structure.
 * @return The ADC conversion result as a 16-bit unsigned integer.
 */
static uint16_t ReadAdc(ADC_HandleTypeDef* hadc);
/**
 * @brief Validates the raw ADC readings from both accelerator channels.
 * @param adc1 First ADC channel value.
 * @param adc2 Second ADC channel value.
 * @return True if readings are valid, false otherwise.
 */
static bool ValidateAdcReadings(uint16_t adc1, uint16_t adc2);
/**
 * @brief Normalizes the ADC readings to a DAC value for throttle output.
 * @param adc1 First ADC channel value.
 * @param adc2 Second ADC channel value.
 * @return Normalized DAC value as a 16-bit unsigned integer.
 */
static uint16_t NormalizeToDac(uint16_t adc1, uint16_t adc2);

/* ADC DRIVER FUNCTIONALITY */
static uint16_t ReadAdc(ADC_HandleTypeDef* hadc)
{
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY);
    return HAL_ADC_GetValue(hadc);
}

uint16_t AccelDriverReadThrottle(void)
{
    g_last_error = ADC_FAULT_NONE;

    uint16_t adc1 = ReadAdc(&hadc1);
    DiagnosticSetRawADC1(adc1);

    uint16_t adc2 = 0;

    #if ADC_2_ACTIVE
    adc2 = ReadAdc(&hadc2);
    DiagnosticSetRawADC2(adc2);
    #endif

    if (!ValidateAdcReadings(adc1, adc2))
    {
        return MC_DAC_MIN; // Invalid sensors = no throttle
    }

    return AccelNormalizeToDac((float)adc1, (float)ADC_LOWEST_VALID, (float)ADC_HIGHEST_VALID);
}

/* VALIDATION AND ERROR HANDLING */
AdcError AccelDriverGetAdcError(void) { return g_last_error; }

static bool ValidateAdcReadings(uint16_t adc1, uint16_t adc2)
{
    g_last_error = ADC_FAULT_NONE;

    // Check sensor 1 is in valid range
    if (adc1 < ADC_LOWER_DEADZONE || adc1 > ADC_UPPER_DEADZONE)
    {
        g_last_error |= ADC1_SENSOR_FAULT;
        DiagnosticSetThrottleADCOutOfRange(true);
        return false;
    }

    #if ADC_2_ACTIVE
    // Check sensor 2 is in valid range
    if (adc2 < ADC_LOWER_DEADZONE || adc2 > ADC_UPPER_DEADZONE)
    {
        g_last_error |= ADC2_SENSOR_FAULT;
        DiagnosticSetThrottleADCOutOfRange(true);
        return false;
    }

    // Check sensors agree
    if (abs(adc1 - adc2) > ADC_MAX_DIFFERENCE)
    {
        g_last_error |= ADC_ERROR_DISAGREEMENT;
        DiagnosticSetThrottleADCMismatch(true);
        return false;
    }
    #endif
    //TODO: This is app in driver dont do that
    DiagnosticSetThrottleADCOutOfRange(false);
    DiagnosticSetThrottleADCMismatch(false);

    return true;
}

/* ADC CONVERSION TO DAC */
static uint16_t ConvertToDac(uint16_t adc)
{
    adc = MIN(MAX(adc, ADC_NO_THROTTLE_MAX),
              ADC_FULL_THROTTLE_MIN); // Keep adc val within throttle range
    return ((adc - ADC_NO_THROTTLE_MAX) * MC_DAC_MAX) /
           (ADC_FULL_THROTTLE_MIN -
            ADC_NO_THROTTLE_MAX); // Find ratio between 0 to 1 and then * 1023
}

uint16_t AccelNormalizeToDac(float value, float min, float max) {
    if (value <= min) return MC_DAC_MIN;
    if (value >= max) return MC_DAC_MAX;

    float range = max - min;
    float shift = value - min;
    float scaled = (shift * MC_DAC_MAX) / range;

    uint16_t dac_value = (uint16_t)(scaled);
    return dac_value;
}

uint16_t AccelCruiseNormalizeToDac(float accel) {
    return AccelNormalizeToDac(accel, ACCEL_MIN, ACCEL_MAX);
}