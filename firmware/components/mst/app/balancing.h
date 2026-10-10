#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"

/**
 * @brief Balances modules sitting above the minimum so all voltages stay 
 * around the same level.
 *
 * @param pack_state Pointer to the pack state with min voltage and enable flag
 * @param pack_modules Array of NUM_MODULES module measurements and flags
 * @param slaves Array of SLAVE_NUM_DEVICES slave state and config registers
 */
void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]);

/**
 * @brief Briefly turns off all balancing via MUTE command. Does not modify MST's
 * state of "which modules should be balanced"
 */
void PauseAllBalancing();

/**
 * @brief Resume balancing via UNMUTE. Does not modify MST's
 * state of "which modules should be balanced"
 */
void ResumeAllBalancing();
#if (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
/**
 * @brief Lets unit/integration tests characterize balancing behaviour independent of
 * the normal voltage-window logic.
 *
 * @param slaves Array of SLAVE_NUM_DEVICES slave state and config registers
 * @param module_enables Array of NUM_MODULES booleans selecting discharge
 */
void Debug_SetBalancingForModules(slave_t slaves[SLAVE_NUM_DEVICES], bool module_enables[NUM_MODULES]);
#endif // (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
