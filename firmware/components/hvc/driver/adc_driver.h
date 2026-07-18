#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_tim.h"

#define ADC1_NUM_CHANNELS 5
#define ADC1_SAMPLE_COUNT 100

#define ADC_RESOLUTION 4095
#define ADC_VOLTAGE_REFERENCE 3300 // mV

#define THERMISTOR_LUT_TABLE_SIZE 17

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

typedef struct
{
    int32_t temperature_mC;
    uint32_t voltage_uV;
    uint32_t resistance_Ohm;
} thermistor_mapping_t;

void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3);

ADC_Readings ADC_GetReadings(void);
ADC_Voltages ADC_GetVoltages(void);

void InitTemperatureReading();
int32_t ThermistorVoltToTemp_(uint32_t thermistor_uV);
