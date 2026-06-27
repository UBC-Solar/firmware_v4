#ifndef __IWDG_APP_H__
#define __IWDG_APP_H__

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
