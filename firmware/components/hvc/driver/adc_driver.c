#include "adc_driver.h"

#include "adc_process_runtime.h"

static void ADC1_ProcessReadings(int half);

static ADC_HandleTypeDef *hadc1;

static volatile uint16_t adc_buffer[ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2];

static ADC_Readings adc1_readings = {0};
static ADC_Voltages adc1_voltages = {0};

/**
 * @brief Initialize ADC driver and start DMA in circular mode.
 * @param _hadc1 Pointer to the ADC handle configured in CubeMX.
 */
void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3) {
    hadc1 = _hadc1;

    HAL_TIM_Base_Start(_htim3); // TIM3 triggered ADC conversion

    HAL_ADC_Start_DMA(hadc1, (uint32_t*)adc_buffer, ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2);

    //UART_Printf("ADC Initialized with DMA buffer size: %d\n\r", ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2);
}

/**
 * @brief Read averaged ADC values for a given half of the buffer, then convert to voltages and store in global structs.
 * @param half 0 for the first half, 1 for the second half.
 * @return None
 */
static void ADC1_ProcessReadings(int half) {
    uint32_t sum[ADC1_NUM_CHANNELS] = {0};
    uint16_t results[ADC1_NUM_CHANNELS] = {0};

    // Sum readings
    for (uint8_t sample = 0; sample < ADC1_SAMPLE_COUNT; sample++) {
        for (uint8_t channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
            sum[channel] += adc_buffer[half * ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT + sample * ADC1_NUM_CHANNELS + channel];
        }
    }

    // Average readings (divide sum by SAMPLE_COUNT)
    for (int channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
        results[channel] = sum[channel] / ADC1_SAMPLE_COUNT;
    }

    // Store readings and voltages
    for (int channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
        uint16_t voltage = (uint32_t)results[channel] * ADC_VOLTAGE_REFERENCE / ADC_RESOLUTION; // Convert to millivolts with uint32_t for no truncation
        switch (channel) {
            case 0:
                adc1_readings.dcdc_thermistor = results[channel];
                adc1_voltages.dcdc_thermistor = voltage;
            break;

            case 1:
                adc1_readings.motor_precharge = results[channel];
                adc1_voltages.motor_precharge = voltage;
            break;
            
            case 2:
                adc1_readings.mppt_precharge = results[channel];
                adc1_voltages.mppt_precharge = voltage;
            break;
            
            case 3:
                adc1_readings.supp_sense = results[channel];
                adc1_voltages.supp_sense = voltage;
            break;

            case 4:
                adc1_readings.lv_curr_sense = results[channel];
                adc1_voltages.lv_curr_sense = voltage;
            break;
        }
    }

    ADC_Runtime_ProcessReadings();
}

ADC_Readings ADC_GetReadings(void) {
    return adc1_readings;
}

ADC_Voltages ADC_GetVoltages(void) {
    return adc1_voltages;
}

/**
 * @brief HAL invokes this callback when the first half of the DMA buffer is filled.
 * @param hadc Pointer to the ADC handle that triggered the callback.
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == hadc1) {
        ADC1_ProcessReadings(0);
    }
}

/**
 * @brief HAL invokes this callback when the second half of the DMA buffer is filled.
 * @param hadc Pointer to the ADC handle that triggered the callback.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == hadc1) {
        ADC1_ProcessReadings(1);
    }
}