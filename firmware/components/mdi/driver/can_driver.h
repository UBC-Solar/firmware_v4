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
#define MDI_MOTOR_TEMP_CAN_ID 0x502U
#define MDI_COMMIT_HASH_CAN_ID 0x504U
#define STR_DRD_CONTROL_CAN_ID 0x580U

typedef void (*CanDriverRxCallback)(const CAN_RxHeaderTypeDef *header, const uint8_t *data);

/**
 * @brief Initializes CAN hardware, filter, and RX notification handling.
 */
void CanDriverInit(void);

/**
 * @brief Sends a CAN frame using the provided header and payload bytes.
 * @param header Pointer to a configured CAN transmit header.
 * @param data Pointer to payload bytes for the frame.
 * @retval true Frame queued successfully.
 * @retval false Invalid input or HAL transmit failure.
 */
bool CanDriverSend(const CAN_TxHeaderTypeDef *header, const uint8_t *data);

/**
 * @brief Registers the callback invoked on received CAN messages.
 * @param callback Function pointer called for each RX frame.
 */
void CanDriverRegisterRxCallback(CanDriverRxCallback callback);

/**
 * @brief  Reads the CAN transmit error counter (TEC) from the controller.
 *         The hardware adds 8 per transmit error and subtracts 1 per successful transmit.
 * @return TEC value (0–255). Above 127 the node is error passive.
 */
uint8_t CAN_comms_get_tec(void);

/**
 * @brief  Reads the CAN receive error counter (REC) from the controller.
 *         The hardware adds 1 per receive error (up to 8) and subtracts 1 per
 *         successful reception.
 * @return REC value (0–255). Above 127 the node is error passive.
 */
uint8_t CAN_comms_get_rec(void);

#endif /* __CAN_DRIVER_H__ */
