/**
 * @file    tasks.h
 * @brief   FreeRTOS task declarations for TEL board application logic
 *
 * This header declares all FreeRTOS task prototypes for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

#ifndef __TASKS_H__
#define __TASKS_H__

/* DEFINES */
#define TIME_SINCE_STARTUP_TASK_DELAY 1000
#define IMU_TASK_DELAY 100
#define DIAGNOSTICS_TASK_DELAY 100

/**
 * @brief   IMU task function
 * @param   argument: Not used
 * @retval  None
 */
void TasksIMU(void* argument);

/**
 * @brief   Diagnostics task function
 * @param   argument: Not used
 * @retval  None
 */
void TasksDiagnostics(void* argument);

/**
 * @brief   Time since startup task function
 * @param   argument: Not used
 * @retval  None
 */
void TimeSinceStartup(void* argument);



#endif /* __TASKS_H__ */