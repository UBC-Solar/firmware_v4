
#include "gpio_driver.h"
#include "main.h"
#include "mst_defs.h"
#include "logging.h"
#include "mst_main.h"
#include "mst_types.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_gpio.h"


void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings) {
    // Warnings are not latched across cycles, so reset pack-wide warnings each run.
    pack_warnings->raw = 0U;

    for (int i = 0; i < NUM_MODULES; i++) {
        module_t *module = &pack_modules[i];

        module->faults.bits.fault_over_voltage = module->voltage_mv >= MAX_VOLTAGE_mV;
        module->faults.bits.fault_under_voltage = module->voltage_mv <= MIN_VOLTAGE_mV;
        module->faults.bits.fault_over_temperature = module->temperature_mC >= (MAX_TEMP_degC*1000);
        module->faults.bits.fault_under_temperature = module->temperature_mC <= (MIN_TEMP_degC*1000);

        module->warnings.bits.warn_low_voltage = module->voltage_mv <= WARN_LOW_VOLTAGE_mV;
        module->warnings.bits.warn_high_voltage = module->voltage_mv >= WARN_HIGH_VOLTAGE_mV;
        module->warnings.bits.warn_high_temperature = module->temperature_mC >= (WARN_HIGH_TEMP_degC*1000);

        // Sum up all faults and warnings across each module
        pack_faults->raw |= module->faults.raw;
        pack_warnings->raw |= module->warnings.raw;
    }
}


void Fault() {
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
}

