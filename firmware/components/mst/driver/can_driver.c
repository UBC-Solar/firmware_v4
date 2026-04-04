#include "can_driver.h"

#include "stm32f1xx_hal_def.h"

CAN_Driver_t CAN_driver;

static void tryTransmitFromQueue()
{
    uint32_t mailbox;

    if (HAL_CAN_GetTxMailboxesFreeLevel(CAN_driver.can_handle) == 0)
    {
        return;
    }

    CAN_TxMessage_t *next_message = (CAN_TxMessage_t *) &CAN_driver.tx_queue[CAN_driver.tx_queue_pop_index];
    HAL_StatusTypeDef canStatus = HAL_CAN_AddTxMessage(
        CAN_driver.can_handle, 
        &next_message->tx_header, 
        next_message->data, 
        &mailbox);

    if (HAL_OK != canStatus)
    {
        Error_Handler();
    }
    CAN_driver.tx_queue_pop_index = (CAN_driver.tx_queue_pop_index + 1U) % CAN_TX_QUEUE_CAPACITY;
}

/**
 * Queue a message to send over CAN. Commands will be sent in order of queuing as soon as bus is available after calling startTx().
 *
 * This function is non-blocking and commands will be transmitted asynchronously.
 * The message buffer is copied into a queue buffer before the function returns.
 *
 * @param message Pointer to buffer containing message to queue
 */
static void queueCanMessage(CAN_TxMessage_t *message)
{
    // Start critical section - do not want a CAN TX complete interrupt to be serviced during this function call
    HAL_NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);

    uint32_t next_push_index = (CAN_driver.tx_queue_push_index + 1U) % CAN_TX_QUEUE_CAPACITY;

    // Check if there is space in queue
    if (next_push_index == CAN_driver.tx_queue_pop_index)
    {
        Error_Handler();
    }

    volatile CAN_TxMessage_t *next_free_queue_slot = &CAN_driver.tx_queue[CAN_driver.tx_queue_push_index];
    memcpy((uint8_t *) next_free_queue_slot, (uint8_t *) message, sizeof(CAN_TxMessage_t));
    CAN_driver.tx_queue_push_index = next_push_index;

    tryTransmitFromQueue();

    HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn); // End critical section
}


/**
 * @brief Configure bxCAN filters for a list of standard 11-bit IDs.
 *
 * Filter Information (see page 664 onward of stm32f103 reference manual)
 *
 * In ARM, CAN subsystem known as bxCAN (basic-extended)
 * 14 configurable filter banks (STM32F103 has only one CAN interface)
 * Each filter bank consists of two, 32-bit registers, CAN_FxR0 and CAN_FxR1
 * Filter scale: a filter bank contains either two 32-bit filters registers (currently implemented), 
 *               or four 16 bit filters registers
 * Filter mode: choose between mask and list mode
 *  - Mask mode: 
 *      - Use FilterMask (first 32-bit reg) to mark each bit of CAN ID as "must match" or "don't care"
 *      - Use FilterId (second 32-bit reg) to provide the bit pattern to "match" against
 *  - List mode (currently implemented): 
 *      - incoming ID must match exactly to what is specified in the filters
 *      - note we have two 32-bit registers, "FilterMask" and "FilterId", per bank. These are
 *        instead repurposed to contain two separate, complete filter IDs to match against
 * For a 32 bit filter register, [31:21] map to STID [10:0], other bits we don't care about.
 *
 * Other V3 CAN driver documentation: 
 *  - Filter mask mode explanation: https://www.microchip.com/forums/m456043.aspx
 *  - Guide on filter config: https://controllerstech.com/can-protocol-in-stm32/
 *
 * @param handle CAN handle for the peripheral being configured.
 * @param std_ids Array of standard 11-bit CAN IDs to accept.
 * @param count Number of IDs in std_ids.
 */
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count)
{
    const uint32_t max_ids = CAN_FILTER_NUM_BANKS * CAN_FILTER_NUM_ID_PER_BANK;

    if ((handle == NULL) || (std_ids == NULL) || (count == 0U) || (count > max_ids))
    {
        Error_Handler();
    }

    CAN_FilterTypeDef filter_config;
    size_t id_index = 0U;
    uint32_t bank_index = 0U;

    while ((id_index < count) && (bank_index < CAN_FILTER_NUM_BANKS))
    {
        // If even number of IDs, put each ID in a separate register
        // If odd number of IDs, the last bank will contain two duplicate IDs
        uint16_t id_first = std_ids[id_index];
        uint16_t id_second = id_first;
        if ((id_index + 1U) < count)
        {
            id_second = std_ids[id_index + 1U];
        }

        filter_config.FilterActivation = CAN_FILTER_ENABLE;
        filter_config.SlaveStartFilterBank = CAN_FILTER_NUM_BANKS;

        // FIFO assignment: determines which FIFO (0 or 1) to store received messages.
        // Here we'll "try to" evenly distribute the received messages.
        filter_config.FilterBank = bank_index;
        filter_config.FilterFIFOAssignment = (bank_index % 2) ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
        filter_config.FilterMode = CAN_FILTERMODE_IDLIST;
        filter_config.FilterScale = CAN_FILTERSCALE_32BIT;

        // Bitshift 5: the 11 most-significant-bits of the 16 bit integer are used as the ID to filter
        filter_config.FilterIdHigh = (uint16_t)((id_first & 0x7FFU) << 5);
        filter_config.FilterIdLow = 0U;

        // Bitshift 5, same as above
        filter_config.FilterMaskIdHigh = (uint16_t)((id_second & 0x7FFU) << 5);
        filter_config.FilterMaskIdLow = 0U;

        if (HAL_CAN_ConfigFilter(handle, &filter_config) != HAL_OK)
        {
            Error_Handler();
        }

        id_index += CAN_FILTER_NUM_ID_PER_BANK;
        bank_index += 1U;
    }
}


