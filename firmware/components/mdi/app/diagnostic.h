/**
 * @file    diagnostic.h
 * @brief   MDI diagnostic flags.
 */

#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_OVER_TEMP_SET_C 100

typedef union {
    struct {
        volatile bool mdi_crash_iwdg : 1;
        volatile bool mdi_voltage_over_threshold : 1;
        volatile bool mdi_motor_over_temp : 1;
    } bits;
    volatile uint8_t raw;
} MdiDiagnosticFlags;

/**
 * @brief Resets all diagnostic flags to their default cleared state.
 */
void DiagnosticInit(void);

/**
 * @brief Sets or clears the watchdog crash diagnostic bit.
 * @param has_crash true when an IWDG crash/reset is detected.
 */
void DiagnosticSetIwdgCrash(bool has_crash);

/**
 * @brief Sets or clears the voltage threshold diagnostic bit.
 * @param is_over_threshold true when voltage is above configured threshold.
 */
void DiagnosticSetVoltageOverThreshold(bool is_over_threshold);

/**
 * @brief Sends the time-since-boot diagnostic frame over CAN.
 */
void DiagnosticSendTimeSinceBootup(void);

/**
 * @brief Reads RTD temperature, updates thermal fault state, and transmits RTD telemetry.
 */
void DiagnosticSendRtdTemp(void);

/**
 * @brief Sends packed diagnostic flag bits over CAN.
 */
void DiagnosticSendFlags(void);

#endif /* DIAGNOSTIC_H_ */
