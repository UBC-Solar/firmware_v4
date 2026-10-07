/**
 * @file    drd_sense_driver.c
 * @brief   MCU supply voltage and chip temperature from DRD's built-in sensors.
 */

#include "drd_health_driver.h"
#include "stm32f1xx_hal.h"

// datasheet typical values
#define VREFINT_MV 1200 // built-in reference voltage
#define V25_MV 1430     // temp sensor voltage at 25 degC

extern ADC_HandleTypeDef hadc1; // configured by CubeMX in main.c


static uint16_t ReadADC( uint32_t channel ) {
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // temp sensor needs >= 17 us
    };

}

uint16_t ReadTemp() {
    
}