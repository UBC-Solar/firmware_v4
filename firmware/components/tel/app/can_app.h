#ifndef __CAN_APP__H__
#define __CAN_APP__H__

// Definitions for CAN IDs (add or adjust as needed)
#define DRD_DIAGNOSTICS_ID                  0x403
#define MDI_DIAGNOSTICS_ID                  0x501
#define STR_DIAGNOSTICS_ID                  0x581
#define TEL_DIAGNOSTICS_ID                  0x751

#define DRD_TIME_SINCE_BOOTUP_ID            0x404
#define MDI_TIME_SINCE_BOOTUP_ID            0x500
#define STR_TIME_SINCE_BOOTUP_ID            0x582
#define TEL_TIME_SINCE_BOOTUP_ID            0x750

#define GPS_LONG_LAT_ID                     0x756

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

    { TEL_TIME_SINCE_BOOTUP_ID,             1,     0               },
    { TEL_DIAGNOSTICS_ID,                   1,     0               },
    { MDU_FRAME_0_ID,                       1,     0               },
    { MDU_FRAME_1_ID,                       5,     0               },
    { MDU_FRAME_2_ID,                       5,     0               },
    { OBC_STATUS_ID,                        1,     0               },
    { GPS_LONG_LAT_ID,                      1,     0               },
};

/**
 * @brief Initialize the CAN application.
 * @param None
 * @retval None
 */
void CanAppInit();

#endif /* __CAN_APP__H__ */