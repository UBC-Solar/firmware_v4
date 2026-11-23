/**
 * @file hvc_main.h
 * @brief HVC entry points for cube main.c: init and one pass of the main loop.
 *
 * Everything the HVC does starts from these two: cube main.c calls HVC_Init() once before its main
 * loop, then HVC_Main() every pass of it.
 */

#pragma once

// HAL and CubeMX
#include "stm32f1xx_hal.h" // The peripheral handle types (UART_HandleTypeDef, ...)

// Called from cube main.c

void HVC_Init(
    UART_HandleTypeDef *uart2,
    ADC_HandleTypeDef *adc1,
    TIM_HandleTypeDef *adc_trigger_tim3,
    TIM_HandleTypeDef *fan_pwm_tim4,
    I2C_HandleTypeDef *shunt_i2c1,
    CAN_HandleTypeDef *can);
void HVC_Main(void);
