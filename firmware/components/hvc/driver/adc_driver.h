#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_tim.h"

#define ADC1_NUM_CHANNELS 5
#define ADC1_SAMPLE_COUNT 100

#define ADC_RESOLUTION 4095
#define ADC_VOLTAGE_REFERENCE 3300000 // in microvolts (3.3V)

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
 * @brief ADC values converted to microvolts (0–3,300,000 uV), precision is 800 uV.
 */
typedef struct {
    uint32_t dcdc_thermistor;     
    uint32_t motor_precharge;
    uint32_t mppt_precharge; 
    uint32_t supp_sense;     
    uint32_t lv_curr_sense;  
} ADC_Voltages;

extern volatile uint16_t adc_buffer[ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2];

extern ADC_Readings adc1_readings;
extern ADC_Voltages adc1_voltages;

extern int callback_count;
extern int last_half;

void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3);

void ADC1_ProcessReadings(int half);