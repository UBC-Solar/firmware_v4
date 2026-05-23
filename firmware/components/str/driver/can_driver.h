#ifndef __CAN_DRIVER_H__
#define __CAN_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

void CanDriverInit(void);
bool CanDriverSend(const CAN_TxHeaderTypeDef *header, const uint8_t *data);

#endif /* __CAN_DRIVER_H__ */