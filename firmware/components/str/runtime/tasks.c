/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementation for the UBC Solar STR board.
 */

/* INCLUDES */
#include "tasks.h"

#include "cmsis_os.h"
#include "can_app.h"
#include "gpio_app.h"
#include "gpio_driver.h"
#include "hex_driver.h"
#include "hex_app.h"
#include "main.h"

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
        HexDisplayWriteDecimal((uint8_t)ReadCurrentVelocity());
        osDelay(HEX_TASK_DELAY);
    }
}
