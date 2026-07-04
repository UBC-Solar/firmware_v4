#include "iwdg_driver.h"
#include "main.h"

void IwdgDriverRefresh(IWDG_HandleTypeDef* hiwdg1)
{
	HAL_IWDG_Refresh(hiwdg1);
}

bool IwdgDriverIsReset()
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

void IwdgDriverResetHandle()
{
    HAL_GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);
    HAL_Delay(50);
    IwdgDriverRefresh(&hiwdg);
}
