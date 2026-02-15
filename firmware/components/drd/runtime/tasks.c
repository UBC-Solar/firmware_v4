/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementations for board-specific application logic
 * 
 * This file contains the implementation of all FreeRTOS tasks for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 * 
 * Tasks defined here include the following boards and respective task:
 * - DRD
 *   - TasksDriveState
 * 
 * @author  UBC Solar
 * @date    Feb 4 2026
 */

#include "tasks.h"
#include "cmsis_os2.h"

/* DRIVE STATE TASK */
void TasksDriveState(void *argument) {
    (void) argument;  // Unused parameter
    for(;;)
    {
        osDelay(DRIVE_STATE_FSM_DELAY);
        DriveStateFsmHandler();
    }
}