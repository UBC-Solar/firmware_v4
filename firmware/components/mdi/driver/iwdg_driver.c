/**
 * @file    iwdg_driver.c
 * @brief   Independent watchdog driver implementation for MDI.
 */
#include "iwdg_driver.h"

#include "main.h"

static IWDG_HandleTypeDef s_hiwdg;

void IwdgDriverInit(void)
{
#ifdef DEBUG
    return;
#endif

    s_hiwdg.Instance = IWDG;
    s_hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
    s_hiwdg.Init.Reload = 4095;

    if (HAL_IWDG_Init(&s_hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
}

void IwdgDriverRefresh(void)
{
#ifndef DEBUG
    HAL_IWDG_Refresh(&s_hiwdg);
#endif
}

bool IwdgDriverIsReset(void)
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        return true;
    }

    return false;
}
