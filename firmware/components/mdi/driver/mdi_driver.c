#include "mdi_driver.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

void MdiSetDacVoltage(MdiDacAddr dac_addr, uint16_t voltage_value)
{
    uint8_t i2c_buffer[2] = {0};

    if (voltage_value > MDI_MAX_DAC_VALUE)
    {
        voltage_value = MDI_MAX_DAC_VALUE;
    }

    voltage_value = (uint16_t)(voltage_value << 2);
    i2c_buffer[0] = (uint8_t)(voltage_value >> 8);
    i2c_buffer[1] = (uint8_t)(voltage_value & 0xFFU);

    HAL_I2C_Master_Transmit(&hi2c2, (uint16_t)dac_addr, i2c_buffer, sizeof(i2c_buffer), HAL_MAX_DELAY);
}

void MdiSetMotorCommand(const MdiMotorCommand *command)
{
    if (command == NULL)
    {
        return;
    }

    MdiSetDacVoltage(MDI_DAC_ACCEL, command->accel_DAC_value);
    MdiSetDacVoltage(MDI_DAC_REGEN, command->regen_DAC_value);

    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin,
                      command->direction_value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ECO_MCU_GPIO_Port, ECO_MCU_Pin,
                      command->eco_mode_value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void MdiParseMotorCommand(const uint8_t *buffer, MdiMotorCommand *command)
{
    if (buffer == NULL || command == NULL)
    {
        return;
    }

    command->accel_DAC_value = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
    command->regen_DAC_value = (uint16_t)buffer[2] | ((uint16_t)buffer[3] << 8);
    command->direction_value = ((buffer[4] & (1U << 0)) != 0U);
    command->eco_mode_value = ((buffer[4] & (1U << 1)) != 0U);
}

void MdiStopMotor(void)
{
    MdiMotorCommand command = {0};

    command.direction_value = MDI_DIRECTION_FORWARD;
    command.eco_mode_value = MDI_ECO_MODE;

    MdiSetMotorCommand(&command);
}
