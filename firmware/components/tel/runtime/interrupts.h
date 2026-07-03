/**
 * @file    interrupts.h
 * @brief   Interrupt Service Routines (ISRs) for hardware events
 *
 * This file contains the prototypes and variables for the interrupt service routines across
 * all of UBC Solar's boards.
 */

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

/**
 * @brief  Tx Transfer completed callback for UART. Triggered by DMA when final byte is sent
 * 
 * If the uart is USART1, set the done_uart_tx flag to true so that next transmit over radio can occur
 * 
 * @param  huart: UART handle
 * 
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);

#endif // __INTERRUPTS_H__