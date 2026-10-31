/**
 * @file    can_app.c
 * @brief   MDI CAN application handling.
 */

#include "can_app.h"
#include "can_driver.h"
#include "main.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include "gpio_driver.h"


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

    if (header->IDE != CAN_ID_STD)
    {
        return;
    }

    if(header->StdId != DRD_MOTOR_COMMAND_CAN_ID && header->StdId != STR_DRD_CONTROL_CAN_ID){
        return;
    }

    uint32_t CanId = header -> StdId;

    if (header -> StdId == DRD_MOTOR_COMMAND_CAN_ID){
        MdiParseMotorCommand(CanId, data, &s_motor_command);
        s_motor_command_received = true;
        s_last_command_tick = HAL_GetTick();

    }

    CanMdiLedHandler(CanId, data);


}

void CanMdiLedHandler(uint32_t msg_id, const uint8_t* data)
{
    switch (msg_id)
    {
    case DRD_MOTOR_COMMAND_CAN_ID:{
        uint8_t direction = data[4] & 0x01;
        uint8_t eco_mode = (data[4] >> 1) & 0x01;

        if(eco_mode == 1){
            GpioDriverEnableEcoLeds();
        }
        else {
            GpioDriverDisableEcoLeds();
        }
        if(direction == 1){
            GpioDriverToggleFwdLeds();
        }
        else {
            GpioDriverToggleRevLeds();
        }
        break;
    }
    case STR_DRD_CONTROL_CAN_ID:{
        if((data[0] & 0x01) == 1){
            GpioDriverEnableRegenLed();
        }
        else {
            GpioDriverDisableRegenLed();
        }
        break;
    }
    default:
        break;
    }
}


