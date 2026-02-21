#include <stdbool.h>
#include "diagnostic.h"
#include "iwdg_app.h"


void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2)
{
	IwdgDriverRefresh(hiwdg2);
}

void IwdgAppResetHandle(){
    if (IwdgDriverIsReset())
	{

		g_diagnostics.flags.watchdog_reset = true;

        // Refresh the watchdog and flash the LED a few times to indicate that a reset occurred
		for (int i = 0; i < 10; i++)
		{
			IwdgDriverResetHandle();
		}
	}
}