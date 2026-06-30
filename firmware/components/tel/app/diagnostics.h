/**
 * @file    diagnostics.h
 * @brief   Diagnostic application header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the diagnostic functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include "stdint.h"
#include "stdbool.h"

/* DATA TYPES */
typedef union {
    struct {
        volatile bool tel_crash_iwdg : 1;         
        volatile bool imu_read_fail : 1;
        volatile bool imu_write_fail : 1;
        volatile bool gps_read_fail : 1;
        volatile bool gps_write_fail : 1;
    } bits;
    volatile uint8_t raw;
} DiagnosticTEL;

/* FUNCTION PROTOTYPES */
/**
 * @brief  Initializes the diagnostics module
 * @retval None
 */
void DiagnosticsInit();

/**
 * @brief  Sends the TEL diagnostic flags via CAN
 * @retval None
 */
void DiagnosticsSendTelFlags();

/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticsTimeSinceBootup();

/**
 * @brief  Sets the TEL crash IWDG flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetTelCrashIwdgFlag(bool value);

/**
 * @brief  Sets the IMU read fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetImuReadFailFlag(bool value);

/**
 * @brief  Sets the IMU write fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetImuWriteFailFlag(bool value);

/**
 * @brief  Sets the GPS read fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetGpsReadFailFlag(bool value);

/**
 * @brief  Sets the GPS write fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetGpsWriteFailFlag(bool value);


#endif /* DIAGNOSTIC_H_ */
