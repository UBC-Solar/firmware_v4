/**
 * @file dist_main.h
 * @brief DIST entry points for cube main.c: init and one pass of the main loop.
 *
 * Everything the DIST does starts from these two: cube main.c calls DIST_Init() once before its
 * main loop, then DIST_Main() every pass of it.
 */

#pragma once

// HAL and CubeMX
#include "stm32f1xx_hal.h" // The peripheral handle types (ADC_HandleTypeDef, ...)

// Called from cube main.c

void DIST_Init(ADC_HandleTypeDef *adc1, I2C_HandleTypeDef *led_i2c1, CAN_HandleTypeDef *can);
void DIST_Main(void);
