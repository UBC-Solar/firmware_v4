#ifndef __TASKS_H__
#define __TASKS_H__

/* INCLUDES */
#include "drive_state.h"

/* TASK FUNCTION PROTOTYPES */

/**
 * @brief Drive state task main loop.
 * 
 * Manages the drive system state machine and control logic.
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksDriveState(void *argument);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif //__TASKS_H__