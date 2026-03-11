/**
 * @file    iwdg_driver.c
 * @brief   IWDG driver implementation for the UBC Solar DRD board
 *
 * This file contains the implementation of the IWDG (Independent Watchdog) driver for the UBC Solar DRD board.
 * It provides functions to refresh the watchdog, check for watchdog resets, and handle watchdog reset events.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

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
    HAL_GPIO_TogglePin(DEBUG_LEDA1_GPIO_Port, DEBUG_LEDA1_Pin);
    HAL_Delay(50);
    IwdgDriverRefresh(&hiwdg);
}