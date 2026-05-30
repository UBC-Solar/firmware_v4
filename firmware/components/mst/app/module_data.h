#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "stm32f1xx_hal.h"

void Module_Init(
	SPI_HandleTypeDef *SPI_handle,
	slave_t slaves[SLAVE_NUM_DEVICES],
	uint8_t config_val_a[SLAVE_REG_SIZE_BYTES],
	uint8_t config_val_b[SLAVE_REG_SIZE_BYTES]);
void RequestVoltageMeasurement(void);
void RetrieveVoltageMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);
void RequestTemperatureMeasurement(void);
void RetrieveTemperatureMeasurement(slave_t slaves[SLAVE_NUM_DEVICES], module_t pack_modules[NUM_MODULES]);
void ComputePackStatistics(module_t pack_modules[NUM_MODULES], pack_state_t *pack_state);
void SetTempMuxState(slave_t slaves[SLAVE_NUM_DEVICES], unsigned new_state);