#include "stm32f1xx_hal.h"
#include <stdint.h>
#include "mcu_health_driver.h"
#include "adc.h"

// datasheet typical values
#define VREFINT_MV 1200 // built-in reference voltage
#define V25_MV 1430     // temp sensor voltage at 25 degC
#define ADC_MAX_VALUE 4095
#define TIMEOUT_MS 10
#define TEMP_MV_SLOPE -4.3
#define STANDARD_TEMP 25

static uint16_t ReadAdc(uint32_t channel);

static uint16_t ReadAdc(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // Temp sensor needs >= 17 us, ADC freq is 12 Mhz
    };

    HAL_ADC_ConfigChannel(&hadc1, &config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, TIMEOUT_MS;
    return HAL_ADC_GetValue(&hadc1);
}


int16_t ReadVdd_mV(void)
{   
    int16_t vdd_mv = (int16_t) (VREFINT_MV *  ADC_MAX_VALUE  / ReadAdc(ADC_CHANNEL_VREFINT));
    return vdd_mv;
}

int16_t ReadTempC(void)
{
    // Temp sensor voltage drops 4.3 mV per degree C
    int32_t sensor_voltage_mv = ReadAdc(ADC_CHANNEL_TEMPSENSOR) * ReadVdd_mV() /  ADC_MAX_VALUE;
    int16_t temp_c = (int16_t) ((sensor_voltage_mv - V25_MV) / TEMP_MV_SLOPE + STANDARD_TEMP);
    return temp_c;
}   