/**
 * @file    can_app.c
 * @brief   CAN message dispatch implementation for UBC Solar DRD board.
 *
 * Routes received CAN message IDs to the appropriate DRD application handlers.
 */

/* INCLUDES */
#include "drive_state.h"
#include "can_driver.h"
#include "can_app.h"
#include "CAN_comms.h"

/* FUNCTION PROTOTYPES */
/**
 * @brief Handles received CAN messages for vehicle state and dispatches to appropriate handlers.
 *
 * @param msg_id The CAN message ID.
 * @param data Pointer to the received CAN message data.
 */
void VehicleStateCanRxHandler(uint32_t msg_id, uint8_t* data);
/**
 * @brief Initializes CAN hardware filters for message acceptance.
 *
 * @param can_filter Pointer to CAN filter configuration structure.
 */
void CanFilterInit(CAN_FilterTypeDef* can_filter);
/**
 * @brief Callback for processing received CAN messages.
 * @param CAN_comms_Rx_msg Pointer to the received CAN message structure.
 */
void VehicleDriveStateRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);


void CanTasksInit(void)
{
    CAN_comms_config_t CAN_comms_config_drd = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanFilterInit(&can_filter);

    CAN_comms_config_drd.hcan = &hcan;
    CAN_comms_config_drd.CAN_Filter = can_filter;
    CAN_comms_config_drd.CAN_comms_Rx_callback = VehicleDriveStateRxCallback;

    CAN_comms_init(&CAN_comms_config_drd);
}

void VehicleDriveStateRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
{
	uint32_t CAN_ID = 0;
	/*
	 *	handle parsing rx messages
	 */
	if (CAN_comms_Rx_msg == NULL)
	{
			return;
	}

	if(CAN_comms_Rx_msg->header.IDE == CAN_ID_EXT)
	{
		CAN_ID = CAN_comms_Rx_msg->header.ExtId; // Get CAN ID
	}
	else
	{
		CAN_ID = CAN_comms_Rx_msg->header.StdId; // Get CAN ID
	}

    VehicleStateCanRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
}

/* CAN RX */
void VehicleStateCanRxHandler(uint32_t msg_id, uint8_t* data)
{

    switch (msg_id)
    {
    case FRAME0:
        DriveStateVelocityCanMsgHandler(data);
        break;
    case STR_CAN_MSG_ID:
        DriveStateSteeringCanMsgHandler(data);
        break;

#ifdef DEBUG
    case 0x500:
        StateRequestCanMsgHandler(data);
        break;
#endif
    }
}