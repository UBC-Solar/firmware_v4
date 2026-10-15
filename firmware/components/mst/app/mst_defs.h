#pragma once

#include "logging.h"


/*==============================================================================
 * === MAIN-LOOP BEHAVIOUR AND FEATURES ========================================
 *============================================================================*/

// MST pulls FAULT HIGH at startup to notify HVC while it initializes.
#define INIT_FAULT_HOLD_DURATION_MS 2000

// Runs the main loop. Disable only when running a test cycle instead.
#define RUN_MAIN_LOOP true

// Number of main-loop iterations upon init before module data is complete enough to 
// analyze and transmit. Needed since not all cell data is measured every iteration.
#define NUM_INIT_MAINLOOPS 4

// If false, MST will not halt even if isoSPI comms fail.
#define ISOSPI_CONNECTED true

// If false, MST will not attempt to send CAN messages.
#define CAN_CONNECTED true

// Temperature strategy: true = measure all mux channels every main loop,
// false = measure one multiplexer setting per main loop (round-robin).
#define TEMP_STRATEGY_ALL_AT_ONCE false

// CAN strategy: true = send every module-data group every main loop,
// false = send one group per main loop (round-robin).
#define CAN_STRATEGY_ALL_AT_ONCE false

// If true, override all real measurements with fake constants
// (3.6 V and 21 degC for every module). Normal analysis still runs.
#define GENERATE_FAKE_BATTERY_DATA false

// Slaveboard temperature circuitry was damaged during testing and can
// report incorrect temperatures. If true, ignore all temperature warnings
// and faults.
#define IGNORE_TEMPEREATURE_VALUES false


/*==============================================================================
 * === BATTERY PACK CONFIG =====================================================
 *============================================================================*/

// Total number of cell modules in the pack. Whether scrutineering is
// enabled does not change this count; it only changes slave behaviour.
#define NUM_MODULES 32U

// Number of series cells monitored per module.
#define NUM_CELLS_PER_MODULE 13U

// Number of ADBMS1818 ICs daisy-chained on the isoSPI bus.
#define SLAVE_NUM_DEVICES 2U

// Slaveboard hardware revision, starting from 1.
// e.g. a value of 1 corresponds to slavebord rev 2.0. 
// a value of 2 corresponds to slaveboard rev 2.1
// a value of 3 for rev 2.2, and so on...
#define SLAVEBOARD_REV 1


/*==============================================================================
 * === CELL SAFETY LIMITS (Molicel M35A) ====================================
 *============================================================================*/

// --- Absolute limits ---
#define MAX_VOLTAGE_mV 4200U
#define MIN_VOLTAGE_mV 2750U

#define MAX_TEMP_degC 60U
#define MIN_TEMP_degC 0U

// --- Warning thresholds ---
#define WARN_HIGH_VOLTAGE_mV 4100U
#define WARN_LOW_VOLTAGE_mV 2850U

#define WARN_HIGH_TEMP_degC 55U


/*==============================================================================
 * === CELL BALANCING ========================================================
 *============================================================================*/

#define MIN_BALANCE_VOLT_DIFF_MV 100U
#define MAX_BALANCE_VOLT_DIFF_MV 300U


/*==============================================================================
 * === CAN INTERFACAE ===========================================================
 *============================================================================*/

// --- Message IDs and periods ---
#define CAN_STATUS_ID                       0x200
#define CAN_STATUS_PERIOD_MS                1000
#define CAN_MODULE_VOLT_SUMMARY_ID          0x201
#define CAN_MODULE_TEMP_SUMMARY_ID          0x202
#define CAN_BALANCE_DATA_ID                 0x203
#define CAN_MODULE_VOLT_DATA_ID_START       0x210
#define CAN_MODULE_TEMP_DATA_ID_START       0x220
#define CAN_MODULE_STATUS_ID_START          0x230

// --- Module-to-frame grouping (derived, do not hand-edit) ---
#define CAN_NUM_MODULES_PER_DATA_GROUP      4U
#define CAN_NUM_DATA_GROUPS                 NUM_MODULES / CAN_NUM_MODULES_PER_DATA_GROUP
#define CAN_NUM_MODULES_PER_STATS_GROUP     8U
#define CAN_NUM_STATS_GROUPS                NUM_MODULES / CAN_NUM_MODULES_PER_STATS_GROUP


