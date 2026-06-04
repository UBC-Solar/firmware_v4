/**
 * @file    can_app.c
 * @brief   MDI CAN application handling.
 */

#include "can_app.h"
#include "can_driver.h"
#include "main.h"

static MdiMotorCommand s_motor_command = {0};
static bool s_motor_command_received = false;
static uint32_t s_last_command_tick = 0;

/**
 * @brief Handles received CAN frames and updates cached motor command state.
 * @param header Pointer to received CAN header metadata.
 * @param data Pointer to received CAN payload bytes.
 */
static void CanAppRxCallback(const CAN_RxHeaderTypeDef *header, const uint8_t *data);

void CanAppInit(void)
{
    CanDriverInit();
    CanDriverRegisterRxCallback(CanAppRxCallback);
    s_motor_command_received = false;
    s_last_command_tick = HAL_GetTick();
}

bool CanAppTryGetMotorCommand(MdiMotorCommand *command)
{
    if (command == NULL)
    {
        return false;
    }

    if (!s_motor_command_received)
    {
        return false;
    }

    *command = s_motor_command;
    s_motor_command_received = false;
    return true;
}

uint32_t CanAppGetLastCommandTick(void)
{
    return s_last_command_tick;
}

static void CanAppRxCallback(const CAN_RxHeaderTypeDef *header, const uint8_t *data)
{
    if (header == NULL || data == NULL)
    {
        return;
    }

    if (header->IDE != CAN_ID_STD || header->StdId != DRD_MOTOR_COMMAND_CAN_ID)
    {
        return;
    }

    MdiParseMotorCommand(data, &s_motor_command);
    s_motor_command_received = true;
    s_last_command_tick = HAL_GetTick();
}
