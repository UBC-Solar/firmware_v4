#include "diagnostics.h"
#include "stdint.h"
#include "CAN_comms.h"
#include "telemetry_app.h"
#include "can_driver.h"
#include "can_app.h"

// TODO: change to static variables
DiagnosticTEL g_tel_diagnostic_flags = {0};
volatile uint32_t g_time_since_bootup;

/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
static void DiagnosticsTimeSinceBootup();

/**
 * @brief  Sends the TEL diagnostic flags via CAN
 * @retval None
 */
static void DiagnosticsSendTelFlags();

static void DiagnosticsTimeSinceBootup()
{
    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };

    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
    osDelay(3);
    TelAppTransmitMsg_tx(&time_since_bootup_can_tx);
}

static void DiagnosticsSendTelFlags()
{
    CAN_comms_Tx_msg_t tel_flags_can_tx = {
        .data[0] = g_tel_diagnostic_flags.raw,
        .header = tel_flags_can_header,
    };

    CAN_comms_Add_Tx_message(&tel_flags_can_tx);
    osDelay(3);
    TelAppTransmitMsg_tx(&tel_flags_can_tx);
}

void DiagnosticsTransmit()
{
    DiagnosticsTimeSinceBootup();
    DiagnosticsSendTelFlags();
}

void DiagnosticsInit()
{
    // Initilaize diagnostic flags as cleared at first
    g_tel_diagnostic_flags.raw = false;
    g_time_since_bootup = 0;
}
