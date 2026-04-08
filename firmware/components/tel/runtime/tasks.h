/**
 * @file    tasks.h
 * @brief   FreeRTOS task declarations for DRD board application logic
 *
 * This header declares all FreeRTOS task prototypes for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

#ifndef __TASKS_H__
#define __TASKS_H__

void TasksDriveState(void* argument);

#endif /* __TASKS_H__ */