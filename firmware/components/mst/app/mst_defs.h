#pragma once

#include "logging.h"

/**
 * Molicel M35A Voltage & Temperature Limits
 */
// Absolute limits. MST will fault if exceeded
#define MAX_VOLTAGE_mV 4200U
#define MIN_VOLTAGE_mV 2750U

#define MAX_TEMP_degC 60U
#define MIN_TEMP_degC 0U

// Warning values
#define WARN_HIGH_VOLTAGE_mV 4100U
#define WARN_LOW_VOLTAGE_mV 2850U

#define WARN_HIGH_TEMP_degC 55U

/**
 * SPI interface error handling
 */
// SPI interface should not have too manny
// consecutive fails within a specified timeframe
#define NUM_CONSECUTIVE_COMM_ERR 3
#define CONSECUTIVE_TIMEFRAME_MS 2000

/**
 * Battery Pack Layout Specifications
 */
// The number of modules used by the pack depends on
// whether scrutineering is enabled.
#define NUM_MODULES 32U
#define NUM_CELLS_PER_MODULE 13U

/**
 * CAN messages
 */
#define CAN_STATUS_ID               0x310
#define CAN_STATUS_PERIOD_MS        1000
#define CAN_MODULE_VOLT_SUMMARY_ID  0x311
#define CAN_MODULE_TEMP_SUMMARY_ID  0x312
#define CAN_MODULE_VOLT_DATA_ID     0x313
#define CAN_MODULE_TEMP_DATA_ID     0x314
#define CAN_MODULE_STATUS_ID        0x315
#define CAN_BALANCE_DATA_ID         0x316
#define CAN_NUM_MODULE_GROUPS       8U
#define CAN_NUM_MODULES_PER_GROUP   NUM_MODULES / CAN_NUM_MODULE_GROUPS


// Slave(board)s
#define SLAVE_NUM_DEVICES 2U // Number of ADBMS1818 ICs daisy chained
#define SLAVEBOARD_REV 1

// Enable / disable MST algorithms
#define INIT_FAULT_HOLD_DURATION_MS 2000 // MST pulls FAULT HIGH at startup to notify HVC
#define RUN_MAIN_LOOP true
#define ISOSPI_CONNECTED true
#define CAN_CONNECTED true
#define TEMP_STRATEGY_ALL_AT_ONCE false
#define CAN_STRATEGY_ALL_AT_ONCE false


/**
 * Slaveboards configuration and ADBMS1818 options
 */

#define SLAVE_REG_SIZE_BYTES 6 // All of the ADBMS1818 register groups consist of 6 bytes

#define SLAVE_NUM_CONFIG_REG 2

#define SLAVE_NUM_VOLT_REG 6
#define SLAVE_NUM_MODULES_PER_VOLT_REG 3

#define SLAVE_NUM_TEMP_REG 3 // there are actually 4 totall auxillary registers, but we only ever use 2 of them
#define SLAVE_NUM_VAL_PER_TEMP_REG 3
#define SLAVE_NUM_MODULES_PER_TEMP_VAL 4

#define SLAVE_NUM_BAL_REG 2
#define SLAVE_NUM_VAL_PER_BAL_REG 12
#define SLAVE_NUM_MODULES_PER_BAL_VAL 4

#define SLAVE_TIMEOUT_MS 100U // ms - safety timeout threshold for Slave functions

#define THERMISTOR_LUT_TABLE_SIZE 17

#define MIN_BALANCE_VOLT_DIFF_MV 100U
#define MAX_BALANCE_VOLT_DIFF_MV 300U

/* Configuration Register Group Parameters */

// Keep voltage references on between ADC reads (significantly speeds up reads,
//   increases power consumption)
#define REFON 1
// 0 = References Shut Down After Conversions,
// 1 = References Remain Powered Up Until Watchdog Timeout

// Under-voltage threshold for ADBMS1818
#define VUV 1687U // (2.7V / (16 * 0.0001V)) - 1 = 1687
// Over-voltage threshold for ADBMS1818
#define VOV 2624U // (4.2V / (16 * 0.0001V)) - 1 = 2624
// Note that these thresholds are internal to the ADBMS1818; they only
//   impact the behaviour of the UV and OV bit flags in the status registers

// ADCOPT selects the ADC mode together with MD, but is in the CFG register
#define ADCOPT 0
/* End Configuration Register Group Parameters */

// Discharge Permitted during cell measurement
#define DCP 0 // 0 = Discharge Not Permitted 1 = Discharge Permitted
// ADC Mode (speed)
#define MD MD_7KHZ_3KHZ // Normal mode


/**
 * Firmware-Specific Settings
 */
// Set current logging level - users can adjust this to filter log output
// This affects which logging function are allowed to print in logging.h
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO


/**
 * Tests. 
 */
//These hold no meaning outside the context of hardware unit tests and integration.
#define RUN 1
#define SKIP 0

// Hardware unit tests (UNIT_...) to verify that the hardware works properly.
// Integration tests (INT_...) to verify functionality with rest of BMS.
// A test is either set to RUN or SKIP.
#define UNIT_TEST_MCU SKIP
#define UNIT_TEST_IO SKIP
#define UNIT_TEST_CAN SKIP
#define UNIT_TEST_ISOSPI SKIP
#define INT_TEST_SLAVE SKIP
#define INT_TEST_SLAVE_BAL_VOLT SKIP
#define INT_TEST_SLAVE_BAL_SCRUT SKIP
#define INT_TEST_SLAVE_MUX SKIP
#define INT_TEST_JUNE_16th RUN
