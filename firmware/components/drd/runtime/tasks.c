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

/* DRIVE STATE TASK */
void TasksDriveState(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        osDelay(DRIVE_STATE_FSM_DELAY);
        DriveStateFsmHandler();
    }
}

/* SOC CALCULATION TASK */
void TasksCalculateSoc(void *argument)
{
    (void)argument; // Unused parameter

    // Wait for first pack voltage message to init SoC
    osEventFlagsWait(
        calculate_soc_flagHandle,
        SOC_CALCULATE_ON,
        osFlagsWaitAny | osFlagsNoClear,
        osWaitForever
    );

    // TODO: Might be unsafe if load instruction was not done before calling this function
    SocInitSoc(g_total_pack_voltage_soc + 0.5f); // voltage is rounded to nearest integer. Done by adding 0.5

    // Clear flag for next wait
    osEventFlagsClear(calculate_soc_flagHandle, SOC_CALCULATE_ON);

    for (;;)
    {
        // Wait until calculate SOC flag is set
        osEventFlagsWait(
            calculate_soc_flagHandle,
            SOC_CALCULATE_ON,
            osFlagsWaitAny | osFlagsNoClear,
            osWaitForever
        );

        SocPredictThenUpdate(g_total_pack_voltage_soc, g_pack_current_soc, SOC_TIME_STEP);
        uint8_t soc = (uint8_t)(SocGetSoc() * 100);
        // set_cyclic_soc(soc); TODO: Cyclic Data

        osEventFlagsClear(calculate_soc_flagHandle, SOC_CALCULATE_ON);

        osDelay(CALCULATE_SOC_DELAY);
    }
}