#include <main.h>
#include <mcu_health_driver.h>

#define V25_MV          1430.0f    // Voltage at 25 C
#define VREF_MV         1200.0f    // Internal reference voltage
#define AVG_SLOPE_MV    0.0043f    // degrees C / mV
#define ADC_COUNT       4095.0f    // Max ADC bit width

extern ADC_HandleTypeDef hadc1;


static uint16_t ReadADC(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {
        .Channel = channel,
        .Rank  = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5
    };

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    return HAL_ADC_GetValue(&hadc1);
}


uint16_t McuSenseStrReadVddMv(void) 
{
    uint16_t V_raw = ReadADC(ADC_CHANNEL_VREFINT);
    uint16_t V_mv = VREF_MV / V_raw * ADC_COUNT;

    return V_mv;
}

int16_t McuSenseStrReadTempC(void) 
{
    int32_t temp_raw = ReadADC(ADC_CHANNEL_TEMPSENSOR);
    int32_t temp_mv = temp_raw * McuSenseStrReadVddMv() / ADC_COUNT;
    int16_t temp_c = (temp_mv - V25_MV) * AVG_SLOPE_MV + 25;

    return temp_c;
}


void McuHealthDriverInit(void)
{
    // Initialize the MCU health driver
    HAL_ADCEx_Calibration_Start(&hadc1);
    McuSenseStrReadTempC();
    McuSenseStrReadVddMv();
}