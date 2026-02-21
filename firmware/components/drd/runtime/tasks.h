#ifndef __TASKS_H__
#define __TASKS_H__

#define DEFAULT_TASK_DELAY 			  100
#define TIME_SINCE_STARTUP_TASK_DELAY 1000

/* TASK FUNCTION PROTOTYPES */

/**
 * @brief Drive state task main loop.
 *
 * Manages the drive system state machine and control logic.
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksDriveState(void);

/**
 * @brief LCD update task main loop
 *
 * Initialilzes the LCD Driver and manages the LCD pages switch logic
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksLcdUpdate(void *argument);

void TasksDiagnostic(void *argument);

void TasksTimeSinceStartup(void *argument);

#endif //__TASKS_H__