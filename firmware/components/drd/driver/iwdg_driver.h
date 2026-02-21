#ifndef __IWDG_DRIVER_H__
#define __IWDG_DRIVER_H__

#include "iwdg.h"
#include "stm32f1xx_hal_iwdg.h"
#include <stdbool.h>


void IwdgDriverRefresh(IWDG_HandleTypeDef* hiwdg1);
bool IwdgDriverIsReset();
void IwdgDriverResetHandle();


#endif //__IWDG_DRIVER_H__