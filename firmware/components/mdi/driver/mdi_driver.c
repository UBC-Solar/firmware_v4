/**
 * @file    mdi_driver.c
 * @brief   MDI actuator output driver implementation.
 *
 * @author  Martin Wu & Tony Chen
 * @date    May 22, 2026
 */

#include "mdi_driver.h"

#include "main.h"
#include "gpio_driver.h"
#include "i2c.h"

/* At least 5 us at 72 MHz. Running I2C slower than spec is fine. */
static void MdiI2cHalfBitDelay(void)
{
    for (volatile uint32_t i = 0; i < 100U; i++)
    {
    }
}

/**
 * @brief Clocks SCL until the slave releases SDA, sends a STOP, then re-inits the I2C peripheral.
 */
static void MdiI2cBusRecover(I2C_HandleTypeDef* hi2c,
                             GPIO_TypeDef* port,
                             uint16_t scl_pin,
                             uint16_t sda_pin)
{
    GPIO_InitTypeDef gpio = {0};

    HAL_I2C_DeInit(hi2c);

    HAL_GPIO_WritePin(port, scl_pin | sda_pin, GPIO_PIN_SET);
    gpio.Pin = scl_pin | sda_pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &gpio);
    MdiI2cHalfBitDelay();

    for (uint8_t i = 0; (i < 9U) && (HAL_GPIO_ReadPin(port, sda_pin) == GPIO_PIN_RESET); i++)
    {
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
        MdiI2cHalfBitDelay();
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
        MdiI2cHalfBitDelay();
    }

    /* STOP */
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
    MdiI2cHalfBitDelay();
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
    MdiI2cHalfBitDelay();
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    MdiI2cHalfBitDelay();
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    MdiI2cHalfBitDelay();

    HAL_I2C_Init(hi2c);
}

static void MdiDacWrite(I2C_HandleTypeDef* hi2c,
                        GPIO_TypeDef* port,
                        uint16_t scl_pin,
                        uint16_t sda_pin,
                        uint8_t* buffer)
{
    /* No retry; the next motor command rewrites the DAC. */
    if (HAL_I2C_Master_Transmit(hi2c, MDI_DAC7571_WRITE_ADDR, buffer, 2U, MDI_I2C_TIMEOUT_MS) !=
        HAL_OK)
    {
        MdiI2cBusRecover(hi2c, port, scl_pin, sda_pin);
    }
}

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

    if (dac_addr == MDI_DAC_ACCEL)
    {
        MdiDacWrite(&hi2c2, ACC_SCL_GPIO_Port, ACC_SCL_Pin, ACC_SDA_Pin, i2c_buffer);
    }

    if (dac_addr == MDI_DAC_REGEN)
    {
        MdiDacWrite(&hi2c1, REG_SCL_GPIO_Port, REG_SCL_Pin, REG_SDA_Pin, i2c_buffer);
    }
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
    HAL_GPIO_WritePin(PWR_GPIO_Port, PWR_Pin,
                      command->eco_mode_value ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void MdiParseMotorCommand(uint32_t msg_id, const uint8_t *buffer, MdiMotorCommand *command)
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
