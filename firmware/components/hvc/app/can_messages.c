/**
 * @file can_messages.c
 * @brief HVC CAN messages: builds and queues HVC TX messages, and handles received messages.
 *
 * TX MESSAGES fills each message the HVC sends (ID, length, data) and queues it on the CAN driver.
 * RX MESSAGES turns each received message into g_shared values and fault flags for the FSM.
 * The message IDs, and what else to update when a message changes, are in can_messages.h.
 */

#include "can_messages.h"

// Standard library
#include <stdbool.h>
#include <stdint.h>

// Common
#include "git_version.h"

// Runtime
#include "shunt_runtime.h"

// HVC app
#include "debug_output.h"
#include "fsm.h"
#include "hvc_defs.h"

// One function per TX message, called by CAN_Messages_SendAll()
static void SendHeartbeat(void);
static void SendCommitHash(void);
static void SendState(void);
static void SendFaults(void);
static void SendShuntCurrent(void);
static void SendLvCurrent(void);
static void SendSuppVoltage(void);
static void SendDcdcTemp(void);
static void SendMotorPrecharge(void);
static void SendMpptPrecharge(void);

// ==== TX messages ====
// Every message the HVC sends.
// FSM_Run() calls CAN_Messages_SendAll() every pass, which sends the periodic ones when their
// interval is up.
// LvPowerup() sends the LV power-up request itself.
// Each Send*() function builds one message and queues it; the CAN driver sends it from the queue.

/**
 * @brief Sends every periodic HVC CAN message whose interval has elapsed.
 *
 * Heartbeat and commit hash every HVC_HEARTBEAT_INTERVAL_MS.
 * State, faults, and measurements every CAN_TX_INTERVAL_MS.
 * Call from FSM_Run() every pass.
 *
 * Side effects: updates g_ticks.hvc_heartbeat and g_ticks.can_tx.
 */
void CAN_Messages_SendAll(void) {
    if (FSM_TimerElapsed(HVC_HEARTBEAT_INTERVAL_MS, &g_ticks.hvc_heartbeat)) {
        SendHeartbeat();
        SendCommitHash();
    }

    if (FSM_TimerElapsed(CAN_TX_INTERVAL_MS, &g_ticks.can_tx)) {
        SendState();
        SendFaults();
        SendShuntCurrent();
        SendLvCurrent();
        SendSuppVoltage();
        SendDcdcTemp();
        SendMotorPrecharge();
        SendMpptPrecharge();
    }
}

/**
 * @brief Sends the HVC heartbeat (HEARTBEAT_ID) so other boards can tell the HVC is alive.
 *
 * Contains a counter that increases by 1 on every send.
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

/**
 * @brief Sends the current FSM state (STATE_ID).
 */
