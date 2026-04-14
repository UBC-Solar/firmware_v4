#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"

void DoBalancing(pack_state_t *pack_state, module_t *pack_modules);
void PauseAllBalancing(module_t *pack_modules);
void ResumeAllBalancing(module_t *pack_modules);