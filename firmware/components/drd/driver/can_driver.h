#ifndef __CAN_DRIVER_H___
#define __CAN_DRIVER_H___

#include "can.h"
#include "CAN_comms.h"

#define MOTOR_DRIVE_CONTROL_ADDRESS 0x401
#define DRIVE_COMMAND_SIZE 5

#define MDU_REQUEST_COMMAND_ID 0x08F89540
#define MDU_REQUEST_SIZE 1
#define MDU_REQUEST_FRAME 0b111
#define FRAME0 0x08850225
#define STR_CAN_MSG_ID 0x580

extern const CAN_TxHeaderTypeDef drive_control_header;
extern const CAN_TxHeaderTypeDef mdu_request_header;

void CAN_comms_Rx_callback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);

#endif /* __CAN_DRIVER_H___ */