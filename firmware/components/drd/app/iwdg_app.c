/**
 * @file    iwdg_app.c
 * @brief   Independent Watchdog application for the UBC Solar DRD board
 *
 * This file contains the IWDG handler for to refresh the watchdog every ~100ms and the reset handler when
 * the watchdog does not refresh in time.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#include <stdbool.h>
#include "diagnostic.h"
#include "iwdg_app.h"


void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2)
{
	#ifndef DEBUG
		IwdgDriverRefresh(hiwdg2); 	
	#endif // DEBUG
}

void IwdgAppResetHandle(){
    if (IwdgDriverIsReset())
	{
		// Set diagnostic flag to indicate that a watchdog reset occurred
		DiagnosticSetWatchdogReset(true);

        // Refresh the watchdog and flash the LED a few times to indicate that a reset occurred
		for (int i = 0; i < 10; i++)
		{
			IwdgDriverResetHandle();
		}
	}
}	