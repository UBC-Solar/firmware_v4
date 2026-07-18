/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementation for the UBC Solar STR board.
 */

/* INCLUDES */
#include "tasks.h"
#include "iwdg_app.h"
#include "cmsis_os.h"
#include "can_app.h"
#include "cyclic_data_handler.h"
#include "gpio_app.h"
#include "gpio_driver.h"
#include "hex_driver.h"
#include "hex_app.h"
#include "iwdg_app.h"
#include "diagnostic_app.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

/* DEFINES */
#define STEERING_TASK_DELAY 100
#define HEX_TASK_DELAY 100 // adjust

/* RTOS TASKS */
void StartSteeringOutputsTask(void *argument)
{
    for(;;)
    {
        StrState();
        TransmitDriveControlState();
        osDelay(STEERING_TASK_DELAY);
    }
}

void StartHexDisplayTask(void *argument)
{
    HexDisplayInit();
    for(;;)
    {
        HexAppUpdate();
        osDelay(HEX_TASK_DELAY);
    }
}

void TasksDiagnostic(void *argument)
{
    IwdgAppResetHandle();

    for (;;)
    {
        // Refresh the watchdog PPtimer to prevent reset and transmit diagnostics over CAN
        IwdgAppRefresh(&hiwdg);
        
        osDelay(DIAGNOSTIC_TASK_DELAY);
    }
}

void TimeSinceBootUp(void *argument)
{
    
    for (;;)
    {
        DiagnosticTimeSinceBootup();
        osDelay(TIME_SINCE_BOOTUP_DELAY);
    }
}
