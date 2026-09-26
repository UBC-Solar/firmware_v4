/**
 * @file    can_app.h
 * @brief   CAN application interface for the UBC Solar STR board.
 */

#ifndef __CAN_APP_H__
#define __CAN_APP_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

/* DEFINES */
#define STR_LCD_PAGE_CAN_ID 0x580U
#define FRAME0 0x08850225

/* FUNCTION PROTOTYPES */
/**
 * @brief Initializes STR CAN communication.
 */
void CanAppInit(void);

/**
 * @brief Transmits the next-page request for the dashboard LCD.
 */
void CanAppTransmitNextPage(void);

/**
 * @brief Routes a received CAN message to the matching STR handler.
 * @param msg_id Received CAN message ID.
 * @param data Pointer to CAN payload bytes.
 */
void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data);

/**
 * @brief Packs and transmits the current steering wheel control state.
 */
void TransmitDriveControlState(void);

/**
 * @brief Extracts the driver-selected speed units from a DRD motor command frame.
 * @param data Pointer to the motor command CAN payload.
 */
void SteeringSpeedUnitsCanMsgHandler(uint8_t* data);

#endif /* __CAN_APP_H__ */
