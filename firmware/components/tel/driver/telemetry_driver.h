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
  * @brief Transmits a telemetry message via UART on the link selected by CAR_CONFIG_CELLULAR
  * @param buf Bytes to transmit. Sent by DMA, so it must stay valid until usart1_tx_semaphore is released
  * @param len Number of bytes to transmit
  */
void UART_telemetry_transmit(const uint8_t* buf, uint16_t len);

#endif /* __TELEMETRY_DRIVER__H__ */