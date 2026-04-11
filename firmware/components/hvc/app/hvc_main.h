#pragma once

#include "stm32f1xx_hal.h"

void HVC_Init(UART_HandleTypeDef *_huart2, ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3, I2C_HandleTypeDef *_hi2c1);
void HVC_Main(void);