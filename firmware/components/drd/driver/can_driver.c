#include "can_driver.h"
#include "CAN_comms.h"
#include "can.h"
#include "drive_state.h" // for now MMR change later

/* FUNCTION PROTOTYPES */
void VechicleStateCANRxHandler(uint32_t msg_id, uint8_t* data);
void CAN_filter_init(CAN_FilterTypeDef* can_filter);

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

void CAN_filter_init(CAN_FilterTypeDef* can_filter) {
    CAN_FilterTypeDef can_filter1;
    CAN_FilterTypeDef can_filter2;

    // ---- Filter Bank 0 ----
    // can_filter->FilterIdHigh = (CAN_ID_BATT_FAULTS << 5);
    // can_filter->FilterMaskIdHigh = (CAN_ID_PACK_VOLTAGE << 5);
    // can_filter->FilterIdLow = (CAN_ID_PACK_HEALTH << 5);
    // can_filter->FilterMaskIdLow = (CAN_ID_PACK_CURRENT << 5);
    // can_filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
    // can_filter->FilterBank = 0;
    // can_filter->FilterMode = CAN_FILTERMODE_IDLIST;
    // can_filter->FilterScale = CAN_FILTERSCALE_16BIT;
    // can_filter->FilterActivation = ENABLE;
    // HAL_CAN_ConfigFilter(&hcan, can_filter);

    // ---- Filter Bank 4 ----
    can_filter1.FilterIdHigh = (STR_CAN_MSG_ID << 5);
    can_filter1.FilterMaskIdHigh = (STR_CAN_MSG_ID << 5);
    can_filter1.FilterIdLow = (STR_CAN_MSG_ID << 5);
    can_filter1.FilterMaskIdLow = (STR_CAN_MSG_ID << 5);
    can_filter1.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter1.FilterBank = 4;
    can_filter1.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter1.FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter1.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter1);

    // ---- Filter Bank 2 ----
    uint32_t extId1 = CAN_ID_MTR_FAULTS;
    uint32_t extId2 = FRAME0;
    can_filter2.FilterIdHigh = (extId1 << 3) >> 16;
    can_filter2.FilterIdLow  = ((extId1 << 3) & 0xFFFF) | 0x0004;
    can_filter2.FilterMaskIdHigh = (extId2 << 3) >> 16;
    can_filter2.FilterMaskIdLow  = ((extId2 << 3) & 0xFFFF) | 0x0004;
    can_filter2.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter2.FilterBank = 5;
    can_filter2.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter2.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter2.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter2);
}

/**
 * @brief Initializes the CAN filter and CAN Rx callback function as CAN_comms_Rx_callback().
 *
 * Note: This uses the CAN_comms abstraction layer which will initialize two freeRTOS tasks. As a result it is recommended to
 * Call this function inside the MX_FREERTOS_Init() function in freertos.c
 */
void CAN_tasks_init()
{
    CAN_comms_config_t CAN_comms_config_drd = {0};
    CAN_FilterTypeDef can_filter = {0};
    CAN_filter_init(&can_filter);

    CAN_comms_config_drd.hcan = &hcan;
    CAN_comms_config_drd.CAN_Filter = can_filter;
    CAN_comms_config_drd.CAN_comms_Rx_callback = CAN_comms_Rx_callback;

    CAN_comms_init(&CAN_comms_config_drd);
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
        VelocityCanMsgHandler(data);
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