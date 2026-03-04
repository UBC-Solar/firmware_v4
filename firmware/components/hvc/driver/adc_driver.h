#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_tim.h"

#define ADC1_NUM_CHANNELS 5
#define ADC1_SAMPLE_COUNT 100

#define ADC_RESOLUTION 4095
#define ADC_VOLTAGE_REFERENCE 3300 // mV

/**
 * @brief Raw 12-bit ADC readings, one per channel, in scan order.
 */
typedef struct {
    uint16_t dcdc_thermistor;     
    uint16_t motor_precharge;
    uint16_t mppt_precharge; 
    uint16_t supp_sense;     
    uint16_t lv_curr_sense;  
} ADC_Readings;

/**
 * @brief ADC values converted to millivolts (0–3300 mV).
 */
typedef struct {
    uint16_t dcdc_thermistor;     
    uint16_t motor_precharge;
    uint16_t mppt_precharge; 
    uint16_t supp_sense;     
    uint16_t lv_curr_sense;  
} ADC_Voltages;

void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3);

ADC_Readings ADC_GetReadings(void);
ADC_Voltages ADC_GetVoltages(void);