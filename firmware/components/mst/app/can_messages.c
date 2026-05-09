#include "can_messages.h"
#include "can_driver.h"
#include "mst_main.h"

extern module_t pack_modules[NUM_MODULES];
extern faults_t pack_faults;
extern warnings_t pack_warnings;
extern pack_state_t pack_state;

void CAN_SendMessage0x622(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = 0x622;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 4;

    uint32_t payload = 0;
    
    // Bits 0-4 Faults
    payload |= (pack_state.error_comm_fail                     ? 1 : 0) << 0;
    payload |= (pack_state.error_self_test                     ? 1 : 0) << 1;
    payload |= (pack_faults.bits.fault_over_temperature             ? 1 : 0) << 2;
    payload |= (pack_faults.bits.fault_under_voltage                ? 1 : 0) << 3;
    payload |= (pack_faults.bits.fault_over_voltage                 ? 1 : 0) << 4;
    // Bit 5 is Isolation Loss Fault, not tracked here
    // Bit 6 is Voltage Out of Range (Short)
    payload |= (pack_faults.bits.fault_under_temperature            ? 1 : 0) << 7;
    payload |= (pack_state.balancing_active                    ? 1 : 0) << 8;
    payload |= (pack_state.llim_enable                         ? 1 : 0) << 9;
    payload |= (pack_state.hlim_enable                         ? 1 : 0) << 10;
    payload |= (pack_warnings.bits.trip_charge_over_temperature     ? 1 : 0) << 11;
    payload |= (pack_warnings.bits.warn_low_voltage                 ? 1 : 0) << 12;
    payload |= (pack_warnings.bits.warn_high_voltage                ? 1 : 0) << 13;
    payload |= (pack_warnings.bits.warn_discharge_high_temperature  ? 1 : 0) << 14;
    payload |= (pack_warnings.bits.warn_charge_high_temperature     ? 1 : 0) << 15;
    payload |= (pack_state.balancing_enable                    ? 1 : 0) << 16;
    payload |= (pack_state.scrutineering_enable                ? 1 : 0) << 17;

    msg.data[0] = payload & 0xFF;
    msg.data[1] = (payload >> 8) & 0xFF;
    msg.data[2] = (payload >> 16) & 0xFF;
    msg.data[3] = (payload >> 24) & 0xFF;

    CAN_QueueTxMessage(&msg);
}

void CAN_SendMessage0x623(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = 0x623;
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

void CAN_SendMessage0x625(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = 0x625;
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

void CAN_SendMessage0x626(void) {
    // We have 8 multiplex groups, sending 4 module readouts per group
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = 0x626;
        msg.tx_header.IDE = CAN_ID_STD;
        msg.tx_header.RTR = CAN_RTR_DATA;
        msg.tx_header.DLC = 5;

        msg.data[0] = mux_group & 0x07;
        
        int start_idx = mux_group * 4;
        for (int i = 0; i < 4; i++) {
            int module_idx = start_idx + i;
            if (module_idx < NUM_MODULES) {
                // Taking 8 MSB as specified by docs. voltage_mv is mV, 
                // wait, "take the 8 MSB of voltage values". E.g., (voltage_mv >> 8).
                msg.data[1 + i] = (pack_modules[module_idx].voltage_mv >> 8) & 0xFF;
            } else {
                msg.data[1 + i] = 0;
            }
        }
        CAN_QueueTxMessage(&msg);
    }
}

void CAN_SendMessage0x627(void) {
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = 0x627;
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

void CAN_SendMessage0x628(void) {
    for (int mux_group = 0; mux_group < 8; mux_group++) {
        CAN_TxMessage_t msg = {0};
        msg.tx_header.StdId = 0x628;
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

void CAN_SendMessage0x629(void) {
    CAN_TxMessage_t msg = {0};
    msg.tx_header.StdId = 0x629;
    msg.tx_header.IDE = CAN_ID_STD;
    msg.tx_header.RTR = CAN_RTR_DATA;
    msg.tx_header.DLC = 4;

    uint32_t payload = 0;
    
    for (int i = 0; i < NUM_MODULES; i++) {
        if (pack_modules[i].should_balance) {
            payload |= (1 << i);
        }
    }

    msg.data[0] = payload & 0xFF;
    msg.data[1] = (payload >> 8) & 0xFF;
    msg.data[2] = (payload >> 16) & 0xFF;
    msg.data[3] = (payload >> 24) & 0xFF;

    CAN_QueueTxMessage(&msg);
}
