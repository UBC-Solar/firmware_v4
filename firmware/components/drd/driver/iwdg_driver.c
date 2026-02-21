#include "iwdg_driver.h"
#include "main.h"

/*
 * @brief Refresh the IWDG.
 * @param hiwdg1 pointer to a IWDG_HandleTypeDef
 */
void IwdgDriverRefresh(IWDG_HandleTypeDef* hiwdg1)
{
	HAL_IWDG_Refresh(hiwdg1);
}

/**
 * @brief Check if the IWDG reset occurred
 *
 * @return true if the IWDG reset occurred, and reset watchdog flags.
 */
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

/**
 * @brief Flashes the LED and refreshes the IWDG to indicate that a reset occurred
 *
 * Note: This function is intended to be called in a loop to continuously flash the LED and refresh the IWDG
 */
void IwdgDriverResetHandle()
{
    HAL_GPIO_TogglePin(DEBUG_LEDA1_GPIO_Port, DEBUG_LEDA1_Pin);
    HAL_Delay(50);
    IwdgDriverRefresh(&hiwdg);
}