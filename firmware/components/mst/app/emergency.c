
#include "mst_defs.h"
#include "logging.h"
#include "mst_main.h"
#include "mst_types.h"


void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings) {
    // Warnings are not latched across cycles, so reset pack-wide warnings each run.
    pack_warnings->raw = 0U;

    for (int i = 0; i < NUM_MODULES; i++) {
        module_t *module = &pack_modules[i];

        const bool over_voltage = module->voltage_mv >= MAX_VOLTAGE_mV;
        const bool under_voltage = module->voltage_mv <= MIN_VOLTAGE_mV;
        const bool over_temperature = module->temperature >= MAX_DISCHARGE_TEMP_degC;
        const bool under_temperature = module->temperature <= MIN_TEMPERATURE_degC;

        const bool warn_low_voltage = module->voltage_mv <= WARN_LOW_VOLTAGE_mV;
        const bool warn_high_voltage = module->voltage_mv >= WARN_HIGH_VOLTAGE_mV;
        const bool warn_discharge_high_temperature = module->temperature >= WARN_DISCHARGE_TEMP_degC;
        const bool warn_charge_high_temperature = module->temperature >= WARN_CHARGE_TEMP_degC;
        const bool trip_charge_over_temperature = module->temperature >= MAX_CHARGE_TEMP_degC;

        module->faults.bits.fault_under_voltage = under_voltage;
        module->faults.bits.fault_over_voltage = over_voltage;
        module->faults.bits.fault_over_temperature = over_temperature;
        module->faults.bits.fault_under_temperature = under_temperature;

        module->warnings.bits.trip_charge_over_temperature = trip_charge_over_temperature;
        module->warnings.bits.warn_low_voltage = warn_low_voltage;
        module->warnings.bits.warn_high_voltage = warn_high_voltage;
        module->warnings.bits.warn_discharge_high_temperature = warn_discharge_high_temperature;
        module->warnings.bits.warn_charge_high_temperature = warn_charge_high_temperature;

        // Pack faults are latched once set; never cleared here.
        pack_faults->bits.fault_under_voltage |= module->faults.bits.fault_under_voltage;
        pack_faults->bits.fault_over_voltage |= module->faults.bits.fault_over_voltage;
        pack_faults->bits.fault_over_temperature |= module->faults.bits.fault_over_temperature;
        pack_faults->bits.fault_under_temperature |= module->faults.bits.fault_under_temperature;

        // Pack warnings reflect current cycle status after warning reset above.
        pack_warnings->bits.trip_charge_over_temperature |= module->warnings.bits.trip_charge_over_temperature;
        pack_warnings->bits.warn_low_voltage |= module->warnings.bits.warn_low_voltage;
        pack_warnings->bits.warn_high_voltage |= module->warnings.bits.warn_high_voltage;
        pack_warnings->bits.warn_discharge_high_temperature |= module->warnings.bits.warn_discharge_high_temperature;
        pack_warnings->bits.warn_charge_high_temperature |= module->warnings.bits.warn_charge_high_temperature;

    }
}