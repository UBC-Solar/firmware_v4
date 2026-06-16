#include "dist_main.h"
#include "fsm.h"
#include "i2c_driver.h"
#include "adc_driver.h"
#include "fault_handler.h"
#include "can.h"
#include "can_driver.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Configure bxCAN receive filters and start the CAN peripheral.
 *
 * Defines which CAN IDs this board accepts. Filter configuration is
 * application-level — it lives here rather than in the driver because the
 * driver has no knowledge of the application's message set.
 */
static void init_can(void)
{
    static const uint16_t can_rx_ids[] = { 0x323U };
    CAN_InitFilterList(&hcan, can_rx_ids, sizeof(can_rx_ids) / sizeof(can_rx_ids[0]));
    CAN_Init(&hcan);
}

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
    init_can();
    FSM_Init(led_ready);

    for (;;)
    {
        FSM_Run();
    }
}