static void SendState(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = STATE_ID;
    tx_message.tx_header.DLC = 1;

    tx_message.data[0] = (uint8_t)g_state;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends every fault flag (FAULT_ID), one bit per flag.
 *
 * Bits follow FaultFlags (hvc_types.h) in order, starting at data[0] bit 7.
 * data[2] bits 1-0 are free.
 */
static void SendFaults(void) {
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = FAULT_ID;
    tx_message.tx_header.DLC = 3;

    tx_message.data[0] = (g_fault_flags.estop << 7) |
                        (g_fault_flags.imd_fault << 6) |
                        (g_fault_flags.mst_fault << 5) |
                        (g_fault_flags.mst_unhealthy << 4) |
                        (g_fault_flags.overcurrent << 3) |
                        (g_fault_flags.reverse_current << 2) |
                        (g_fault_flags.dist_fault << 1) |
                        (g_fault_flags.dcdc_fault << 0);

    tx_message.data[1] = (g_fault_flags.supp_fault << 7) |
                        (g_fault_flags.watchdog_fired << 6) |
                        (g_fault_flags.dcdc_overtemp << 5) |
                        (g_fault_flags.tel_startup_timeout << 4) |
                        (g_fault_flags.mst_handshake_timeout << 3) |
                        (g_fault_flags.mst_status_check_timeout << 2) |
                        (g_fault_flags.hv_connect_timeout << 1) |
                        (g_fault_flags.open_discharge_relay_timeout << 0);

    tx_message.data[2] = (g_fault_flags.motor_precharge_timeout << 7) |
                        (g_fault_flags.mppt_precharge_timeout << 6) |
                        (g_fault_flags.lv_powerup_timeout << 5) |
                        (g_fault_flags.tel_heartbeat_timeout << 4) |
                        (g_fault_flags.mst_heartbeat_timeout << 3) |
                        (g_fault_flags.dist_heartbeat_timeout << 2);

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the HV current measured by the shunt (SHUNT_CURRENT_ID).
 *
 * Sends the last finished reading.
 * The new read finishes later in the I2C interrupt, so the value sent is from the previous call.
 * Current is in mA; negative means reverse current (charging the pack).
 *
 * Side effects: starts a new shunt read over I2C.
 */
static void SendShuntCurrent(void) {
    SHUNT_ReadShuntVoltage(); // Starts a new shunt read
    int32_t current_ma = SHUNT_GetShuntCurrent_mA(); // Most recently calculated current
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = SHUNT_CURRENT_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (current_ma >> 24) & 0xFF;
    tx_message.data[1] = (current_ma >> 16) & 0xFF;
    tx_message.data[2] = (current_ma >> 8)  & 0xFF;
    tx_message.data[3] = (current_ma)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the LV current (LV_CURRENT_ID).
 *
 * Current is in mA, from g_shared.adc (converted in adc_runtime).
 */
static void SendLvCurrent(void) {
    int16_t lv_current_ma = (int16_t)g_shared.adc.lv_current_ma;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = LV_CURRENT_ID;
    tx_message.tx_header.DLC = 2;

    tx_message.data[0] = (lv_current_ma >> 8) & 0xFF;
    tx_message.data[1] = (lv_current_ma)      & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the supplemental battery voltage (SUPP_VOLTAGE_ID).
 *
 * Voltage is in mV, from g_shared.adc (converted in adc_runtime).
 */
static void SendSuppVoltage(void) {
    uint32_t supp_mv = (uint32_t)g_shared.adc.supp_mv;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = SUPP_VOLTAGE_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (supp_mv >> 24) & 0xFF;
    tx_message.data[1] = (supp_mv >> 16) & 0xFF;
    tx_message.data[2] = (supp_mv >> 8)  & 0xFF;
    tx_message.data[3] = (supp_mv)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the DCDC thermistor temperature (DCDC_TEMP_ID).
 *
 * Temperature is in milli-degrees C, from g_shared.adc (converted in adc_runtime).
 */
static void SendDcdcTemp(void) {
    int32_t dcdc_temp_mc = g_shared.adc.dcdc_temp_mc;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = DCDC_TEMP_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (dcdc_temp_mc >> 24) & 0xFF;
    tx_message.data[1] = (dcdc_temp_mc >> 16) & 0xFF;
    tx_message.data[2] = (dcdc_temp_mc >> 8)  & 0xFF;
    tx_message.data[3] = (dcdc_temp_mc)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the motor precharge voltage (MOTOR_PRECHARGE_ID).
 *
 * HV-side voltage in mV, from g_shared.adc (converted in adc_runtime).
 */
static void SendMotorPrecharge(void) {
    int32_t motor_precharge_mv = g_shared.adc.motor_precharge_mv;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = MOTOR_PRECHARGE_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (motor_precharge_mv >> 24) & 0xFF;
    tx_message.data[1] = (motor_precharge_mv >> 16) & 0xFF;
    tx_message.data[2] = (motor_precharge_mv >> 8)  & 0xFF;
    tx_message.data[3] = (motor_precharge_mv)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the MPPT precharge voltage (MPPT_PRECHARGE_ID).
 *
 * HV-side voltage in mV, from g_shared.adc (converted in adc_runtime).
 */
static void SendMpptPrecharge(void) {
    int32_t mppt_precharge_mv = g_shared.adc.mppt_precharge_mv;
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = MPPT_PRECHARGE_ID;
    tx_message.tx_header.DLC = 4;

    tx_message.data[0] = (mppt_precharge_mv >> 24) & 0xFF;
    tx_message.data[1] = (mppt_precharge_mv >> 16) & 0xFF;
    tx_message.data[2] = (mppt_precharge_mv >> 8)  & 0xFF;
    tx_message.data[3] = (mppt_precharge_mv)       & 0xFF;

    CAN_QueueTxMessage(&tx_message);
}

/**
 * @brief Sends the LV power-up request (LV_POWERUP_REQUEST_ID).
 *
 * LvPowerup() resends this every LV_POWERUP_RETRY_MS until LV_POWERUP_ACK_ID comes back.
 */
void CAN_Messages_SendLvPowerupRequest(void) {
    static uint32_t request_count = 0U; // Requests sent so far, for the debug output
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = LV_POWERUP_REQUEST_ID;
    tx_message.tx_header.DLC = 1;

    tx_message.data[0] = 1U;

    CAN_QueueTxMessage(&tx_message);
    request_count++;
    PRINT_CAN("LV power-up request sent (try %lu)", (unsigned long)request_count);
}

// --- Template ---

/**
 * @brief Template for a new TX message. Copy it, then set the ID, DLC, and data.
 */
void CAN_Messages_SendMessageTemplate(void) {
    CAN_TxMessage_t tx_message = {0};

    // Note: replace with actual values
    tx_message.tx_header.StdId = 0x0U;
    tx_message.tx_header.DLC = 0;

    // Note: modify tx_message.data

    CAN_QueueTxMessage(&tx_message);
}

// ==== RX messages ====
// What the HVC does with each message it receives.
// The CAN driver calls CAN_Messages_HandleRx() from the CAN receive interrupt, so it only writes
// g_shared and g_fault_flags; the FSM acts on them in the main loop.
// CAN_RX_IDS lists the IDs the RX filters let in.

// Every ID CAN_Messages_HandleRx() handles, in RX CAN IDS order.
// Keep the two in sync: an ID missing here is dropped by the RX filters, so the handler never
// sees it.
const uint16_t CAN_RX_IDS[] = {
    TEL_HEARTBEAT_ID,
    DIST_HEARTBEAT_ID,
    DIST_FAULT_ID,
    LV_POWERUP_ACK_ID,
    MST_HEARTBEAT_ID,
    MST_VOLT_SUMMARY_ID,
};

/**
 * @brief Acts on a received CAN message based on its ID.
 *
 * Called by the CAN driver from the RX FIFO interrupt, after the frame has been read from hardware.
 * Also called by the faked boards in tests.c (main loop).
 *
 * Side effects: writes g_shared (received flags, MST status, pack voltage) and g_fault_flags.
 *
 * @param rx_message The received message.
 */
void CAN_Messages_HandleRx(const CAN_RxMessage_t *rx_message) {
    switch (rx_message->rx_header.StdId) {
        case TEL_HEARTBEAT_ID:
            g_shared.tel_heartbeat_received = true;
            break;
        case DIST_HEARTBEAT_ID:
            g_shared.dist_heartbeat_received = true;
            break;
        case DIST_FAULT_ID:
            g_fault_flags.dist_fault = true;
            break;
        case LV_POWERUP_ACK_ID:
            g_shared.lv_powerup_received = true;
            break;
        case MST_HEARTBEAT_ID:
            g_shared.mst_heartbeat_received = true;
            g_shared.mst_healthy =
                (rx_message->data[4] == 0) &&
                ((rx_message->data[5] & 1) == 0);
            break;
        case MST_VOLT_SUMMARY_ID:
            g_shared.mst_pack_voltage_mv = (int32_t)(
                (uint32_t)rx_message->data[0] |
                (uint32_t)rx_message->data[1] << 8 |
                (uint32_t)rx_message->data[2] << 16 |
                (uint32_t)rx_message->data[3] << 24);
            break;
        default:
            break;
    }
}
