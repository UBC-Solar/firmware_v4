#include "adc_process_runtime.h"

#include "adc_driver.h"
#include "uart_driver.h"

static ADC_Values adc_values = {0};

/**
 * @brief Read the raw ADC values from the ADC Driver and convert into useful data (eg. LVS current, motor controller precharge voltage)
 */
void ADC_Runtime_ProcessReadings(void) {
    ADC_Readings readings = ADC_GetReadings();
    ADC_Voltages voltages = ADC_GetVoltages();

    adc_values.dcdc_thermistor = readings.dcdc_thermistor; // TODO

    adc_values.motor_precharge = readings.motor_precharge; // TODO

    adc_values.mppt_precharge = readings.mppt_precharge; // TODO

    adc_values.supp_sense = voltages.supp_sense / SUPP_DIVIDER_RATIO; // Convert from mV at ADC pin to mV before voltage divider

    adc_values.lv_curr_sense = (readings.lv_curr_sense - LV_CURR_SENSE_ZERO_CURRENT_VOLTAGE) / LV_CURR_SENSE_SENSITIVITY; // Convert mV at ADC pin to mA of current

    UART_Printf("ADC Values - DCDC Thermistor: %u, Motor Precharge: %u mV, MPPT Precharge: %u mV, Supp Sense: %u mV, LV Curr Sense: %u mA\n\r",
        adc_values.dcdc_thermistor,
        adc_values.motor_precharge,
        adc_values.mppt_precharge,
        adc_values.supp_sense,
        adc_values.lv_curr_sense
    );
}

/**
 * @brief Get the latest processed ADC values.
 * @return The latest ADC values.
 */
ADC_Values ADC_Runtime_GetValues(void) {
    return adc_values;
}