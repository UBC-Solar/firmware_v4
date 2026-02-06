#ifndef __TASKS_H__
#define __TASKS_H__

/* TASK FUNCTION PROTOTYPES */

/**
 * @brief Drive state task main loop.
 * 
 * Manages the drive system state machine and control logic.
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void drive_state_task(void);

#endif //__TASKS_H__