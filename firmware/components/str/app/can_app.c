#include "can_app.h"

#include "CAN_comms.h"
#include "can_driver.h"
#include "hex_app.h"
#include "main.h"
#include "gpio_app.h"
#include "stm32f1xx_hal_gpio.h"

#define STR_DISPLAY_MAX 99U
#define STR_WHEEL_RADIUS_M 0.283f
#define M_PI 3.14159

void CanTasksInit(void)
{
    CAN_comms_config_t CAN_comms_config_str = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanFilterInit(&can_filter);

    CAN_comms_config_str.hcan = &hcan;
    CAN_comms_config_str.CAN_Filter = can_filter;
    CAN_comms_config_str.CAN_comms_Rx_callback = CANCommsRxCallback;

    CAN_comms_init(&CAN_comms_config_str);
}

void CANCommsRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
{
	uint32_t CAN_ID = 0;
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
    void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data);
}

void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data)
{
    if (msg_id == FRAME0)
    {
        SteeringVelocityCanMsgHandler(data);
    }
}

void SteeringVelocityCanMsgHandler(uint8_t* data)
{
    if (data == NULL)
    {
        return;
    }

    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity_mps = (STR_WHEEL_RADIUS_M * 2.0f * (float)M_PI * (float)rpm) / 60.0f;
    uint32_t velocity_kmh = (uint32_t)(velocity_mps * 3.6f);

    GetVelocity(velocity_kmh);
}

void TurnSignalHornPtt(bool turn_left, bool turn_right, bool horn, bool ptt, bool regen, bool next_page, bool cruise, uint16_t set_velocity_kmh)
{
    CAN_comms_Tx_msg_t msg;
    msg.header = drive_control_header;
    msg.header.DLC = (uint8_t)CAN_DATA_SIZE;

    uint8_t data[CAN_DATA_SIZE] = {0};

    data[0] =
        (regen      ? (1U << 0) : 0U) |
        (cruise     ? (1U << 1) : 0U) |
        (next_page  ? (1U << 2) : 0U) |
        (turn_left  ? (1U << 3) : 0U) |
        (turn_right ? (1U << 4) : 0U) |
        (horn       ? (1U << 5) : 0U) |
        (ptt        ? (1U << 6) : 0U);

    /* pack set velocity (km/h) into data[1..2] little-endian */
    data[1] = (uint8_t)(set_velocity_kmh & 0xFF);
    data[2] = (uint8_t)((set_velocity_kmh >> 8) & 0xFF);

    memcpy(msg.data, data, CAN_DATA_SIZE);

    CAN_comms_Add_Tx_message(&msg);
}