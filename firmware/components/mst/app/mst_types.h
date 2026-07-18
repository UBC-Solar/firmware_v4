#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "mst_defs.h"

/**
 * MST Datatypes
 * MARK: types
 */
typedef union {
    struct {
        bool fault_under_voltage : 1;
        bool fault_over_voltage : 1;
        bool fault_over_temperature : 1;
        bool fault_under_temperature : 1; // most likely a measurement failure

        uint8_t _reserved : 4;
    } bits;
    uint8_t raw;
} faults_t;


typedef union {
    struct {
        bool warn_low_voltage : 1;
        bool warn_high_voltage : 1;
        bool warn_high_temperature : 1;

        uint8_t _reserved : 5;
    } bits;
    uint8_t raw;
} warnings_t;


typedef struct {
    bool balancing_active;
    bool balancing_enable;
    bool scrutineering_enable;

    bool llim_enable;
    bool hlim_enable;
    bool contactor_enable;

    bool error_comm_fail;
    bool error_self_test;
    uint32_t last_comm_fail_time;
    uint8_t num_consecutive_comm_fails;
    
    uint32_t total_voltage_mV;
    uint32_t avg_voltage_mV;
    int32_t avg_temp_mC;

    uint32_t min_voltage_mV;
    uint8_t min_voltage_idx;

    uint32_t max_voltage_mV;
    uint8_t max_voltage_idx;

    int32_t min_temp_mC;
    uint8_t min_temp_idx;

    int32_t max_temp_mC;
    uint8_t max_temp_idx;
} pack_state_t;


typedef struct
{
    uint32_t voltage_mv;
    int32_t temperature_mC; // milli-Celsius

    // Module should be discharged. This is only a suggestion to balance.
    // 
    bool is_balancing; 

    faults_t faults;
    warnings_t warnings;
} module_t;


typedef struct
{
    int slave_reg_num;
    int module_num;
} slave_mapping_t;

typedef struct
{
    /**
     * @brief Runtime state representing the selected temperature multiplexer channel.
     * Only the 2 LSBs are used.
     */
    unsigned temp_mux_state : 2;

    // Notice: Static hardware mappings for module bindings (volt_mappings & temp_mappings)
    // have been historically kept here. If memory becomes an issue, consider migrating 
    // them to a globally defined `const` array to save RAM.
    int volt_mappings[SLAVE_NUM_VOLT_REG][SLAVE_NUM_MODULES_PER_VOLT_REG];
    int temp_mappings[SLAVE_NUM_TEMP_REG][SLAVE_NUM_VAL_PER_TEMP_REG][SLAVE_NUM_MODULES_PER_TEMP_VAL];
    int bal_mappings[SLAVE_NUM_BAL_REG][SLAVE_NUM_VAL_PER_BAL_REG];

    uint8_t config_regs[SLAVE_NUM_CONFIG_REG][SLAVE_REG_SIZE_BYTES];
} slave_t;

typedef struct
{
    int32_t temperature_mC;
    uint32_t voltage_uV;
    uint32_t resistance_Ohm;
} thermistor_mapping_t;
