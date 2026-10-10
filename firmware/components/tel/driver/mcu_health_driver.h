#ifndef __MCU_HEALTH_DRIVER_H__
#define __MCU_HEALTH_DRIVER_H__

#include <stdint.h>

int16_t ReadVdd_mV(void);
int16_t ReadTempC(void);

#endif