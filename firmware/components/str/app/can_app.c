/**
 * @file    can_app.c
 * @brief   CAN application implementation for the UBC Solar STR board.
 */

/* INCLUDES */
#include "can_app.h"

#include "CAN_comms.h"
#include "can_driver.h"
#include "gpio_driver.h"
#include "hex_app.h"
#include "main.h"
#include "gpio_app.h"
#include "stm32f1xx_hal_gpio.h"

#include <string.h>

/* DEFINES */
#define STR_DISPLAY_MAX 99U
#define STR_WHEEL_RADIUS_M 0.283f
#define M_PI 3.14159

/* FUNCTION PROTOTYPES */
/**
 * @brief Callback for processing received CAN messages.
 * @param CAN_comms_Rx_msg Pointer to the received CAN message structure.
 */
static void CANCommsRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);

/* CAN INIT */
void CanAppInit(void)
{
    CAN_comms_config_t CAN_comms_config_str = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanFilterInit(&can_filter);

    CAN_comms_config_str.hcan = &hcan;
    CAN_comms_config_str.CAN_Filter = can_filter;
    CAN_comms_config_str.CAN_comms_Rx_callback = CANCommsRxCallback;

    CAN_comms_init(&CAN_comms_config_str);
}

/* CAN RX */
static void CANCommsRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
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

    SteeringCanRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
}

/**
 * @brief Handles received steering-wheel CAN dependencies.
 * @param msg_id Received CAN message ID.
 * @param data Pointer to CAN payload bytes.
 */
void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data)
{
    if (msg_id == FRAME0)
    {
        SteeringVelocityCanMsgHandler(data);
    }
}

/**
 * @brief Converts MDU wheel speed data into vehicle speed for STR state.
 * @param data Pointer to the MDU velocity CAN payload.
 */
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

/* CAN TX */
void TransmitDriveControlState(void)
{
    CAN_comms_Tx_msg_t msg;
    uint16_t cruise_set_velocity_kmh = 0U;

    msg.header = steering_header;
    msg.header.DLC = (uint8_t)CAN_DATA_SIZE;

    uint8_t data[CAN_DATA_SIZE] = {0};

    if (gpio_pin_state.cruise_state.cruise_en)
    {
        cruise_set_velocity_kmh = (uint16_t)ReadCruiseSetVelocity();
    }

    data[0] =
        ((uint8_t)gpio_pin_state.regen_en << 0) |
        ((uint8_t)gpio_pin_state.cruise_state.cruise_en << 1) |
        ((uint8_t)gpio_pin_state.next_page << 2) |
        ((uint8_t)gpio_pin_state.lights_state.lts_en << 3) |
        ((uint8_t)gpio_pin_state.lights_state.rts_en << 4) |
        ((uint8_t)gpio_pin_state.horn_en << 5) |
        ((uint8_t)gpio_pin_state.ptt_en << 6);

    data[1] = (uint8_t)(cruise_set_velocity_kmh & 0xFFU);
    data[2] = (uint8_t)((cruise_set_velocity_kmh >> 8) & 0xFFU);

    memcpy(msg.data, data, CAN_DATA_SIZE);

    CAN_comms_Add_Tx_message(&msg);
}
