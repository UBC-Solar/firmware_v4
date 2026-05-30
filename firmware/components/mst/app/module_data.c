 #include "module_data.h"
#include "main.h"
#include "mst_defs.h"
#include "mst_types.h"
#include "spi_driver.h"
#include "balancing.h"
#include <string.h>

// Look-up table (LUT) to index cell voltage commands
const Slave_Command_t cell_voltage_commands_lut[SLAVE_NUM_VOLT_REG] = {
    CMD_RDCVA,
    CMD_RDCVB,
    CMD_RDCVC,
    CMD_RDCVD,
    CMD_RDCVE,
    CMD_RDCVF
};

/**
 * Thermistor voltage-to-temperature look-up table.
 * See https://www.vishay.com/docs/29050/ntclg100.pdf.
 * First value is temperature in milli-Celsius,
 * second value is resistance (computed at initialization time),
 * third value is thermistor resistance.
 */
const thermistor_mapping_t thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE] = {
    { -30000, 0, 175200 },
    {      0, 0, 32554  },
    {  10000, 0, 19872  },
    {  20000, 0, 12488  },
    // values above are out of expected range, and thus more sparse
    {  25000, 0, 10000  },
    {  30000, 0, 8059   },
    {  35000, 0, 6535   },
    {  40000, 0, 5330   },
    {  45000, 0, 4372   },
    {  50000, 0, 3605   },
    {  55000, 0, 2989   },
    {  60000, 0, 2490   },
    {  65000, 0, 2084   },
    // values below are out of expected range, and thus more sparse
    {  80000, 0, 1256   },
    { 100000, 0, 677.3  },
    { 150000, 0, 182.6  },
    { 200000, 0, 63.67  }
};

void Module_Init(
	SPI_HandleTypeDef *SPI_handle,
	slave_t slaves[SLAVE_NUM_DEVICES],
	uint8_t config_val_a[SLAVE_REG_SIZE_BYTES],
	uint8_t config_val_b[SLAVE_REG_SIZE_BYTES]) {
    
    // --- Slave 0 Topology Mappings ---
    slaves[0] = (slave_t) 
    {
        // 2 bytes per module value, so each register group (6 bytes total) holds data of 3 modules.
        .volt_mappings = {
            // Cell Voltage Register Group A (C1, C2, C3)
            { 0,  1,  2}, 
            // Cell Voltage Register Group B (C4, C5, C6)
            { 3,  4,  5}, 
            // Cell Voltage Register Group C (...)
            { 6,  7,  8}, 
            // Cell Voltage Register Group D
            { 9, 10, 11}, 
            // Cell Voltage Register Group E
            {12, 13, 14}, 
            // Cell Voltage Register Group F
            {15, -1, -1}
        },
        // 2 bytes per value, each value can hold data for one of 4 modules at any point.
        // So each register group holds data for 12 modules.
        // Multiplexer documentation: https://www.ti.com/lit/ds/symlink/sn74lv4052a.pdf
        .temp_mappings = {
            // Auxiliary Register Group A (GPIO1, GPIO2, GPIO3)
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, { 0,  1,  2,  3} },
            // Auxiliary Register Group B (GPIO4, GPIO5, GPIO6)
            { { 4,  5,  6,  7}, { 8,  9, 10, 11}, {-1, -1, -1, -1} },
            // Auxiliary Register Group C (...)
            { {12, 13, 14, 15}, {-1, -1, -1, -1}, {-1, -1, -1, -1} },
            // Auxiliary Register Group D
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1} }
        },
        // 4 bits per module value, so each 
        .bal_mappings = {
            // S Control Register Group
            { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
            // PWM/S Control Register Group B
            {-1, -1, -1, -1, -1, -1, 12, 13, 14, 15, -1, -1}
        }
    };

    #if SLAVE_NUM_DEVICES > 1U
    // --- Slave 1 Topology Mappings ---
    slaves[1] = (slave_t)
    {
        // 2 bytes per module value, so each register group (6 bytes total) holds data of 3 modules.
        .volt_mappings = {
            // Cell Voltage Register Group A
            {16, 17, 18}, 
            // Cell Voltage Register Group B
            {19, 20, 21}, 
            // Cell Voltage Register Group C
            {22, 23, 24}, 
            // Cell Voltage Register Group D
            {25, 26, 27}, 
            // Cell Voltage Register Group E
            {28, 29, 30},
            // Cell Voltage Register Group F,
            {31, -1, -1}
        },
        // 2 bytes per value, each value can hold data for one of 4 modules at any point.
        // So each register group holds data for 12 modules.
        // Multiplexer documentation: https://www.ti.com/lit/ds/symlink/sn74lv4052a.pdf
        .temp_mappings = {
            // Auxiliary Register Group A
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {16, 17, 18, 19} },
            // Auxiliary Register Group B
            { {20, 21, 22, 23}, {24, 25, 26, 27}, {28, 29, 30, 31} },
            // Auxiliary Register Group C
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1} },
            // Auxiliary Register Group D
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1} }
        },
        // 4 bits per module value, so each 
        .bal_mappings = {
            // S Control Register Group
            {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27},
            // PWM/S Control Register Group B
            {-1, -1, -1, -1, -1, -1, 28, 29, 30, 31, -1, -1}
        }
    };
    #endif // SLAVE_NUM_DEVICES > 1

    // for (int i = 0; i < THERMISTOR_LUT_TABLE_SIZE; i++) {
    //     thermistor_temp_lut[i]
    // }

    Balancing_Init(slaves);

    Slave_Init(SPI_handle, config_val_a, config_val_b);
}

void RequestVoltageMeasurement(void) {
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADCV);
}

