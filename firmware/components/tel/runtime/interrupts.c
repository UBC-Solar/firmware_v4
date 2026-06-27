#include "usart.h"
#include "telemetry_driver.h"

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
    if (huart->Instance == UART4 || huart->Instance == USART2)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
    }
}