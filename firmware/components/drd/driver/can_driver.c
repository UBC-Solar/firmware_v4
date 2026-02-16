#include "can_driver.h"
#include "CAN_comms.h"
#include "can.h"
#include "drive_state.h"

/**
 *  CAN Message Header for drive control
 */
const CAN_TxHeaderTypeDef drive_control_header = {
    .StdId = MOTOR_DRIVE_CONTROL_ADDRESS,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = DRIVE_COMMAND_SIZE};

const CAN_TxHeaderTypeDef mdu_request_header = {
    .StdId = 0,
    .ExtId = MDU_REQUEST_COMMAND_ID,
    .IDE = CAN_ID_EXT,
    .RTR = CAN_RTR_DATA,
    .DLC = MDU_REQUEST_SIZE};