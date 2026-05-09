#include "can_driver.h"
#include "can_app.h"
#include "CAN_comms.h"
#include "rtc_app.h"
#include "telemetry_app.h"

/**
 * @brief Can Comms Callback Function for processing received CAN messages.
 * @param CAN_comms_Rx_msg Pointer to the received CAN message structure.
 */
static void CanAppRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);

/**
 * @brief RTC Driver CAN RX Handler for processing RTC-related CAN messages.
 * @param msg_id The CAN message ID.
 * @param data Pointer to the received CAN message data.
 */
static void RtcDriverRxHandler(uint32_t msg_id, uint8_t* data);


void CanAppInit(){
    // Initialize CAN Comms RX Callback function 
    CAN_comms_config_t CAN_comms_config_tel = CanDriverInit();
    CAN_comms_config_tel.CAN_comms_Rx_callback = CanAppRxCallback;

    CAN_comms_init(&CAN_comms_config_tel);
}

static void CanAppRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg){
	uint32_t CAN_ID = 0;

	if (CAN_comms_Rx_msg == NULL)
	{
		return;
	}

	if(CAN_comms_Rx_msg->header.IDE == CAN_ID_EXT)
	{
		CAN_ID = CAN_comms_Rx_msg->header.ExtId; // Get extended CAN ID
	}
	else
	{
		CAN_ID = CAN_comms_Rx_msg->header.StdId; // Get standard CAN ID
	}
    // RX Functions go here
	RtcDriverRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
}

static void RtcDriverRxHandler(uint32_t msg_id, uint8_t* data)
{
    switch (msg_id)
    {
    case RTC_TIMESTAMP_MSG_ID:
        RtcAppSyncMemoratorToTel(data);
        break;
    default:
        break;
    }
}