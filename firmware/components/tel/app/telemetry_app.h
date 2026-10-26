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

/* Initialize the Digi device and send one test packet per call.
 * TEL3.1 USART2 (PA2/PA3), configured at 230400 8N1.
 * Modem AP=1. Call from a task after RTOS startup, with normal traffic paused.
 * HAL_OK means DMA started; HAL_BUSY means the transmit semaphore is in use.
 * The DMA completion callback releases the buffer. No delivery acknowledgement.
 */
HAL_StatusTypeDef CellularXbeeInit(void);

#endif /* __TELEMETRY__APP__H__ */