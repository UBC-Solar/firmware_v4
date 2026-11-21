/**
 * @file can_driver.h
 * @brief bxCAN driver: RX filters, a software TX queue, and the HAL CAN callbacks.
 *
 * Knows nothing about what DIST messages mean: received frames go to CAN_Messages_HandleRx()
 * (can_messages.c), and can_messages.c builds every frame it queues.
 */

#pragma once

// Standard library
#include <stddef.h>
#include <stdint.h>

// HAL and CubeMX
#include "stm32f1xx_hal.h"

// CAN protocol and STM32F103 limits
#define CAN_MAX_DATAFRAME_BYTES     8U
#define CAN_FILTER_NUM_BANKS        14U // STM32F103 has 14 configurable filter banks
#define CAN_FILTER_NUM_ID_PER_BANK  2U  // Two 32-bit ID slots per bank in list mode

// Most messages the TX queue holds at once. Size it to the most messages queued at the same time.
#define CAN_TX_QUEUE_CAPACITY       32U

/**
 * @brief One frame to send: HAL TX header and data.
 */
typedef struct {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
} CAN_TxMessage_t;

/**
 * @brief One received frame: HAL RX header, data, and when it arrived.
 */
typedef struct {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
    uint32_t timestamp; // HAL_GetTick() when it was read from the RX FIFO
} CAN_RxMessage_t;

// Init: filters first, then CAN_Init()
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count);
void CAN_Init(CAN_HandleTypeDef *handle);

// TX
void CAN_QueueTxMessage(const CAN_TxMessage_t *message);

// Called from HAL callbacks (CAN_ErrorCallback() from HAL_CAN_ErrorCallback() in cube
// stm32f1xx_it.c)
void CAN_TxCompleteCallback(void);
void CAN_ErrorCallback(void);
