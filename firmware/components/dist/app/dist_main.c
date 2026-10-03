#include "dist_main.h"
#include "fsm.h"
#include "adc_driver.h"
#include "faulting_runtime.h"
#include "can.h"
#include "can_driver.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Application entry point, called from main() after HAL and peripheral init.
 *
 * Initialises the ADC, fault monitoring and CAN, then enters the FSM loop.
 */
void AppMain(void)
{
    ADC_Driver_Init();
    Fault_Init();

    static const uint16_t can_rx_ids[] = { HVC_FAULT_ID, LV_POWERUP_ID};
    CAN_InitFilterList(&hcan, can_rx_ids, sizeof(can_rx_ids) / sizeof(can_rx_ids[0]));
    CAN_Init(&hcan);

    FSM_Init();

    for (;;)
    {
        FSM_Run();
    }
}
