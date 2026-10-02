#pragma once

#include "mst_types.h"

/**
 * @brief Checks every module against the voltage/temperature fault and warning
 * limits and modify fault/warning flags accordingly.
 *
 * @param pack_modules Pointer to the first element of the pack modules array
 * @param pack_faults Pointer to the pack-wide latched faults accumulator
 * @param pack_warnings Pointer to the pack-wide warnings accumulator
 */
void CheckForEmergency(module_t *pack_modules, faults_t *pack_faults, warnings_t *pack_warnings);

/**
 * @brief Summarize pack voltage and temperature data. This produces
 * new pack state totals, averages, and min/max values
 *
 * @param pack_modules Array of NUM_MODULES module measurements
 * @param pack_state Pointer to the pack state structure to update
 */
void ComputePackStatistics(module_t pack_modules[NUM_MODULES], pack_state_t *pack_state);
