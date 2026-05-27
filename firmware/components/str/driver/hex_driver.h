#ifndef __HEX__DRIVER__H__
#define __HEX__DRIVER__H__

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

#define AS1115_REG_DIGIT0      0x01
#define AS1115_REG_DIGIT1      0x02

bool HexDisplayInit(void);
HAL_StatusTypeDef HexDisplayWriteReg(uint8_t reg, uint8_t data);

#endif /* __HEX__DRIVER__H__ */