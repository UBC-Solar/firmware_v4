/**
 * @file    can_driver.c
 * @brief   CAN driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the CAN driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
 #include "can_driver.h"
 #include "can.h"
 #include "CAN_comms.h"
 #include "can_app.h"

const CAN_TxHeaderTypeDef time_since_bootup_can_header = {
    .StdId = TIME_SINCE_BOOTUP_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = TIME_SINCE_BOOTUP_CAN_DATA_LENGTH};

const CAN_TxHeaderTypeDef tel_flags_can_header = {
    .StdId = TEL_FLAGS_BOOTUP_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = TEL_FLAGS_CAN_DATA_LENGTH};

//IMU Can header definitions
const CAN_TxHeaderTypeDef imu_ag_x = {
    .StdId = IMU_AG_X_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_AG_LENGTH
};

const CAN_TxHeaderTypeDef imu_ag_y = {
    .StdId = IMU_AG_Y_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_AG_LENGTH
};

const CAN_TxHeaderTypeDef imu_ag_z = {
    .StdId = IMU_AG_Z_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_AG_LENGTH
};
const CAN_TxHeaderTypeDef imu_m_x = {
    .StdId = IMU_M_X_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_M_LENGTH
};

const CAN_TxHeaderTypeDef imu_m_y = {
    .StdId = IMU_M_Y_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_M_LENGTH
};

const CAN_TxHeaderTypeDef imu_m_z = {
    .StdId = IMU_M_Z_CAN_MESSAGE_ID,
    .ExtId = 0x0000,
    .IDE   = CAN_ID_STD,
    .RTR   = CAN_RTR_DATA,
    .DLC   = IMU_CAN_MESSAGE_M_LENGTH
};


/**
 * @brief Initialize CAN filter configuration
 * @param can_filter Pointer to CAN filter configuration structure
 */
static void CanDriverFilterInit(CAN_FilterTypeDef* can_filter);

/* CAN Driver initialization function */
CAN_comms_config_t CanDriverInit(){
    // Initialize CAN Comms configuration struct
    CAN_comms_config_t CAN_comms_config_tel = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanDriverFilterInit(&can_filter);

    CAN_comms_config_tel.hcan = &hcan;
    CAN_comms_config_tel.CAN_Filter = can_filter;

    return CAN_comms_config_tel;
}

static void CanDriverFilterInit(CAN_FilterTypeDef* can_filter){
    // TODO: Figure out which CAN IDs need to be filtered.
    can_filter->FilterIdHigh = 0x0000;
    can_filter->FilterMaskIdHigh = 0x0000;

    can_filter->FilterIdLow = 0x0000;
    can_filter->FilterMaskIdLow = 0x0000;

    can_filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter->FilterBank = (uint32_t) 0;
    can_filter->FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter->FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter->FilterActivation = CAN_FILTER_ENABLE;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    // Handle CAN error callback if needed
}