#include "adc_driver.h"
#include "drivers.h"
#include <stdlib.h>

/* PRIVATE VARIABLES */
static AdcError g_last_error = ADC_FAULT_NONE;

/* PRIVATE FUNCTION DECLARATIONS */
static uint16_t ReadAdc(ADC_HandleTypeDef* hadc);
static bool ValidateAdcReadings(uint16_t adc1, uint16_t adc2);
static uint16_t NormalizeToDac(uint16_t adc1, uint16_t adc2);

/* ADC DRIVER FUNCTIONALITY */
static uint16_t ReadAdc(ADC_HandleTypeDef* hadc)
{
	HAL_ADC_Start(hadc);
	HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY);
	return HAL_ADC_GetValue(hadc);
}

uint16_t AdcDriverReadThrottle(void)
{
    g_last_error = ADC_FAULT_NONE;
    
    uint16_t adc1 = ReadAdc(&hadc1);
    uint16_t adc2 = ReadAdc(&hadc2);
    
    if (!ValidateAdcReadings(adc1, adc2)) {
        return 0;  // Invalid sensors = no throttle
    }
    
    return NormalizeToDac(adc1, adc2);
}

/* VALIDATION AND ERROR HANDLING */
AdcError AdcDriverGetError(void)
{
    return g_last_error;
}

static bool ValidateAdcReadings(uint16_t adc1, uint16_t adc2)
{
    g_last_error = ADC_FAULT_NONE;
    
    // Check sensor 1 is in valid range
    if (adc1 < ADC_LOWER_DEADZONE || adc1 > ADC_UPPER_DEADZONE) {
        g_last_error |= ADC1_SENSOR_FAULT;
        return false;
    }
    
    // Check sensor 2 is in valid range
    if (adc2 < ADC_LOWER_DEADZONE || adc2 > ADC_UPPER_DEADZONE) {
        g_last_error |= ADC2_SENSOR_FAULT;
        return false;
    }
    
    // Check sensors agree
    if (abs(adc1 - adc2) > ADC_MAX_DIFFERENCE) {
        g_last_error |= ADC_ERROR_DISAGREEMENT;
        return false;
    }
    
    return true;
}

/* ADC CONVERSION TO DAC */
static uint16_t convert_to_dac(uint16_t adc)
{
    adc = MIN(MAX(adc, ADC_NO_THROTTLE_MAX), ADC_FULL_THROTTLE_MIN);    // Keep adc val within throttle range
    return ((adc - ADC_NO_THROTTLE_MAX) * MC_DAC_MAX) / (ADC_FULL_THROTTLE_MIN - ADC_NO_THROTTLE_MAX);      // Find ratio between 0 to 1 and then * 1023
}

static uint16_t NormalizeToDac(uint16_t adc1, uint16_t adc2)
{
    (void)adc2; // unused adc value
    
    if (adc1 <= ADC_LOWEST_VALID) {
        return 433;
    }
    
    if (adc1 >= ADC_HIGHEST_VALID) {
        return 0;
    }
    
    // Linear interpolation from ADC range to DAC range
    uint32_t range = (uint32_t)(ADC_HIGHEST_VALID - ADC_LOWEST_VALID);
    uint32_t value = (uint32_t)(adc_avg - ADC_LOWEST_VALID);
    uint32_t scaled = (value * 433) / range;
    
    uint16_t dac_value = (uint16_t)(433 - scaled);
    
    // Clamp to valid range
    if (dac_value > 433) {
        dac_value = 433;
    }
    
    return dac_value;
}