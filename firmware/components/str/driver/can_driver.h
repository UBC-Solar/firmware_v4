#ifndef __CAN_DRIVER_H__
#define __CAN_DRIVER_H__

#include "stm32f1xx_hal.h"

#define STR_CAN_MSG_ID 0x580U

extern CAN_HandleTypeDef hcan;
extern const CAN_TxHeaderTypeDef steering_header;

void CanFilterInit(CAN_FilterTypeDef* can_filter);

#endif /* __CAN_DRIVER_H__ */