/**
 * @file can_messages.c
 * @brief DIST CAN messages: builds and queues DIST TX messages, and handles received messages.
 *
 * TX MESSAGES fills each message the DIST sends (ID, length, data) and queues it on the CAN driver.
 * RX MESSAGES turns each received message into g_shared values and fault flags for the FSM.
 * The message IDs, and what else to update when a message changes, are in can_messages.h.
 */

#include "can_messages.h"

// Standard library
#include <stdbool.h>
#include <stdint.h>

// Common
#include "git_version.h"

// DIST app
#include "dist_defs.h"
#include "fsm.h"

// One function per TX message, called by CAN_Messages_SendAll()
static void SendHeartbeat(void);
static void SendFault(void);
static void SendCurrents(void);
static void SendTelCurrent(void);
static void SendBranchNumber(void);
static void SendCommitHash(void);

// Packing helpers
static uint8_t MilliampsTo5mA(int32_t current_ma);
static uint16_t MilliampsToU16(int32_t current_ma);

// ==== TX messages ====
// Every message the DIST sends.
// FSM_Run() calls CAN_Messages_SendAll() every pass, which sends the periodic ones when their
// interval is up.
// EnableEfuses() also sends the LV power-up acknowledge itself.
// Each Send*() function builds one message and queues it; the CAN driver sends it from the queue.

/**
 * @brief Sends every periodic DIST CAN message whose interval has elapsed.
 *
 * Heartbeat every HEARTBEAT_INTERVAL_MS.
 * LV power-up acknowledge every LV_POWERUP_ACK_INTERVAL_MS, in every state.
 * Currents every CURRENTS_TX_INTERVAL_MS.
 * Branch number and commit hash every BRANCH_NUMBER_INTERVAL_MS.
 * Fault every FAULT_TX_INTERVAL_MS, only in STATE_FAULT.
 * Call from FSM_Run() every pass.
 *
 * Side effects: updates the periodic-task timers in g_ticks.
 */
void CAN_Messages_SendAll(void) {
    if (FSM_TimerElapsed(HEARTBEAT_INTERVAL_MS, &g_ticks.heartbeat)) {
        SendHeartbeat();
    }

    // TODO: sent in every state, even before the HVC asks for LV power, so the HVC's wait for it in
    // STATE_LV_POWERUP passes straight away. Kept from the old firmware; decide if it should only
    // be sent once the eFuses are on.
    if (FSM_TimerElapsed(LV_POWERUP_ACK_INTERVAL_MS, &g_ticks.lv_powerup_ack)) {
        CAN_Messages_SendLvPowerupAck();
    }

    if (FSM_TimerElapsed(CURRENTS_TX_INTERVAL_MS, &g_ticks.currents_tx)) {
        SendCurrents();
        SendTelCurrent();
    }

    if (FSM_TimerElapsed(BRANCH_NUMBER_INTERVAL_MS, &g_ticks.branch_number)) {
        SendBranchNumber();
        SendCommitHash();
    }

    // The HVC faults on any FAULT_ID message, whatever its data, so only send it in STATE_FAULT
    if ((g_state == STATE_FAULT) && FSM_TimerElapsed(FAULT_TX_INTERVAL_MS, &g_ticks.fault_tx)) {
        SendFault();
    }
}

/**
 * @brief Sends the LV power-up acknowledge (LV_POWERUP_ACK_ID), data[0] = 0x01.
 *
 * Tells the HVC the LV boards are powered.
 */
