#include "balancing.h"
#include "logging.h"

#include "module_data.h"
#include "mst_defs.h"
#include "mst_main.h"
#include "mst_types.h"
#include "spi_driver.h"


void SetBalancingForModuleGroup_(
    pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES],
    int slave_num, int reg_num, int val_offset, int module_start_idx) 
{
    uint8_t module_balance_enables = 0;

    for (int module_offset = 0; module_offset < SLAVE_NUM_MODULES_PER_BAL_VAL; module_offset++)
    {
        int module_idx = module_start_idx + module_offset;

        // Absolute difference in mV of this module's voltage vs the minimum voltage in the pack
        if (pack_modules[module_idx].voltage_mv < pack_state->min_voltage_mV) {
            ERROR_HANDLER_LOGGED();
        }
        uint32_t voltage_diff_mv = pack_modules[module_idx].voltage_mv - pack_state->min_voltage_mV;

        bool voltage_in_balancing_range = MIN_BALANCE_VOLT_DIFF_MV <= voltage_diff_mv && voltage_diff_mv <= MAX_BALANCE_VOLT_DIFF_MV;
        bool is_safe_to_balance = !(pack_faults.raw) && !(pack_warnings.bits.warn_low_voltage) && !(pack_warnings.bits.warn_high_temperature);

        // Store status in pack data structs and balancing mask for ADBMS
        bool if_balance = pack_state->balancing_enable && voltage_in_balancing_range && is_safe_to_balance;

        pack_modules[module_idx].is_balancing = if_balance;
        pack_state->balancing_active |= if_balance;
        module_balance_enables |= if_balance << module_offset;
    }

    // Find the corresponding "DCC" (discharge cell) bits inside ADBMS' config registers, and set them.
    // We store two 4-bit DCC values per register (of size 8 bits), hence needing reg_bitshift_bits to address only those 4 bits
    // There are 6 registers in a group, hence the reg_offset
    int reg_offset = val_offset / 2;
    int reg_bitshift_bits = (val_offset % 2) * SLAVE_NUM_MODULES_PER_BAL_VAL;
    uint8_t mask = (uint8_t)(module_balance_enables << reg_bitshift_bits);
    // Clear the bits...
    slaves[slave_num].config_regs[reg_num][reg_offset] &= (uint8_t)(~(0x0F << reg_bitshift_bits));
    // ...then set them with new data
    slaves[slave_num].config_regs[reg_num][reg_offset] |= mask;
}

void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]) {
    // clear status before accumulating balancing status of each module in SetBalancingForModuleGroup_
    pack_state->balancing_active = false;

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

#if (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
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
#endif // (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
