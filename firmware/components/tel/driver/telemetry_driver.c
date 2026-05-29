#include "telemetry_driver.h"

telemetry_diagnostics_t telemetry_diagnostic;

void UART_telemetry_transmit(TEL_Msg_TypeDef* can_tel_msg)
{
    UART_HandleTypeDef *huart = CELLULAR ? &huart2 : &huart4;

    if (HAL_UART_Transmit_DMA(huart, (uint8_t *)can_tel_msg, sizeof(TEL_Msg_TypeDef)) != HAL_OK)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);
    } else {
        telemetry_diagnostic.successful_telemetry_tx++;
    }
}

/**
 * @brief  Tx Transfer completed callback for UART. Triggered by DMA when final byte is sent
 * 
 * If the uart is USART1, set the done_uart_tx flag to true so that next transmit over radio can occur
 * 
 * @param  huart: UART handle
 * 
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{ 
    if (huart->Instance == USART1)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
    }
}