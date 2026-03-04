/**
 * @file    can_app.h
 * @brief   CAN message dispatch interface for UBC Solar DRD board.
 */

#ifndef __CAN_APP_H__
#define __CAN_APP_H__

/* INCLUDES */
#include <stdint.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief Routes a received CAN message to the corresponding DRD handler.
 * @param msg_id Received CAN message ID.
 * @param data Pointer to CAN payload bytes.
 */
void VehicleStateCanRxHandler(uint32_t msg_id, uint8_t* data);
/**
 * @brief Initializes CAN communication tasks.
 */
void CanTasksInit(void);

#endif /* __CAN_APP_H__ */