/**
 * @file    iwdg_driver.h
 * @brief   Independent watchdog driver for MDI.
 */
#ifndef __IWDG_DRIVER_H__
#define __IWDG_DRIVER_H__

#include <stdbool.h>

#include "stm32f1xx_hal.h"

void IwdgDriverInit(void);
void IwdgDriverRefresh(void);
bool IwdgDriverIsReset(void);

#endif /* __IWDG_DRIVER_H__ */
