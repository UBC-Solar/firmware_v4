/**
 * @file adc_driver.c
 * @brief ADC1 raw readings for the DIST's current sense inputs.
 *
 * Each channel is read with one blocking, software-triggered conversion.
 */

#include "adc_driver.h"

// Standard library
#include <stddef.h>

#define ADC_CONVERSION_TIMEOUT_MS   10U

static uint16_t ReadChannel(uint32_t channel);

static ADC_HandleTypeDef *hadc1 = NULL;

/**
 * @brief Stores the ADC handle and calibrates the ADC. Call once from DIST_Init().
 *
 * The STM32F1 ADC reads several counts off without a calibration run after power-up.
 *
 * @param _hadc1 HAL ADC handle (hadc1), already set up by MX_ADC1_Init().
 */
void ADC_Init(ADC_HandleTypeDef *_hadc1) {
    hadc1 = _hadc1;
    HAL_ADCEx_Calibration_Start(hadc1);
}

/**
 * @brief Reads every current sense channel once.
 *
 * Blocking: one conversion per channel, one after the other.
 *
 * @return Raw counts of every channel.
 */
ADC_Readings ADC_GetReadings(void) {
    ADC_Readings readings;

    readings.drd_ilm = ReadChannel(ADC_CHANNEL_14);
    readings.mdi_ilm = ReadChannel(ADC_CHANNEL_7);
    readings.spare_ilm = ReadChannel(ADC_CHANNEL_4);
    readings.spare_mux_ilm = ReadChannel(ADC_CHANNEL_5);
    readings.spare_ctrl_ilm = ReadChannel(ADC_CHANNEL_6);
    readings.tel_sense = ReadChannel(ADC_CHANNEL_15);

    return readings;
}

/**
 * @brief Points ADC1 at one channel, runs one conversion, and returns the result.
 * @param channel HAL channel (ADC_CHANNEL_x).
 * @return Raw 12-bit reading.
 */
static uint16_t ReadChannel(uint32_t channel) {
    ADC_ChannelConfTypeDef channel_config = {0};
    uint16_t value;

    channel_config.Channel = channel;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(hadc1, &channel_config);

    HAL_ADC_Start(hadc1);
    HAL_ADC_PollForConversion(hadc1, ADC_CONVERSION_TIMEOUT_MS);
    value = (uint16_t)HAL_ADC_GetValue(hadc1);
    HAL_ADC_Stop(hadc1);

    return value;
}
