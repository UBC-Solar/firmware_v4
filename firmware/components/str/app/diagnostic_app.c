#include "diagnostic_app.h"

#include "CAN_comms.h"
#include "can_driver.h"
#include "gpio_driver.h"

static uint32_t g_time_since_bootup = 0U;

void DiagnosticTimeSinceBootup()
{
    g_time_since_bootup++;
    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };
    GpioDriverToggleDebugLed();
    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
}

void DiagnosticSetWatchdogReset(bool reset)
{
    (void)reset;
}
