#include "tasks.h"
#include "drive_state.h"

/* DRIVE STATE TASK */
void TasksDriveState(void) {
    for(;;)
    {
        osDelay(DRIVE_STATE_FSM_DELAY);
        drive_state_fms_handler();
    }
}