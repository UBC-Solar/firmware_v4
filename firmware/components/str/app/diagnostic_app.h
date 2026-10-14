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
        volatile bool adc_read_fail : 1;
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
 * @brief  Sends the STR diagnostic flags via CAN
 * @retval None
 */
void DiagnosticsSendStrFlags();

/**
 * @brief Records whether the last reset was caused by the watchdog.
 * @param reset Watchdog reset state.
 */
void DiagnosticSetWatchdogReset(bool reset);

/**
 * @brief Sends MCU input voltage in mv and temperature in Celcius
 */
void McuDiagnosticSendHealth();

/**
 * @brief  Sets the MCU Under Voltage fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetMcuUnderVoltageFailFlag(bool value);

/**
 * @brief  Sets the MCU Over Temperature fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetMcuOverTempFailFlag(bool value);

/**
 * @brief  Sets the STR read fail flag
 * @param  value: The value to set the flag to
 * @retval None
 */
void DiagnosticsSetStrReadFailFlag(bool value);

#endif /* DIAGNOSTIC_APP_H_ */
