/**
 * @file    can_driver.c
 * @brief   CAN bus driver implementation for UBC Solar DRD board
 *
 * This file contains the implementation of CAN bus communication functions for the DRD board.
 * It handles CAN message transmission, reception, and filter configuration for vehicle state and control.
 */

/* INCLUDES */
#include "can_driver.h"
#include "can.h"
#include "git_version.h"
#include "stm32f1xx_hal_can.h"

/**
 *  CAN Message Header for drive control
 */
const CAN_TxHeaderTypeDef steering_header = {.StdId = MOTOR_DRIVE_CONTROL_ADDRESS,
                                                  .ExtId = 0x0000,
                                                  .IDE = CAN_ID_STD,
                                                  .RTR = CAN_RTR_DATA,
                                                  .DLC = DRIVE_COMMAND_SIZE};

const CAN_TxHeaderTypeDef mdu_request_header = {.StdId = 0,
                                                .ExtId = MDU_REQUEST_COMMAND_ID,
                                                .IDE = CAN_ID_EXT,
                                                .RTR = CAN_RTR_DATA,
                                                .DLC = MDU_REQUEST_SIZE};

const CAN_TxHeaderTypeDef drd_diagnostic_header = { .StdId = DRD_DIAGNOSTIC_MESSAGE,
                                                    .ExtId = 0x0000,
                                                    .IDE = CAN_ID_STD,
                                                    .RTR = CAN_RTR_DATA,
                                                    .DLC = DRD_DIAGNOSTIC_SIZE};

const CAN_TxHeaderTypeDef time_since_bootup_can_header = {  .StdId = TIME_SINCE_BOOTUP_CAN_ID,
                                                            .ExtId = 0x0000,
                                                            .IDE = CAN_ID_STD,
                                                            .RTR = CAN_RTR_DATA,
                                                            .DLC = TIME_SINCE_BOOTUP_CAN_DATA_LENGTH};

const CAN_TxHeaderTypeDef commit_hash_can_header = {    .StdId = COMMIT_HASH_CAN_ID,
                                                        .ExtId = 0x0000,
                                                        .IDE = CAN_ID_STD,
                                                        .RTR = CAN_RTR_DATA,
                                                        .DLC = GIT_VERSION_CAN_DLC};
                                        
const CAN_TxHeaderTypeDef drd_mcu_health_header = { .StdId = DRD_MCU_HEALTH_CAN_ID,
                                                    .ExtId = 0x0000,
                                                    .IDE = CAN_ID_STD,
                                                    .RTR = CAN_RTR_DATA,
                                                    .DLC = DRD_MCU_HEALTH_CAN_DATA_LENGTH};

