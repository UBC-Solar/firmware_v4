/**
 * @file    mcu_health_driver.c
 * @brief   MCU supply voltage and chip temperature from ADC1's built-in sensors.
 */

#include "mcu_health_driver.h"

// datasheet typical values 
#define VREFINT_MV 1200 // built-in reference voltage, 1160-1240 mV
#define V25_MV 1430     // temp sensor voltage at 25 degC, 1340-1520 mV

// per board temperature calibration: set to (true temp - reported temp) measured once at a
// known temperature, for exampleroom temp with the board idle. 0 = uncalibrated.
#define MCU_TEMP_OFFSET_C 0

#define ADC_POLL_TIMEOUT_MS 10

extern ADC_HandleTypeDef hadc1; // configured by CubeMX in main.c

/**
 * @brief Switches ADC1 to a channel, converts once, and returns the raw result (0-4095).
 * @param channel ADC channel to convert.
 * @param raw Output raw conversion result. Only written on HAL_OK.
 * @retval HAL_OK on success, otherwise the failing HAL call's status.
 */
static HAL_StatusTypeDef ReadAdc(uint32_t channel, uint16_t* raw);

static HAL_StatusTypeDef ReadAdc(uint32_t channel, uint16_t* raw)
{
    ADC_ChannelConfTypeDef config = {
        .Channel = channel,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5, // temp sensor needs >= 17 us
    };
    HAL_StatusTypeDef status;

    status = HAL_ADC_ConfigChannel(&hadc1, &config);
    if (status != HAL_OK)
    {
        return status;
    }

    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK)
    {
        return status;
    }

    // on timeout the register still holds the previous result
    status = HAL_ADC_PollForConversion(&hadc1, ADC_POLL_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return status;
    }

    *raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    return HAL_OK;
}

void McuHealthDriverInit(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1); // ADC measures and corrects its own offset error
}

HAL_StatusTypeDef McuHealthDriverReadVddMv(uint16_t* vdd_mv)
{
    uint16_t raw = 0;
    HAL_StatusTypeDef status = ReadAdc(ADC_CHANNEL_VREFINT, &raw);

    if (status != HAL_OK)
    {
        return status;
    }

    if (raw == 0U) // VREFINT can't read 0 on a working ADC
    {
        return HAL_ERROR;
    }

    // VREFINT is a fixed 1.2 V. The higher VDD is, the smaller 1.2 V reads.
    *vdd_mv = (uint16_t)(VREFINT_MV * 4095 / raw);
    return HAL_OK;
}

HAL_StatusTypeDef McuHealthDriverReadTempC(uint16_t vdd_mv, int16_t* temp_c)
{
    uint16_t raw = 0;
    HAL_StatusTypeDef status = ReadAdc(ADC_CHANNEL_TEMPSENSOR, &raw);

    if (status != HAL_OK)
    {
        return status;
    }

    // Temp sensor voltage drops 4.3 mV per degC
    int32_t sense_mv = (int32_t)raw * vdd_mv / 4095;
    *temp_c = (int16_t)((V25_MV - sense_mv) * 10 / 43 + 25 + MCU_TEMP_OFFSET_C);
    return HAL_OK;
}
