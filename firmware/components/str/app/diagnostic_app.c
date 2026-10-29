#include "diagnostic_app.h"
#include "CAN_comms.h"
#include "can_driver.h"
#include "git_version.h"
#include "gpio_driver.h"
#include "mcu_health_driver.h"

#define MCU_VOLTAGE_THRESHOLD 2400
#define MCU_TEMP_THRESHOLD 105


/* STATIC VARIABLES */
static DiagnosticSTR g_tel_diagnostic_flags = {0};
static uint32_t g_time_since_bootup = 0U;


/* FUNCTION DEFINITIONS */
void DiagnosticTimeSinceBootup()
{
    g_time_since_bootup++;

    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };

    GpioDriverToggleDebugLed();
    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
}

void DiagnosticSendCommitHash()
{
    CAN_comms_Tx_msg_t commit_hash_can_tx = {
        .header = commit_hash_can_header,
    };
    GitVersionPackCan(commit_hash_can_tx.data);
    CAN_comms_Add_Tx_message(&commit_hash_can_tx);
}

void McuDiagnsoticInit() {
    McuHealthDriverInit();
}

void DiagnosticsSendStrFlags()
{
    CAN_comms_Tx_msg_t str_flags_can_tx = {
        .data[0] = g_tel_diagnostic_flags.raw,
        .header = str_flags_can_header,
    };

    CAN_comms_Add_Tx_message(&str_flags_can_tx);
    osDelay(3);
}

void McuDiagnosticSendHealth() {
    /* Read Voltage and Temperature */
    uint16_t mcu_voltage_mv = McuHealthReadVddMv();
    int16_t mcu_temp_c = McuHealthReadTempC();

    /* Return flag if threshold is crossed */
    DiagnosticsSetMcuUnderVoltageFlag(mcu_voltage_mv < MCU_VOLTAGE_THRESHOLD);
    DiagnosticsSetMcuOverTempFlag(mcu_temp_c > MCU_TEMP_THRESHOLD);


    CAN_comms_Tx_msg_t mcu_diagnostics_can_tx = {
    .data[0] = (mcu_voltage_mv & 0x00FFU),  // LE order so data[0] corresponds to least significant
    .data[1] = (mcu_voltage_mv & 0xFF00U) >> 8,
    .data[2] = (mcu_temp_c & 0x00FFU),
    .data[3] = (mcu_temp_c & 0xFF00U) >> 8,

    .header = str_mcu_health_can_header,
    };

    CAN_comms_Add_Tx_message(&mcu_diagnostics_can_tx);
}

void DiagnosticSetWatchdogReset(bool reset)
{
    (void)reset;
}

/* DIAGNOSTIC FLAG SETTERS */
void DiagnosticsSetMcuUnderVoltageFlag(bool value) {
    g_tel_diagnostic_flags.bits.mcu_under_voltage = value;
}

void DiagnosticsSetMcuOverTempFlag(bool value) {
    g_tel_diagnostic_flags.bits.mcu_over_temp = value;
}
