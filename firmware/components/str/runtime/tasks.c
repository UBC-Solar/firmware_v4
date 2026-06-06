#include "tasks.h"

#include "cmsis_os.h"
#include "can_app.h"
#include "gpio_app.h"
#include "gpio_driver.h"
#include "hex_driver.h"
#include "hex_app.h"
#include "main.h"

#define STEERING_TASK_DELAY 100
#define HEX_TASK_DELAY 100 // adjust

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
    uint32_t display_velocity_kmh = 0U;

    HexDisplayInit();

    for(;;)
    {
        if (gpio_pin_state.cruise_state.cruise_en)
        {
            display_velocity_kmh = ReadCruiseSetVelocity();
        }
        else
        {
            display_velocity_kmh = ReadCurrentVelocity();
        }

        HexDisplayWriteDecimal((uint8_t)display_velocity_kmh);
        osDelay(HEX_TASK_DELAY);
    }
}