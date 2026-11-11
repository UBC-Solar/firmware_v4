/**
 * @file adc_runtime.c
 * @brief ADC measurements in real units, built on adc_driver.
 *
 * adc_driver averages every ADC channel in the background (DMA) and gives each one in mV at
 * the ADC pin (ADC_GetVoltages()). This file converts those to what they measure, in real
 * units. The scaling constants are in hvc_defs.h. The thermistor table and its conversion,
 * ThermistorVoltToTemp_(), stay in adc_driver.c (set up by InitTemperatureReading()).
 */

#include "adc_runtime.h"

// Drivers
#include "adc_driver.h"

// HVC app
#include "hvc_defs.h"

/**
 * @brief Reads the ADC once and converts every channel to real units.
 *
 * Call once per pass and keep the result (HVC_Main() stores it in g_shared.adc), so the
 * whole pass uses the same values: adc_driver updates its values in the background, so two
 * reads in one pass can differ.
 *
 * @return Every ADC measurement, in real units.
 */
AdcMeasurements AdcRuntimeRead(void)
{
    ADC_Voltages adc = ADC_GetVoltages(); // mV at the ADC pins
    AdcMeasurements measurements;

    // Supp battery: scaled back up through the sense divider
    measurements.supp_mv =
        (int32_t)((uint32_t)adc.supp_sense * SUPP_SENSE_DIVIDER_NUM / SUPP_SENSE_DIVIDER_DEN);

    // Motor and MPPT precharge: scaled up to the HV side
    measurements.motor_precharge_mv = (int32_t)adc.motor_precharge * PRECHARGE_SENSE_SCALE;
    measurements.mppt_precharge_mv = (int32_t)adc.mppt_precharge * PRECHARGE_SENSE_SCALE;

    // LV current: sensor offset removed, then mV to mA through the sensor sensitivity
    measurements.lv_current_ma =
        ((int32_t)adc.lv_curr_sense - LV_CURRENT_SENSOR_OFFSET_MV) * 1000 /
        LV_CURRENT_SENSOR_MV_PER_A;

    // DCDC temperature: thermistor voltage (mV to uV) to milli-degrees C
    measurements.dcdc_temp_mc = ThermistorVoltToTemp_((uint32_t)adc.dcdc_thermistor * 1000U);

    return measurements;
}