void CAN_Messages_SendLvPowerupAck(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = LV_POWERUP_ACK_ID;
    tx_message.tx_header.DLC = 1;

    tx_message.data[0] = 0x01U;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the DIST heartbeat (HEARTBEAT_ID) so other boards can tell the DIST is alive.
 *
 * Contains a counter that increases by 1 on every send, MSB first.
 */
static void SendHeartbeat(void) {
    static uint32_t heartbeat_count = 0U;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = HEARTBEAT_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (heartbeat_count >> 24) & 0xFF;
    tx_message.data[1] = (heartbeat_count >> 16) & 0xFF;
    tx_message.data[2] = (heartbeat_count >> 8)  & 0xFF;
    tx_message.data[3] = (heartbeat_count)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
    heartbeat_count++;
}

/**
 * @brief Sends the DIST fault message (FAULT_ID), data[0] = 0x01.
 */
static void SendFault(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = FAULT_ID;
    tx_message.tx_header.DLC = 1;

    tx_message.data[0] = 0x01U;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the five eFuse load currents (LV_CURRENTS_ID), one byte each.
 *
 * Each byte is the current in 5 mA steps, so 0-1275 mA (see MilliampsTo5mA()).
 * Order: DRD, MDI, SPARE_CTRL, SPARE_MUX, SPARE.
 * Currents are from g_shared.adc (converted in adc_runtime.c).
 */
static void SendCurrents(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = LV_CURRENTS_ID;
    tx_message.tx_header.DLC = 5;

    tx_message.data[0] = MilliampsTo5mA(g_shared.adc.drd_ma);
    tx_message.data[1] = MilliampsTo5mA(g_shared.adc.mdi_ma);
    tx_message.data[2] = MilliampsTo5mA(g_shared.adc.spare_ctrl_ma);
    tx_message.data[3] = MilliampsTo5mA(g_shared.adc.spare_mux_ma);
    tx_message.data[4] = MilliampsTo5mA(g_shared.adc.spare_ma);

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the TEL rail current (TEL_CURRENT_ID), in mA, MSB first.
 */
static void SendTelCurrent(void) {
    uint16_t current_ma = MilliampsToU16(g_shared.adc.tel_ma);
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = TEL_CURRENT_ID;
    tx_message.tx_header.DLC = 2;

    tx_message.data[0] = (current_ma >> 8) & 0xFF;
    tx_message.data[1] = (current_ma)      & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the firmware branch number (BRANCH_NUMBER_ID), data[0] = DIST_BRANCH_NUMBER.
 */
static void SendBranchNumber(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = BRANCH_NUMBER_ID;
    tx_message.tx_header.DLC = 1;

    tx_message.data[0] = DIST_BRANCH_NUMBER;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the git commit hash of this build (COMMIT_HASH_ID).
 *
 * Lets you confirm over CAN which firmware is flashed.
 * Data is packed by GitVersionPackCan().
 */
static void SendCommitHash(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = COMMIT_HASH_ID;
    tx_message.tx_header.DLC = GIT_VERSION_CAN_DLC;

    GitVersionPackCan(tx_message.data);

    CAN_QueueTxMessage(&tx_message);
}

// --- Packing helpers ---

/**
 * @brief Packs a current into one byte of 5 mA steps, clamped to 0-255 (0-1275 mA).
 *
 * A slightly negative reading (sensor offset) packs as 0.
 *
 * @param current_ma Current in mA.
 * @return Current / 5 mA, clamped to a byte.
 */
static uint8_t MilliampsTo5mA(int32_t current_ma) {
    int32_t steps = current_ma / 5;

    if (steps < 0) {
        return 0U;
    }
    if (steps > UINT8_MAX) {
        return UINT8_MAX;
    }
    return (uint8_t)steps;
}

/**
 * @brief Clamps a current to 0-65535 mA for a 16-bit CAN field.
 *
 * A slightly negative reading (sensor offset) packs as 0.
 *
 * @param current_ma Current in mA.
 * @return The current, clamped to 16 bits.
 */
static uint16_t MilliampsToU16(int32_t current_ma) {
    if (current_ma < 0) {
        return 0U;
    }
    if (current_ma > UINT16_MAX) {
        return UINT16_MAX;
    }
    return (uint16_t)current_ma;
}

// ==== RX messages ====
// What the DIST does with each message it receives.
// The CAN driver calls CAN_Messages_HandleRx() from the CAN receive interrupt, so it only writes
// g_shared and g_fault_flags; the FSM acts on them in the main loop.
// CAN_RX_IDS lists the IDs the RX filters let in.

// Every ID CAN_Messages_HandleRx() handles, in RX CAN IDS order.
// Keep the two in sync: an ID missing here is dropped by the RX filters, so the handler never
// sees it.
const uint16_t CAN_RX_IDS[] = {
    HVC_FAULT_ID,
    LV_POWERUP_REQUEST_ID,
};

/**
 * @brief Acts on a received CAN message based on its ID.
 *
 * Called by the CAN driver from the RX FIFO interrupt, after the frame has been read from hardware.
 *
 * Side effects: may set g_shared.lv_powerup_received or g_fault_flags.hvc_fault.
 *
 * @param rx_message The received message.
 */
void CAN_Messages_HandleRx(const CAN_RxMessage_t *rx_message) {
    // HAL only fills StdId for standard frames; the DIST receives no extended frames
    if (rx_message->rx_header.IDE != CAN_ID_STD) {
        return;
    }

    switch (rx_message->rx_header.StdId) {
        case HVC_FAULT_ID:
            // Any non-zero byte means the HVC has a fault
            for (uint32_t i = 0U; i < rx_message->rx_header.DLC; i++) {
                if (rx_message->data[i] != 0U) {
                    g_fault_flags.hvc_fault = true;
                    break;
                }
            }
            break;

        case LV_POWERUP_REQUEST_ID:
            // Bit 0 set: the HVC asks the DIST to power the LV boards
            if (rx_message->data[0] & 0x01U) {
                g_shared.lv_powerup_received = true;
            }
            break;

        default:
            break;
    }
}
