#pragma once

#include <stdint.h>

/**
 * @brief ADC channels converted to the engineering units they measure.
 */
typedef struct {
    int32_t supp_mv;            // Supplemental battery voltage (mV)
    int32_t motor_precharge_mv; // Motor precharge voltage, HV side (mV)
    int32_t mppt_precharge_mv;  // MPPT precharge voltage, HV side (mV)
    int32_t lv_current_ma;      // LV current (mA)
    int32_t dcdc_temp_mc;       // DCDC thermistor temperature (milli-degrees C)
} ADC_Values;

void ADC_Runtime_ProcessReadings(void);

ADC_Values ADC_Runtime_GetValues(void);