void CanFilterInit(CAN_FilterTypeDef* can_filter) {
    CAN_FilterTypeDef can_filter1;
    CAN_FilterTypeDef can_filter2;
    CAN_FilterTypeDef can_filter3;

    // ---- Filter Bank 0 ----
    can_filter->FilterIdHigh = (CAN_ID_BATT_FAULTS << 5);
    can_filter->FilterMaskIdHigh = (CAN_ID_PACK_VOLTAGE << 5);
    can_filter->FilterIdLow = (CAN_ID_PACK_HEALTH << 5);
    can_filter->FilterMaskIdLow = (CAN_ID_ECU << 5);
    can_filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter->FilterBank = 0;
    can_filter->FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter->FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter->FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, can_filter);

    // ---- Filter Bank 1 ----
    CAN_FilterTypeDef can_filter_mppt = {0};
    can_filter_mppt.FilterIdHigh = (CAN_ID_MPPTA_FAULTS << 5);
    can_filter_mppt.FilterMaskIdHigh = (CAN_ID_MPPTB_FAULTS << 5);
    can_filter_mppt.FilterIdLow = (CAN_ID_MPPTC_FAULTS << 5);
    can_filter_mppt.FilterMaskIdLow = (CAN_ID_MPPTD_FAULTS << 5);
    can_filter_mppt.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter_mppt.FilterBank = 1;
    can_filter_mppt.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter_mppt.FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter_mppt.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_mppt);

    // ---- Filter Bank 2 ---- 
    CAN_FilterTypeDef can_filter_boards = {0};
    can_filter_boards.FilterIdHigh = (CAN_ID_MST_FAULTS << 5);
    can_filter_boards.FilterMaskIdHigh = (CAN_ID_STR_FAULTS << 5);
    can_filter_boards.FilterIdLow = (CAN_ID_TEL_FAULTS << 5);
    can_filter_boards.FilterMaskIdLow = (CAN_ID_TEL_FAULTS << 5); 
    can_filter_boards.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter_boards.FilterBank = 2;
    can_filter_boards.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter_boards.FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter_boards.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_boards);

    // ---- Filter Bank 3 ----
    can_filter3.FilterIdHigh = (MPPTA_TEMPERATURE_CAN_ID << 5);
    can_filter3.FilterMaskIdHigh = (MPPTB_TEMPERATURE_CAN_ID << 5);
    can_filter3.FilterIdLow = (MPPTC_TEMPERATURE_CAN_ID << 5);
    can_filter3.FilterMaskIdLow = (MPPTC_TEMPERATURE_CAN_ID << 5);
    can_filter3.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter3.FilterBank = 3;
    can_filter3.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter3.FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter3.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter3);

    // ---- Filter Bank 4 ----
    can_filter1.FilterIdHigh = (STR_CAN_MSG_ID << 5); // Set up filter for steering CAN messages
    can_filter1.FilterMaskIdHigh = (BMS_TEMPERATURES_CAN_ID << 5);
    can_filter1.FilterIdLow = (CAN_ID_HVC_FAULTS << 5); // HVC fault flags
    can_filter1.FilterMaskIdLow = (CAN_ID_MDI_FAULTS << 5); // MDI fault flags
    can_filter1.FilterFIFOAssignment = CAN_FILTER_FIFO0; // Route accepted messages to FIFO0
    can_filter1.FilterBank = 4;
    can_filter1.FilterMode = CAN_FILTERMODE_IDLIST; // Use identifier list mode (not mask)
    can_filter1.FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter1.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter1); // Register filter with hardware

    // ---- Filter Bank 5 ----
    uint32_t extId1 = CAN_ID_MTR_FAULTS;
    uint32_t extId2 = FRAME0;
    can_filter2.FilterIdHigh = (extId1 << 3) >> 16; // Set up filter for motor fault messages
    can_filter2.FilterIdLow  = ((extId1 << 3) & 0xFFFF) | 0x0004;
    can_filter2.FilterMaskIdHigh = (extId2 << 3) >> 16; // Accept only specific extended IDs (ensures only target messages pass)
    can_filter2.FilterMaskIdLow  = ((extId2 << 3) & 0xFFFF) | 0x0004;
    can_filter2.FilterFIFOAssignment = CAN_FILTER_FIFO0; // Route accepted messages to FIFO0
    can_filter2.FilterBank = 5;
    can_filter2.FilterMode = CAN_FILTERMODE_IDLIST; // Use identifier list mode (not mask)
    can_filter2.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter2.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter2); // Register filter with hardware

    // ---- Filter Bank 6 ---- 
    CAN_FilterTypeDef can_filter_obc = {0};
    uint32_t obc_id = CAN_ID_OBC_FAULTS;
    can_filter_obc.FilterIdHigh = (obc_id << 3) >> 16;
    can_filter_obc.FilterIdLow  = ((obc_id << 3) & 0xFFFF) | 0x0004;
    can_filter_obc.FilterMaskIdHigh = (obc_id << 3) >> 16; 
    can_filter_obc.FilterMaskIdLow  = ((obc_id << 3) & 0xFFFF) | 0x0004;
    can_filter_obc.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter_obc.FilterBank = 6;
    can_filter_obc.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter_obc.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter_obc.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_obc);
}