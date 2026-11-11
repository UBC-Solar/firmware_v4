/**
 * @file can_messages.h
 * @brief HVC CAN message IDs and the functions that build, send, and handle HVC CAN messages.
 *
 * Sits on top of can_driver.h: the driver owns the CAN peripheral and TX queue, this module
 * owns what each HVC message contains and what each received message means.
 *
 * Changing any CAN message (an ID, a layout, or what a value means, like the FsmState or
 * FaultFlags order) also means updating the DBC file and the BMS telemetry dashboard.
 */

#pragma once

// Standard library
#include <stdint.h>

// Drivers
#include "can_driver.h"

/*==============================================================================
 * === TX CAN IDS ==============================================================
 * IDs of the messages the HVC sends. What each one carries is in its Send*()
 * function in can_messages.c.
 *============================================================================*/

#define SHUNT_CURRENT_ID            0x320U
#define FAULT_ID                    0x321U
#define HEARTBEAT_ID                0x322U
#define LV_POWERUP_REQUEST_ID       0x323U
#define STATE_ID                    0x324U
#define LV_CURRENT_ID               0x325U
#define DCDC_TEMP_ID                0x326U
#define SUPP_VOLTAGE_ID             0x327U
#define MOTOR_PRECHARGE_ID          0x328U
#define MPPT_PRECHARGE_ID           0x329U
#define COMMIT_HASH_ID              0x32AU

/*==============================================================================
 * === RX CAN IDS ==============================================================
 * IDs of the messages from other boards that the HVC acts on, grouped by board.
 * Each one must also be in CAN_RX_IDS (can_messages.c), or the RX filters drop
 * it.
 *============================================================================*/

// --- TEL ---
#define TEL_HEARTBEAT_ID            0x750U

// --- DIST ---
#define DIST_HEARTBEAT_ID           0x3A0U
#define DIST_FAULT_ID               0x3A1U
#define LV_POWERUP_ACK_ID           0x3A2U

// --- MST ---
#define MST_HEARTBEAT_ID            0x200U
#define MST_VOLT_SUMMARY_ID         0x201U

/*==============================================================================
 * === TX MESSAGES =============================================================
 * Implemented in can_messages.c. Entry points: CanMessagesSendAll() (from
 * FsmRun()) and CanMessagesSendLvPowerupRequest() (from LvPowerup()).
 *============================================================================*/

void CanMessagesSendAll(void);
void SendHeartbeat(void);
void SendCommitHash(void);
void SendState(void);
void SendFaults(void);
void SendShuntCurrent(void);
void SendLvCurrent(void);
void SendSuppVoltage(void);
void SendDcdcTemp(void);
void SendMotorPrecharge(void);
void SendMpptPrecharge(void);
void CanMessagesSendLvPowerupRequest(void);

// --- Template ---
void SendMessageTemplate(void);

/*==============================================================================
 * === RX MESSAGES =============================================================
 * Implemented in can_messages.c. Entry point: CanMessagesHandleRx() (from the
 * CAN driver's receive interrupt).
 *============================================================================*/

// Every RX CAN ID above, the ones CanMessagesHandleRx() handles. HVC_Init() sets the RX filters
// from it. Update CAN_RX_ID_COUNT with the list: a wrong count does not compile.
#define CAN_RX_ID_COUNT 6U
extern const uint16_t CAN_RX_IDS[CAN_RX_ID_COUNT];

void CanMessagesHandleRx(const CAN_RxMessage_t* rx_message);
