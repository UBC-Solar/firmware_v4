/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementations for DRD board application logic
 *
 * This file contains the implementation of all FreeRTOS tasks for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

/* INCLUDES */
#include "tasks.h"
#include "cmsis_os2.h"
#include "iwdg_app.h"
#include "lcd_app.h"
#include "debug_io.h"
#include "cyclic_data_handler.h"
#include "spi.h"
#include "diagnostic.h"

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
        CyclicDataSetSoc(soc);

        osEventFlagsClear(calculate_soc_flagHandle, SOC_CALCULATE_ON);

        osDelay(CALCULATE_SOC_DELAY);
    }
}

/* LCD UPDATE TASK */
void TasksLcdUpdate(void *argument)
{
    LcdAppInit(&hspi1);

    // KPH or MPH
    g_lcd_data.speed_units = LCD_APP_MPH;

    for (;;)
    {
        // Handles clearing the screen
        if (g_lcd_page_change == 1)
        {
            LcdAppChangeScreen();
            g_lcd_page_change = 0;
        }
        LcdAppPageController();
        osDelay(LCD_APP_UPDATE_DELAY);
    }
}

void TasksTimeSinceStartup(void *argument)
{
    for (;;)
    {
        // Transmit DRD heartbeat over CAN
        DiagnosticTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY);
    }
}

void TasksDiagnostic(void *argument)
{
    IwdgAppResetHandle();

    for (;;)
    {
        // Refresh the watchdog timer to prevent reset and transmit diagnostics over CAN
        IwdgAppRefresh(&hiwdg);
        DiagnosticTransmit(false);
        osDelay(DIAGNOSTIC_TASK_DELAY);
    }
}

/* CAN COMMUNICATIONS TASK */
void TasksCanInit(void) {
    CanTasksInit();
}
