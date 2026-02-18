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
#include "debug_io.h"
#include "gpio_driver.h"
#include "stm32f1xx_hal.h"

/* DRIVE STATE TASK */
volatile int g_next_1 = 0;
volatile int g_prev_1 = 0;

void TasksDriveState(void* argument)
{
    (void)argument; // Unused parameter
    static int count = 0;
    for (;;)
    {
        // HAL_Delay(1000) ;
        if (count >= 200000)
        {

            // DEBUG_IO_PRINT("g_next count: %d, g_prev count: %d\n", g_next_1, g_prev_1);
        }
        ++count;
        // Now g_next and g_prev are incremented by interrupts
        // You can add logging or further processing here if needed
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
    case DRIVE_NEXT_PIN:
        g_next_1++;
        break;
    case DRIVE_PREV_PIN:
        g_prev_1++;
        break;
    default:
        // DriveStateInterruptHandler(GPIO_Pin);
        break;
    }
}