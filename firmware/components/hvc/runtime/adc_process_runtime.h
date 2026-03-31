#pragma once

#include <stdint.h>

#define SUPP_DIVIDER_RATIO 0.175f // 1k / (4.7k + 1k) = 0.175

#define LV_CURR_SENSE_SENSITIVITY 0.2 // mV/mA
#define LV_CURR_SENSE_ZERO_CURRENT_OFFSET 4095 / 10 // adc bits at 0 mA, see TMCS2209A3U datasheet

typedef struct {
    uint16_t dcdc_thermistor; // C
    uint16_t motor_precharge; // mV
    uint16_t mppt_precharge; // mV
    uint16_t supp_sense; // mV
    uint16_t lv_curr_sense; // mA
} ADC_Values;

void ADC_Runtime_ProcessReadings(void);

ADC_Values ADC_Runtime_GetValues(void);