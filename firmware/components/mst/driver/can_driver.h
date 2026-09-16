#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "can.h"

#include "mst_defs.h"

/**
 * CAN Protocol & STM32F103 constants
 */
#define CAN_MAX_DATAFRAME_BYTES 8U
#define NUM_CAN_TX_MAILBOXES 3U
#define CAN_FILTER_NUM_BANKS 14U
#define CAN_FILTER_NUM_ID_PER_BANK 2U

/**
 * Maximum number of CAN messages allowed at once inside the CAN queue.
 * Change as needed to accomodate all CAN messages transmitted around the same time
 */
#define CAN_TX_QUEUE_CAPACITY 32U


/**
 * CAN types
 */
typedef struct {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
} CAN_TxMessage_t;

typedef struct {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
    uint32_t timestamp;
} CAN_RxMessage_t;

typedef struct {
    CAN_HandleTypeDef *can_handle;
    volatile CAN_TxMessage_t tx_queue[CAN_TX_QUEUE_CAPACITY];
    volatile uint32_t tx_queue_push_index;
    volatile uint32_t tx_queue_pop_index;
    volatile CAN_RxMessage_t rx_message_0x450;
} CAN_Driver_t;



/**
 * Functions
 */

/**
 * @brief Configure STM32 CAN filter banks so that we only receive the listed standard CAN IDs
 *
 * @param handle CAN handle for the peripheral being configured
 * @param std_ids Array of standard 11-bit CAN IDs to accept
 * @param count Number of IDs in std_ids
 */
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count);

/**
 * @brief Start the CAN driver and peripheral
 *
 * @param handle CAN handle for the peripheral to use
 */
void CAN_Init(CAN_HandleTypeDef *handle);

/**
 * @brief Enqueue a frame for transmission
 *
 * @param message Pointer to buffer containing message to queue
 */
void CAN_QueueTxMessage(CAN_TxMessage_t *message);

void CAN_SendMessageXXX();
#if (UNIT_TEST_CAN == RUN)
/**
 * @brief Queue a fixed debug frame for CAN hardware tests
 */
void CAN_SendMessgeDebug();
#endif // UNIT_TEST_CAN

/**
 * @brief Consume one received frame from the RX FIFO
 */
void CAN_RecievedMessageCallback();

/**
 * @brief Advance the TX queue on mailbox-complete interrupt
 */
void CAN_TxCompleteCallback();

/**
 * @brief Recover the TX queue on per-mailbox transmit errors
 */
void CAN_ErrorCallback();
