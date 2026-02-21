#ifndef __TASKS_H__
#define __TASKS_H__

/* INCLUDES */
#include "cmsis_os2.h"
#include "drive_state.h"
#include "soc.h"

/* DEFINES */
#define SOC_CALCULATE_ON (0xFF)
#define SOC_CALCULATE_OFF (0x00)
#define CALCULATE_SOC_DELAY (50)

extern osEventFlagsId_t calculate_soc_flagHandle;

/* TASK FUNCTION PROTOTYPES */

/**
 * @brief Drive state task main loop.
 *
 * Manages the drive system state machine and control logic.
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksDriveState(void* argument);
void TasksCalculateSoc(void* argument);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif //__TASKS_H__