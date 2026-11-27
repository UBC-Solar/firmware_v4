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

static bool telemetry_is_cellular = false;

void TelemetryDriverSetCellular(bool is_cellular)
{
    telemetry_is_cellular = is_cellular;
}

void TelemetryDriverInit(void)
{
    TelemetryDriverSetCellular(HAL_GPIO_ReadPin(RADIO_CEL_STATE_GPIO_Port, RADIO_CEL_STATE_Pin) == GPIO_PIN_SET);
}

void UART_telemetry_transmit(TEL_Msg_TypeDef* can_tel_msg)
{
    // J-Link does not supply enough current for the Radio Module. Only use ST-Link for this.
    if (!telemetry_is_cellular) {
        HAL_GPIO_WritePin(R_RESET_GPIO_Port, R_RESET_Pin, GPIO_PIN_SET);
    }

    UART_HandleTypeDef *huart = telemetry_is_cellular ? &huart2 : &huart4;

    if (HAL_UART_Transmit_DMA(huart, (uint8_t *)can_tel_msg, sizeof(TEL_Msg_TypeDef)) != HAL_OK)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);
    } else {
        telemetry_diagnostic.successful_telemetry_tx++;
    }
}