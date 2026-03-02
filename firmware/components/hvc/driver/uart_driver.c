#include "uart_driver.h"
#include "stm32f1xx_hal_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

UART_HandleTypeDef *huart;

volatile uint8_t uart_tx_buffer[256];

void UART_Init(UART_HandleTypeDef *_huart) {
    huart = _huart;
}

void UART_Transmit(const char *message) {
    if (HAL_UART_GetState(huart) != HAL_UART_STATE_READY)
        HAL_Delay(1);
    if (HAL_UART_GetState(huart) != HAL_UART_STATE_READY)
        return;

    size_t len = strlen(message);
    if (len >= sizeof(uart_tx_buffer)) {
        len = sizeof(uart_tx_buffer) - 1; // Truncate if message is too long
    }

    memcpy((uint8_t *)uart_tx_buffer, message, len);
    HAL_UART_Transmit_DMA(huart, (uint8_t *)uart_tx_buffer, len);
}

void UART_Printf(const char *fmt, ...) {
    if (HAL_UART_GetState(huart) != HAL_UART_STATE_READY)
        HAL_Delay(1);
    if (HAL_UART_GetState(huart) != HAL_UART_STATE_READY)
        return;

    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    size_t len = strlen(message);
    if (len >= sizeof(uart_tx_buffer)) {
        len = sizeof(uart_tx_buffer) - 1; // Truncate if message is too long
    }

    memcpy((uint8_t *)uart_tx_buffer, message, len);
    HAL_UART_Transmit_DMA(huart, (uint8_t *)uart_tx_buffer, len);
}