#include "can_app.h"
#include "can_driver.h"
#include "diagnostic.h"

MdiMotorCommand g_mdi_motor_command = {0};
volatile bool g_mdi_motor_command_received = false;
volatile uint32_t g_mdi_last_command_tick = 0;

static const CAN_TxHeaderTypeDef mdi_time_since_bootup_header = {
    .StdId = MDI_TIME_SINCE_BOOTUP_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = 4
};

static const CAN_TxHeaderTypeDef mdi_diagnostic_flags_header = {
    .StdId = MDI_DIAGNOSTIC_FLAGS_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = 1
};

void CanAppInit(void)
{
    CanDriverInit();
}

void CanAppSendTimeSinceBootup(void)
{
    static uint32_t time_since_bootup_counter = 0;
    uint8_t data[4];
    uint32_t mailbox;

    data[0] = (uint8_t)(time_since_bootup_counter & 0xFFU);
    data[1] = (uint8_t)((time_since_bootup_counter >> 8) & 0xFFU);
    data[2] = (uint8_t)((time_since_bootup_counter >> 16) & 0xFFU);
    data[3] = (uint8_t)((time_since_bootup_counter >> 24) & 0xFFU);

    HAL_CAN_AddTxMessage(&hcan, &mdi_time_since_bootup_header, data, &mailbox);
    time_since_bootup_counter++;
}

void CanAppSendDiagnosticFlags(void)
{
    uint8_t data[1] = {g_mdi_diagnostic_flags.raw};
    uint32_t mailbox;

    HAL_CAN_AddTxMessage(&hcan, &mdi_diagnostic_flags_header, data, &mailbox);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *can_handle)
{
    uint8_t rx_data[8] = {0};
    CAN_RxHeaderTypeDef rx_header;

    if (HAL_CAN_GetRxMessage(can_handle, CAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK)
    {
        return;
    }

    if (rx_header.IDE != CAN_ID_STD || rx_header.StdId != DRD_MOTOR_COMMAND_CAN_ID)
    {
        return;
    }

    MdiParseMotorCommand(rx_data, &g_mdi_motor_command);
    g_mdi_motor_command_received = true;
    g_mdi_last_command_tick = HAL_GetTick();
}
