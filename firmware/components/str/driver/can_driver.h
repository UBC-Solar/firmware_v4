/**
 * @file    can_driver.h
 * @brief   CAN driver interface for the UBC Solar STR board.
 */

#ifndef __CAN_DRIVER_H__
#define __CAN_DRIVER_H__

/* INCLUDES */
#include "stm32f1xx_hal.h"

/* DEFINES */
#define STR_CAN_MSG_ID 0x580U
#define DRD_MOTOR_COMMAND_CAN_ID 0x401U
#define TIME_SINCE_BOOTUP_CAN_ID 0x581U
#define TIME_SINCE_BOOTUP_CAN_DATA_LENGTH 4U

/* EXTERNAL VARIABLES */
extern CAN_HandleTypeDef hcan;
extern const CAN_TxHeaderTypeDef steering_header;
extern const CAN_TxHeaderTypeDef time_since_bootup_can_header;

/* FUNCTION PROTOTYPES */
/**
 * @brief Configures the STR CAN receive filter.
 * @param can_filter Pointer to the filter configuration to populate.
 */
void CanFilterInit(CAN_FilterTypeDef* can_filter);

#endif /* __CAN_DRIVER_H__ */
