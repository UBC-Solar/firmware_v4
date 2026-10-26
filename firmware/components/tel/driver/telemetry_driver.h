/**
 * @file    telemetry_driver.h
 * @brief   Telemetry driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the Telemetry driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
#ifndef __TELEMETRY_DRIVER__H__
#define __TELEMETRY_DRIVER__H__

#include "usart.h"
#include "stdlib.h"
#include "cmsis_os2.h"
#include "car_configs.h"
#include "xbee_platform.h"

#define TELEMETRY_UART                           (car_config_cellular ? &huart2 : &huart4)

#define TEL_DATA_LENGTH                           8U
#define TEL_MSG_TYPEDEF_SIZE                      sizeof(RADIO_Msg_TypeDef)
#define TEL_QUEUE_SIZE                            60

typedef struct {
    uint64_t timestamp;
    char ID_DELIMETER;
    uint32_t can_id;
    uint8_t data[TEL_DATA_LENGTH];
    uint8_t data_len;
    char CARRIAGE_RETURN;
    char NEW_LINE;
} __attribute__((packed)) TEL_Msg_TypeDef;

typedef struct {
   uint32_t dropped_telemetry_msg;
   uint32_t telemetry_hal_transmit_failures;
   uint32_t successful_telemetry_tx;
} telemetry_diagnostics_t;


/* TODO: SEND THIS OVER CAN AND SEE ON GRAFANA NUMBER */
extern telemetry_diagnostics_t telemetry_diagnostic;
extern osSemaphoreId_t usart1_tx_semaphore;

/**
  * @brief Starts DMA transmission on the selected telemetry UART.
  * @param data Buffer that must remain valid and unchanged until DMA completes.
  * @param length Number of bytes to send.
  * @note Caller must hold usart1_tx_semaphore before preparing the buffer.
  *       Released here on failure, or by the UART callback on completion.
  * @retval HAL_OK means DMA started, not that the data reached the receiver.
  */
HAL_StatusTypeDef UART_telemetry_transmit(const uint8_t *data, uint16_t length);

/* Hold the transmit semaphore across reset, Digi frame building, and transmit.
 * Transmit releases it on failure; DMA completion releases it on success. */
void XBeePlatform_ResetTx(void);
HAL_StatusTypeDef XBeePlatform_Transmit(void);

#endif /* __TELEMETRY_DRIVER__H__ */