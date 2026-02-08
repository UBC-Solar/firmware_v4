/**
 * @file    drivers.c
 * @brief   Hardware driver implementations for DRD board peripherals
 * 
 * This file contains the implementation of all hardware drivers for this board component of UBC Solar
 * firmware. Drivers provide abstraction layers between high-level application code and low-level
 * hardware peripherals, enabling consistent interfaces and easier code maintenance.
 * 
 * Drivers implemented here include:
 * - DRD board-specific peripherals
 * 
 * @author  UBC Solar
 * @date    Feb 7 2026
 */

#include "drivers.h"

/* FUNCTION DECLARATIONS */
static uint16_t read_adc(ADC_HandleTypeDef* hadc);

/* DRIVE STATE DRIVERS */
uint8_t gpio_read_pin(GPIO_TypeDef* port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

void gpio_toggle_pin(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}

uint16_t adc_read_accel_1(void) { return read_adc(&hadc1); }
uint16_t adc_read_accel_2(void) { return read_adc(&hadc2); }

static uint16_t read_adc(ADC_HandleTypeDef* hadc)
{
	HAL_ADC_Start(hadc);
	HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY);
	return HAL_ADC_GetValue(hadc);
}