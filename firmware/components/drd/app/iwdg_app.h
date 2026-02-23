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

void IwdgAppRefresh(IWDG_HandleTypeDef* hiwdg2);
void IwdgAppResetHandle();

#endif //__IWDG_APP_H__