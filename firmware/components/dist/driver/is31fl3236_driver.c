/******************************************************************************
 * @file    is31fl3236_driver.c
 * @brief   Register-level I2C driver for the IS31FL3236A 36-channel LED driver.
 ******************************************************************************/

// INCLUDES 
#include "is31fl3236_driver.h"

#include <string.h>


// Register map (datasheet Table 2)
#define REG_SHUTDOWN      0x00U
#define REG_PWM_BASE      0x01U // OUT1..OUT36 -> 0x01..0x24
#define REG_UPDATE        0x25U
#define REG_LED_CTRL_BASE 0x26U // OUT1..OUT36 -> 0x26..0x49
#define REG_PWM_FREQ      0x4BU
#define REG_RESET         0x4FU

// Register values (datasheet Tables 3-7)
#define SHUTDOWN_NORMAL_OPERATION 0x01U // reg 0x00 bit 0 (SSD): 0 = shutdown, 1 = normal operation
#define LED_CTRL_OUT_ON           0x01U // reg 0x26-0x49 bit 0: 1 = LED on
#define LED_CTRL_SL_SHIFT         1U    // reg 0x26-0x49 bits 2:1: output current scale (SL)
#define UPDATE_LATCH              0x00U // writing 0x00 to 0x25 applies staged PWM + LED control
#define RESET_ALL_REGISTERS       0x00U // writing 0x00 to 0x4F resets every register

#define SDB_SETTLE_MS     10U // Time in ms for SDB to rise and the chip to be ready for I2C after a reset
#define IS_PRESENT_TRIALS 3U  // Attemps to check for the chip before giving up

// WriteFrame relies on auto-increment landing on the Update register right after OUT36
_Static_assert(REG_PWM_BASE + NUM_CHANNELS == REG_UPDATE,
               "Frame write needs REG_UPDATE immediately after the PWM block");

/* PRIVATE FUNCTION PROTOTYPES */

static HAL_StatusTypeDef LEDDriverWriteMultiRegister(const LEDDriverHandle* handle, uint8_t start_reg, uint8_t* data, uint16_t length);
static HAL_StatusTypeDef LEDDriverWriteRegister(const LEDDriverHandle* handle, uint8_t reg, uint8_t value);

/* PUBLIC FUNCTIONS */

HAL_StatusTypeDef LEDDriverIsPresent(const LEDDriverHandle* handle)
{
    return HAL_I2C_IsDeviceReady(handle->hi2c, handle->address, IS_PRESENT_TRIALS, handle->timeout_ms);
}

HAL_StatusTypeDef LEDDriverInit(const LEDDriverHandle* handle, LEDCurrentScale_t current, LED_PWMFrequency_t pwm_freq) {
   
    HAL_StatusTypeDef status;
    uint8_t led_ctrl_value;
    uint8_t led_ctrl[NUM_CHANNELS];
    uint8_t pwm_off[NUM_CHANNELS] = {0};

    // Validate parameters
if ((current > CurrentQuarter) || (pwm_freq > FREQ_22KHZ))    
    {
        return HAL_ERROR;
    }

    // 1. Release hardware shutdown
    HAL_GPIO_WritePin(handle->sdb_port, handle->sdb_pin, GPIO_PIN_SET);
    HAL_Delay(SDB_SETTLE_MS);

    // 2. Start from a known state: the chip keeps its registers through an MCU reset
    status = LEDDriverWriteRegister(handle, REG_RESET, RESET_ALL_REGISTERS);
    if (status != HAL_OK)
    {
        return status;
    }

    // 3. PWM frequency
    status = LEDDriverWriteRegister(handle, REG_PWM_FREQ, (uint8_t)pwm_freq);
    if (status != HAL_OK)
    {
        return status;
    }

    // 4. Every channel on at the chosen current. Staged until the Update write in step 5.
    led_ctrl_value = (uint8_t)(((uint8_t)current << LED_CTRL_SL_SHIFT) | LED_CTRL_OUT_ON);
    memset(led_ctrl, led_ctrl_value, sizeof(led_ctrl));
    status = LEDDriverWriteMultiRegister(handle, REG_LED_CTRL_BASE, led_ctrl, sizeof(led_ctrl));
    if (status != HAL_OK)
    {
        return status;
    }

    // 5. Every PWM duty cycle to 0, and apply step 4 (WriteFrame ends with the Update latch)
    status = LEDDriverWriteFrame(handle, pwm_off);
    if (status != HAL_OK)
    {
        return status;
    }

    // 6. Leave software shutdown last, so outputs only go live once fully configured
    return LEDDriverWriteRegister(handle, REG_SHUTDOWN, SHUTDOWN_NORMAL_OPERATION);
}

HAL_StatusTypeDef LEDDriverWriteFrame(const LEDDriverHandle* handle, const uint8_t pwm[NUM_CHANNELS])
{
    uint8_t frame[NUM_CHANNELS + 1U]; 

    memcpy(frame, pwm, NUM_CHANNELS);   // Copy the caller's PWM values into the frame buffer
    frame[NUM_CHANNELS] = UPDATE_LATCH; // auto-increment reaches 0x25 after OUT36

    return LEDDriverWriteMultiRegister(handle, REG_PWM_BASE, frame, sizeof(frame));
}

/* PRIVATE FUNCTIONS */

/**
 * @brief Writes consecutive registers in one I2C transaction, starting at start_reg.
 *
 * The chip advances its register pointer after each byte, so data[i] lands in register
 * start_reg + i. Blocks until the transfer finishes or handle->timeout_ms elapses.
 *
 * @param handle The chip to write to
 * @param start_reg Address of the first register to write
 * @param data Bytes to write
 * @param length Number of bytes in data
 * @return HAL status of the I2C transfer
 */
static HAL_StatusTypeDef LEDDriverWriteMultiRegister(const LEDDriverHandle* handle, uint8_t start_reg, uint8_t* data, uint16_t length) 
{

    return HAL_I2C_Mem_Write(handle->hi2c, handle->address, start_reg, I2C_MEMADD_SIZE_8BIT, data, length, handle->timeout_ms);
}

/**
 * @brief Writes a single register.
 *
 * Passes the address of the value parameter as the data buffer. That is only safe because the
 * transfer blocks: value stays on the stack until this function returns.
 *
 * @param handle The chip to write to
 * @param reg Address of the register to write
 * @param value Byte to write
 * @return HAL status of the I2C transfer
 */
static HAL_StatusTypeDef LEDDriverWriteRegister(const LEDDriverHandle* handle, uint8_t reg, uint8_t value)
{
    return LEDDriverWriteMultiRegister(handle, reg, &value, 1U);
}