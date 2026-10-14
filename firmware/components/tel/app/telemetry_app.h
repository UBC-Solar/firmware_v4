/**
 * @file    telemetry_app.h
 * @brief   Telemetry application header file for the UBC Solar telemetry system
 *
 * This header file contains the implementation of the telemetry application logic for the UBC Solar telemetry system. 
 * It handles the transmission and processing of telemetry messages.
 *
 * @author  Tony Chen and Gregory Bian
 * @date    May 22 2026
 *
 * The format sent depends on CAR_CONFIG_CELLULAR:
 *
 * Radio (UART4): 24-byte binary TEL_Msg_TypeDef per message.
 *
 * Cellular (USART2): one ASCII line per message, read by the MicroPython script on the Digi
 * XBee Cellular module:
 *
 *   @<epoch ms>,<ID>#<DATA>*<CK>\n          e.g.  @1791648000123,622#0102030405060708*05\n
 *
 *   epoch ms   UTC milliseconds since 1970, in decimal. Set from GPS once it has valid, fully
 *              resolved time, else from the memorator (0x300). Until then it counts up from
 *              2000-01-01 (946684800000) at boot.
 *   ID         Uppercase hex. 3 digits for a standard ID, 8 for an extended ID.
 *   DATA       2 uppercase hex digits per data byte, DLC bytes (0-8). DLC = len(DATA) / 2.
 *   CK         XOR of every character after '@' and before '*', as 2 uppercase hex digits
 *              (NMEA style). Drop lines that fail it, or that don't start with '@'.
 *
 * A line is at most 44 characters with a 13-digit timestamp (51 worst case). The ID and data
 * never contain '@', '*' or '\n', so a reader that loses bytes recovers at the next newline.
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

#endif /* __TELEMETRY__APP__H__ */