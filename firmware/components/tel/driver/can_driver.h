/**
 * @file    can_driver.h
 * @brief   CAN driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the CAN driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#ifndef __CAN_DRIVER__H__
#define __CAN_DRIVER__H__

#include "CAN_comms.h"

#define TIME_SINCE_BOOTUP_CAN_DATA_LENGTH          4       
#define TIME_SINCE_BOOTUP_CAN_ID                   0x750

#define TEL_FLAGS_CAN_DATA_LENGTH                  1       
#define TEL_FLAGS_BOOTUP_CAN_ID                    0x751

/* CAN Message Headers */
extern const CAN_TxHeaderTypeDef time_since_bootup_can_header;
extern const CAN_TxHeaderTypeDef tel_flags_can_header;

/**
 * @brief Initializes CAN Comms hardware requirements and configures CAN filters for the TEL subsystem.
 * @return CAN comms configuration structure
 */
CAN_comms_config_t CanDriverInit();

#endif /* __CAN_DRIVER__H__ */