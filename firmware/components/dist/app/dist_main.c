#include "dist_main.h"
#include "fsm.h"
#include "led_runtime.h"
#include "adc_driver.h"
#include "faulting_runtime.h"
#include "can.h"
#include "can_driver.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Application entry point, called from main() after HAL and peripheral init.
 *
 * Initialises optional hardware (LED driver), sets up CAN, then enters the
 * FSM loop. The LED driver is treated as optional — the FSM degrades gracefully
 * if the IS31FL3236A is not found on the bus.
 */
void AppMain(void)
{
    uint8_t led_ready = LED_Driver_Init();
    ADC_Driver_Init();
    Fault_Init();

    static const uint16_t can_rx_ids[] = { HVC_FAULT_ID, LV_POWERUP_ID};
    CAN_InitFilterList(&hcan, can_rx_ids, sizeof(can_rx_ids) / sizeof(can_rx_ids[0]));
    CAN_Init(&hcan);

    FSM_Init(led_ready);

    for (;;)
    {
        FSM_Run();
    }
}
