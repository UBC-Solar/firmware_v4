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
        bool fault_comm_fail : 1;
        bool fault_self_test : 1;
        bool fault_under_voltage : 1;
        bool fault_over_voltage : 1;
        bool fault_over_temperature : 1;
        bool fault_under_temperature : 1; // most likely a measurement failure

        uint8_t _reserved : 1;
    } bits;
    uint8_t raw;
} faults_t;


typedef union {
    struct {
        bool trip_charge_over_temperature : 1;

        bool warn_low_voltage : 1;
        bool warn_high_voltage : 1;
        bool warn_discharge_high_temperature : 1;
        bool warn_charge_high_temperature : 1;

        uint8_t _reserved : 3;
    } bits;
    uint8_t raw;
} warnings_t;


typedef union {
    struct {
        bool balancing_active : 1;
        bool balancing_enable : 1;
        bool scrutineering_enable : 1;

        bool llim_enable : 1;
        bool hlim_enable : 1;
        bool contactor_enable : 1;

        uint8_t _reserved : 2;
    } bits;
    uint8_t raw;
} pack_state_t;


typedef struct
{
    uint32_t voltage_mv;
    float temperature;
    bool should_balance; // module is currently being balanced (discharged)

    faults_t faults;
    warnings_t warnings;
} module_t;


typedef struct
{
    module_t *modules[NUM_MODULES_PER_SLAVE];
} slave_t;

