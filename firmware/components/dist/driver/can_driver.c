/**
 * @file can_driver.c
 * @brief bxCAN driver: RX filters, a software TX queue, and the HAL CAN callbacks.
 *
 * TX: CAN_QueueTxMessage() copies a frame into a ring buffer and starts it if a mailbox is free;
 * each TX-complete (or abort, or error) interrupt starts the next queued frame.
 * RX: each RX FIFO interrupt reads one frame and passes it to CAN_Messages_HandleRx().
 */

#include "can_driver.h"

// Standard library
#include <stdbool.h>
#include <string.h>

// HAL and CubeMX
#include "main.h" // Error_Handler()

// DIST app
#include "can_messages.h" // CAN_Messages_HandleRx()

static void TryTransmitFromQueue(void);
static void ReceiveFromFifo(uint32_t fifo);

/**
 * @brief Driver state: the peripheral handle and the TX ring buffer.
 *
 * The queue is empty when push and pop indexes are equal, so it holds at most
 * CAN_TX_QUEUE_CAPACITY - 1 frames.
 */
typedef struct {
    CAN_HandleTypeDef *can_handle;
    volatile CAN_TxMessage_t tx_queue[CAN_TX_QUEUE_CAPACITY];
    volatile uint32_t tx_queue_push_index;
    volatile uint32_t tx_queue_pop_index;
} CAN_Driver_t;

static CAN_Driver_t can_driver;

// ==== Init ====

/**
 * @brief Configures bxCAN ID-list filters for a list of standard 11-bit IDs.
 *
 * Call before CAN_Init().
 *
 * How the filters work (STM32F103 reference manual, p. 664 onward):
 *   The bxCAN has 14 filter banks, each two 32-bit registers (FilterId and FilterMask).
 *   In list mode (used here) both registers hold a complete ID, and a frame is accepted only if
 *   its ID matches one exactly, so each bank holds two IDs.
 *   In a 32-bit register, bits [31:21] hold the 11-bit standard ID.
 * Banks alternate between RX FIFO 1 and FIFO 0 to spread the load.
 *
 * @param handle  CAN handle for the peripheral being configured.
 * @param std_ids Standard 11-bit CAN IDs to accept.
 * @param count   Number of IDs in std_ids. An odd count repeats the last ID in its bank.
 */
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count) {
    const uint32_t max_ids = CAN_FILTER_NUM_BANKS * CAN_FILTER_NUM_ID_PER_BANK;
    CAN_FilterTypeDef filter_config = {0};
    size_t id_index = 0U;
    uint32_t bank_index = 0U;

    if ((handle == NULL) || (std_ids == NULL) || (count == 0U) || (count > max_ids)) {
        Error_Handler();
    }

    while ((id_index < count) && (bank_index < CAN_FILTER_NUM_BANKS)) {
        uint16_t id_first = std_ids[id_index];
        uint16_t id_second = id_first;

        if ((id_index + 1U) < count) {
            id_second = std_ids[id_index + 1U];
        }

        filter_config.FilterActivation = CAN_FILTER_ENABLE;
        filter_config.SlaveStartFilterBank = CAN_FILTER_NUM_BANKS;
        filter_config.FilterBank = bank_index;
        filter_config.FilterFIFOAssignment = (bank_index % 2U) ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
        filter_config.FilterMode = CAN_FILTERMODE_IDLIST;
        filter_config.FilterScale = CAN_FILTERSCALE_32BIT;

        // Shift 5: the 11-bit ID goes in the top bits of the high half-word
        filter_config.FilterIdHigh = (uint16_t)((id_first & 0x7FFU) << 5);
        filter_config.FilterIdLow = 0U;
        filter_config.FilterMaskIdHigh = (uint16_t)((id_second & 0x7FFU) << 5);
        filter_config.FilterMaskIdLow = 0U;

        if (HAL_CAN_ConfigFilter(handle, &filter_config) != HAL_OK) {
            Error_Handler();
        }

        id_index += CAN_FILTER_NUM_ID_PER_BANK;
        bank_index++;
    }
}

/**
 * @brief Clears the driver state, turns on the TX and RX interrupts, and starts the CAN peripheral.
 *
 * Call once, after CAN_InitFilterList() and before queueing any message.
 *
 * @param handle CAN handle (hcan), already set up by MX_CAN_Init().
 */
