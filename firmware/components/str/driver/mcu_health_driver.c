#include "main.h"
#include "mcu_health_driver.h"
#include "adc.h"

#define V25_MV          1430.0f     // Voltage at 25 C
#define VREF_MV         1200.0f     // Internal reference voltage
#define AVG_SLOPE_MV    -4.3f       // mv / degrees C
#define ADC_COUNT       4095.0f     // Max ADC bit width
#define TIMEOUT_MS      10          // Safe delay between ADC reads 

static uint16_t ReadADC(uint32_t channel) {
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // Temp sensor needs >= 17 us, ADC freq is 12 Mhz
    };

    HAL_ADC_ConfigChannel(&hadc1, &config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, TIMEOUT_MS);
    uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return adc_val;
}

uint16_t McuHealthReadVddMv() 
{
    uint16_t vdd_raw = ReadADC(ADC_CHANNEL_VREFINT);
    uint16_t vdd_mv = VREF_MV / vdd_raw * ADC_COUNT;

    return vdd_mv;
}

int16_t McuHealthReadTempC() 
{
    int16_t temp_raw = ReadADC(ADC_CHANNEL_TEMPSENSOR);
    uint16_t vdd_mv = McuHealthReadVddMv();

    int32_t temp_mv = temp_raw * vdd_mv / ADC_COUNT;
    int16_t temp_c = (temp_mv - V25_MV) / AVG_SLOPE_MV + 25;

    return temp_c;
}

void McuHealthDriverInit(void)
{
    // Initialize the MCU health driver
    HAL_ADCEx_Calibration_Start(&hadc1);
}

