#include "adc_runtime.h"

#include "adc_driver.h"
#include "hvc_defs.h"

static ADC_Values adc_values = {0};

/**
 * @brief Read the ADC pin voltages from the ADC driver and convert them into engineering units
 *        (eg. LV current, motor precharge voltage). Scaling constants are in hvc_defs.h.
 */
void ADC_Runtime_ProcessReadings(void) {
    ADC_Voltages voltages = ADC_GetVoltages(); // mV at the ADC pins

    // Convert mV at the ADC pin to mV before the supp sense voltage divider
    adc_values.supp_mv =
        (int32_t)((uint32_t)voltages.supp_sense * SUPP_SENSE_DIVIDER_NUM / SUPP_SENSE_DIVIDER_DEN);

    // Convert mV at the ADC pin to mV on the HV side
    adc_values.motor_precharge_mv = (int32_t)voltages.motor_precharge * PRECHARGE_SENSE_SCALE;
    adc_values.mppt_precharge_mv = (int32_t)voltages.mppt_precharge * PRECHARGE_SENSE_SCALE;

    // Remove the zero-current offset, then convert mV at the ADC pin to mA of current
    adc_values.lv_current_ma = ((int32_t)voltages.lv_curr_sense - LV_CURRENT_SENSOR_OFFSET_MV) *
                               1000 / LV_CURRENT_SENSOR_MV_PER_A;

    // Convert thermistor mV to uV, then to milli-degrees C through the lookup table
    adc_values.dcdc_temp_mc = ADC_ThermistorVoltToTemp((uint32_t)voltages.dcdc_thermistor * 1000U);
}

/**
 * @brief Get the latest processed ADC values.
 * @return The latest ADC values, in engineering units.
 */
ADC_Values ADC_Runtime_GetValues(void) {
    return adc_values;
}
