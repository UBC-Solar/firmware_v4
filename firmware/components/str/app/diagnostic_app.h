#ifndef DIAGNOSTIC_APP_H_
#define DIAGNOSTIC_APP_H_

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

/*	DIAGNOSTIC TX FUNCTION PROTOTYPES	*/

/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticTimeSinceBootup();

/**
 * @brief Records whether the last reset was caused by the watchdog.
 * @param reset Watchdog reset state.
 */
void DiagnosticSetWatchdogReset(bool reset);

#endif /* DIAGNOSTIC_APP_H_ */
