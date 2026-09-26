#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"

void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]);
void PauseAllBalancing();
void ResumeAllBalancing();
#if (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
void Debug_SetBalancingForModules(slave_t slaves[SLAVE_NUM_DEVICES], bool module_enables[NUM_MODULES]);
#endif // (INT_TEST_SLAVE == RUN || INT_TEST_SLAVE_BAL_VOLT == RUN)
