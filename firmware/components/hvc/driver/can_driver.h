#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "can.h"

#include "hvc_main.h"


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
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count);
void CAN_Init(CAN_HandleTypeDef *handle);

void CAN_QueueTxMessage(CAN_TxMessage_t *message);

void CAN_SendMessageXXX();
void CAN_SendStatusMsg();
void CAN_SendMessage323();
#if (INT_TEST_CAN == RUN)
void CAN_SendMessgeDebug();
#endif // UNIT_TEST_CAN

void CAN_RecievedMessageCallback(uint32_t fifo_num);

void CAN_TxCompleteCallback();
void CAN_ErrorCallback();
