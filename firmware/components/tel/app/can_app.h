/**
 * @file    can_app.h
 * @brief   CAN application header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the CAN application, 
 * which handles the reception and processing of CAN messages.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#ifndef __CAN_APP__H__
#define __CAN_APP__H__

#include "can_driver.h"     // CAN IDs used by TEL (sent, received and forwarded)

typedef struct {
    uint32_t id;
    uint8_t  mod;     // transmit on radio only every 'mod' occurrences
    uint32_t count;   // count of received messages
} CanFilter_t;

static CanFilter_t filter_whitelist[]  __attribute__((unused)) = {
    //         CAN ID                   MOD    COUNT
    { DRD_MOTOR_COMMAND_ID,                 4,     0               },
    { DRD_DIAGNOSTICS_ID,                   4,     0               },
    { DRD_TIME_SINCE_BOOTUP_ID,             1,     0               },
    { ECU_STATUS_ID,                        1,     0               },
    { MDI_TIME_SINCE_BOOTUP_ID,             1,     0               },
    { MDI_DIAGNOSTICS_ID,                   1,     0               },
    { STR_DIAGNOSTICS_ID,                   1,     0               },
    { STR_TIME_SINCE_BOOTUP_ID,             1,     0               },
    { BMS_VOLTAGE_SUMMARY_VOLTAGE_ID,       10,    0               },
    { BMS_MODULE_VOLTAGES_ID,               9,     0               },
    { BMS_FAULTS_ID,                        1,     0               },
    { BMS_TEMP_SUMMARY_ID,                  10,    0               },
    { BMS_PACK_HEALTH_ID,                   1,     0               },
    { BMS_MODULE_TEMPERATURES_ID,           9,     0               },
    { MPPT_A_INPUT_MEASUREMENTS_ID,         1,     0               },
    { MPPT_B_INPUT_MEASUREMENTS_ID,         1,     0               },
    { MPPT_C_INPUT_MEASUREMENTS_ID,         1,     0               },

    { MPPT_A_OUTPUT_MEASUREMENTS_ID,        1,     0               },
    { MPPT_B_OUTPUT_MEASUREMENTS_ID,        1,     0               },
    { MPPT_C_OUTPUT_MEASUREMENTS_ID,        1,     0               },

    { MPPT_A_TEMPERATURE_ID,                5,     0               },
    { MPPT_B_TEMPERATURE_ID,                5,     0               },
    { MPPT_C_TEMPERATURE_ID,                5,     0               },
    
    { MPPT_A_STATUS_ID,                     3,     0               },
    { MPPT_B_STATUS_ID,                     3,     0               },
    { MPPT_C_STATUS_ID,                     3,     0               },

    { MPPT_A_POWER_CONNECTOR_ID,            4,     0               },
    { MPPT_B_POWER_CONNECTOR_ID,            4,     0               },
    { MPPT_C_POWER_CONNECTOR_ID,            4,     0               },

    { MPPT_A_LIMITS_ID,                     10,    0               },
    { MPPT_B_LIMITS_ID,                     10,    0               },
    { MPPT_C_LIMITS_ID,                     10,    0               },

    { TIME_SINCE_BOOTUP_CAN_ID,             1,     0               },
    { TEL_FLAGS_BOOTUP_CAN_ID,              1,     0               },
    { MDU_FRAME_0_ID,                       1,     0               },
    { MDU_FRAME_1_ID,                       5,     0               },
    { MDU_FRAME_2_ID,                       5,     0               },
    { OBC_STATUS_ID,                        1,     0               },
    { GPS_LONG_LAT_CAN_ID,                  1,     0               },
    { GPS_ALT_SPEED_HEADING_CAN_ID,         1,     0               },
    { GPS_STATUS_CAN_ID,                    1,     0               },
    { GPS_UTC_TIME_CAN_ID,                  1,     0               },

    { IMU_AG_X_CAN_MESSAGE_ID,              1,     0               },
    { IMU_AG_Y_CAN_MESSAGE_ID,              1,     0               },
    { IMU_AG_Z_CAN_MESSAGE_ID,              1,     0               },
    { IMU_M_X_CAN_MESSAGE_ID,               1,     0               },
    { IMU_M_Y_CAN_MESSAGE_ID,               1,     0               },
    { IMU_M_Z_CAN_MESSAGE_ID,               1,     0               },
};

/**
 * @brief Initialize the CAN application.
 * @param None
 * @retval None
 */
void CanAppInit();



/**
 * @brief Transmit ht CANload.
 * @param None
 * @retval None
 */
void CAN_tx_canload_msg();


#endif /* __CAN_APP__H__ */