/**
 * @file    can_app.c
 * @brief   CAN message dispatch implementation for UBC Solar DRD board.
 *
 * Routes received CAN message IDs to the appropriate DRD application handlers.
 */

/* INCLUDES */
#include "drive_state.h"
#include "tasks.h"
#include "lcd_app.h"
#include "soc.h"
#include "can_driver.h"
#include "can_app.h"
#include "CAN_comms.h"
#include "cyclic_data_handler.h"
#include "fault_handler.h"
#include "external_lights.h"
#include <string.h>


/* FUNCTION PROTOTYPES */
/**
 * @brief Handles received CAN messages for vehicle state and dispatches to appropriate handlers.
 *
 * @param msg_id The CAN message ID.
 * @param data Pointer to the received CAN message data.
 */
void VehicleStateCanRxHandler(uint32_t msg_id, uint8_t* data);

/**
 * @brief Callback for processing received CAN messages.
 * @param CAN_comms_Rx_msg Pointer to the received CAN message structure.
 */
void CANCommsRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);

/* CAN INIT */
void CanTasksInit(void)
{
    CAN_comms_config_t CAN_comms_config_drd = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanFilterInit(&can_filter);

    CAN_comms_config_drd.hcan = &hcan;
    CAN_comms_config_drd.CAN_Filter = can_filter;
    CAN_comms_config_drd.CAN_comms_Rx_callback = CANCommsRxCallback;

    CAN_comms_init(&CAN_comms_config_drd);
}

/* CAN TX */
void MotorControlQueryData(void)
{
    CAN_comms_Tx_msg_t msg;

    msg.header = mdu_request_header;
    msg.data[0] = MDU_REQUEST_FRAME;
    CAN_comms_Add_Tx_message(&msg);
}

void MotorCommandPackAndSend(DriveStateMotorControl *motor_command, bool isr)
{
    CAN_comms_Tx_msg_t msg;
    msg.header = drive_control_header;

    uint8_t data[8] = {0};

    uint8_t accel_first_byte = (motor_command->accel_DAC_value & 0xFF);
    uint8_t accel_second_byte = ((motor_command->accel_DAC_value >> 8) & 0xFF);
    uint8_t regen_first_byte = (motor_command->regen_DAC_value & 0xFF);
    uint8_t regen_second_byte = ((motor_command->regen_DAC_value >> 8) & 0xFF);

    data[0] = accel_first_byte;
    data[1] = accel_second_byte;
    data[2] = regen_first_byte;
    data[3] = regen_second_byte;
    data[4] = motor_command->motor_control_flags;

    memcpy(msg.data, data, CAN_DATA_SIZE);

    if (isr)
    {
        CAN_comms_Add_Tx_messageISR(&msg);
    }
    else
    {
        CAN_comms_Add_Tx_message(&msg);
    }
}

/* CAN RX */
void CANCommsRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
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
    FaultHandlerCanRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
    VehicleStateCanRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
    LcdAppCanRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
    ExternalLightsCanRxHandle(CAN_ID, CAN_comms_Rx_msg->data);
}

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

void LcdAppCanRxHandler(uint32_t msg_id, uint8_t* data)
{ 
    switch (msg_id) {
    case CAN_ID_ECU: { // Update pack current and voltage for SOC calculation
        int16_t tmp_pack_current = (data[1] << 8) | (data[0]);
        tmp_pack_current /= 65.535;
        CyclicDataSetPackCurrent(tmp_pack_current);
        g_pack_current_soc = tmp_pack_current;
        break;
    } 
    case CAN_ID_PACK_VOLTAGE: { // Update pack voltage and trigger SOC calculation
        uint16_t tmp_pack_voltage = (data[1] << 8) | (data[0]);
        tmp_pack_voltage /= PACK_VOLTAGE_DIVISOR;
        CyclicDataSetPackVoltage(tmp_pack_voltage);
        g_total_pack_voltage_soc = tmp_pack_voltage;

        osEventFlagsSet(calculate_soc_flagHandle, SOC_CALCULATE_ON);
        break;
    }
    case STR_CAN_MSG_ID: { // Handle page changes for the LCD
        bool next_page = ((data[0] >> 2) & 0x1);
        LcdHandlerChangePage(next_page);
        break;
    }
    }
}

void FaultHandlerCanRxHandler(uint32_t msg_id, uint8_t* data)
{
    // How this is handled: https://docs.google.com/document/d/1lpAI_UW_a7WqzGdEOpQaXZ8MB8aLU-1VR9yYNUQGnVI/edit?usp=sharing
    switch (msg_id) {
    case CAN_ID_BATT_FAULTS:
        FaultHandlerParseBatteryFaults(data);
        FaultHandlerParseBatteryWarnings(data);
        break;
    case CAN_ID_ECU:
        FaultHandlerEStop(data);
        FaultHandlerParseECUFaults(data);
        FaultHandlerParseECUWarnings(data);
        break;
    case CAN_ID_PACK_VOLTAGE:
        FaultHandlerParsePackVoltageFaults(data);
        break;
    case CAN_ID_MTR_FAULTS:
        FaultHandlerParseMotorFaults(data);
        break;
        // Other motor faults are handled from drd diagnostics
        
    // Temperatures come from multiple CAN messages so we parse them in the same handler based on the message ID
    FaultHandlerParseTemperatures(msg_id, data);
    }
}