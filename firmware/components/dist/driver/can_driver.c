#include "can_driver.h"
#include "debug_io.h"

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
        return;
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
void CAN_QueueTxMessage(CAN_TxMessage_t *message)
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

    // Activate TX and both RX FIFO interrupts
    if (HAL_CAN_ActivateNotification(CAN_driver.can_handle,
            CAN_IT_TX_MAILBOX_EMPTY |
            CAN_IT_RX_FIFO0_MSG_PENDING |
            CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_CAN_Start(CAN_driver.can_handle);
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

    CAN_QueueTxMessage(&txMessage);
}
#endif // UNIT_TEST_CAN 


/**
 * @brief Dequeue one frame from the given RX FIFO and process it.
 *        Called from the FIFO0 and FIFO1 message-pending callbacks.
 */
static void process_rx(uint32_t fifo)
{
    CAN_RxMessage_t msg;
    if (HAL_CAN_GetRxMessage(
        CAN_driver.can_handle,
        fifo,
        (CAN_RxHeaderTypeDef *) &msg.rx_header,
        (uint8_t *) msg.data) != HAL_OK)
    {
        Error_Handler();
    }
    msg.timestamp = HAL_GetTick();

    DEBUG_IO_PRINT("CAN RX: ID=0x%03lX data[0]=0x%02X\r\n",
                   msg.rx_header.StdId, msg.data[0]);

    // Startup authorisation: HVC sends 0x303 with bit 0 set to release the board
    // from the STARTUP state once the bus is live and the pack is ready.
    if (msg.rx_header.StdId == 0x303U && (msg.data[0] & 0x01U))
    {
        CAN_driver.startup_received = 1U;
    }

    // External fault: any non-zero byte in 0x304 signals a fault condition.
    if (msg.rx_header.StdId == 0x304U)
    {
        for (uint8_t i = 0; i < msg.rx_header.DLC; i++)
        {
            if (msg.data[i] != 0U)
            {
                CAN_driver.ext_fault_received = 1U;
                break;
            }
        }
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    process_rx(CAN_FILTER_FIFO0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    process_rx(CAN_FILTER_FIFO1);
}

uint8_t CAN_Startup_Received(void)
{
    return CAN_driver.startup_received;
}

uint8_t CAN_ExtFault_Received(void)
{
    return CAN_driver.ext_fault_received;
}

void CAN_Send_Heartbeat(void)
{

    static uint16_t HeartBeatCounter = 0U;
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = DIST_HEARTBEAT_ID;
    msg.tx_header.IDE   = CAN_ID_STD;
    msg.tx_header.RTR   = CAN_RTR_DATA;
    msg.tx_header.DLC   = 3U;
    msg.data[0]         = 0x01U;
    msg.data[1]         = (uint8_t)(HeartBeatCounter>>8); // Bit shift to get the most significant byte of the 16-bit counter
    msg.data[2]         = (uint8_t)(HeartBeatCounter & 0xFFU);
    CAN_QueueTxMessage(&msg);
    HeartBeatCounter++;
}

void CAN_Send_Currents(uint8_t drd_mA, uint8_t mdi_mA, uint8_t spare_ctrl_mA,
                       uint8_t spare_mux_mA, uint8_t spare_mA)
{
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = LV_CURRENTS_ID;
    msg.tx_header.IDE   = CAN_ID_STD;
    msg.tx_header.RTR   = CAN_RTR_DATA;
    msg.tx_header.DLC   = 5U;
    msg.data[0]         = drd_mA;
    msg.data[1]         = mdi_mA;
    msg.data[2]         = spare_ctrl_mA;
    msg.data[3]         = spare_mux_mA;
    msg.data[4]         = spare_mA;
    CAN_QueueTxMessage(&msg);
}

void CAN_Send_Fault(void)
{
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = DIST_FAULT_ID;
    msg.tx_header.IDE   = CAN_ID_STD;
    msg.tx_header.RTR   = CAN_RTR_DATA;
    msg.tx_header.DLC   = 1U;
    msg.data[0]         = 0x01U;
    CAN_QueueTxMessage(&msg);
}

void CAN_Send_LV_ON_0x303(void)
{
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = LV_POWERUP_RECIEVED_ID;
    msg.tx_header.IDE   = CAN_ID_STD;
    msg.tx_header.RTR   = CAN_RTR_DATA;
    msg.tx_header.DLC   = 1U;
    msg.data[0]         = 0x01U;
    CAN_QueueTxMessage(&msg);
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

// Arbitration-lost frames are treated as completions to prevent queue deadlock
void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
    CAN_TxCompleteCallback();
}

/**
 * @brief Advance the TX queue after a mailbox completes or is aborted.
 *        Called from all six TX callbacks (complete + abort for each mailbox).
 */
void CAN_TxCompleteCallback(void)
{
    // Initiate next message transmission if possible
    if (CAN_driver.tx_queue_pop_index != CAN_driver.tx_queue_push_index)
    {
        tryTransmitFromQueue();
    }
}

/**
 * @brief Drain any TX mailboxes that completed with a transmission error.
 *
 * AutoRetransmission is DISABLED, so an unacknowledged frame (no other device
 * on the bus) immediately completes with TX_TERR rather than retrying forever.
 * Each errored mailbox must be explicitly drained here to prevent the queue
 * from stalling. Called from HAL_CAN_ErrorCallback in stm32f1xx_it.c.
 */
void CAN_ErrorCallback(void)
{
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