void CAN_Init(CAN_HandleTypeDef *handle) {
    memset((void*)&can_driver, 0, sizeof(can_driver));
    can_driver.can_handle = handle;

    if (HAL_CAN_ActivateNotification(can_driver.can_handle,
                                     CAN_IT_TX_MAILBOX_EMPTY |
                                     CAN_IT_RX_FIFO0_MSG_PENDING |
                                     CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK) {
        Error_Handler();
    }

    HAL_CAN_Start(can_driver.can_handle);
}

// ==== TX ====

/**
 * @brief Queues a frame to send. Frames are sent in the order they are queued.
 *
 * Non-blocking: the frame is copied into the queue before this returns, and sent from a TX
 * interrupt if no mailbox is free now.
 * A full queue calls Error_Handler().
 *
 * @param message Frame to queue.
 */
void CAN_QueueTxMessage(const CAN_TxMessage_t *message) {
    uint32_t next_push_index;

    // Critical section: a TX-complete interrupt also moves the queue indexes
    HAL_NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);

    next_push_index = (can_driver.tx_queue_push_index + 1U) % CAN_TX_QUEUE_CAPACITY;
    if (next_push_index == can_driver.tx_queue_pop_index) {
        Error_Handler(); // Queue full
    }

    memcpy((void*)&can_driver.tx_queue[can_driver.tx_queue_push_index], message,
           sizeof(CAN_TxMessage_t));
    can_driver.tx_queue_push_index = next_push_index;

    TryTransmitFromQueue();

    HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
}

/**
 * @brief Moves the oldest queued frame into a free TX mailbox, if there is a frame and a mailbox.
 */
static void TryTransmitFromQueue(void) {
    CAN_TxMessage_t *next_message;
    uint32_t mailbox;

    if (can_driver.tx_queue_pop_index == can_driver.tx_queue_push_index) {
        return; // Queue empty
    }
    if (HAL_CAN_GetTxMailboxesFreeLevel(can_driver.can_handle) == 0U) {
        return; // Sent later, from the TX-complete interrupt
    }

    next_message = (CAN_TxMessage_t*)&can_driver.tx_queue[can_driver.tx_queue_pop_index];
    if (HAL_CAN_AddTxMessage(can_driver.can_handle, &next_message->tx_header, next_message->data,
                             &mailbox) != HAL_OK) {
        return;
    }

    can_driver.tx_queue_pop_index = (can_driver.tx_queue_pop_index + 1U) % CAN_TX_QUEUE_CAPACITY;
}

/**
 * @brief Starts the next queued frame after a mailbox completes, aborts, or errors.
 */
void CAN_TxCompleteCallback(void) {
    TryTransmitFromQueue();
}

/**
 * @brief Frees the TX queue after a transmission error.
 *
 * Automatic retransmission is off, so a frame nobody acknowledges (no other board on the bus)
 * ends in a TX error instead of retrying forever.
 * Each errored mailbox starts the next queued frame, so the queue does not stall.
 * Called from HAL_CAN_ErrorCallback() in cube stm32f1xx_it.c.
 */
void CAN_ErrorCallback(void) {
    if (can_driver.can_handle->ErrorCode & HAL_CAN_ERROR_TX_TERR0) {
        CAN_TxCompleteCallback();
    }
    if (can_driver.can_handle->ErrorCode & HAL_CAN_ERROR_TX_TERR1) {
        CAN_TxCompleteCallback();
    }
    if (can_driver.can_handle->ErrorCode & HAL_CAN_ERROR_TX_TERR2) {
        CAN_TxCompleteCallback();
    }
}

// ==== RX ====

/**
 * @brief Reads one frame from an RX FIFO and passes it to CAN_Messages_HandleRx().
 * @param fifo CAN_RX_FIFO0 or CAN_RX_FIFO1.
 */
static void ReceiveFromFifo(uint32_t fifo) {
    CAN_RxMessage_t rx_message;

    if (HAL_CAN_GetRxMessage(can_driver.can_handle, fifo, &rx_message.rx_header,
                             rx_message.data) != HAL_OK) {
        Error_Handler();
    }
    rx_message.timestamp = HAL_GetTick();

    CAN_Messages_HandleRx(&rx_message);
}

// ==== HAL callbacks ====
// Weak HAL functions overridden here. HAL_CAN_ErrorCallback() is in cube stm32f1xx_it.c.

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ReceiveFromFifo(CAN_RX_FIFO0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ReceiveFromFifo(CAN_RX_FIFO1);
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

// Aborted frames count as done, so the queue does not stall
void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}
