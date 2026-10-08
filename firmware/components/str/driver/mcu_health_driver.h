#ifndef __MCU_SENSE_DRIVER_H__
#define __MCU_SENSE_DRIVER_H__

#include <stdint.h>

uint16_t McuSenseStrReadVddMv(void);
int16_t McuSenseStrReadTempC(void);
void McuHealthDriverInit(void);

#endif