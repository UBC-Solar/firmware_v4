#pragma once

#include "logging.h"

/**
 * Molicel M35A Voltage & Temperature Limits
 */
// Absolute limits. MST will fault if exceeded
#define MAX_VOLTAGE_mV 4200U
#define MIN_VOLTAGE_mV 2500U

#define MAX_DISCHARGE_TEMP_degC 60U
#define MAX_CHARGE_TEMP_degC 60U
#define MIN_TEMPERATURE_degC 0U

// Warning values
#define WARN_HIGH_VOLTAGE_mV 4100U
#define WARN_LOW_VOLTAGE_mV 2600U

#define WARN_DISCHARGE_TEMP_degC 55U
#define WARN_CHARGE_TEMP_degC 48U


/**
 * Battery Pack Layout Specifications
 */
// The number of modules used by the pack depends on
// whether scrutineering is enabled.
#define NUM_MODULES 32U
#define NUM_CELLS_PER_MODULE 13U

// Slave(board)s
#define NUM_SLAVES 2U
#define NUM_MODULES_PER_SLAVE 16U


/**
 * Firmware-Specific Settings
 */
// Set current logging level - users can adjust this to filter log output
// This affects which logging function are allowed to print in logging.h
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG


/**
 * Hardware unit tests. 
 */
//These hold no meaning outside the context of hardware unit tests.
#define RUN 1
#define SKIP 0

// Hardware unit tests to verify that the hardware works properly.
// A test is either set to RUN or SKIP.
#define UNIT_TEST_MCU SKIP
#define UNIT_TEST_IO SKIP
#define UNIT_TEST_CAN SKIP
#define UNIT_TEST_ISOSPI SKIP
