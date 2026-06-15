#include "analysis.h"
#include "mst_defs.h"
#include "mst_main.h"
#include "main.h"
#include "gpio_driver.h"
#include <stdbool.h>


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

    LOG_INFO("Pack stats - Total V: %lu mV, Avg V: %lu mV, Total T: %ld mC, Avg T: %ld mC", pack_state->total_voltage_mV, pack_state->avg_voltage_mV, total_temp_mC, pack_state->avg_temp_mC);
    LOG_INFO("Pack statuses - Balancing Active: %d, Balancing Enable: %d, Scrutineering: %d", 
              pack_state->balancing_active, pack_state->balancing_enable, pack_state->scrutineering_enable);
    LOG_INFO("Pack statuses - LLIM: %d, HLIM: %d, Contactor: %d", 
              pack_state->llim_enable, pack_state->hlim_enable, pack_state->contactor_enable);
    LOG_INFO("Pack warnings - Raw: 0x%02X, Low V: %d, High V: %d, High T: %d", 
              pack_warnings.raw, pack_warnings.bits.warn_low_voltage, pack_warnings.bits.warn_high_voltage, pack_warnings.bits.warn_high_temperature);
    LOG_INFO("Pack faults - Raw: 0x%02X, UV: %d, OV: %d, OT: %d, UT: %d", 
              pack_faults.raw, pack_faults.bits.fault_under_voltage, pack_faults.bits.fault_over_voltage, 
              pack_faults.bits.fault_over_temperature, pack_faults.bits.fault_under_temperature);

    
    for (int i = 0; i < NUM_MODULES; i++) {

        bool is_module_outlier = 
            pack_modules[i].voltage_mv < (pack_state->avg_voltage_mV * 0.7) || 
            pack_modules[i].voltage_mv > (pack_state->avg_voltage_mV * 1.3) ||
            pack_modules[i].temperature_mC < (pack_state->avg_temp_mC - 1000) ||
            pack_modules[i].temperature_mC > (pack_state->avg_temp_mC + 1000);

        LOG_INFO("Module %d - Voltage: %d.%02dV, Temp: %d.%02dC %s", 
            i, 
            pack_modules[i].voltage_mv / 1000, pack_modules[i].voltage_mv % 1000, 
            pack_modules[i].temperature_mC / 1000, pack_modules[i].temperature_mC % 1000,
            is_module_outlier ? " <-- OUTLIER" : "");
    }
}


void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings) {
    // Warnings are not latched across cycles, so reset pack-wide warnings each run.
    pack_warnings->raw = 0U;

    for (int i = 0; i < NUM_MODULES; i++) {
        module_t *module = &pack_modules[i];

        module->faults.bits.fault_over_voltage = module->voltage_mv >= MAX_VOLTAGE_mV;
        module->faults.bits.fault_under_voltage = module->voltage_mv <= MIN_VOLTAGE_mV;
        module->faults.bits.fault_over_temperature = module->temperature_mC >= (int32_t)(MAX_TEMP_degC*1000);
        module->faults.bits.fault_under_temperature = module->temperature_mC <= (int32_t)(MIN_TEMP_degC*1000);

        module->warnings.bits.warn_low_voltage = module->voltage_mv <= WARN_LOW_VOLTAGE_mV;
        module->warnings.bits.warn_high_voltage = module->voltage_mv >= WARN_HIGH_VOLTAGE_mV;
        module->warnings.bits.warn_high_temperature = module->temperature_mC >= (WARN_HIGH_TEMP_degC*1000);

        // Sum up all faults and warnings across each module
        pack_faults->raw |= module->faults.raw;
        pack_warnings->raw |= module->warnings.raw;
    }

    #if (INT_TEST_JUNE_16th == RUN)
    // Then ignore temperature-related faults. Temperature circuitry on slaveboard is not working at the moment...
    pack_faults->bits.fault_over_temperature = false;
    pack_faults->bits.fault_under_temperature = false;
    #endif // (INT_TEST_JUNE_16th == RUN)
}


