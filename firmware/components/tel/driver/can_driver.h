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

/* CAN IDs TEL sends */
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

#define CANLOAD_DATA_LENGTH                        1
#define CANLOAD_MSG_ID                             0x763

/* CAN IDs from other boards that TEL receives or forwards */
#define RTC_TIMESTAMP_MSG_ID            0x300

#define DRD_DIAGNOSTICS_ID                  0x403
#define MDI_DIAGNOSTICS_ID                  0x501
#define STR_DIAGNOSTICS_ID                  0x581

#define DRD_TIME_SINCE_BOOTUP_ID            0x404
#define MDI_TIME_SINCE_BOOTUP_ID            0x500
#define STR_TIME_SINCE_BOOTUP_ID            0x582

#define DRD_MOTOR_COMMAND_ID                0x401

#define ECU_STATUS_ID                       0x450
#define BMS_FAULTS_ID                       0x622
#define BMS_VOLTAGE_SUMMARY_VOLTAGE_ID      0x623
#define BMS_PACK_HEALTH_ID                  0x624
#define BMS_TEMP_SUMMARY_ID                 0x625
#define BMS_MODULE_VOLTAGES_ID              0x626
#define BMS_MODULE_TEMPERATURES_ID          0x627

#define MPPT_A_INPUT_MEASUREMENTS_ID          0x6A0
#define MPPT_B_INPUT_MEASUREMENTS_ID          0x6B0
#define MPPT_C_INPUT_MEASUREMENTS_ID          0x6C0

#define MPPT_A_OUTPUT_MEASUREMENTS_ID         0x6A1
#define MPPT_B_OUTPUT_MEASUREMENTS_ID         0x6B1
#define MPPT_C_OUTPUT_MEASUREMENTS_ID         0x6C1

#define MPPT_A_TEMPERATURE_ID                 0x6A2
#define MPPT_B_TEMPERATURE_ID                 0x6B2
#define MPPT_C_TEMPERATURE_ID                 0x6C2

#define MPPT_A_STATUS_ID                      0x6A5
#define MPPT_B_STATUS_ID                      0x6B5
#define MPPT_C_STATUS_ID                      0x6C5

#define MPPT_A_POWER_CONNECTOR_ID             0x6A6
#define MPPT_B_POWER_CONNECTOR_ID             0x6B6
#define MPPT_C_POWER_CONNECTOR_ID             0x6C6

#define MPPT_A_LIMITS_ID                      0x6A4
#define MPPT_B_LIMITS_ID                      0x6B4
#define MPPT_C_LIMITS_ID                      0x6C4

#define MDU_FRAME_0_ID                      0x08850225
#define MDU_FRAME_1_ID                      0x08950225
#define MDU_FRAME_2_ID                      0x08A50225

#define OBC_STATUS_ID                       0x18FF50E5

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