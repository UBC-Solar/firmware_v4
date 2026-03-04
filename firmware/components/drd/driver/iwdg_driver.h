/**
 * @file    iwdg_driver.h
 * @brief   Independent Watchdog driver header file for the UBC Solar DRD board
 *
 * This header contains the function definitions for the independent watchdog driver, including refreshing the watchdog,
 * checking if a reset was caused by the watchdog, and handling watchdog resets.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#ifndef __IWDG_DRIVER_H__
#define __IWDG_DRIVER_H__

#include "iwdg.h"
#include "stm32f1xx_hal_iwdg.h"
#include <stdbool.h>


void IwdgDriverRefresh(IWDG_HandleTypeDef* hiwdg1);
bool IwdgDriverIsReset();
void IwdgDriverResetHandle();


#endif //__IWDG_DRIVER_H__