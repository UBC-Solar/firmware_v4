#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"

UART_HandleTypeDef *huart;

void UART_Init(UART_HandleTypeDef *huart);
void UART_Transmit(const char *message);
void UART_Printf(const char *fmt, ...);