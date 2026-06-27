#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"

void DoBalancing(pack_state_t *pack_state, module_t pack_modules[NUM_MODULES], slave_t slaves[SLAVE_NUM_DEVICES]);
void PauseAllBalancing();
void ResumeAllBalancing();
#if (INT_TEST_SLAVE == RUN)
void Debug_DoBalancing(slave_t slaves[SLAVE_NUM_DEVICES], bool enable);
#endif // UNIT_TEST_ISOSPI
