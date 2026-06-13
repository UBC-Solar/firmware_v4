#include <stdbool.h>
#include "diagnostic_app.h"
#include "iwdg_app.h"

void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2)
{
	#ifndef DEBUG
		IwdgDriverRefresh(hiwdg2);
	#endif // DEBUG
}

void IwdgAppResetHandle()
{
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
