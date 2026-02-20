#include "can_driver.h"
#include "CAN_comms.h"
#include "can.h"

/* FUNCTION PROTOTYPES */
void VechicleStateCANRxHandler(uint32_t msg_id, uint8_t* data);

/**
 *  CAN Message Header for drive control
 */
const CAN_TxHeaderTypeDef drive_control_header = {.StdId = MOTOR_DRIVE_CONTROL_ADDRESS,
                                                  .ExtId = 0x0000,
                                                  .IDE = CAN_ID_STD,
                                                  .RTR = CAN_RTR_DATA,
                                                  .DLC = DRIVE_COMMAND_SIZE};

const CAN_TxHeaderTypeDef mdu_request_header = {.StdId = 0,
                                                .ExtId = MDU_REQUEST_COMMAND_ID,
                                                .IDE = CAN_ID_EXT,
                                                .RTR = CAN_RTR_DATA,
                                                .DLC = MDU_REQUEST_SIZE};

/**
 * @brief Initializes the CAN filter and CAN Rx callback function as CAN_comms_Rx_callback().
 *
 * Note: This uses the CAN_comms abstraction layer which will initialize two freeRTOS tasks. As a result it is recommended to
 * Call this function inside the MX_FREERTOS_Init() function in freertos.c
 */
void CAN_tasks_init()
{
    CAN_comms_config_t CAN_comms_config_tel = {0};
    // CAN_FilterTypeDef can_filter = {0};
    // CAN_filter_init(&can_filter);

    CAN_comms_config_tel.hcan = &hcan;
    // CAN_comms_config_tel.CAN_Filter = can_filter;
    CAN_comms_config_tel.CAN_comms_Rx_callback = CAN_comms_Rx_callback;

    CAN_comms_init(&CAN_comms_config_tel);
}


void CAN_comms_Rx_callback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
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

	VechicleStateCANRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
}

/* CAN RX */
void VechicleStateCANRxHandler(uint32_t msg_id, uint8_t* data)
{

    switch (msg_id)
    {
    case FRAME0:
        VelocityHandler(data);
        break;
    case STR_CAN_MSG_ID:
        SteeringCanMsgHandler(data);
        break;

#ifdef DEBUG
    case 0x500:
        StateRequestCanMsgHandler(data);
        break;
#endif
    }
}

/* CAN TX */