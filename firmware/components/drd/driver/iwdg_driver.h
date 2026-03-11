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

/*
 * @brief Refresh the IWDG.
 * @param hiwdg1 pointer to a IWDG_HandleTypeDef
 */
void IwdgDriverRefresh(IWDG_HandleTypeDef* hiwdg1);

/**
 * @brief Check if the IWDG reset occurred
 *
 * @return true if the IWDG reset occurred, and reset watchdog flags.
 */
bool IwdgDriverIsReset();

/**
 * @brief Flashes the LED and refreshes the IWDG to indicate that a reset occurred
 *
 * Note: This function is intended to be called in a loop to continuously flash the LED and refresh the IWDG
 */
void IwdgDriverResetHandle();


#endif //__IWDG_DRIVER_H__