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
#include "lcd_handler.h"
#include "fault_handler.h"
#include "debug_io.h"
#include "cyclic_data_handler.h"
#include "spi.h"
#include "external_lights.h"
#include "diagnostic.h"
#include "lcd_app.h"
#include "car_configs.h"

/* The car config encoding must stay in step with the LCD display encoding */
_Static_assert(CAR_CONFIG_SPEED_KPH == LCD_APP_KPH || CAR_CONFIG_SPEED_KPH == LCD_APP_MPH,
               "CAR_CONFIG_SPEED_KPH must be LCD_APP_KPH (1) or LCD_APP_MPH (0)");

/* DRIVE STATE TASK */
void TasksDriveState(void* argument)
{
    (void)argument; // Unused parameter

    uint32_t motor_controller_count = 0;

    for (;;)
    {
        if ((motor_controller_count % 4) == 0) {
            MotorControlQueryData(); // Motor controller transmit
        }

        osDelay(DRIVE_STATE_FSM_DELAY);
        DriveStateFsmHandler();

        motor_controller_count++;
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

/* EXTERNAL LIGHTS TASK */
void TasksExternalLights(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        ExternalLightsStateMachine();
        osDelay(EXTERNAL_LIGHTS_STATE_MACHINE_DELAY_MS);
    }
}

/* LCD UPDATE TASK */
void TasksLcdUpdate(void *argument)
{
    LcdHandlerInit(&hspi1, car_config_speed_units);

    for (;;)
    {
        LcdHandlerPageController();
        osDelay(LCD_HANDLER_UPDATE_DELAY);
    }
}

/* DRD HEARTBEAT TASK */
void TasksTimeSinceStartup(void *argument)
{
    for (;;)
    {
        // Transmit DRD heartbeat over CAN
        DiagnosticTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY);
    }
}

/* DIAGNOSTIC TASK */
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

// TODO:
/**
Just leaving a comment so its on our minds: in the future it might be worth making a handful of tasks that run at slower frequencies and then place whatever functionality we need within those tasks.
For example, task_100hz, task_1000hz, etc. This means we would save space on the task stacks/context that we are currently spending on a task that only every runs every 100 ms or even 1s.
However, since we have had no issues, it should be fine for now.
*/

/* FAULT LIGHT FLASH TASK */
void TasksFaultLightFlash(void *argument)
{
    for (;;)
    {
        // Implementation for fault light flash task
        FaultHandlerFlashLED();
        osDelay(FAULT_LIGHT_FLASH_DELAY);
    }
}