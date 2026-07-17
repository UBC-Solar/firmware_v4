#include "can_messages.h"
#include "can_driver.h"
#include "mst_defs.h"
#include "mst_main.h"
#include "stm32f1xx_hal.h"


void CAN_SendHeartbeatMessage(void) {
    static uint32_t last_heartbeat_time_ms = 0;
    static uint32_t current_heartbeat = 0;

    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_STATUS_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 8;

    uint32_t status = 0;

    if (HAL_GetTick() - last_heartbeat_time_ms < CAN_STATUS_PERIOD_MS) {
        return;
    }
    last_heartbeat_time_ms = HAL_GetTick();

    status |= (pack_state.error_comm_fail                   ? 1 : 0) << 0;
    status |= (pack_state.error_self_test                   ? 1 : 0) << 1;
    status |= (pack_faults.bits.fault_over_temperature      ? 1 : 0) << 2;
    status |= (pack_faults.bits.fault_under_voltage         ? 1 : 0) << 3;
    status |= (pack_faults.bits.fault_over_voltage          ? 1 : 0) << 4;
    status |= (pack_faults.bits.fault_under_temperature     ? 1 : 0) << 5;
    status |= (pack_warnings.bits.warn_low_voltage          ? 1 : 0) << 6;
    status |= (pack_warnings.bits.warn_high_voltage         ? 1 : 0) << 7;
    status |= (pack_warnings.bits.warn_high_temperature     ? 1 : 0) << 8;
    status |= (pack_state.balancing_active                  ? 1 : 0) << 9;
    status |= (pack_state.llim_enable                       ? 1 : 0) << 10;
    status |= (pack_state.hlim_enable                       ? 1 : 0) << 11;
    status |= (pack_state.balancing_enable                  ? 1 : 0) << 12;
    status |= (pack_state.scrutineering_enable              ? 1 : 0) << 13;

    msg.data[0] = current_heartbeat & 0xFF;
    msg.data[1] = (current_heartbeat >> 8) & 0xFF;
    msg.data[2] = (current_heartbeat >> 16) & 0xFF;
    msg.data[3] = (current_heartbeat >> 24) & 0xFF;
    msg.data[4] = status & 0xFF;
    msg.data[5] = (status >> 8) & 0xFF;
    msg.data[6] = (status >> 16) & 0xFF;
    msg.data[7] = (status >> 24) & 0xFF;
    
    CAN_QueueTxMessage(&msg);
    current_heartbeat++;
}
void CAN_SendVoltageSummaryMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_MODULE_VOLT_SUMMARY_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 8;

    uint32_t total_voltage = pack_state.total_voltage_mV;

    msg.data[0] = (total_voltage)       & 0xFF;
    msg.data[1] = (total_voltage >> 8)  & 0xFF;
    msg.data[2] = (total_voltage >> 16) & 0xFF;
    msg.data[3] = (total_voltage >> 24) & 0xFF;
    msg.data[4] = pack_state.min_voltage_idx+1;
    msg.data[5] = pack_state.max_voltage_idx+1;
    msg.data[6] = 0;
    msg.data[7] = 0;

    CAN_QueueTxMessage(&msg);
}

void CAN_SendTempSummaryMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_MODULE_TEMP_SUMMARY_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 4;

    int16_t avg_temp_c = (int16_t) (pack_state.avg_temp_mC / 1000);

    msg.data[0] = (uint8_t)avg_temp_c & 0xFF;
    msg.data[1] = (uint8_t)avg_temp_c >> 8;
    msg.data[2] = pack_state.min_temp_idx+1;
    msg.data[3] = pack_state.max_temp_idx+1;

    CAN_QueueTxMessage(&msg);
}

void SendModuleVoltMessage_(uint8_t group_idx) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = CAN_MODULE_VOLT_DATA_ID_START + group_idx;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 8;
        
        int start_idx = group_idx * CAN_NUM_MODULES_PER_DATA_GROUP;
        for (int i = 0; i < CAN_NUM_MODULES_PER_DATA_GROUP; i++) {
            int module_idx = start_idx + i;
            if (module_idx >= NUM_MODULES) {
                continue;
            }

            // Note: max value representable with 16 bits is 65536.
            // If the unit of this value is mV, then we can report a max of 65V.
            // This should be sufficient for cell voltages.
            uint16_t volt_mv_16;
            if (pack_modules[module_idx].voltage_mv >= 0xFFFF) {
                // Clamp value just in case
                volt_mv_16 = 0xFFFF;
            }
            else {
                volt_mv_16 = (uint16_t) pack_modules[module_idx].voltage_mv;
            }

            msg.data[i*2]   = volt_mv_16 & 0xFF;
            msg.data[i*2+1] = volt_mv_16 >> 8;
        }
        CAN_QueueTxMessage(&msg);
}

