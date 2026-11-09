/**
 * @file CAN_comms.c
 * 
 * @brief CAN communications layer library.
 * This library contains the implementation for the CAN communications layer, which is responsible
 * for handling the CAN Rx and Tx messages using tasks and queues. This library is designed for the
 * STM32F1 series and uses the HAL_CAN library and CMSIS_v2 FreeRTOS.
 * 
 * @attention See README for details on how to implement this library.
 */


/* Includes */
#include "CAN_comms.h"
#include "cmsis_os2.h"
#include "stm32f1xx_hal_can.h"


/* Private defines */
#define COMMS_INIT_SUCCESS     0
#define COMMS_INIT_FAILURE     1
#define CAN_RX_TASK_STACK_SIZE 1028
#define CAN_RX_QUEUE_SIZE 64
#define CAN_TX_QUEUE_SIZE 16
#define CAN_RX_STRUCT_SIZE sizeof(CAN_comms_Rx_msg_t)
#define CAN_TX_STRUCT_SIZE sizeof(CAN_comms_Tx_msg_t)


/* Private varibles */
CAN_comms_config_t CAN_comms_config;
volatile CAN_Error_Stats_t CAN_error_stats = {0};
osMessageQueueId_t CAN_comms_Rx_queue;
osMessageQueueId_t CAN_comms_Tx_queue;
osThreadId_t CAN_comms_Rx_task_handle;
StaticTask_t CAN_comms_Rx_task_control_block;
uint32_t CAN_comms_Rx_task_buffer[CAN_RX_TASK_STACK_SIZE];


CAN_comms_diagnostics_t CAN_comms_diagnostic = {
    .comms_init_error = COMMS_INIT_SUCCESS,
    .dropped_rx_msg = 0,
    .rx_queue_count = 0,
    .success_rx = 0,
    .success_tx = 0,
    .failure_tx = 0,
    .hal_failure_rx = 0,
    .hal_failure_tx = 0
};

const osThreadAttr_t CAN_comms_Rx_task_attributes = {
    .name = "CAN_comms_Rx_task",
    .cb_mem = &CAN_comms_Rx_task_control_block,
    .cb_size = sizeof(CAN_comms_Rx_task_control_block),
    .stack_mem = &CAN_comms_Rx_task_buffer[0],
    .stack_size = sizeof(CAN_comms_Rx_task_buffer),
    .priority = (osPriority_t) osPriorityHigh,
};


/* Private function declarations */
void CAN_comms_Rx_task(void* argument);
static void CAN_comms_Dequeue_Tx_message(void);

/* Function definitions */
/**
 * @brief  Initializes CAN communications layer
 * This function copies the config struct to the global variable and creates the necessary queues, semaphores, and tasks.
 * 
 * @attention This function needs to be called by the user AFTER osKernelInitialize() and BEFORE osKernelStart().
 * 
 * @param config: Pointer to the config struct
 */
void CAN_comms_init(CAN_comms_config_t* config)
{
    /* Check config is not NULL */
    if (config == NULL)
    {
        CAN_comms_diagnostic.comms_init_error = COMMS_INIT_FAILURE;
        return;
    }

    /* Set config */
    CAN_comms_config = *config;

    /* Create queues */
    CAN_comms_Rx_queue = osMessageQueueNew(CAN_RX_QUEUE_SIZE, CAN_RX_STRUCT_SIZE, NULL);
    CAN_comms_Tx_queue = osMessageQueueNew(CAN_TX_QUEUE_SIZE, CAN_TX_STRUCT_SIZE, NULL);
    
    /* Create tasks */
    CAN_comms_Rx_task_handle = osThreadNew(CAN_comms_Rx_task, NULL, &CAN_comms_Rx_task_attributes);
}


/**
 * @brief Initializes CAN filters, activates notifications and starts CAN
 * This function is called once at the start of the CAN_comms_Rx_task RTOS task.
 * 
 * @attention This function needs to be called after the FreeRTOS kernel has started. So it cannot
 * be called in the CAN_comms_init function.
 */
 static void CAN_comms_HAL_CAN_init(void)
 {
    /* Configure CAN filter */
    HAL_CAN_ConfigFilter(CAN_comms_config.hcan, &CAN_comms_config.CAN_Filter);

    /* Start CAN */
    if (HAL_CAN_Start(CAN_comms_config.hcan) != HAL_OK) { // handles a failure and sets the flag, but no action is taken once the flag is set
        CAN_comms_diagnostic.comms_init_error = COMMS_INIT_FAILURE;
    }

    /* Activate notifications */
    HAL_CAN_ActivateNotification(CAN_comms_config.hcan, CAN_IT_RX_FIFO0_MSG_PENDING); // interrupt if CAN received a frame
    HAL_CAN_ActivateNotification(CAN_comms_config.hcan, CAN_IT_TX_MAILBOX_EMPTY); // interrupt if a mailbox slot has freed up
    HAL_CAN_ActivateNotification(CAN_comms_config.hcan, CAN_IT_ERROR | CAN_IT_LAST_ERROR_CODE | CAN_IT_BUSOFF | CAN_IT_RX_FIFO0_OVERRUN); // interrupt if error on the bus
    /* Kick start dequeing CAN Tx messages */
    taskENTER_CRITICAL();
    CAN_comms_Dequeue_Tx_message();
    taskEXIT_CRITICAL();
 }

 /**
  * @brief Enqueues Tx message in queue and dequeues immediately if a mailbox is free
  * @param CAN_comms_Tx_msg: Pointer to the CAN_comms_Tx_msg_t struct to be added to the queue
  */
