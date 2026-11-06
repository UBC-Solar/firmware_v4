/**
 * @file adc_runtime.h
 * @brief ADC measurements in real units, built on adc_driver.
 *
 * AdcRuntimeRead() gives every ADC measurement in real units (AdcMeasurements). HvcMainRun()
 * calls it once per pass and stores the result in g_shared.adc.
 */

#pragma once

// Standard library
#include <stdint.h>

/**
 * @brief Every ADC measurement in real units, from one AdcRuntimeRead().
 */
typedef struct
{
    int32_t supp_mv;            // Supplemental battery voltage (mV)
    int32_t motor_precharge_mv; // Motor precharge voltage, HV side (mV)
    int32_t mppt_precharge_mv;  // MPPT precharge voltage, HV side (mV)
    int32_t lv_current_ma;      // LV current (mA)
    int32_t dcdc_temp_mc;       // DCDC thermistor temperature (milli-degrees C)
} AdcMeasurements;

AdcMeasurements AdcRuntimeRead(void);