void CAN_SendModuleVoltMessage(void) {
    // We have 8 multiplex groups, sending 4 module readouts per group
    #if CAN_STRATEGY_ALL_AT_ONCE
    for (int group_idx = 0; group_idx < CAN_NUM_DATA_GROUPS; group_idx++) {
        SendModuleVoltMessage_(group_idx);
    }
    #else// CAN_STRATEGY_ALL_AT_ONCE is false
    static uint8_t current_volt_group_idx = 0;
    SendModuleVoltMessage_(current_volt_group_idx);
    current_volt_group_idx = (current_volt_group_idx + 1U) % CAN_NUM_DATA_GROUPS;
    #endif // CAN_STRATEGY_ALL_AT_ONCE
}

void SendModuleTempMessage_(uint8_t group_idx) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = CAN_MODULE_TEMP_DATA_ID_START + group_idx;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 8;
        
        int start_idx = group_idx * CAN_NUM_MODULES_PER_DATA_GROUP;
        for (int i = 0; i < CAN_NUM_MODULES_PER_DATA_GROUP; i++) {
            int module_idx = start_idx + i;
            if (module_idx >= NUM_MODULES) {
                continue;
            }

            int16_t temp_c_16 = (int16_t) (pack_modules[module_idx].temperature_mC / 1000);
            
            msg.data[i*2]   = (uint8_t)temp_c_16 & 0xFF;
            msg.data[i*2+1] = (uint8_t)temp_c_16 >> 8;
        }
        CAN_QueueTxMessage(&msg);
}

void CAN_SendModuleTempMessage(void) {
    // We have 8 multiplex groups, sending 4 module readouts per group

    #if CAN_STRATEGY_ALL_AT_ONCE
    for (int group_idx = 0; group_idx < CAN_NUM_DATA_GROUPS; group_idx++) {
        SendModuleTempMessage_(group_idx);
    }
    #else// CAN_STRATEGY_ALL_AT_ONCE is false
    static uint8_t current_temp_group_idx = 0;
    SendModuleTempMessage_(current_temp_group_idx);
    current_temp_group_idx = (current_temp_group_idx + 1U) % CAN_NUM_DATA_GROUPS;
    #endif // CAN_STRATEGY_ALL_AT_ONCE
}

void SendModuleStatusMessage_(uint8_t group_idx) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_MODULE_STATUS_ID_START + group_idx;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 8;
    
    int start_idx = group_idx * CAN_NUM_MODULES_PER_STATS_GROUP;
    for (int i = 0; i < CAN_NUM_MODULES_PER_STATS_GROUP; i++) {
        int module_idx = start_idx + i;
        if (module_idx >= NUM_MODULES) {
            continue;
        }
        msg.data[i] = 
            ((pack_modules[module_idx].faults.raw & 0x0F) << 4) |
            (pack_modules[module_idx].warnings.raw & 0x07);
    }
    CAN_QueueTxMessage(&msg);
}

void CAN_SendModuleStatusMessage(void) {
    #if CAN_STRATEGY_ALL_AT_ONCE
    for (int group_idx = 0; group_idx < CAN_NUM_STATS_GROUPS; group_idx++) {
        SendModuleStatusMessage_(group_idx);
    }
    #else // CAN_STRATEGY_ALL_AT_ONCE is false
    static uint8_t current_status_group_idx = 0;
    SendModuleStatusMessage_(current_status_group_idx);
    current_status_group_idx = (current_status_group_idx + 1U) % CAN_NUM_STATS_GROUPS;
    #endif // CAN_STRATEGY_ALL_AT_ONCE
}

void CAN_SendBalanceStatusMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_BALANCE_DATA_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 4;

    uint32_t payload = 0;
    
    for (int i = 0; i < NUM_MODULES; i++) {
        if (pack_modules[i].is_balancing) {
            payload |= (1 << i);
        }
    }

    msg.data[0] = payload & 0xFF;
    msg.data[1] = (payload >> 8) & 0xFF;
    msg.data[2] = (payload >> 16) & 0xFF;
    msg.data[3] = (payload >> 24) & 0xFF;

    CAN_QueueTxMessage(&msg);
}
