/**
 * @file    tasks.h
 * @brief   FreeRTOS task interface for the UBC Solar STR board.
 */

#ifndef __TASKS_H__
#define __TASKS_H__

/* FUNCTION PROTOTYPES */
/**
 * @brief STR application entry point.
 */
void AppMain(void);

/**
 * @brief FreeRTOS task for updating the steering wheel display.
 * @param argument Task argument provided by the RTOS.
 */
void StartHexDisplayTask(void *argument);

/**
 * @brief FreeRTOS task for polling and transmitting steering wheel outputs.
 * @param argument Task argument provided by the RTOS.
 */
void StartSteeringOutputsTask(void *argument);

#endif /* __TASKS_H__ */
