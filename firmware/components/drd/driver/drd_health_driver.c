/**
 * @file    drd_sense_driver.c
 * @brief   MCU supply voltage and chip temperature from DRD's built-in sensors.
 */

#include "drd_health_driver.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"

// datasheet typical values
#define VREFINT_MV 1200 // built-in reference voltage
#define V25_MV 1430     // temp sensor voltage at 25 degC

extern ADC_HandleTypeDef hadc1; 

static uint16_t ReadAdc( uint32_t channel );

void DRD_Health_Init() {
    HAL_ADCEx_Calibration_Start(&hadc1); // 
}

static uint16_t ReadAdc( uint32_t channel ) {

    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // temp sensor needs >= 17 us
    };

    HAL_ADC_ConfigChannel(&hadc1, &config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    return raw;

}

uint16_t DRD_Read_Vdd( void ) {

    return (uint16_t)(VREFINT_MV * 4095 / ReadAdc(ADC_CHANNEL_VREFINT)); //reads as a fraction of VDD

}

int16_t DRD_Read_Temp( void ) {

    int32_t sense_mv = ReadAdc(ADC_CHANNEL_TEMPSENSOR) * DRD_Read_Vdd() / 4095;
    return (int16_t)((V25_MV - sense_mv) * 10 / 43 + 25);

}