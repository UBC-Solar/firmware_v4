/**
 * @file    mcu_health_driver.c
 * @brief   MCU supply voltage and chip temperature from DRD's built-in sensors.
 */

#include "mcu_health_driver.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "adc.h"

static uint16_t ReadAdc( uint32_t channel );

int32_t sense_mv = 0;
uint16_t g_vdd_mv = 0;
int16_t g_temp_c = 0;

void McuHealthDriverInit() {
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
    uint16_t raw = ReadAdc(ADC_CHANNEL_VREFINT);
    if (raw == 0) {
        return 0;
    }
    
    uint16_t Vdd = VREFINT_MV * ADC_FULL_SCALE / raw; 

    return Vdd;
}


int16_t DRD_Read_Temp( void ) {
    sense_mv = ReadAdc(ADC_CHANNEL_TEMPSENSOR) * DRD_Read_Vdd() / ADC_FULL_SCALE;
    
    int16_t temp = (V25_MV - sense_mv) * 1000 / AVG_SLOPE_UV_PER_C + TS_REF_TEMP_C;

    return temp;
}