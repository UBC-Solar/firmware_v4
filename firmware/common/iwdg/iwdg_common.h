/** 
* @file    iwdg.h
* @brief   Common header file for the Internal Watchdog that all boards on the car use.
*
* This header contains the function definitions for the watchdog reset handler and the refresh functionality.
*
* @author Shlok Lande
* @date Oct 3 2026
*/

#ifndef __IWDG_COMMON_H__
#define __IWDG_COMMON_H__

#include "iwdg.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>

#define IWDG_TASK_DELAY 100

/*
 * @brief Refresh the IWDG.
 * @param hiwdg1 pointer to a IWDG_HandleTypeDef
 */
void IwdgRefresh(IWDG_HandleTypeDef* hiwdg1);

/**
 * @brief Check if the IWDG reset occurred
 *
 * @return true if the IWDG reset occurred, and reset watchdog flags.
 */
bool IwdgIsReset();

/**
 * @brief Checks if the last reset was caused by the independent watchdog and handles it.
 * If a watchdog reset is detected, it sets a diagnostic flag and performs a series of refreshes to prevent an infinite reset loop.
 */ 
void IwdgResetHandle();

/**
 * @brief Checks if the last reset was caused by the independent watchdog and handles it.
 * If a watchdog reset is detected, it sets a diagnostic flag and performs a series of refreshes to prevent an infinite reset loop.
 */ 
 void IwdgReset();

/**
 * @brief Records whether the last reset was caused by the watchdog.
 * @param reset Watchdog reset state.
 */
 void SetWatchdogReset(bool reset);


#endif //__IWDG_COMMON_H__