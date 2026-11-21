/**
 * @file adc_runtime.h
 * @brief eFuse and TEL currents in mA, converted from the ADC driver's raw readings.
 *
 * DIST_Main() calls ADC_Runtime_ProcessReadings() once per pass and stores
 * ADC_Runtime_GetValues() in g_shared.adc.
 */

#pragma once

// Standard library
#include <stdint.h>

/**
 * @brief ADC channels converted to the currents they measure, in mA.
 *
 * The first five are the eFuse load currents; tel_ma is the TEL rail's current sensor.
 * Can be slightly negative from sensor offset.
 */
typedef struct {
    int32_t drd_ma;        // DRD eFuse load current
    int32_t mdi_ma;        // MDI eFuse load current
    int32_t spare_ctrl_ma; // SPARE_CTRL eFuse load current
    int32_t spare_mux_ma;  // SPARE_MUX eFuse load current
    int32_t spare_ma;      // SPARE eFuse load current
    int32_t tel_ma;        // TEL rail current (ACS70331)
} ADC_Values;

void ADC_Runtime_ProcessReadings(void);
ADC_Values ADC_Runtime_GetValues(void);
