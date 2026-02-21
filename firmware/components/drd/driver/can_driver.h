#ifndef __CAN_DRIVER_H___
#define __CAN_DRIVER_H___

#include "can.h"
#include "CAN_comms.h"

#define MOTOR_DRIVE_CONTROL_ADDRESS 0x401
#define DRIVE_COMMAND_SIZE 5
#define DRD_DIAGNOSTIC_MESSAGE				0x403
#define TIME_SINCE_BOOTUP_CAN_ID			0x404

#define CAN_ID_MTR_FAULTS 0x08A50225
#define MDU_REQUEST_COMMAND_ID 0x08F89540
#define MDU_REQUEST_SIZE 1
#define MDU_REQUEST_FRAME 0b111
#define FRAME0 0x08850225
#define STR_CAN_MSG_ID 0x580

#define DRD_DIAGNOSTIC_SIZE					8
#define TIME_SINCE_BOOTUP_CAN_DATA_LENGTH	4

extern const CAN_TxHeaderTypeDef drive_control_header;
extern const CAN_TxHeaderTypeDef mdu_request_header;
extern const CAN_TxHeaderTypeDef drd_diagnostic_header;
extern const CAN_TxHeaderTypeDef time_since_bootup_can_header;

void CAN_comms_Rx_callback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);
void CAN_tasks_init();
void CAN_filter_init(CAN_FilterTypeDef* can_filter);



#endif /* __CAN_DRIVER_H___ */