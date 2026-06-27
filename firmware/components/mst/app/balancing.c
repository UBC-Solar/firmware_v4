#include "balancing.h"
#include "module_data.h"
#include "mst_defs.h"
#include "mst_types.h"
#include "spi_driver.h"


void SetBalancingForModuleGroup_(
    pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES],
    int slave_num, int reg_num, int val_offset, int module_start_idx) 
{
    uint8_t module_balance_enables = 0;
    pack_state->balancing_active = false; // clear status before accumulating balancing status of each module

    for (int module_offset = 0; module_offset < SLAVE_NUM_MODULES_PER_BAL_VAL; module_offset++)
    {
        int module_idx = module_start_idx + module_offset;

        // Absolute difference in mV of this module's voltage vs the average pack voltage
        uint32_t voltage_diff_mv = (pack_modules[module_idx].voltage_mv > pack_state->avg_voltage_mV)
            ? (pack_modules[module_idx].voltage_mv - pack_state->avg_voltage_mV)
            : (pack_state->avg_voltage_mV - pack_modules[module_idx].voltage_mv);
        
        // Store status in pack data structs and balancing mask for ADBMS
        bool if_balance = 
            pack_state->balancing_enable && 
            MIN_BALANCE_VOLT_DIFF_MV <= voltage_diff_mv && 
            voltage_diff_mv <= MAX_BALANCE_VOLT_DIFF_MV;
        pack_modules[module_idx].is_balancing = if_balance;
        pack_state->balancing_active |= if_balance;
        module_balance_enables |= if_balance < module_offset;
    }

    // Find the corresponding "DCC" (discharge cell) bits inside ADBMS' config registers, and set them
    int reg_offset = val_offset / 2;
    int reg_bitshift_bits = (val_offset % 2) * SLAVE_NUM_MODULES_PER_BAL_VAL;
    uint8_t mask = (uint8_t)(module_balance_enables << reg_bitshift_bits);
    if (mask) {
        slaves[slave_num].config_regs[reg_num][reg_offset] |= mask;
    }
    else {
        slaves[slave_num].config_regs[reg_num][reg_offset] &= (uint8_t)(~mask);
    }
}

void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]) {
    if (!pack_state->balancing_enable) {
        pack_state->balancing_active = false;
        return;
    }

    // Note from module_data's bal_mappings initialization that each "value" here is 4 bits. So, each register holds 
    // 6(bytes) * 2(value-per-byte) = 12(values)
    // Here we iterate through all available "values".
    for (int dev_num = 0; dev_num < SLAVE_NUM_DEVICES; dev_num++) {
        for (int reg_num = 0; reg_num < SLAVE_NUM_BAL_REG; reg_num++) {
            for (int val_offset = 0; val_offset < SLAVE_NUM_VAL_PER_BAL_REG; val_offset++) {

                int module_start_idx = slaves[dev_num].bal_mappings[reg_num][val_offset];
                if (module_start_idx < 0 || module_start_idx >= NUM_MODULES) {
                    continue;
                }

                SetBalancingForModuleGroup_(
                    pack_state, pack_modules, slaves,
                    dev_num, reg_num, val_offset, module_start_idx);
            }
        }
    }

    WriteConfigRegisters(slaves);
}

void PauseAllBalancing() {
    Slave_WakeUp();
    Slave_SendCmd(CMD_MUTE);
}

void ResumeAllBalancing() {
    Slave_WakeUp();
    Slave_SendCmd(CMD_UNMUTE);
}

#if (INT_TEST_SLAVE == RUN)
void Debug_DoBalancing(slave_t slaves[SLAVE_NUM_DEVICES], bool enable) {

    for (int device_num = 0; device_num < SLAVE_NUM_DEVICES; device_num++) {
        if (enable) {
            slaves[device_num].config_regs[0][4] = 0xFF;
            slaves[device_num].config_regs[0][5] = 0x0F;
            slaves[device_num].config_regs[1][0] |= 0xF0;
        }
        else {
            slaves[device_num].config_regs[0][4] = 0x00;
            slaves[device_num].config_regs[0][5] = 0x00;
            slaves[device_num].config_regs[1][0] &= (uint8_t)(~0xF0);
        }
    }
    WriteConfigRegisters(slaves);
}
#endif // UNIT_TEST_ISOSPI
