#include <stdbool.h>
#include "diagnostic.h"
#include "iwdg_app.h"


void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2)
{
	IwdgDriverRefresh(hiwdg2);
}

/**
 * @brief Checks if the last reset was caused by the independent watchdog and handles it.
 * If a watchdog reset is detected, it sets a diagnostic flag and performs a series of refreshes to prevent an infinite reset loop.
 */ 
void IwdgAppResetHandle(){
    if (IwdgDriverIsReset())
	{
		// Set diagnostic flag to indicate that a watchdog reset occurred
		g_diagnostics.flags.watchdog_reset = true;

        // Refresh the watchdog and flash the LED a few times to indicate that a reset occurred
		for (int i = 0; i < 10; i++)
		{
			IwdgDriverResetHandle();
		}
	}
}	