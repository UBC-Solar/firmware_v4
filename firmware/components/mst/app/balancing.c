#include "balancing.h"
#include "mst_defs.h"
#include "mst_types.h"



void DoBalancing(pack_state_t *pack_state, module_t *pack_modules) {
    if (!pack_state->bits.balancing_enable) {
        return;
    }

    for (int i = 0; i < NUM_MODULES_PER_SLAVE; i++) {
        
    }
}

void PauseAllBalancing(module_t *pack_modules) {
    // TODO:
}

void ResumeAllBalancing(module_t *pack_modules) {
    // TOOD: 
}

void SendBalancingCommand_(uint32_t module_num, bool if_balance) {

}
