/**
 * @file    can_app.h
 * @brief   CAN message dispatch interface for UBC Solar DRD board.
 */

#ifndef __CAN_APP_H__
#define __CAN_APP_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

#include "CAN_comms.h"

/* FUNCTION PROTOTYPES */
/**
 * @brief Initializes CAN communication tasks.
 */
void CanTasksInit(void);

/**
 * @brief Routes a received CAN message to the corresponding DRD handler.
 * @param msg_id Received CAN message ID.
 * @param data Pointer to CAN payload bytes.
 */
void VehicleStateCanRxHandler(uint32_t msg_id, uint8_t* data);

/**
 * @brief CAN rx function which parses message data needed by the LCD
 *
 * @param msg_id 	The id of the CAN message
 * @param data  	The data of the CAN message
 */
void LcdAppCanRxHandler(uint32_t msg_id, uint8_t* data);

/**
 * @brief Transmits a motor command CAN message from task or ISR context.
 * @param msg Motor command CAN message to transmit.
 * @param isr True to use ISR-safe transmit path, false for normal task context.
 */
void MotorCommandTransmit(CAN_comms_Tx_msg_t msg, bool isr);

/**
 * @brief Queries and processes data related to motor control.
 */
void MotorControlQueryData(void);

#endif /* __CAN_APP_H__ */