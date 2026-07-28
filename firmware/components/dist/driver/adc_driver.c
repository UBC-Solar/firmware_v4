#include "adc_driver.h"

/* TPS2596 current monitor gain: IILM/IOUT = 657 µA/A (typical, datasheet Eq. 6) */
#define GIMON              (653e-6f)

/* RILM resistor values per channel — sets both the current limit and monitor scale */
#define RILM_DRD           910.0f
#define RILM_MDI           7500.0f
#define RILM_SPARE_CTRL    910.0f
#define RILM_SPARE_MUX     910.0f
#define RILM_SPARE         910.0f

#define ADC_VREF           3.3f
#define ADC_COUNTS         4095.0f

void ADC_Driver_Init(void)
{
    /* STM32F1 ADC requires a calibration run after init — skipping this causes
     * readings to drift by several LSBs from the true value. */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * @brief Reconfigure hadc1 to the given channel, perform one software-triggered
 *        conversion, and return the raw 12-bit result.
 */
static uint16_t adc_read_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t value;

    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return value;
}

/* IOUT(A) = VILM / (GIMON × RILM),  VILM = (raw / 4095) × 3.3V  (Eq. 6) */
static float ilm_to_amps(uint16_t raw, float rilm)
{
    float vilm = ((float)raw / ADC_COUNTS) * ADC_VREF;
    return vilm / (GIMON * rilm);
}

/* Correction for 910Ω channels: measured = 0.961085×actual - 3.28378 mA
 * Inverted: actual = (measured + 3.28378 mA) / 0.961085 */
static float correct_910(float amps)
{
    return (amps + 0.00328378f) / 0.961085f;
}

uint16_t ADC_Read_ESTOP(void)
{
    return adc_read_channel(ADC_CHANNEL_10); /* PC0 */
}

AdcCurrentReadings ADC_Read_Currents(void)
{
    AdcCurrentReadings readings;

    readings.drd        = correct_910(ilm_to_amps(adc_read_channel(ADC_CHANNEL_14), RILM_DRD));        /* PC4 */
    readings.mdi        = ilm_to_amps(adc_read_channel(ADC_CHANNEL_7),  RILM_MDI);        /* PA7 */
    readings.spare      = correct_910(ilm_to_amps(adc_read_channel(ADC_CHANNEL_4),  RILM_SPARE));      /* PA4 */
    readings.spare_mux  = correct_910(ilm_to_amps(adc_read_channel(ADC_CHANNEL_5),  RILM_SPARE_MUX));  /* PA5 */
    readings.spare_ctrl = correct_910(ilm_to_amps(adc_read_channel(ADC_CHANNEL_6),  RILM_SPARE_CTRL)); /* PA6 */

    return readings;
}
