/**<
 * @file    telemetry_driver.c
 * @brief   Telemetry driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the telemetry driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
#include "telemetry_driver.h"

telemetry_diagnostics_t telemetry_diagnostic;

void UART_telemetry_transmit(const uint8_t* buf, uint16_t len)
{
    // J-Link does not supply enough current for the Radio Module. Only use ST-Link for this.
    if (!car_config_cellular) {
        HAL_GPIO_WritePin(R_RESET_GPIO_Port, R_RESET_Pin, GPIO_PIN_SET);
    }

    UART_HandleTypeDef *huart = car_config_cellular ? &huart2 : &huart4;

    if (HAL_UART_Transmit_DMA(huart, buf, len) != HAL_OK)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);
    } else {
        telemetry_diagnostic.successful_telemetry_tx++;
    }
}