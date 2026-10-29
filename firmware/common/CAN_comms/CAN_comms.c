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


/* Private defines */
#define COMMS_INIT_SUCCESS     0
#define COMMS_INIT_FAILURE     1
#define CAN_RX_TASK_STACK_SIZE 1028
#define CAN_TX_TASK_STACK_SIZE 1028
#define CAN_RX_QUEUE_SIZE 64
#define CAN_TX_QUEUE_SIZE 64
#define CAN_RX_STRUCT_SIZE sizeof(CAN_comms_Rx_msg_t)
#define CAN_TX_STRUCT_SIZE sizeof(CAN_comms_Tx_msg_t)
#define NUM_CAN_TX_MAILBOXES 3
#define HIGH_PRIORITY_ID_LIMIT 580 // temporary


/* Private varibles */
CAN_comms_config_t CAN_comms_config;
osMessageQueueId_t CAN_comms_Rx_queue;
osMessageQueueId_t CAN_comms_Tx_queue_normal;
osMessageQueueId_t CAN_comms_Tx_queue_high;
osThreadId_t CAN_comms_Rx_task_handle;
osThreadId_t CAN_comms_Tx_task_handle;
StaticTask_t CAN_comms_Rx_task_control_block;
StaticTask_t CAN_comms_Tx_task_control_block;
uint32_t CAN_comms_Rx_task_buffer[CAN_RX_TASK_STACK_SIZE];
uint32_t CAN_comms_Tx_task_buffer[CAN_TX_TASK_STACK_SIZE];


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
    CAN_comms_Tx_queue_normal = osMessageQueueNew(CAN_TX_QUEUE_SIZE, CAN_TX_STRUCT_SIZE, NULL);
    CAN_comms_Tx_queue_high = osMessageQueueNew(CAN_TX_QUEUE_SIZE, CAN_TX_STRUCT_SIZE, NULL);
    
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
 void CAN_comms_HAL_CAN_init()
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

 }

/**
 * @brief returns true if high priority, false otherwise
 * @param msg is the message we would like to distinguish
*/
 static bool is_high_priority(const CAN_comms_Tx_msg_t* msg) {
    return (msg->header.IDE == CAN_ID_STD) && (msg->header.StdId < HIGH_PRIORITY_ID_LIMIT);
 }



/**
 * @brief Adds a CAN Tx message to the appropriate priority CAN_comms_Tx_queue
 * This function can be called be a user to add a CAN message to the CAN_comms_Tx_queue.
 * 
 * @param CAN_comms_Tx_msg: Pointer to the CAN_comms_Tx_msg_t struct to be added to the queue
 */
void CAN_comms_Add_Tx_message(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
    /* TASK that enqueues CAN message */
    taskENTER_CRITICAL();

        /* Enqueue the incoming message */
        if (is_high_priority(CAN_comms_Tx_msg)) {
            if (osOK != osMessageQueuePut(CAN_comms_Tx_queue_high, CAN_comms_Tx_msg, 0U, 0)) {
                CAN_comms_diagnostic.failure_tx++;
            }
        }
        else {
            if (osOK != osMessageQueuePut(CAN_comms_Tx_queue_normal, CAN_comms_Tx_msg, 0U, 0)) {
                CAN_comms_diagnostic.failure_tx++;
            } 
        }

        /* Dequeue immediately if mailbox is free */
        if (HAL_CAN_GetTxMailboxesFreeLevel(CAN_comms_config.hcan) > 0) {
            CAN_comms_Dequeue_Tx_message();
        }

    taskEXIT_CRITICAL();
}

/**
 * @brief Dequeues the oldest CAN Tx message from the CAN_comms_Tx_queue
 */
static void CAN_comms_Dequeue_Tx_message(void)
{
    CAN_comms_Tx_msg_t msg;
    
    while (HAL_CAN_GetTxMailboxesFreeLevel(CAN_comms_config.hcan) > 0) {
        /* if message from high priority queue is empty */

        if (osOK != osMessageQueueGet(CAN_comms_Tx_queue_high, &msg, NULL, 0)) {
            // High priority queue is empty
            if (osOK != osMessageQueueGet(CAN_comms_Tx_queue_normal, &msg, NULL, 0)) {
                break; // Both queues are empty
            }
        }

        uint32_t canMailbox; // throwaway local variable to store which mailbox was used
        
        if (HAL_OK == HAL_CAN_AddTxMessage(CAN_comms_config.hcan, &msg.header, msg.data, &canMailbox)) {
            CAN_comms_diagnostic.success_tx++;
        }
        else {
            CAN_comms_diagnostic.failure_tx++;
        }
    }
}


/**
 * @brief Adds a CAN Tx message to the CAN_comms_Tx_queue
 * This function can be called be a user to add a CAN message to the CAN_comms_Tx_queue.
 *
 * @param CAN_comms_Tx_msg: Pointer to the CAN_comms_Tx_msg_t struct to be added to the queue
 */
void CAN_comms_Add_Tx_messageISR(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
		UBaseType_t uxSavedInterruptStatus;
		uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
		uint32_t canMailbox;
		if (HAL_OK == HAL_CAN_AddTxMessage(CAN_comms_config.hcan, &CAN_comms_Tx_msg->header, CAN_comms_Tx_msg->data, &canMailbox))
		{
			CAN_comms_diagnostic.success_tx++;

		}
		else
		{
			CAN_comms_diagnostic.hal_failure_tx++;
		}
		taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

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

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message();
    }

}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message();
    }

}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN_comms_config.hcan->Instance)
    {
        CAN_comms_Dequeue_Tx_message();
    }

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
	CAN_comms_diagnostic.rx_queue_count = osMessageQueueGetCount(CAN_comms_Rx_queue);
	*diagnostic = CAN_comms_diagnostic;
}