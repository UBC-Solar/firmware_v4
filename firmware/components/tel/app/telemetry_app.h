/**
 * @file    telemetry_app.h
 * @brief   Telemetry application header file for the UBC Solar telemetry system
 *
 * This header file contains the implementation of the telemetry application logic for the UBC Solar telemetry system. 
 * It handles the transmission and processing of telemetry messages.
 *
 * @author  Tony Chen and Gregory Bian
 * @date    May 22 2026
 */

#ifndef __TELEMETRY__APP__H__
#define __TELEMETRY__APP__H__

/* INCLUDES */
#include "can.h"
#include "telemetry_driver.h"
#include "CAN_comms.h"
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

/* Initialize Digi and send the known test CAN frame once (ID 1878, bytes 01..08).
 * Uses the same path as normal CAN forwarding below.
 */
HAL_StatusTypeDef CellularXbeeInit(void);

/* Send one classic CAN data frame as newline-delimited JSON over cellular TCP.
 * Supports 11/29-bit IDs and DLC 0..8. Call from a task after RTOS startup.
 * Waits for the previous UART DMA send; HAL_OK means the new DMA send started.
 * Modem: 230400 baud, AP=1. No delivery acknowledgement or echo check.
 */
HAL_StatusTypeDef CellularXbeeSendCAN(uint32_t can_id, const uint8_t *data, uint32_t dlc);

#endif /* __TELEMETRY__APP__H__ */
