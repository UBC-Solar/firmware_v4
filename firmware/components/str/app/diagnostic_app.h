#ifndef DIAGNOSTIC_APP_H_
#define DIAGNOSTIC_APP_H_

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

/* DATA TYPES */
typedef union {
    struct {
        volatile bool mcu_under_voltage : 1;
        volatile bool mcu_over_temp : 1;
    } bits;
    volatile uint8_t raw;
} DiagnosticSTR;


/* FUNCTION PROTOTYPES	*/
/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticTimeSinceBootup();

/**
 * @brief  Sends the git commit hash the firmware was built from via CAN
 * @retval None
 */
void DiagnosticSendCommitHash();
 * @brief Calibrates ADC
 * @retval None
 */
void McuDiagnsoticInit();

/**
 * @brief Records whether the last reset was caused by the watchdog.
 * @param reset Watchdog reset state.
 */
void DiagnosticSetWatchdogReset(bool reset);

/**
 * @brief Sends MCU input voltage in mv and temperature in Celcius
 * @retval None
 */
void McuDiagnosticSendHealth();

/**
 * @brief  Sends the STR flags over CAN
 * @retval None
 */
void DiagnosticsSendStrFlags();

/**
 * @brief  Sets the MCU Under Voltage fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetMcuUnderVoltageFlag(bool value);

/**
 * @brief  Sets the MCU Over Temperature fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetMcuOverTempFlag(bool value);

#endif /* DIAGNOSTIC_APP_H_ */
