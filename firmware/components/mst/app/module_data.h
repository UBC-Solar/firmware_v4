#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "spi_driver.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Establishes the ADBMS1818 topology and thermistor tables so all later
 * sampling has a valid foundation
 *
 * @param SPI_handle HAL SPI handle used for ADBMS1818 communication
 * @param slaves Array of SLAVE_NUM_DEVICES slave state to initialize
 */
void Module_Init(
	SPI_HandleTypeDef *SPI_handle,
	slave_t slaves[SLAVE_NUM_DEVICES]);

/**
 * @brief Applies pending balancing, mux, and threshold changes to the slaves.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state with config data
 */
void WriteConfigRegisters(slave_t slaves[SLAVE_NUM_DEVICES]);

/**
 * @brief Starts the ADC ahead of the read so fresh voltages are ready on retrieval.
 */
void RequestVoltageMeasurement(void);

/**
 * @brief Reads back the conversion and updates module voltages for safety checks.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state with volt mappings
 * @param pack_modules Array of NUM_MODULES modules to fill with voltages
 * @return Slave_OK on success, otherwise the slave communication error
 */
Slave_Error_t RetrieveVoltageMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);

/**
 * @brief Starts the AUX ADC ahead of the read so fresh temperatures are ready on
 * retrieval.
 */
void RequestTemperatureMeasurement(void);

/**
 * @brief Reads back the conversion and updates module temperatures for safety checks.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state with temp mappings
 * @param pack_modules Array of NUM_MODULES modules to fill with temperatures
 * @return Slave_OK on success, otherwise the slave communication error
 */
Slave_Error_t RetrieveTemperatureMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);

/**
 * @brief Rotates through mux states so a few ADC channels can cover every module.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state to update
 * @param new_state Requested mux channel; only the 2 LSBs are used
 */
void SetTempMuxState(slave_t slaves[SLAVE_NUM_DEVICES], unsigned new_state);

/**
 * @brief Switches the scrutineering circuitry for compliance testing.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state to update
 * @param enable True to enable scrutineering mode, false to disable
 */
void SetScrutineeringMode(slave_t slaves[SLAVE_NUM_DEVICES], bool enable);
