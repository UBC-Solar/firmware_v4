/**
 * @file can_messages.h
 * @brief DIST CAN message IDs and the functions that build, send, and handle DIST CAN messages.
 *
 * Sits on top of can_driver.h: the driver owns the CAN peripheral and TX queue, this module owns
 * what each DIST message contains and what each received message means.
 *
 * Changing any CAN message (an ID, a layout, or what a value means) also means updating the DBC
 * file.
 */

#pragma once

// Standard library
#include <stdint.h>

// Drivers
#include "can_driver.h"

// ==== TX CAN IDs ====
// IDs of the messages the DIST sends.
// What each one carries is in its Send*() function in can_messages.c.

#define HEARTBEAT_ID                0x3A0U
#define FAULT_ID                    0x3A1U
#define LV_POWERUP_ACK_ID           0x3A2U
#define LV_CURRENTS_ID              0x3A3U
#define BRANCH_NUMBER_ID            0x3A4U
#define COMMIT_HASH_ID              0x3A5U
#define TEL_CURRENT_ID              0x3A6U

// ==== RX CAN IDs ====
// IDs of the messages from other boards that the DIST acts on, grouped by board.
// Each one must also be in CAN_RX_IDS (can_messages.c), or the RX filters drop it.

// --- HVC ---
#define HVC_FAULT_ID                0x321U
#define LV_POWERUP_REQUEST_ID       0x323U

// TX: CAN_Messages_SendAll() from FSM_Run(), CAN_Messages_SendLvPowerupAck() from EnableEfuses()
void CAN_Messages_SendAll(void);
void CAN_Messages_SendLvPowerupAck(void);

// RX: CAN_Messages_HandleRx() from the CAN driver's receive interrupt

// Every RX CAN ID above, the ones CAN_Messages_HandleRx() handles.
// DIST_Init() sets the RX filters from it.
// Update CAN_RX_ID_COUNT with the list: a wrong count does not compile.
#define CAN_RX_ID_COUNT 2U
extern const uint16_t CAN_RX_IDS[CAN_RX_ID_COUNT];

void CAN_Messages_HandleRx(const CAN_RxMessage_t *rx_message);
