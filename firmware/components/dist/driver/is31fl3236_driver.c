#include "is31fl3236_driver.h"

void IS31FL3236_Init(IS31FL3236_HandleTypeDef* handle)
{
    IS31FL3236_Reset(handle);
    IS31FL3236_SetSoftwareShutdown(handle, IS31FL3236_SOFTWARE_SHUTDOWN_DISABLED);
    IS31FL3236_SetChipEnable(handle, IS31FL3236_CHIP_ENABLED);
}

void IS31FL3236_SetChipEnable(IS31FL3236_HandleTypeDef* handle, uint8_t enable_state)
{
    if (handle->Init.Chip_Enable_Signal_Port != NULL)
        HAL_GPIO_WritePin(handle->Init.Chip_Enable_Signal_Port, handle->Init.Chip_Enable_Signal_Pin, enable_state);
}

HAL_StatusTypeDef IS31FL3236_WriteRegister(IS31FL3236_HandleTypeDef* handle, uint8_t register_address, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = register_address;
    buf[1] = value;
    return HAL_I2C_Master_Transmit(handle->Init.I2C_Bus, handle->Init.I2C_Device_Address, buf, 2, handle->Init.I2C_Transmit_Timeout_Milliseconds);
}

void IS31FL3236_Reset(IS31FL3236_HandleTypeDef* handle)
{
    IS31FL3236_WriteRegister(handle, IS31FL3236_REGISTER_RESET, 0x00);
}

void IS31FL3236_Update(IS31FL3236_HandleTypeDef* handle)
{
    IS31FL3236_WriteRegister(handle, IS31FL3236_REGISTER_UPDATE, 0x00);
}

void IS31FL3236_WriteLEDControl(IS31FL3236_HandleTypeDef* handle, uint8_t channel, uint8_t led_current_setting, uint8_t led_state)
{
    if (channel < IS31FL3236_MAX_CHANNELS)
        IS31FL3236_WriteRegister(handle, IS31FL3236_REGISTER_LED_CTRL + channel, (led_current_setting | led_state));
}

void IS31FL3236_WriteGlobalLEDControl(IS31FL3236_HandleTypeDef* handle, uint8_t led_current_setting, uint8_t led_state)
{
    for (uint8_t i = 0; i < IS31FL3236_MAX_CHANNELS; i++)
        IS31FL3236_WriteLEDControl(handle, i, led_current_setting, led_state);
}

void IS31FL3236_WritePWM(IS31FL3236_HandleTypeDef* handle, uint8_t channel, uint8_t pwm_value)
{
    if (channel < IS31FL3236_MAX_CHANNELS)
        IS31FL3236_WriteRegister(handle, IS31FL3236_REGISTER_PWM + channel, pwm_value);
}

void IS31FL3236_SetSoftwareShutdown(IS31FL3236_HandleTypeDef* handle, uint8_t software_shutdown_mode)
{
    IS31FL3236_WriteRegister(handle, IS31FL3236_REGISTER_SHUTDOWN, software_shutdown_mode);
}