/*==============================================================================
 * === ISOSPI INTERFACAE =======================================================
 *============================================================================*/

#define NUM_CONSECUTIVE_COMM_ERR 3
#define CONSECUTIVE_TIMEFRAME_MS 2000


/*==============================================================================
 * === ADBMS1818 SLAVE CHAIN AND REGISTER GEOMETRY ==========================
 *============================================================================*/

// All ADBMS1818 register groups consist of 6 bytes.
#define SLAVE_REG_SIZE_BYTES 6

// Configuration registers.
#define SLAVE_NUM_CONFIG_REG 2

// Cell-voltage registers: 6 groups x 3 modules per group.
#define SLAVE_NUM_VOLT_REG 6
#define SLAVE_NUM_MODULES_PER_VOLT_REG 3

// Temperature (AUX) registers: 3 groups x 3 values x 4 muxed modules.
// (The IC has 4 AUX groups total, but only 3 are wired for thermistors.)
#define SLAVE_NUM_TEMP_REG 3
#define SLAVE_NUM_VAL_PER_TEMP_REG 3
#define SLAVE_NUM_MODULES_PER_TEMP_VAL 4

// Balancing (DCC) registers: 2 groups x 12 nibbles x 4 modules per nibble.
#define SLAVE_NUM_BAL_REG 2
#define SLAVE_NUM_VAL_PER_BAL_REG 12
#define SLAVE_NUM_MODULES_PER_BAL_VAL 4

// Safety timeout threshold for all blocking slave operations.
#define SLAVE_TIMEOUT_MS 100U // ms


/*==============================================================================
 * === ADBMS1818 ADC / CONFIGURATION REGISTER PARAMETERS ====================
 * Bit-level defaults for ADBMS1818 command parameters
 *============================================================================*/

// Keep voltage references powered between ADC reads. Speeds up reads at the
// cost of higher power consumption.
//   0 = references shut down after conversions
//   1 = references remain powered up until watchdog timeout
#define REFON 1

// ADCOPT selects the ADC mode together with MD, but lives in the CFG register.
#define ADCOPT 0

// Under-voltage threshold for the ADBMS1818 internal UV flag:
// (2.7V / (16 * 0.0001V)) - 1 = 1687
#define VUV 1687U

// Over-voltage threshold for the ADBMS1818 internal OV flag:
// (4.2V / (16 * 0.0001V)) - 1 = 2624
// NOTE: VUV/VOV only drive the UV/OV status bits; pack faults use the
// limits defined in the "CELL SAFETY LIMITS" section.
#define VOV 2624U

// Discharge permitted during cell measurement.
//   0 = discharge not permitted, 1 = discharge permitted
#define DCP 0

// ADC conversion mode (speed). Normal mode.
#define MD MD_7KHZ_3KHZ


/*==============================================================================
 * === TEMPERATURE SENSING ===================================================
 *============================================================================*/

#define THERMISTOR_LUT_TABLE_SIZE 17


/*==============================================================================
 * === FIRMWARE-SPECIFIC SETTINGS =============================================
 *============================================================================*/

// Current logging level - adjust to filter log output. Controls which logging
// macros in logging.h expand to output.
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO


/*==============================================================================
 * === HARDWARE UNIT TESTS AND INTEGRATION TESTS ==============================
 * Set exactly one test to RUN (or none for normal operation). These hold no
 * meaning outside hardware unit tests and integration. UNIT_* verify the
 * hardware works; INT_* verify functionality with the rest of the BMS.
 * All must be SKIP before flashing for real.
 *============================================================================*/

#define RUN 1
#define SKIP 0

#define UNIT_TEST_MCU SKIP // Check if MCU can run code
#define UNIT_TEST_IO SKIP // Check if GPIOs work
#define UNIT_TEST_CAN SKIP // Test if CAN peripheral can send pulses
#define UNIT_TEST_ISOSPI SKIP // Test if IsoSPI peripheral can send pulses
#define INT_TEST_SLAVE SKIP // Test basic communication with slaveboards
#define INT_TEST_SLAVE_BAL_VOLT SKIP // Characterize measured voltage drop when balancing is enabled
#define INT_TEST_SLAVE_BAL_SCRUT SKIP // Test if scrutineering and balancing modes can be enabled/disabled
#define INT_TEST_SLAVE_MUX SKIP // Characterize slaveboard multiplexer behaviour
