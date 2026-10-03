/******************************************************************************
* @file    is31fl3236_driver.h
* @brief   Header file for the IS31FL3236 LED driver IC.
******************************************************************************/

// 

#ifndef IS31FL3236_DRIVER_H
#define IS31FL3236_DRIVER_H

// Includes
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define NUM_CHANNELS 36U

#define IS31FL3236_I2C_ADDRESS 0x78U // 8-bit address 0x3C, left-shifted for STM32 HAL I2C functions


// LED Current Scale Enum
typedef enum
{
    CurrentMax = 0U,     // SL = 00
    CurrentHalf = 1U,    // SL = 01
    CurrentThird = 2U,   // SL = 10
    CurrentQuarter = 3U  // SL = 11
} LEDCurrentScale_t;

// PWM Frequency Enum
typedef enum
{
FREQ_3KHZ = 0U,      // OFS = 0
FREQ_22KHZ = 1U     // OFS = 1
}LED_PWMFrequency_t;

typedef struct
{
    I2C_HandleTypeDef* hi2c;
    uint8_t address;
    GPIO_TypeDef* sdb_port;
    uint16_t sdb_pin;
    uint32_t timeout_ms;
} LEDDriverHandle;

/**
 * @brief Checks whether the chip acknowledges its I2C address.
 * Changes no chip state. Works before Init, including while SDB is low.
 * @param handle Information about is31fl3236 chip, including I2C handle, address, SDB GPIO, and timeout. 
 * @return HAL_OK if the chip ACKed; HAL_ERROR if not; HAL_BUSY or HAL_TIMEOUT on bus faults
 */
HAL_StatusTypeDef LEDDriverIsPresent(const LEDDriverHandle* handle);


/** 
* @brief Initializes the chip for normal operation. Configures PWM frequency, current scale, and enables all channels.
* @param handle Information about is31fl3236 chip, including I2C handle, address, SDB GPIO, and timeout. 
* @param current LED current scale (CurrentMax, CurrentHalf, CurrentThird, CurrentQuarter)
* @param pwm_freq PWM frequency (FREQ_3KHZ, FREQ_22KHZ)
* @return HAL_OK if successful; HAL_ERROR if invalid parameters; HAL_BUSY or HAL_TIMEOUT on bus faults
*/
HAL_StatusTypeDef LEDDriverInit(const LEDDriverHandle* handle,
                                LEDCurrentScale_t current,
                                LED_PWMFrequency_t pwm_freq);
/** 
* @brief Writes a full frame of PWM values to the chip. Each channel's PWM value is 0-255.
* @param handle Information about is31fl3236 chip, including I2C handle, address, SDB GPIO, and timeout. 
* @param pwm Array of 36 PWM values, one per channel
* @return HAL_OK if successful; HAL_BUSY or HAL_TIMEOUT on bus faults
 */
HAL_StatusTypeDef LEDDriverWriteFrame(const LEDDriverHandle* handle,
                                      const uint8_t pwm[NUM_CHANNELS]);

#endif // IS31FL3236_DRIVER_H