/**
 * @brief Initialize CAN data, configures interrupts, and starts the CAN hardware
 * NOTE: filters should be configured before calling this function
 * 
 * Call this function once before sending any CAN messages
 */
void CAN_Init(CAN_HandleTypeDef *handle)
{
    memset((uint8_t *) &CAN_driver, 0, sizeof(CAN_driver));

    CAN_driver.can_handle = handle;

    // Activate interrupt for completion of message transmission
    if (HAL_CAN_ActivateNotification(CAN_driver.can_handle, CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_CAN_Start(CAN_driver.can_handle);
}


/**
 * @brief TEMPLATE function for sending CAN messages
 *
 * @param pack pack data structure that data will be read from
 */
void CAN_SendMessageXXX()
{
    CAN_TxMessage_t txMessage = {0};

    // Note: replace with actual values
    txMessage.tx_header.StdId = 0x0U;
    txMessage.tx_header.DLC = 0;

    // Note: modify txMessage.data

    queueCanMessage(&txMessage);
}


#if (UNIT_TEST_CAN == RUN)
/**
 * @brief Send DEBUG CAN message intended for hardware unit tests
 *
 * @param pack pack data structure that data will be read from
 */
void CAN_SendMessgeDebug()
{
    CAN_TxMessage_t txMessage = {0};

    txMessage.tx_header.StdId = 0x42U;
    txMessage.tx_header.DLC = 8;

    // Data contains alternating bytes of all 0's and all 1's
    for (int i = 0; i < 8; i++)
    {
        txMessage.data[i] = (i % 2) ? 0x00U : 0xFFU;
    }

    queueCanMessage(&txMessage);
}
#endif // UNIT_TEST_CAN 


/**
 * @brief TEMPLATE function to be registered for receiving CAN messages
 * 
 * TODO: register this function to be called inside the appropriate "Rx FIFO pending callback"
 * as determined by the filter configuration. I.e. one of:
 *  - HAL_CAN_RxFifo0MsgPendingCallback()
 *  - HAL_CAN_RxFifo1MsgPendingCallback()
 */
void CAN_RecievedMessageCallback()
{
    CAN_RxMessage_t new_rx_message;
    if (HAL_CAN_GetRxMessage(
        CAN_driver.can_handle, 
        0, 
        (CAN_RxHeaderTypeDef *) &new_rx_message.rx_header, 
        (uint8_t *) new_rx_message.data) != HAL_OK)
    {
        Error_Handler();
    }

    new_rx_message.timestamp = HAL_GetTick();

    /**
     * TODO: do something with the new_rx_message!
     *  Do you want to add it to some global data structure for futur processing?
     *  Or do you want to call your own functions here to parse into it right away?
     */
}

/**
 * @brief Handle a CAN TX complete interrupt for any of the 3 CAN TX mailboxes
 * 
 * TODO: register this function to be called inside all of the following:
 *  - HAL_CAN_TxMailbox0CompleteCallback()
 *  - HAL_CAN_TxMailbox1CompleteCallback()
 *  - HAL_CAN_TxMailbox2CompleteCallback()
 */
void CAN_TxCompleteCallback()
{
    // Initiate next message transmission if possible
    if (CAN_driver.tx_queue_pop_index != CAN_driver.tx_queue_push_index)
    {
        tryTransmitFromQueue();
    }
}

/**
 * @brief Handle a CAN TX complete interrupt with error for any of the 3 CAN TX mailboxes
 * 
 * TODO: register this function to be called from HAL_CAN_ErrorCallback()
 * 
 * @note This function assumes auto retransmission is ENABLED!
 */
void CAN_ErrorCallback()
{
    // This callback is needed if automatic retransmission is DISABLED in the CAN peripheral's configuration
    // If board is disconnected from CAN bus, TX requests will complete with error
    // due to lack of CAN message acknowledgement from another device.
    // Continue as if this was a clean transmission completion; note that multiple
    // TX mailboxes may need an error serviced in one interrupt

    // With automatic retransmission ENABLED, and board is disconnected from CAN bus or no other devices on bus are active),
    // messages go unacknowledged, do not complete and are repeatedly sent indefinitely.
    // The CAN_driver.tx_queue will overflow and the firmware will fault

    if (CAN_driver.can_handle->ErrorCode & (HAL_CAN_ERROR_TX_TERR0))
    {
        CAN_TxCompleteCallback();
    }
    if (CAN_driver.can_handle->ErrorCode & (HAL_CAN_ERROR_TX_TERR1))
    {
        CAN_TxCompleteCallback();
    }
    if (CAN_driver.can_handle->ErrorCode & (HAL_CAN_ERROR_TX_TERR2))
    {
        CAN_TxCompleteCallback();
    }
}
