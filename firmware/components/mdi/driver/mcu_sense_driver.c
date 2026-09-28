/**
 * @file    mcu_sense_driver.c
 * @brief   MCU supply voltage and chip temperature from ADC1's built-in sensors.
 */

#include "mcu_sense_driver.h"

#include "stm32f1xx_hal.h"

// datasheet typical values
#define VREFINT_MV 1200 // built-in reference voltage
#define V25_MV 1430     // temp sensor voltage at 25 degC

extern ADC_HandleTypeDef hadc1; // configured by CubeMX in main.c

/**
 * @brief Switches ADC1 to a channel, converts once, and returns the raw result (0-4095).
 */
static uint16_t ReadAdc(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // temp sensor needs >= 17 us
    };

    HAL_ADC_ConfigChannel(&hadc1, &config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    return HAL_ADC_GetValue(&hadc1);
}

void McuSenseDriverInit(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1); // ADC measures and corrects its own offset error
}

uint16_t McuSenseDriverReadVddMv(void)
{
    // VREFINT is a fixed 1.2 V. The higher VDD is, the smaller 1.2 V reads.
    return (uint16_t)(VREFINT_MV * 4095 / ReadAdc(ADC_CHANNEL_VREFINT)); //reads as a fraction of VDD
}

int16_t McuSenseDriverReadTempC(void)
{
    // Temp sensor voltage drops 4.3 mV per degC
    int32_t sense_mv = ReadAdc(ADC_CHANNEL_TEMPSENSOR) * McuSenseDriverReadVddMv() / 4095;
    return (int16_t)((V25_MV - sense_mv) * 10 / 43 + 25);
}
