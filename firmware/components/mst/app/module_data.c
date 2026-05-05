#include "module_data.h"
#include "main.h"
#include "mst_defs.h"
#include "spi_driver.h"
#include <string.h>

void RequestVoltageMeasurement(void) {
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADCV);
}

void GetVoltageForRegister_(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES], int reg_idx) {
    if (reg_idx >= SLAVE_NUM_VOLT_REG) {
        LOG_ERROR("Voltage register %d is out of range!\r\n", reg_idx);
        Error_Handler();
    }
    
    const Slave_Command_t cell_voltage_commands[SLAVE_NUM_VOLT_REG] = {
        CMD_RDCVA,
        CMD_RDCVB,
        CMD_RDCVC,
        CMD_RDCVD,
        CMD_RDCVE,
        CMD_RDCVF
    };
    Slave_Command_t current_cmd = cell_voltage_commands[reg_idx];
    
    uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    memset(rx_data, 0, sizeof(rx_data));
    Slave_ReadRegisterGroup(current_cmd, rx_data);

    for (int slave_idx = 0; slave_idx < SLAVE_NUM_DEVICES; slave_idx++) {

        for (int module_offset = 0; module_offset < SLAVE_NUM_MODULES_PER_VOLT_REG; module_offset++) {
            
            uint16_t voltage =  (rx_data[slave_idx][module_offset * 2]) | 
                                (rx_data[slave_idx][module_offset * 2 + 1] << 8);

            int module_idx = slaves[slave_idx].volt_mappings[reg_idx][module_offset];
            if (module_idx >= 0) {
                pack_modules[module_idx].voltage_mv = voltage / 10;
            }
        }
    }
}

void RetrieveVoltageMeasurement(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES]) {
    Slave_WakeUp();
    


    for (int reg_idx = 0; reg_idx < SLAVE_NUM_VOLT_REG; reg_idx++) {
        GetVoltageForRegister_(slaves, pack_modules, reg_idx);
    }
    
}

void RequestTemperatureMeasurement(void) {
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADAX_ALL);
}

void GetTemperatureForRegister_(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES], int reg_idx) {
    if (reg_idx >= SLAVE_NUM_TEMP_REG) {
        LOG_ERROR("Temperature register %d is out of range!\r\n", reg_idx);
        Error_Handler();
    }

    const Slave_Command_t aux_commands[SLAVE_NUM_TEMP_REG] = {
        CMD_RDAUXA,
        CMD_RDAUXB,
        CMD_RDAUXC,
        CMD_RDAUXD
    };
    Slave_Command_t current_cmd = aux_commands[reg_idx];

    uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    memset(rx_data, 0, sizeof(rx_data));
    Slave_ReadRegisterGroup(current_cmd, rx_data);

    for (int slave_idx = 0; slave_idx < SLAVE_NUM_DEVICES; slave_idx++) {
        
        int mux_state = slaves[slave_idx].temp_mux_state;

        for (int val_offset = 0; val_offset < SLAVE_NUM_VAL_PER_TEMP_REG; val_offset++) {
            
            uint16_t raw_temp_adc = (rx_data[slave_idx][val_offset * 2]) | 
                                    (rx_data[slave_idx][val_offset * 2 + 1] << 8);

            int module_idx = slaves[slave_idx].temp_mappings[reg_idx][val_offset][mux_state];
            
            if (module_idx >= 0) {
                // TODO: convert ADC value / voltage into proper float degrees C
                pack_modules[module_idx].temperature = (float)raw_temp_adc;
            }
        }
    }
}

void RetrieveTemperatureMeasurement(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES]) {
    Slave_WakeUp();
    for (int reg_idx = 0; reg_idx < SLAVE_NUM_TEMP_REG; reg_idx++) {
        GetTemperatureForRegister_(slaves, pack_modules, reg_idx);
    }
}
