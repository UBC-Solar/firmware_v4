#ifndef __MCU_HEALTH_DRIVER_H__
#define __MCU_HEALTH_DRIVER_H__

#include <stdint.h>
#define MAX_TEMP_C 85
#define MIN_VOLT_MV 2000

void McuHealthDriverInit(void);
int16_t ReadVdd_mV(void);
int16_t ReadTempC(void);

#endif