/**
 * @file adc_driver.h
 * @brief ADC1 raw readings for the DIST's current sense inputs.
 *
 * Gives raw 12-bit counts only; adc_runtime.c converts them to currents.
 */

#pragma once

// Standard library
#include <stdint.h>

// HAL and CubeMX
#include "stm32f1xx_hal.h"

#define ADC_VREF_V                  3.3f    // ADC reference voltage (V)
#define ADC_FULL_SCALE_COUNTS       4095.0f // Highest 12-bit reading

/**
 * @brief One raw reading (0-4095) of every current sense channel.
 */
typedef struct {
    uint16_t drd_ilm;        // DRD eFuse ILM pin (PC4, IN14)
    uint16_t mdi_ilm;        // MDI eFuse ILM pin (PA7, IN7)
    uint16_t spare_ctrl_ilm; // SPARE_CTRL eFuse ILM pin (PA6, IN6)
    uint16_t spare_mux_ilm;  // SPARE_MUX eFuse ILM pin (PA5, IN5)
    uint16_t spare_ilm;      // SPARE eFuse ILM pin (PA4, IN4)
    uint16_t tel_sense;      // TEL current sensor output (PC5, IN15)
} ADC_Readings;

void ADC_Init(ADC_HandleTypeDef *_hadc1);
ADC_Readings ADC_GetReadings(void);
