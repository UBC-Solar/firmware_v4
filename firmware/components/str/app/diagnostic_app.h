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

#endif /* DIAGNOSTIC_APP_H_ */
