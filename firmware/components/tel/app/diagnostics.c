#include "diagnostics.h"
#include "stdint.h"
#include "CAN_comms.h"
#include "telemetry_app.h"
#include "can_driver.h"
#include "can_app.h"

/* STATIC VARIABLES */
static DiagnosticTEL g_tel_diagnostic_flags = {0};
static volatile uint32_t g_time_since_bootup = 0;

/** FUNCTION DEFINITIONS */
void DiagnosticsTimeSinceBootup()
{
    g_time_since_bootup++;

    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };

    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
    osDelay(3);
    TelAppTransmitInternalMsg(&time_since_bootup_can_tx);
}

void DiagnosticsSendTelFlags()
{
    CAN_comms_Tx_msg_t tel_flags_can_tx = {
        .data[0] = g_tel_diagnostic_flags.raw,
        .header = tel_flags_can_header,
    };

    CAN_comms_Add_Tx_message(&tel_flags_can_tx);
    osDelay(3);
    TelAppTransmitInternalMsg(&tel_flags_can_tx);
}

/* DIAGNOSTIC FLAG SETTERS */
void DiagnosticsSetTelCrashIwdgFlag(bool value)
{
    g_tel_diagnostic_flags.bits.tel_crash_iwdg = value;
}

void DiagnosticsSetImuReadFailFlag(bool value)
{
    g_tel_diagnostic_flags.bits.imu_read_fail = value;
}

void DiagnosticsSetImuWriteFailFlag(bool value)
{
    g_tel_diagnostic_flags.bits.imu_write_fail = value;
}

void DiagnosticsSetGpsReadFailFlag(bool value)
{
    g_tel_diagnostic_flags.bits.gps_read_fail = value;
}

void DiagnosticsSetGpsWriteFailFlag(bool value)
{
    g_tel_diagnostic_flags.bits.gps_write_fail = value;
}