#include "balancing.h"
#include "mst_defs.h"
#include "mst_types.h"
#include "spi_driver.h"

uint8_t s_ctrl_regs[SLAVE_NUM_BAL_REG][SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES] = {0};

void Balancing_Init(slave_t slaves[SLAVE_NUM_DEVICES]) {
    (void)slaves;

    // The first 3 bytes of s_ctrl_reg2 (PWM/S register) is 
    // intended for S-pin PWM values. Since we want PWM (duty cycle) to always be
    // 100%, we'll write all 1's to these bytes to make sure of it.
    for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
        for (int j = 0; j < 3; j++) {
            s_ctrl_regs[1][j][i] = 0xFF;
        }
    }
}

void SetBalancingForModule_(int slave_num, int bal_reg_num, int module_offset, bool if_balance) {
    int reg_offset = module_offset / 2;
    int reg_bitshift_bits = (module_offset % 2) * 4;

    uint8_t mask = (uint8_t)(0x08 << reg_bitshift_bits);

    if (if_balance) {
        s_ctrl_regs[bal_reg_num][slave_num][reg_offset / 2] |= mask;
    }
    else {
        s_ctrl_regs[bal_reg_num][slave_num][reg_offset / 2] &= (uint8_t)(~mask);
    }
}

void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]) {
    if (!pack_state->balancing_enable) {
        return;
    }

    // Omg...
    for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
        for (int j = 0; j < SLAVE_NUM_BAL_REG; j++) {
            for (int k = 0; k < SLAVE_NUM_MODULE_PER_BAL_REG; k++) {

                int module_idx = slaves[i].bal_mappings[j][k];
                if (module_idx < 0 || module_idx >= NUM_MODULES) {
                    continue;
                }
    
                // Absolute difference in mV of this module's voltage vs the average pack voltage
                uint32_t voltage_diff_mv = (pack_modules[module_idx].voltage_mv > pack_state->avg_voltage_mV)
                    ? (pack_modules[module_idx].voltage_mv - pack_state->avg_voltage_mV)
                    : (pack_state->avg_voltage_mV - pack_modules[module_idx].voltage_mv);
                
                bool if_balance = voltage_diff_mv >= MIN_BALANCE_VOLT_DIFF_MV;
        
                SetBalancingForModule_(i, j, k, if_balance);
            }
        }
    }

    Slave_WriteRegisterGroup(CMD_WRSCTRL, s_ctrl_regs[0]);
    Slave_WriteRegisterGroup(CMD_WRPSB, s_ctrl_regs[1]);
}

void PauseAllBalancing() {
    Slave_WakeUp();
    Slave_SendCmd(CMD_MUTE);
}

void ResumeAllBalancing() {
    Slave_WakeUp();
    Slave_SendCmd(CMD_UNMUTE);
}

#if (UNIT_TEST_SLAVE == RUN)
void Debug_DoBalancing(slave_t slaves[SLAVE_NUM_DEVICES], bool enable) {

    for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
        for (int j = 0; j < SLAVE_NUM_BAL_REG; j++) {
            for (int k = 0; k < SLAVE_NUM_MODULE_PER_BAL_REG; k++) {

                int module_idx = slaves[i].bal_mappings[j][k];
                if (module_idx < 0 || module_idx >= NUM_MODULES) {
                    continue;
                }
    

                SetBalancingForModule_(i, j, k, enable);
            }
        }
    }

    Slave_WriteRegisterGroup(CMD_WRSCTRL, s_ctrl_regs[0]);
    Slave_WriteRegisterGroup(CMD_WRPSB, s_ctrl_regs[1]);
}
#endif // UNIT_TEST_ISOSPI
