#pragma once

#include "mst_types.h"

void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings);

void ComputePackStatistics(module_t pack_modules[NUM_MODULES], pack_state_t *pack_state);