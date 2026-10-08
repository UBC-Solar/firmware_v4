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

#define IMU_CAN_MESSAGE_AG_LENGTH 8
#define IMU_AG_X_CAN_MESSAGE_ID 0x752
#define IMU_AG_Y_CAN_MESSAGE_ID 0x753
#define IMU_AG_Z_CAN_MESSAGE_ID 0x754

#define IMU_CAN_MESSAGE_M_LENGTH 4
#define IMU_M_X_CAN_MESSAGE_ID 0x755
#define IMU_M_Y_CAN_MESSAGE_ID 0x756
#define IMU_M_Z_CAN_MESSAGE_ID 0x757

#define GPS_LONG_LAT_CAN_DATA_LENGTH               8
#define GPS_LONG_LAT_CAN_ID                        0x758

#define GPS_ALT_SPEED_HEADING_CAN_DATA_LENGTH      8
#define GPS_ALT_SPEED_HEADING_CAN_ID               0x759

#define GPS_STATUS_CAN_DATA_LENGTH                 8
#define GPS_STATUS_CAN_ID                          0x75A

#define GPS_UTC_TIME_CAN_DATA_LENGTH               6
#define GPS_UTC_TIME_CAN_ID                        0x75B

/* CAN Message Headers */
extern const CAN_TxHeaderTypeDef time_since_bootup_can_header;
extern const CAN_TxHeaderTypeDef tel_flags_can_header;
extern const CAN_TxHeaderTypeDef CANLOAD_busload;

extern const CAN_TxHeaderTypeDef imu_ag_x;
extern const CAN_TxHeaderTypeDef imu_ag_y;
extern const CAN_TxHeaderTypeDef imu_ag_z;
extern const CAN_TxHeaderTypeDef imu_m_x;
extern const CAN_TxHeaderTypeDef imu_m_y;
extern const CAN_TxHeaderTypeDef imu_m_z;

extern const CAN_TxHeaderTypeDef gps_long_lat_can_header;
extern const CAN_TxHeaderTypeDef gps_alt_speed_heading_can_header;
extern const CAN_TxHeaderTypeDef gps_status_can_header;
extern const CAN_TxHeaderTypeDef gps_utc_time_can_header;

/**
 * @brief Initializes CAN Comms hardware requirements and configures CAN filters for the TEL subsystem.
 * @return CAN comms configuration structure
 */
CAN_comms_config_t CanDriverInit();

#endif /* __CAN_DRIVER__H__ */