 #include "module_data.h"
#include "main.h"
#include "mst_defs.h"
#include "mst_types.h"
#include "spi_driver.h"
#include <string.h>


/**
 * Thermistor voltage-to-temperature look-up table.
 * See https://www.vishay.com/docs/29050/ntclg100.pdf.
 * First value is temperature in milli-Celsius,
 * second value is resistance (computed at initialization time),
 * third value is thermistor resistance.
 */
thermistor_mapping_t thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE] = {
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
	slave_t slaves[SLAVE_NUM_DEVICES]) {

    // --- Slave 0 Topology Mappings ---
    slaves[0] = (slave_t) 
    {
        .config_regs = {
            {
                0xF8 | (REFON << 2) | ADCOPT, // GPIO 1-5 pull-downs off, REFON, ADCOPT
                (VUV & 0xFF), // VUV[7:0]
                ((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F), // VOV[4:0] | VUV[11:8]
                (VOV >> 4), // VOV[11:4]
                0x00, // Discharge off for cells 1 through 8
                0x00  // Discharge off for cells 9 through 12, Discharge timer disabled
            },
            {
                0x0F, // Discharge off for cells 13 through 16, GPIO 6-9 = 1
                0x00, // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
                0x00,
                0x00,
                0x00,
                0x00
            }
        },
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
            { {12, 13, 14, 15}, {-1, -1, -1, -1}, {-1, -1, -1, -1} }
        },
        // 4 bits per value, each value holds data for 4 continuous modules at once (e.g. value == 0 --> data for modules 0 to 3)
        // so each register group holds data for 12 modules
        .bal_mappings = {
            // Configuration Register group A
            { -1, -1, -1, -1, -1, -1, -1, -1,  0,  4,  8, -1 },
            // Configuration Register Group B
            { -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
        }
    };

    #if SLAVE_NUM_DEVICES > 1U
    // --- Slave 1 Topology Mappings ---
    slaves[1] = (slave_t)
    {
        .config_regs = {{
            0xF8 | (REFON << 2) | ADCOPT, // GPIO 1-5 pull-downs off, REFON, ADCOPT
            (VUV & 0xFF), // VUV[7:0]
            ((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F), // VOV[4:0] | VUV[11:8]
            (VOV >> 4), // VOV[11:4]
            0x00, // Discharge off for cells 1 through 8
            0x00  // Discharge off for cells 9 through 12, Discharge timer disabled
        },
        {
            0x0F, // Discharge off for cells 13 through 16, GPIO 6-9 = 1
            0x00, // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
            0x00,
            0x00,
            0x00,
            0x00
        }},
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
            { {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1} }
        },
        // 4 bits per value, each value holds data for 4 continuous modules at once (e.g. value == 0 --> data for modules 0 to 3)
        // so each register group holds data for 12 modules
        .bal_mappings = {
            // Configuration Register group A
            { -1, -1, -1, -1, -1, -1, -1, -1, 16, 20, 24, -1 },
            // Configuration Register Group B
            { -1, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
        }
    };
    #endif // SLAVE_NUM_DEVICES > 1

    // TODO: measure V_ref2 here instead of hard-coding 3V
    for (int i = 0; i < THERMISTOR_LUT_TABLE_SIZE; i++) {
        //V_T: thermistor voltage. R_T: thermistor resistance.
        // V_T = V_ref2 * (R_T)/(10,000 + R_T)
        uint64_t resistance_Ohm = (uint64_t) 3000000 * thermistor_temp_lut[i].resistance_Ohm / (10000 + thermistor_temp_lut[i].resistance_Ohm);
        thermistor_temp_lut[i].voltage_uV = resistance_Ohm;
    }

    Slave_Init(SPI_handle);

#if (UNIT_TEST_ISOSPI != RUN)
    WriteConfigRegisters(slaves);
#endif
}


void WriteConfigRegisters(slave_t slaves[SLAVE_NUM_DEVICES])
{
    uint8_t cfgra_2d[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    uint8_t cfgrb_2d[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++) {
        memcpy(cfgra_2d[ic_num], slaves[ic_num].config_regs[0], SLAVE_REG_SIZE_BYTES);
        memcpy(cfgrb_2d[ic_num], slaves[ic_num].config_regs[1], SLAVE_REG_SIZE_BYTES);
    }

    Slave_WakeUp();
    Slave_WriteRegisterGroup(CMD_WRCFGA, cfgra_2d);
    Slave_WriteRegisterGroup(CMD_WRCFGB, cfgrb_2d);
}


void RequestVoltageMeasurement(void) {
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADCV);
}

void GetVoltageForRegister_(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES], int reg_idx) {
    
    const Slave_Command_t cell_voltage_commands[SLAVE_NUM_VOLT_REG] = {
        CMD_RDCVA,
        CMD_RDCVB,
        CMD_RDCVC,
        CMD_RDCVD,
        CMD_RDCVE,
        CMD_RDCVF
    };

    if (reg_idx >= SLAVE_NUM_VOLT_REG) {
        LOG_ERROR("Voltage register %d is out of range!", reg_idx);
        ERROR_HANDLER_LOGGED();
    }

    Slave_Command_t current_cmd = cell_voltage_commands[reg_idx];
    
    uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    memset(rx_data, 0, sizeof(rx_data));
    // TODO: remove test-only wakeup
    Slave_WakeUp();
    Slave_Status_t status = Slave_ReadRegisterGroup(current_cmd, rx_data);
    if (status.error != Slave_OK) {
        LOG_ERROR("SPI comm error getting voltage. Err: %d, Dev: %d", status.error, status.device_num);
        ERROR_HANDLER_LOGGED();
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
    uint32_t start_ms = HAL_GetTick();
    Slave_WakeUp();
    Slave_SendCmd(CMD_ADAX_ALL);
    uint32_t end_ms = HAL_GetTick();
    LOG_DEBUG("RequestTemperatureMeasurement driver interact time: %lu ms", end_ms - start_ms);
}

/**
 * @brief Converts a voltage measurement to Temperature in milli-Celsius using pure integer math
 * @param thermistor_uV The raw ADC reading in microvolts
 * @return Temperature in milli-Celsius (e.g., 25500 = 25.5 C)
 */
int32_t ThermistorVoltToTemp_(uint32_t thermistor_uV) {
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
            int64_t temperature_mC = (int64_t) t1 + (((int64_t)temp_diff * meas_diff) / adc_diff);
            
            return temperature_mC;
        }
    }

    // Fallback error code (e.g., absolute zero in mC)
    return -273150; 
}

