#pragma once

#include "adc.h"
#include <stdint.h>

/** Load current in amperes for all five eFuse output channels.
 *  Derived from the ILM pin voltage using GIMON = 657 µA/A (typ) and
 *  the per-channel RILM resistor values. */
typedef struct {
    float drd;
    float mdi;
    float spare_ctrl;
    float spare_mux;
    float spare;
} AdcCurrentReadings;

/**
 * @brief Calibrate the ADC. Must be called once after MX_ADC1_Init().
 *        STM32F1 ADC accuracy degrades significantly without calibration.
 */
void ADC_Driver_Init(void);

/**
 * @brief Read the ESTOP ADC channel (PC0 / IN10).
 * @return Raw 12-bit count (0–4095).
 */
uint16_t ADC_Read_ESTOP(void);

/**
 * @brief Read all five eFuse current sense channels and convert to amperes.
 *        Uses datasheet Equation 6: IOUT = VILM / (GIMON × RILM).
 * @return Struct containing load current in amperes for each channel.
 */
AdcCurrentReadings ADC_Read_Currents(void);
