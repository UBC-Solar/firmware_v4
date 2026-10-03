/** 
* @file    iwdg.c
* @brief   Application filr for the WDG.
*
* Contains the app layer functions for refreshing the WDG and also the reset handler if the WDG is not refreshed.
*
* @author Shlok Lande
* @date Oct 3 2026
*/

#include <stdbool.h>
#include "main.h"
#include "iwdg_common.h"

void IwdgRefresh(IWDG_HandleTypeDef* hiwdg1)
{
	//#ifndef DEBUG
		HAL_IWDG_Refresh(hiwdg1);
	//#endif // DEBUG
}


bool IwdgIsReset()
{
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		return true;
	}
	else
	{
		return false;
	}
}

void SetWatchdogReset(bool reset)
{
    (void)reset;
}

void IwdgReset(){
    IwdgRefresh(&hiwdg);
}

void IwdgResetHandle(){
    if (IwdgIsReset())
	{
		// Set diagnostic flag to indicate that a watchdog reset occurred
		SetWatchdogReset(true);

        // Refresh the watchdog and flash the LED a few times to indicate that a reset occurred
		for (int i = 0; i < 10; i++)
		{
			IwdgReset();
		}
	}
}	