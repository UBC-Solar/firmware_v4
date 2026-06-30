/**
 * @file    interrupts.c
 * @brief   Interrupt Service Routines (ISRs) for hardware events
 *
 * This file contains the implementation of all hardware interrupt service routines across
 * all of UBC Solar's boards.
 */

#include "usart.h"
#include "telemetry_driver.h"

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4 || huart->Instance == USART2)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
    }
}