/**
 * @file    iwdg_app.h
 * @brief   Internal Watchdog header file for the UBC Solar DRD board
 *
 * This header contains the function definitions for the watchdog reset handler and the refresh functionality.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#ifndef __IWDG_APP_H__
#define __IWDG_APP_H__

#include "iwdg.h"
#include "iwdg_driver.h"

/**
 * @brief Refreshes the independent watchdog timer to prevent a reset.
 * @param hiwdg2 Pointer to the IWDG handle structure.
 */ 
void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2);

/**
 * @brief Checks if the last reset was caused by the independent watchdog and handles it.
 * If a watchdog reset is detected, it sets a diagnostic flag and performs a series of refreshes to prevent an infinite reset loop.
 */ 
void IwdgAppResetHandle();

#endif //__IWDG_APP_H__