static void CAN_comms_enqueue_msg(const CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{

    /* Enqueue the incoming message */
    if (osOK != osMessageQueuePut(CAN_comms_Tx_queue, CAN_comms_Tx_msg, 0U, 0)) {
        CAN_comms_diagnostic.failure_tx++;
    } 

    /* Dequeue immediately if mailbox is free */
    if (HAL_CAN_GetTxMailboxesFreeLevel(CAN_comms_config.hcan) > 0) {
        CAN_comms_Dequeue_Tx_message();
    }
}

/**
 * @brief This function can be called by a user to add a CAN Tx message within a task to the appropriate priority CAN_comms_Tx_queue.
 * @param CAN_comms_Tx_msg: Pointer to the CAN_comms_Tx_msg_t struct to be added to the queue
 */
void CAN_comms_Add_Tx_message(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
    /* TASK that enqueues CAN message */
    taskENTER_CRITICAL();
    CAN_comms_enqueue_msg(CAN_comms_Tx_msg);
    taskEXIT_CRITICAL();
}

/**
 * @brief Dequeues the oldest CAN Tx message from the CAN_comms_Tx_queue
 */
static void CAN_comms_Dequeue_Tx_message(void)
{
    CAN_comms_Tx_msg_t msg;
    
    while (HAL_CAN_GetTxMailboxesFreeLevel(CAN_comms_config.hcan) > 0) {
        if (osOK != osMessageQueueGet(CAN_comms_Tx_queue, &msg, NULL, 0)) {
            break; // Queue is empty
        }
        
        uint32_t canMailbox; // throwaway local variable to store which mailbox was used
        
        if (HAL_OK == HAL_CAN_AddTxMessage(CAN_comms_config.hcan, &msg.header, msg.data, &canMailbox)) {
            CAN_comms_diagnostic.success_tx++;
        }
        else {
            CAN_comms_diagnostic.hal_failure_tx++;
            break;
        }
    }
}


/**
 * @brief ISR-safe variant of adding a CAN Tx message to the queue.
 *
 * @param CAN_comms_Tx_msg: Pointer to the CAN_comms_Tx_msg_t struct to be added to the queue
 */
void CAN_comms_Add_Tx_messageISR(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
		UBaseType_t uxSavedInterruptStatus;

		uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        CAN_comms_enqueue_msg(CAN_comms_Tx_msg);
		taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

/**
 * @brief ISR-safe dequeing message in critical section.
 */
static void CAN_comms_Dequeue_Tx_message_ISR(void)
{
    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    CAN_comms_Dequeue_Tx_message();
    taskEXIT_CRITICAL_FROM_ISR(saved);
}


/**
 * @brief Task for handling CAN Rx messages
 * This task waits for a message in the CAN_comms_Rx_queue and calls the callback function 
 * defined in the CAN_comms_config struct.
 * 
 * @param argument: Unused
 */
void CAN_comms_Rx_task(void* argument)
{
    UNUSED(argument);

    /* Initialize HAL_CAN */
    CAN_comms_HAL_CAN_init(); 
    
    /* Infinite loop */
    for(;;)
    {
        /* Wait until there is a message in the queue */ 
    	CAN_comms_Rx_msg_t CAN_comms_Rx_msg;
    	if (osOK == osMessageQueueGet(CAN_comms_Rx_queue, &CAN_comms_Rx_msg, NULL, osWaitForever))
    	{
		    /* Call the handle function pointer */
		    CAN_comms_config.CAN_comms_Rx_callback(&CAN_comms_Rx_msg);
    	}
   }
}


/**
 * @brief  Interrupt Service Routine for CAN Rx
 * This function gets the CAN message from the CAN Rx FIFO and adds it to the CAN_comms_Rx_queue.
 * 
 * @attention This function needs to be added to the CAN Rx FIFO interrupt handler.
 */
void CAN_comms_Rx_message_pending_ISR()
{
    /* Get CAN Rx message and populate CAN_comms_Rx_msg_t struct */
    CAN_comms_Rx_msg_t CAN_Rx_msg;
    if(HAL_OK != HAL_CAN_GetRxMessage(CAN_comms_config.hcan, CAN_RX_FIFO0, &CAN_Rx_msg.header, CAN_Rx_msg.data))
    {
    	CAN_comms_diagnostic.hal_failure_rx++;
        return;
    }

    /* Add CAN message to the queue */
    if(osOK != osMessageQueuePut(CAN_comms_Rx_queue, &CAN_Rx_msg, 0, 0))
    {
        CAN_comms_diagnostic.dropped_rx_msg++;

    }
    else
    {
    	CAN_comms_diagnostic.success_rx++;
    }
}



/**
 * @attention The following functions are callback functions called by the STM32 HAL_CAN library.
 * These functions are weakly defined in the HAL_CAN library and are overridden in this library.
 */

/**
  * @brief  Rx FIFO 0 message pending callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
   if (hcan->Instance == CAN_comms_config.hcan->Instance)
   {
       CAN_comms_Rx_message_pending_ISR();   
    }
}

/**
  * @brief Callback for freed Tx mailbox
  * @param hcan pointer to CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
*/
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message_ISR();
    }
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message_ISR();
    }
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message_ISR();
    }
}

