/**
 * @file    can_driver.h
 * @brief   CAN driver definitions for the MDI board.
 */
#ifndef __CAN_DRIVER_H__
#define __CAN_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

#define DRD_MOTOR_COMMAND_CAN_ID 0x401U
#define MDI_TIME_SINCE_BOOTUP_CAN_ID 0x500U
#define MDI_DIAGNOSTIC_FLAGS_CAN_ID 0x501U

typedef void (*CanDriverRxCallback)(const CAN_RxHeaderTypeDef *header, const uint8_t *data);

void CanDriverInit(void);
bool CanDriverSend(const CAN_TxHeaderTypeDef *header, const uint8_t *data);
void CanDriverRegisterRxCallback(CanDriverRxCallback callback);

#endif /* __CAN_DRIVER_H__ */