void GetVoltageForRegister_(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES], int reg_idx) {
    if (reg_idx >= SLAVE_NUM_VOLT_REG) {
        LOG_ERROR("Voltage register %d is out of range!\r\n", reg_idx);
        Error_Handler();
    }
    

    Slave_Command_t current_cmd = cell_voltage_commands_lut[reg_idx];
    
    uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    memset(rx_data, 0, sizeof(rx_data));
    // TODO: remove test-only wakeup
    Slave_WakeUp();
    Slave_Status_t status = Slave_ReadRegisterGroup(current_cmd, rx_data);
    if (status.error != Slave_OK) {
        LOG_ERROR("SPI comm error getting voltage. Err: %d, Dev: %d", status.error, status.device_num);
        Error_Handler();
    }

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

void RetrieveVoltageMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]) {
    Slave_WakeUp();
    
    Slave_SendCmdAndPoll(CMD_PLADC);

    for (int reg_idx = 0; reg_idx < SLAVE_NUM_VOLT_REG; reg_idx++) {
        GetVoltageForRegister_(slaves, pack_modules, reg_idx);
    }
    
}

void RequestTemperatureMeasurement(void) {
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADAX_ALL);
}

/**
 * @brief Converts a 16-bit ADC value to Temperature in milli-Celsius using pure integer math
 * @param thermistor_uV The raw 16-bit ADC reading (0 - 65535)
 * @return Temperature in milli-Celsius (e.g., 25500 = 25.5 C)
 */
int32_t ThermistorVoltToTemp_(uint16_t thermistor_uV) {
    // 1. Clamp to maximum or minimum values
    if (thermistor_uV >= thermistor_temp_lut[0].voltage_uV) {
        return thermistor_temp_lut[0].temperature_mC;
    }
    if (thermistor_uV <= thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE - 1].voltage_uV) {
        return thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE - 1].temperature_mC;
    }

    // 2. Iterate temperature look up table to find a match for the measured thermistor voltage
    for (uint16_t i = 0; i < THERMISTOR_LUT_TABLE_SIZE - 1; i++) {
        // Find the bounding ADC values (remember: voltage_uV is descending)
        if (thermistor_uV <= thermistor_temp_lut[i].voltage_uV && thermistor_uV >= thermistor_temp_lut[i + 1].voltage_uV) {
            
            int32_t adc1 = thermistor_temp_lut[i].voltage_uV;     // Higher ADC (Colder)
            int32_t t1   = thermistor_temp_lut[i].temperature_mC;       // Lower Temp
            
            int32_t adc2 = thermistor_temp_lut[i + 1].voltage_uV; // Lower ADC (Hotter)
            int32_t t2   = thermistor_temp_lut[i + 1].temperature_mC;   // Higher Temp

            // 3. Linear interpolation between the two values
            // Formula: T = T1 + ( (T2 - T1) * (ADC1 - thermistor_uV) ) / (ADC1 - ADC2)
            // Note: We arrange the subtraction to keep values positive before division
            int32_t temp_diff = t2 - t1;
            int32_t adc_diff = adc1 - adc2;
            int32_t meas_diff = adc1 - thermistor_uV;

            // Multiply before dividing to preserve precision. 
            // Max temp_diff (e.g., 10000 mC) * Max meas_diff (e.g., 10000 ADC) = 100,000,000.
            // This easily fits inside a signed 32-bit integer without overflowing.
            int32_t temperature_mC = t1 + ((temp_diff * meas_diff) / adc_diff);
            
            return temperature_mC;
        }
    }

    // Fallback error code (e.g., absolute zero in mC)
    return -273150; 
}

void GetTemperatureForRegister_(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES], int reg_idx) {
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
    // TODO: remove test-only wakeup
    Slave_WakeUp();
    Slave_Status_t status = Slave_ReadRegisterGroup(current_cmd, rx_data);
    if (status.error != Slave_OK) {
        LOG_ERROR("SPI comm error getting temp. Err: %d, Dev: %d", status.error, status.device_num);
        Error_Handler();
    }

    for (int slave_idx = 0; slave_idx < SLAVE_NUM_DEVICES; slave_idx++) {
        
        int mux_state = slaves[slave_idx].temp_mux_state;

        for (int val_offset = 0; val_offset < SLAVE_NUM_VAL_PER_TEMP_REG; val_offset++) {
            
            uint16_t raw_temp_adc = (rx_data[slave_idx][val_offset * 2]) | 
                                    (rx_data[slave_idx][val_offset * 2 + 1] << 8);

            int module_idx = slaves[slave_idx].temp_mappings[reg_idx][val_offset][mux_state];
            
            if (module_idx >= 0) {
                pack_modules[module_idx].temperature_mC = ThermistorVoltToTemp_(raw_temp_adc);
            }
        }
    }
}

void RetrieveTemperatureMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]) {
    Slave_WakeUp();

    Slave_SendCmdAndPoll(CMD_PLAUX);

    for (int reg_idx = 0; reg_idx < SLAVE_NUM_TEMP_REG; reg_idx++) {
        GetTemperatureForRegister_(slaves, pack_modules, reg_idx);
    }
}

void ComputePackStatistics(module_t pack_modules[NUM_MODULES], pack_state_t *pack_state) {
    uint32_t total_voltage_mV = 0;
    int32_t total_temp_mC = 0;
    for (int i = 0; i < NUM_MODULES; i++) {
        total_voltage_mV += pack_modules[i].voltage_mv;
        total_temp_mC += pack_modules[i].temperature_mC;
    }
    pack_state->total_voltage_mV = total_voltage_mV;
    pack_state->avg_voltage_mV = total_voltage_mV / NUM_MODULES;
    pack_state->avg_temp_mC = total_temp_mC / NUM_MODULES;
}

void SetTempMuxState(slave_t slaves[SLAVE_NUM_DEVICES], unsigned new_state) {
    
}