/** @brief Callback for error on the bus. This is ONLY for bench debugging.
  * @param hcan pointer to CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
*/
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    uint32_t errmask = HAL_CAN_GetError(hcan);
    HAL_CAN_ResetError(hcan); 

    if (errmask & HAL_CAN_ERROR_BR || errmask & HAL_CAN_ERROR_BD) {
        // Bit Error: Transmitted a bit (1 or 0), but read back the opposite value.
        CAN_error_stats.bit_errors++;
    }
    if (errmask & HAL_CAN_ERROR_STF) {
        // Stuff Error: More than 5 consecutive bits of the same polarity detected.
        CAN_error_stats.stuff_errors++;
    }
    if (errmask & HAL_CAN_ERROR_FOR) {
        // Form Error: Fixed-form bit field (e.g., CRC delimiter, ACK delimiter) contained an illegal bit.
        CAN_error_stats.form_errors++;
    }
    if (errmask & HAL_CAN_ERROR_ACK) {
        // Acknowledge Error: No node on the bus pulled the ACK slot low.
        CAN_error_stats.ack_errors++;
    }
    if (errmask & HAL_CAN_ERROR_CRC) {
        // CRC Error: Transmitted CRC did not match the calculated CRC on reception.
        CAN_error_stats.crc_errors++;
    }
    if (errmask & HAL_CAN_ERROR_RX_FOV0 ) {
        // Rx FIFO 0 Overrun: FIFO 0 was full when a new message arrived (message lost).
        CAN_error_stats.rx_overrun++;
    }
    if (errmask & HAL_CAN_ERROR_EPV) {
        // Error passive: Transmit error counter surpassed 127. Essentially, the node is on probation.
        CAN_error_stats.passive_errors++;
    }
    if (errmask & HAL_CAN_ERROR_BOF) {
        // Bus-Off State: Transmit error counter surpassed 255. Node auto-isolated from bus.
        CAN_error_stats.bus_off_events++;
    }
}

/**
 * @brief  Reads the CAN transmit error counter (TEC) from the controller.
 *         The hardware adds 8 per transmit error and subtracts 1 per successful transmit.
 * @return TEC value (0–255). Above 127 the node is error passive.
 */
uint8_t CAN_comms_get_tec()
{
    return (CAN_comms_config.hcan->Instance->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos;
}

/**
 * @brief  Reads the CAN receive error counter (REC) from the controller.
 *         The hardware adds 1 per receive error (up to 8) and subtracts 1 per
 *         successful reception.
 * @return REC value (0–255). Above 127 the node is error passive.
 */
uint8_t CAN_comms_get_rec()
{
    return (CAN_comms_config.hcan->Instance->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos;
}


/**
 * @brief  Returns diagnostic struct populated with diagnostic data
 * This function copies data from the global CAN_comms_diagnostic struct into a CAN_comms_diagnostic_t struct
 * created by the user.
 *
 * @param diagnostic: Pointer to user's diagnostic struct.
 */
void CAN_comms_get_diagnostic(CAN_comms_diagnostics_t* diagnostic)
{
    taskENTER_CRITICAL();
	CAN_comms_diagnostic.rx_queue_count = osMessageQueueGetCount(CAN_comms_Rx_queue);
	*diagnostic = CAN_comms_diagnostic;
    taskEXIT_CRITICAL();
}