void GetTemperatureForRegister_(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES], int reg_idx) {
    if (reg_idx >= SLAVE_NUM_TEMP_REG) {
        LOG_ERROR("Temperature register %d is out of range!", reg_idx);
        ERROR_HANDLER_LOGGED();
    }

    const Slave_Command_t aux_commands[SLAVE_NUM_TEMP_REG] = {
        CMD_RDAUXA,
        CMD_RDAUXB,
        CMD_RDAUXC
    };
    Slave_Command_t current_cmd = aux_commands[reg_idx];

    uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    memset(rx_data, 0, sizeof(rx_data));
    // TODO: remove test-only wakeup
    
    uint32_t driver_start_ms = HAL_GetTick();
    Slave_WakeUp();
    Slave_Status_t status = Slave_ReadRegisterGroup(current_cmd, rx_data);
    uint32_t driver_end_ms = HAL_GetTick();
    
    if (status.error != Slave_OK) {
        LOG_ERROR("SPI comm error getting temp. Err: %d, Dev: %d", status.error, status.device_num);
        ERROR_HANDLER_LOGGED();
    }

    uint32_t compute_start_ms = HAL_GetTick();
    for (int slave_idx = 0; slave_idx < SLAVE_NUM_DEVICES; slave_idx++) {
        
        int mux_state = slaves[slave_idx].temp_mux_state;

        for (int val_offset = 0; val_offset < SLAVE_NUM_VAL_PER_TEMP_REG; val_offset++) {
            // Raw ADC value has LSB = 100 uV
            uint32_t adc_raw_100uV =
                (rx_data[slave_idx][val_offset * 2]) | 
                (rx_data[slave_idx][val_offset * 2 + 1] << 8);
            uint32_t adc_raw_uV = adc_raw_100uV * 100;

            int module_idx = slaves[slave_idx].temp_mappings[reg_idx][val_offset][mux_state];
            
            if (module_idx >= 0) {
                pack_modules[module_idx].temperature_mC = ThermistorVoltToTemp_(adc_raw_uV);
            }
        }
    }
    uint32_t compute_end_ms = HAL_GetTick();

    LOG_DEBUG("GetTemperatureForRegister reg %d - Driver read: %lu ms, Compute: %lu ms", 
        reg_idx, driver_end_ms - driver_start_ms, compute_end_ms - compute_start_ms);
}

void RetrieveTemperatureMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]) {
    uint32_t start_ms = HAL_GetTick();

    Slave_WakeUp();
    Slave_SendCmdAndPoll(CMD_PLADC);
    
    uint32_t poll_end_ms = HAL_GetTick();

    for (int reg_idx = 0; reg_idx < SLAVE_NUM_TEMP_REG; reg_idx++) {
        GetTemperatureForRegister_(slaves, pack_modules, reg_idx);
    }
    
    uint32_t ret_end_ms = HAL_GetTick();
    LOG_DEBUG("RetrieveTemperatureMeasurement total: %lu ms (Poll: %lu ms, Read loop: %lu ms)",
               ret_end_ms - start_ms, poll_end_ms - start_ms, ret_end_ms - poll_end_ms);
}

extern faults_t pack_faults;
extern warnings_t pack_warnings;

void ComputePackStatistics(module_t pack_modules[NUM_MODULES], pack_state_t *pack_state) {
    uint32_t total_voltage_mV = 0;
    int32_t total_temp_mC = 0;
    for (int i = 0; i < NUM_MODULES; i++) {
        total_voltage_mV += pack_modules[i].voltage_mv;
        total_temp_mC += pack_modules[i].temperature_mC;
    }
    pack_state->total_voltage_mV = (uint32_t) total_voltage_mV;
    pack_state->avg_voltage_mV = (uint32_t) total_voltage_mV / NUM_MODULES;
    pack_state->avg_temp_mC = (int32_t ) total_temp_mC / NUM_MODULES;

    LOG_DEBUG("Pack stats - Total V: %lu mV, Avg V: %lu mV, Total T: %ld mC, Avg T: %ld mC", pack_state->total_voltage_mV, pack_state->avg_voltage_mV, total_temp_mC, pack_state->avg_temp_mC);

    LOG_DEBUG("Pack statuses - Balancing Active: %d, Balancing Enable: %d, Scrutineering: %d", 
              pack_state->balancing_active, pack_state->balancing_enable, pack_state->scrutineering_enable);
    LOG_DEBUG("Pack statuses - LLIM: %d, HLIM: %d, Contactor: %d", 
              pack_state->llim_enable, pack_state->hlim_enable, pack_state->contactor_enable);

    LOG_DEBUG("Pack warnings - Raw: 0x%02X, Low V: %d, High V: %d, High T: %d", 
              pack_warnings.raw, pack_warnings.bits.warn_low_voltage, pack_warnings.bits.warn_high_voltage, pack_warnings.bits.warn_high_temperature);

    LOG_DEBUG("Pack faults - Raw: 0x%02X, UV: %d, OV: %d, OT: %d, UT: %d", 
              pack_faults.raw, pack_faults.bits.fault_under_voltage, pack_faults.bits.fault_over_voltage, 
              pack_faults.bits.fault_over_temperature, pack_faults.bits.fault_under_temperature);

    
    for (int i = 0; i < NUM_MODULES; i++) {

        bool is_module_outlier = 
            pack_modules[i].voltage_mv < (pack_state->avg_voltage_mV * 0.7) || 
            pack_modules[i].voltage_mv > (pack_state->avg_voltage_mV * 1.3) ||
            pack_modules[i].temperature_mC < (pack_state->avg_temp_mC - 1000) ||
            pack_modules[i].temperature_mC > (pack_state->avg_temp_mC + 1000);

        LOG_DEBUG("Module %d - Voltage: %d.%02dV, Temp: %d.%02dC %s", 
            i, 
            pack_modules[i].voltage_mv / 1000, pack_modules[i].voltage_mv % 1000, 
            pack_modules[i].temperature_mC / 1000, pack_modules[i].temperature_mC % 1000,
            is_module_outlier ? " <-- OUTLIER" : "");
    }
}

void SetTempMuxState(slave_t slaves[SLAVE_NUM_DEVICES], unsigned new_state) {

    // Extract the lowest 2 bits from new_state
    uint8_t mux_bits = new_state & 0x03;

    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++) {
        // Update the slave data structure with the new state
        slaves[ic_num].temp_mux_state = mux_bits;

        // Clear and set bits 3 and 4 in byte 0 of configuration register group A
        uint8_t *config_a = slaves[ic_num].config_regs[0];
        config_a[0] &= ~(0x18);
        config_a[0] |= (mux_bits << 3);
    }

    Slave_WakeUp();
    WriteConfigRegisters(slaves);
}

void SetScrutineeringMode(slave_t slaves[SLAVE_NUM_DEVICES], bool enable) {
    // config_val_b[i][0] ^ (scrutineering_enabled << 1)
    
    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++) {
        // Clear and set bit 1 in byte 0 of configuration register group B
        uint8_t *config_b = slaves[ic_num].config_regs[1];
        config_b[0] &= ~(0x02);
        config_b[0] |= (!enable << 1); // GPIO (and thus scrutineering circuitry) is active-LOW
    }

    Slave_WakeUp();
    WriteConfigRegisters(slaves);
}
