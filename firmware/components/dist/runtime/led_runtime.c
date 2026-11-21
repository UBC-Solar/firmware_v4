/**
 * @file led_runtime.c
 * @brief The DIST's status LED panel: an IS31FL3236 LED driver on I2C, built on is31fl3236_driver.
 *
 * Every write is blocking (a few hundred us at 100 kHz), from the main loop.
 */

#include "led_runtime.h"

// Standard library
#include <stdint.h>

// HAL and CubeMX
#include "stm32f1xx_hal.h" // HAL_Delay()
#include "main.h"          // Pin names

// Drivers
#include "gpio_driver.h"
#include "i2c_driver.h"
#include "is31fl3236_driver.h"

#define LED_PROBE_ATTEMPTS          3U     // Bus resets to try before giving up on the chip
#define LED_POWER_UP_DELAY_MS       10U    // Wait after enabling the chip, and before setup
#define LED_I2C_TIMEOUT_MS          100U   // Blocking I2C write timeout
#define IS31FL3236_REGISTER_PWM_FREQUENCY 0x4BU // Output frequency register (0x00 = 3 kHz)

static void SetUpChip(uint8_t i2c_address);

static IS31FL3236_HandleTypeDef led_chip;
static bool led_chip_found = false;
static uint32_t led_states = 0U; // Bit n = LED on OUTn is on

// Every possible I2C address: which one answers depends on how the chip's AD pin is wired
static const uint8_t led_address_candidates[] = {
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_GND),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_VCC),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_SCL),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_SDA),
};

/**
 * @brief Enables the LED chip, looks for it at every possible I2C address, and sets it up.
 *
 * Resets the I2C peripheral before each of up to LED_PROBE_ATTEMPTS rounds, to clear a bus left
 * stuck by a failed attempt.
 * Stops at the first address that answers.
 * Call once from DIST_Init(), after I2C_Init().
 *
 * @return true if the chip was found and set up.
 */
bool LED_Init(void) {
    GPIO_Write(LED_TOGGLE_GPIO_Port, LED_TOGGLE_Pin, GPIO_PIN_SET); // Chip enable
    HAL_Delay(LED_POWER_UP_DELAY_MS);

    for (uint32_t attempt = 0U; attempt < LED_PROBE_ATTEMPTS; attempt++) {
        I2C_Reset();

        for (uint32_t i = 0U; i < sizeof(led_address_candidates); i++) {
            if (I2C_IsDeviceReady(led_address_candidates[i])) {
                SetUpChip(led_address_candidates[i]);
                led_chip_found = true;
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Flips one panel LED between fully on and off.
 *
 * Does nothing if LED_Init() did not find the chip.
 *
 * @param out The LED, by its OUT pin.
 */
void LED_Toggle(LedOut out) {
    uint8_t pwm;

    if (!led_chip_found) {
        return;
    }

    led_states ^= (1UL << out);
    pwm = ((led_states >> out) & 1UL) ? 0xFFU : 0x00U;

    // Driver channels are 0-indexed: OUTn is channel n - 1
    IS31FL3236_WritePWM(&led_chip, (uint8_t)(out - 1), pwm);
    IS31FL3236_Update(&led_chip); // Latches the new PWM value
}

/**
 * @brief Sets up the chip found at i2c_address: resets it, turns it on, every LED at max current.
 *
 * The PWM output runs at 3 kHz, out of the audible range. Each LED's brightness is then its PWM
 * value, which starts at 0 (off).
 *
 * @param i2c_address 8-bit I2C address the chip answered on.
 */
static void SetUpChip(uint8_t i2c_address) {
    led_chip.Init.I2C_Bus = I2C_GetHandle();
    led_chip.Init.I2C_Device_Address = i2c_address;
    led_chip.Init.I2C_Transmit_Timeout_Milliseconds = LED_I2C_TIMEOUT_MS;
    led_chip.Init.Chip_Enable_Signal_Port = LED_TOGGLE_GPIO_Port;
    led_chip.Init.Chip_Enable_Signal_Pin = LED_TOGGLE_Pin;

    HAL_Delay(LED_POWER_UP_DELAY_MS);

    IS31FL3236_Init(&led_chip);
    IS31FL3236_WriteRegister(&led_chip, IS31FL3236_REGISTER_PWM_FREQUENCY, 0x00);
    IS31FL3236_WriteGlobalLEDControl(&led_chip, IS31FL3236_LED_CURRENT_MAX,
                                     IS31FL3236_LED_STATE_ON);
    IS31FL3236_Update(&led_chip);
}
