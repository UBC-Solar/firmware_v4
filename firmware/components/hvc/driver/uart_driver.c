#include "uart_driver.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

UART_HandleTypeDef *huart;

void UART_Init(UART_HandleTypeDef *_huart){
    huart = _huart;
}

void UART_Transmit(const char *message){
    HAL_UART_Transmit(huart, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
}

void UART_Printf(const char *fmt, ...){
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    UART_Transmit(buffer);
}