#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "spi_driver.h"
#include "stm32f1xx_hal.h"

void Module_Init(
	SPI_HandleTypeDef *SPI_handle,
	slave_t slaves[SLAVE_NUM_DEVICES]);
void WriteConfigRegisters(slave_t slaves[SLAVE_NUM_DEVICES]);

void RequestVoltageMeasurement(void);
Slave_Error_t RetrieveVoltageMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);

void RequestTemperatureMeasurement(void);
Slave_Error_t RetrieveTemperatureMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);

void SetTempMuxState(slave_t slaves[SLAVE_NUM_DEVICES], unsigned new_state);
void SetScrutineeringMode(slave_t slaves[SLAVE_NUM_DEVICES], bool enable);
