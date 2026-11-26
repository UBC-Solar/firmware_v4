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

static bool s_dac_i2c_error = false;

/* At least 5 us at 72 MHz. Running I2C slower than spec is fine. */
static void MdiI2cHalfBitDelay(void)
{
    for (volatile uint32_t i = 0; i < 100U; i++)
    {
    }
}

/**
 * @brief Frees a DAC stuck holding SDA low, clocks SCL until SDA is released, sends a
 *        STOP, then re-inits the peripheral. Both I2C buses are on GPIOB.
 */
static void MdiI2cBusRecover(I2C_HandleTypeDef* hi2c, uint16_t scl_pin, uint16_t sda_pin)
{
    GPIO_InitTypeDef gpio = {
        .Pin = scl_pin | sda_pin, .Mode = GPIO_MODE_OUTPUT_OD, .Speed = GPIO_SPEED_FREQ_HIGH};

    HAL_I2C_DeInit(hi2c);
    HAL_GPIO_WritePin(GPIOB, scl_pin | sda_pin, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOB, &gpio);

    for (uint8_t i = 0; (i < 9U) && (HAL_GPIO_ReadPin(GPIOB, sda_pin) == GPIO_PIN_RESET); i++)
    {
        HAL_GPIO_WritePin(GPIOB, scl_pin, GPIO_PIN_RESET);
        MdiI2cHalfBitDelay();
        HAL_GPIO_WritePin(GPIOB, scl_pin, GPIO_PIN_SET);
        MdiI2cHalfBitDelay();
    }

    /* stop: set SDA low, then set SDA high while SCL is high*/
    HAL_GPIO_WritePin(GPIOB, sda_pin, GPIO_PIN_RESET);
    MdiI2cHalfBitDelay();
    HAL_GPIO_WritePin(GPIOB, sda_pin, GPIO_PIN_SET);

    HAL_I2C_Init(hi2c);
}

void MdiSetDacVoltage(MdiDacAddr dac_addr, uint16_t voltage_value)
{
    uint8_t i2c_buffer[2] = {0};
    bool is_accel = (dac_addr == MDI_DAC_ACCEL);
    I2C_HandleTypeDef* hi2c = is_accel ? &hi2c2 : &hi2c1;

    if (voltage_value > MDI_MAX_DAC_VALUE)
    {
        voltage_value = MDI_MAX_DAC_VALUE;
    }

    voltage_value = (uint16_t)(voltage_value << 2);
    i2c_buffer[0] = (uint8_t)(voltage_value >> 8);
    i2c_buffer[1] = (uint8_t)(voltage_value & 0xFFU);

    /* No retry; the next motor command rewrites the DAC. */
    if (HAL_I2C_Master_Transmit(hi2c, MDI_DAC7571_WRITE_ADDR, i2c_buffer, 2U, MDI_I2C_TIMEOUT_MS) !=
        HAL_OK)
    {
        s_dac_i2c_error = true;
        MdiI2cBusRecover(hi2c,
                         is_accel ? ACC_SCL_Pin : REG_SCL_Pin,
                         is_accel ? ACC_SDA_Pin : REG_SDA_Pin);
    }
}

bool MdiTakeDacI2cError(void)
{
    bool had_error = s_dac_i2c_error;
    s_dac_i2c_error = false;
    return had_error;
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
