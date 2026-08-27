#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "can.h"

/*============================================================================*/
/* CAN ID's Send*/
#define DIST_HEARTBEAT_ID       0x3A0
#define DIST_FAULT_ID	        0x3A1
#define LV_POWERUP_RECIEVED_ID	0x3A2
#define LV_CURRENTS_ID      	0x3A3
#define SEND_DIST_BRANCH_ID     0x3A4 

/*============================================================================*/
/* CAN ID's Receive*/
#define HVC_FAULT_ID            0x321
#define LV_POWERUP_ID           0x323

/*============================================================================*/
/* Actual Branch ID */
#define DIST_BRANCH_ID          1


/*============================================================================*/
/* CONSTANTS */

#define CAN_MAX_DATAFRAME_BYTES  8U
#define NUM_CAN_TX_MAILBOXES     3U
#define CAN_FILTER_NUM_BANKS     14U   // STM32F103 has 14 configurable filter banks
#define CAN_FILTER_NUM_ID_PER_BANK 2U  // two 32-bit ID slots per bank in list mode

/** Depth of the software TX queue. Size to the peak simultaneous message count. */
#define CAN_TX_QUEUE_CAPACITY    32U

/*============================================================================*/
/* TYPES */

typedef struct {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
} CAN_TxMessage_t;

typedef struct {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t data[CAN_MAX_DATAFRAME_BYTES];
    uint32_t timestamp; // HAL tick at time of reception
} CAN_RxMessage_t;

typedef struct {
    CAN_HandleTypeDef *can_handle;
    volatile CAN_TxMessage_t tx_queue[CAN_TX_QUEUE_CAPACITY];
    volatile uint32_t tx_queue_push_index;
    volatile uint32_t tx_queue_pop_index;
    volatile uint8_t startup_received;   // set when LV_POWERUP_ID (0x323) with bit 0 is received
    volatile uint8_t ext_fault_received; // set when HVC_FAULT_ID (0x321) with any non-zero byte is received
} CAN_Driver_t;

/*============================================================================*/
/* INIT */

/**
 * @brief Configure bxCAN hardware ID-list filters for a set of standard 11-bit IDs.
 *        Must be called before CAN_Init().
 */
void CAN_InitFilterList(CAN_HandleTypeDef *handle, const uint16_t *std_ids, size_t count);

/**
 * @brief Activate TX/RX interrupts and start the CAN peripheral.
 *        Filters must be configured before calling this.
 */
void CAN_Init(CAN_HandleTypeDef *handle);

/*============================================================================*/
/* TX */

/**
 * @brief Copy a message into the TX queue and transmit immediately if a
 *        mailbox is free, otherwise it will be sent from the TX-complete ISR.
 */
void CAN_QueueTxMessage(CAN_TxMessage_t *message);

/** @brief Send the dist board heartbeat (ID 0x3A0). */
void CAN_Send_Heartbeat(void);

/** @brief Send all five eFuse current readings over CAN (ID 0x3A3).
 *         Each LSB = 5 mA, max 1275 mA (255 counts). */
void CAN_Send_Currents(uint8_t drd_mA, uint8_t mdi_mA, uint8_t spare_ctrl_mA,
                       uint8_t spare_mux_mA, uint8_t spare_mA);

/** @brief Send the dist board fault message (ID 0x3A1, data[0] bit 0 set). */
void CAN_Send_Fault(void);

/** @brief Send the dist branch ID (ID 0x3A4, data[0] = DIST_BRANCH_ID). */
void CAN_Send_Branch_ID(void);

/** @brief Send the LV power-on notification (ID 0x3A2, data[0] = 0x01). */
void CAN_Send_LV_ON(void);

/*============================================================================*/
/* RX / STATUS */

/** @return 1 if a valid startup authorisation (0x323, bit 0) has been received. */
uint8_t CAN_Startup_Received(void);

/** @return 1 if a 0x304 message with any non-zero byte has been received. */
uint8_t CAN_ExtFault_Received(void);

/*============================================================================*/
/* ISR CALLBACKS — called from HAL weak-function overrides in can_driver.c */

void CAN_TxCompleteCallback(void);
void CAN_ErrorCallback(void);

#if (UNIT_TEST_CAN == RUN)
void CAN_SendMessgeDebug(void);
#endif
