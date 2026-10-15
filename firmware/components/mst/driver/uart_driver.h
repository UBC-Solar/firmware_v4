#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"

extern UART_HandleTypeDef *huart;

/**
 * @brief Remember which UART to log over
 *
 * @param _huart HAL UART handle to use for transmission
 */
void UART_Init(UART_HandleTypeDef *_huart);

/**
 * @brief Send a log string over UART via DMA
 *
 * @param message Null-terminated string to transmit
 */
void UART_Transmit(const char *message);

/**
 * @brief Send a formatted log message over UART via DMA
 *
 * @param fmt printf-style format string followed by arguments
 */
void UART_Printf(const char *fmt, ...);
