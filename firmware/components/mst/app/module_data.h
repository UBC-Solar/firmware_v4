#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"

void RequestVoltageMeasurement(void);
void RetrieveVoltageMeasurement(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES]);
void RequestTemperatureMeasurement(void);
void RetrieveTemperatureMeasurement(slave_t slaves[NUM_SLAVES], module_t pack_modules[NUM_MODULES]);

