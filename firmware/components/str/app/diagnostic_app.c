#include "diagnostic_app.h"

#include "CAN_comms.h"
#include "can_driver.h"
#include "gpio_driver.h"
#include "mcu_health_driver.h"

/* STATIC VARIABLES */
static DiagnosticTEL g_tel_diagnostic_flags = {0};
static uint32_t g_time_since_bootup = 0U;
static const uint16_t MCU_VOLTAGE_THRESHOLD = 3000; // temporarily defined here with random values 
static const int8_t MCU_TEMP_THRESHOLD = 33; // compare this with #define MCU_VOLTAGE_THRESHOLD 3000; when do you use which?

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

void DiagnosticsSendStrFlags()
{
    CAN_comms_Tx_msg_t str_flags_can_tx = {
        .data[0] = g_tel_diagnostic_flags.raw,
        .header = tel_flags_can_tx,
    };

    CAN_comms_Add_Tx_message(&str_flags_can_tx);
    osDelay(3);
    // TelAppTransmitInternalMsg(&str_flags_can_tx); i don't know what this is for tbh
}

void McuDiagnosticSendHealth() {
    McuHealthDriverInit(); // this is being re-initialized every time, only needs to run once... figure out where to put it

    HAL_StatusTypeDef error_status;

    uint16_t mcu_voltage_mv = 0;
    int8_t mcu_temp_c = 0;

    /* Attempt to read Voltage and Temperature */
    error_status = McuHealthReadVddMv(&mcu_voltage_mv);
    if (error_status != HAL_OK) DiagnosticsSetStrReadFailFlag(1); // raise failed read flag?

    error_status = McuHealthReadTempC(&mcu_temp_c);
    if (error_status != HAL_OK) DiagnosticsSetStrReadFailFlag(1); // raise failed read flag?

    /* Return flag if threshold is crossed */
    DiagnosticsSetStrUnderVoltageFlag(mcu_voltage_mv < MCU_VOLTAGE_THRESHOLD);
    DiagnosticsSetStrOverTempFlag(mcu_temp_c > MCU_TEMP_THRESHOLD);


    CAN_comms_Tx_msg_t mcu_diagnostics_can_tx = {
    .data[0] = (mcu_voltage_mv & 0x00FFU),  // LE order so data[0] corresponds to least significant
    .data[1] = (mcu_voltage_mv & 0xFF00U) >> 8,
    .data[2] = (mcu_temp_c),

    .header = str_mcu_health_can_header,
    };

    CAN_comms_Add_Tx_message(&mcu_diagnostics_can_tx);
}

void DiagnosticSetWatchdogReset(bool reset)
{
    (void)reset;
}

/* DIAGNOSTIC FLAG SETTERS */
void DiagnosticsSetMcuUnderVoltageFailFlag(bool value) {
    g_tel_diagnostic_flags.bits.mcu_under_voltage = value;
}

void DiagnosticsSetMcuOverTempFailFlag(bool value) {
    g_tel_diagnostic_flags.bits.mcu_over_temp = value;
}

void DiagnosticsSetStrReadAdcFailFlag(bool value) {
    g_tel_diagnostic_flags.bits.adc_read_fail = value;
}