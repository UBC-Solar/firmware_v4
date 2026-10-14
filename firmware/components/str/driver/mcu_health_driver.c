#include <main.h>
#include <mcu_health_driver.h>

#define V25_MV          1430.0f     // Voltage at 25 C
#define VREF_MV         1200.0f     // Internal reference voltage
#define AVG_SLOPE_MV    4.3f        // mv / degrees C
#define ADC_COUNT       4095.0f     // Max ADC bit width

extern ADC_HandleTypeDef hadc1;


static HAL_StatusTypeDef ReadADC(uint32_t channel, uint16_t *p_raw_value) {
    HAL_StatusTypeDef status;
    *p_raw_value = 0;

    ADC_ChannelConfTypeDef sConfig = {
        .Channel = channel,
        .Rank  = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5
    };

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // Test ADC Start
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        return status;
    }

    // Test ADC Poll for Conversion
    status = HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return status;
    }
    
    if (!p_raw_value) return HAL_ERROR;

    // Obtain value
    *p_raw_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // Check boundaries that shouldn't be hit
    if (*p_raw_value == 0 || *p_raw_value == 4095) { // max internal voltage reference value and prevents divide by zero errors down the line
        status = HAL_ERROR;
    }
    else {
        status = HAL_OK;
    }

    return status;
}


HAL_StatusTypeDef McuHealthReadVddMv(uint16_t* p_V_mv) 
{
    uint16_t V_raw = 0;
    uint16_t* p_raw_value = &V_raw;

    HAL_StatusTypeDef error_status = ReadADC(ADC_CHANNEL_VREFINT, p_raw_value);

    if (error_status != HAL_OK) return error_status; // idk what to return. catches divide by zero error & NULL pointer error

    *p_V_mv = VREF_MV / V_raw * ADC_COUNT;

    return error_status;
}

HAL_StatusTypeDef McuHealthReadTempC(int8_t* p_temp_c) 
{
    HAL_StatusTypeDef error_status;

    uint16_t temp_raw = 0;
    error_status = ReadADC(ADC_CHANNEL_TEMPSENSOR, &temp_raw);
    if (error_status != HAL_OK) return error_status;


    uint16_t vdd_raw = 0;
    error_status = McuHealthReadVddMv(&vdd_raw);
    if (error_status != HAL_OK) return error_status;

    int32_t temp_mv = temp_raw * vdd_raw / ADC_COUNT;
    *p_temp_c = (V25_MV - temp_mv) / AVG_SLOPE_MV + 25;

    return error_status;
}

void McuHealthDriverInit(void)
{
    // Initialize the MCU health driver
    HAL_ADCEx_Calibration_Start(&hadc1);
}

