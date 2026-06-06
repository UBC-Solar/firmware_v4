/**
 * @file    can_driver.h
 * @brief   CAN bus driver declarations for UBC Solar DRD board
 *
 * This header declares CAN bus communication functions and constants for the DRD board.
 * It provides prototypes for CAN message handling, filter configuration, and defines CAN message IDs and sizes.
 */

#ifndef __CAN_DRIVER_H___
#define __CAN_DRIVER_H___

/* INCLUDES */
#include "can.h"

/* DEFINES */
#define MOTOR_DRIVE_CONTROL_ADDRESS 0x401
#define DRIVE_COMMAND_SIZE 5

#define DRD_DIAGNOSTIC_MESSAGE				0x403
#define DRD_DIAGNOSTIC_SIZE					8
#define TIME_SINCE_BOOTUP_CAN_ID			0x404
#define TIME_SINCE_BOOTUP_CAN_DATA_LENGTH	4

#define CAN_ID_ECU		    		        0x450
#define CAN_ID_BATT_FAULTS		 			0x622
#define CAN_ID_PACK_VOLTAGE		    		0x623
#define CAN_ID_PACK_HEALTH          		0x624

#define MPPTA_TEMPERATURE_CAN_ID            0x6A2
#define MPPTB_TEMPERATURE_CAN_ID            0x6B2
#define MPPTC_TEMPERATURE_CAN_ID            0x6C2

#define BMS_TEMPERATURES_CAN_ID			    0x625

#define CAN_ID_MTR_FAULTS 0x08A50225
#define MDU_REQUEST_COMMAND_ID 0x08F89540
#define MDU_REQUEST_SIZE 1
#define MDU_REQUEST_FRAME 0b111
#define FRAME0 0x08850225
#define STR_CAN_MSG_ID 0x580



/* CAN HEADERS */
extern const CAN_TxHeaderTypeDef steering_header;
extern const CAN_TxHeaderTypeDef mdu_request_header;
extern const CAN_TxHeaderTypeDef drd_diagnostic_header;
extern const CAN_TxHeaderTypeDef time_since_bootup_can_header;

/* FUNCTION PROTOTYPES */
/**
 * @brief Initializes CAN hardware filters for message acceptance.
 *
 * @param can_filter Pointer to CAN filter configuration structure.
 */
void CanFilterInit(CAN_FilterTypeDef* can_filter);

#endif /* __CAN_DRIVER_H___ */