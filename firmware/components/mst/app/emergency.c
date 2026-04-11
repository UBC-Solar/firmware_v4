
#include "mst_defs.h"
#include "logging.h"
#include "mst_main.h"


void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings) {
    // Warnings are not latched across cycles, so reset pack-wide warnings each run.
    pack_warnings->raw = 0U;

    for (int i = 0; i < NUM_MODULES; i++) {
        module_t *module = &pack_modules[i];

        module->faults.bits.fault_over_voltage = module->voltage_mv >= MAX_VOLTAGE_mV;
        module->faults.bits.fault_under_voltage = module->voltage_mv <= MIN_VOLTAGE_mV;
        module->faults.bits.fault_over_temperature = module->temperature >= MAX_DISCHARGE_TEMP_degC;
        module->faults.bits.fault_under_temperature = module->temperature <= MIN_TEMPERATURE_degC;

        module->warnings.bits.trip_charge_over_temperature = module->voltage_mv <= WARN_LOW_VOLTAGE_mV;
        module->warnings.bits.warn_low_voltage = module->voltage_mv >= WARN_HIGH_VOLTAGE_mV;
        module->warnings.bits.warn_high_voltage = module->temperature >= WARN_DISCHARGE_TEMP_degC;
        module->warnings.bits.warn_discharge_high_temperature = module->temperature >= WARN_CHARGE_TEMP_degC;
        module->warnings.bits.warn_charge_high_temperature = module->temperature >= MAX_CHARGE_TEMP_degC;

        // Sum up all faults and warnings across each module
        pack_faults->raw |= module->faults.raw;
        pack_warnings->raw |= module->warnings.raw;

    }
}