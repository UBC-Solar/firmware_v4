#include "can_messages.h"
#include "can_driver.h"
#include "mst_defs.h"
#include "mst_main.h"
#include "stm32f1xx_hal.h"

void CAN_SendHeartbeatMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_STATUS_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 4;

    uint32_t status = 0;
    
    // Bits 0-4 Faults
    status |= (pack_state.error_comm_fail                   ? 1 : 0) << 0;
    status |= (pack_state.error_self_test                   ? 1 : 0) << 1;
    status |= (pack_faults.bits.fault_over_temperature      ? 1 : 0) << 2;
    status |= (pack_faults.bits.fault_under_voltage         ? 1 : 0) << 3;
    status |= (pack_faults.bits.fault_over_voltage          ? 1 : 0) << 4;
    status |= (pack_faults.bits.fault_under_temperature     ? 1 : 0) << 5;
    status |= (pack_state.balancing_active                  ? 1 : 0) << 6;
    status |= (pack_state.llim_enable                       ? 1 : 0) << 7;
    status |= (pack_state.hlim_enable                       ? 1 : 0) << 8;
    status |= (pack_warnings.bits.warn_low_voltage          ? 1 : 0) << 9;
    status |= (pack_warnings.bits.warn_high_voltage         ? 1 : 0) << 10;
    status |= (pack_warnings.bits.warn_high_temperature     ? 1 : 0) << 11;
    status |= (pack_state.balancing_enable                  ? 1 : 0) << 12;
    status |= (pack_state.scrutineering_enable              ? 1 : 0) << 13;

    msg.data[0] = status & 0xFF;
    msg.data[1] = (status >> 8) & 0xFF;
    msg.data[2] = (status >> 16) & 0xFF;
    msg.data[3] = (status >> 24) & 0xFF;

    uint32_t current_time = HAL_GetTick();
    msg.data[4] = current_time & 0xFF;
    msg.data[5] = (current_time >> 8) & 0xFF;
    msg.data[6] = (current_time >> 16) & 0xFF;
    msg.data[7] = (current_time >> 24) & 0xFF;
    

    CAN_QueueTxMessage(&msg);
}

void CAN_SendVoltageSummaryMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_MODULE_VOLT_SUMMARY_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 6;

    uint32_t total_voltage = 0;
    uint32_t min_voltage = 0xFFFFFFFF;
    uint32_t max_voltage = 0;
    uint8_t min_idx = 0;
    uint8_t max_idx = 0;

    for (int i = 0; i < NUM_MODULES; i++) {
        uint32_t v = pack_modules[i].voltage_mv;
        total_voltage += v;
        if (v < min_voltage) { min_voltage = v; min_idx = i; }
        if (v > max_voltage) { max_voltage = v; max_idx = i; }
    }

    msg.data[0] = total_voltage & 0xFF;
    msg.data[1] = (total_voltage >> 8) & 0xFF;
    msg.data[2] = min_idx;
    msg.data[3] = max_idx;
    msg.data[4] = 0;
    msg.data[5] = 0;

    CAN_QueueTxMessage(&msg);
}

void CAN_SendTempSummaryMessage(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = CAN_MODULE_TEMP_SUMMARY_ID;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 5;

    int32_t total_temp = 0;
    int32_t min_temp = 999000;
    int32_t max_temp = -999000;
    uint8_t min_idx = 0;
    uint8_t max_idx = 0;

    for (int i = 0; i < NUM_MODULES; i++) {
        int32_t t = pack_modules[i].temperature_mC;
        total_temp += t;
        if (t < min_temp) { min_temp = t; min_idx = i; }
        if (t > max_temp) { max_temp = t; max_idx = i; }
    }

    int32_t avg_temp_mC = total_temp / NUM_MODULES;
    int32_t avg_temp_C = avg_temp_mC / 1000;
    
    // Clamp to 8-bit signed integer range
    if (avg_temp_C > 127) avg_temp_C = 127;
    if (avg_temp_C < -127) avg_temp_C = -127;

    msg.data[0] = (uint8_t)(int8_t)avg_temp_C;
    msg.data[1] = min_idx;
    msg.data[2] = max_idx;
    msg.data[3] = 0;
    msg.data[4] = 0;

    CAN_QueueTxMessage(&msg);
}

void CAN_SendModuleVoltMessage(void) {
    // We have 8 multiplex groups, sending 4 module readouts per group
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = CAN_MODULE_VOLT_DATA_ID;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 5;

        msg.data[0] = mux_group & 0x0F;
        
        int start_idx = mux_group * 4;
        for (int i = 0; i < 4; i++) {
            int module_idx = start_idx + i;
            if (module_idx >= NUM_MODULES) {
                continue;
            }

            // Taking 8 MSB as specified by docs. voltage_mv is mV, 
            // wait, "take the 8 MSB of voltage values". E.g., (voltage_mv >> 8).
            msg.data[1 + i] = (pack_modules[module_idx].voltage_mv >> 8) & 0xFF;
        }
        CAN_QueueTxMessage(&msg);
    }
}

void CAN_SendModuleTempMessage(void) {
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = CAN_MODULE_TEMP_DATA_ID;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 5;

        msg.data[0] = mux_group & 0x07;
        
        int start_idx = mux_group * 4;
        for (int i = 0; i < 4; i++) {
            int module_idx = start_idx + i;
            if (module_idx < NUM_MODULES) {
                int32_t temp_C = pack_modules[module_idx].temperature_mC / 1000;
                
                // Clamp to 8-bit signed integer range
                if (temp_C > 127) temp_C = 127;
                if (temp_C < -127) temp_C = -127;
                
                int8_t temp_c_8 = (int8_t)temp_C;
                msg.data[1 + i] = (uint8_t)temp_c_8;
            } else {
                msg.data[1 + i] = 0;
            }
        }
        CAN_QueueTxMessage(&msg);
    }
}

void CAN_SendModuleStatusMessage(void) {
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = CAN_MODULE_STATUS_ID;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 5;

        msg.data[0] = mux_group & 0x07;
        
        int start_idx = mux_group * 4;
        for (int i = 0; i < 4; i++) {
            int module_idx = start_idx + i;
            if (module_idx < NUM_MODULES) {
                // Determine module status. There is no specific bit definition for "Module Status" 
                // in the file so filling with zeros/generic placeholders.
                uint8_t module_status = 0;
                msg.data[1 + i] = module_status;
            } else {
                msg.data[1 + i] = 0;
            }
        }
        CAN_QueueTxMessage(&msg);
    }
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
