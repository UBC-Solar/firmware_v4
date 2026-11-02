/**
 * @file    telemetry_driver.h
 * @brief   Telemetry driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the Telemetry driver functions for the TEL
 * board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
#ifndef __TELEMETRY_DRIVER__H__
#define __TELEMETRY_DRIVER__H__

#include "car_configs.h"
#include "cmsis_os2.h"
#include "stdlib.h"
#include "usart.h"
#include "xbee_platform.h"

#define TELEMETRY_UART (car_config_cellular ? &huart2 : &huart4)

#define TEL_DATA_LENGTH 8U
#define TEL_MSG_TYPEDEF_SIZE sizeof(RADIO_Msg_TypeDef)
#define TEL_QUEUE_SIZE 60

typedef struct
{
    uint64_t timestamp;
    char ID_DELIMETER;
    uint32_t can_id;
    uint8_t data[TEL_DATA_LENGTH];
    uint8_t data_len;
    char CARRIAGE_RETURN;
    char NEW_LINE;
} __attribute__((packed)) TEL_Msg_TypeDef;

typedef struct
{
    uint32_t dropped_telemetry_msg;
    uint32_t telemetry_hal_transmit_failures;
    uint32_t successful_telemetry_tx; /* UART DMA starts, not backend deliveries. */
    uint32_t cellular_modem_tx_success;
    uint32_t cellular_modem_tx_failures;
    uint32_t cellular_modem_status_count;
    volatile uint32_t cellular_rx_overflows;
    volatile uint32_t cellular_uart_rx_errors;
    uint8_t cellular_last_frame_id;
    uint8_t cellular_last_tx_status;
    uint8_t cellular_last_modem_status;
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
 * @retval HAL_OK DMA started; remote delivery is not confirmed.
 * @retval HAL_ERROR Invalid buffer/length or UART transmit failure.
 * @retval HAL_BUSY The UART is busy.
 * @retval HAL_TIMEOUT The UART transmit operation timed out.
 */
HAL_StatusTypeDef UART_telemetry_transmit(const uint8_t* data, uint16_t length);

/**
 * @brief   Clear the shared Digi transmit buffer and its write-error flag.
 * @note    Caller must hold usart1_tx_semaphore across reset, frame building, and
 *          XBeePlatform_Transmit(). If frame building fails, the caller must release it.
 * @retval  None
 */
void XBeePlatform_ResetTx(void);

/** @brief Start/rearm cellular interrupt RX. Radio mode is unaffected. */
HAL_StatusTypeDef XBeePlatform_StartRx(void);
/** @brief Called from the HAL UART RX complete/error callbacks. */
void XBeePlatform_RxComplete(UART_HandleTypeDef* uart);
void XBeePlatform_RxError(UART_HandleTypeDef* uart);

/**
 * @brief   Start DMA transmission of the buffered Digi API frame.
 * @note    Caller must hold usart1_tx_semaphore. Releases it on failure;
 *          the UART completion callback releases it after a successful DMA send.
 * @retval  HAL_OK DMA transmission started; remote delivery is not confirmed.
 * @retval  HAL_ERROR A buffered write failed, the buffer is empty, or UART failed.
 * @retval  HAL_BUSY The UART is busy.
 * @retval  HAL_TIMEOUT The UART transmit operation timed out.
 */
HAL_StatusTypeDef XBeePlatform_Transmit(void);

#endif /* __TELEMETRY_DRIVER__H__ */
