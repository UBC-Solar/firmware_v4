/**
 * @file    telemetry_app.h
 * @brief   Telemetry application header file for the UBC Solar telemetry system
 *
 * This header file contains the implementation of the telemetry application logic for the UBC Solar
 * telemetry system. It handles the transmission and processing of telemetry messages.
 *
 * @author  Tony Chen and Gregory Bian
 * @date    May 22 2026
 */

#ifndef __TELEMETRY__APP__H__
#define __TELEMETRY__APP__H__

/* INCLUDES */
#include "CAN_comms.h"
#include "can.h"
#include "telemetry_driver.h"
#include <stdint.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief   Transmit a telemetry message
 * @param   CAN_comms_Rx_msg: Pointer to the received CAN message
 * @retval  None
 */
void TelAppTransmitMsg(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);
/**
 * @brief   Transmit a telemetry message for Tx messages (e.g. GPS, IMU)
 * @param   CAN_comms_Tx_msg: Pointer to the transmitted CAN message
 * @retval  None
 */
void TelAppTransmitInternalMsg(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg);

/** @brief Initialize Digi once without sending test data. Call from an RTOS task. */
HAL_StatusTypeDef CellularXbeeInit(void);

/** @brief Drain modem responses from task context; does not wait for the TX semaphore. */
void CellularXbeePoll(void);

/** @brief Explicit bench test: sends CAN ID 1878, bytes 01..08. Never called automatically. */
HAL_StatusTypeDef CellularXbeeSendTest(void);

/**
 * @brief   Send a classic CAN data frame as newline-delimited JSON over cellular TCP.
 * @param   can_id: Standard or extended CAN identifier (0..0x1FFFFFFF).
 * @param   data: Payload bytes; may be NULL only when dlc is zero.
 * @param   dlc: Number of payload bytes (0..8).
 * @note    Call from a task after RTOS startup. Waits for the previous UART DMA send.
 *          The modem must use 230400 baud and API mode AP=1. Payload bytes are copied
 *          before returning. Modem statuses are counted asynchronously; backend receipt is not confirmed.
 *          JSON ts is RTC Unix time in seconds (millisecond precision); RTC must be set correctly.
 * @retval  HAL_OK DMA transmission started; remote delivery is not confirmed.
 * @retval  HAL_BUSY The transmit semaphore, Digi frame builder, or UART was busy.
 * @retval  HAL_ERROR Invalid arguments/state, encoding, initialization, or transmit failure.
 * @retval  HAL_TIMEOUT The UART transmit operation timed out.
 */
HAL_StatusTypeDef CellularXbeeSendCAN(uint32_t can_id, const uint8_t* data, uint32_t dlc);

#endif /* __TELEMETRY__APP__H__ */
