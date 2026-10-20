#pragma once

#include "stm32f1xx_hal.h"

/**
 * @brief Sample a GPIO pin level
 *
 * @param port GPIO port instance (for example, GPIOA, GPIOB)
 * @param pin GPIO pin mask (for example, GPIO_PIN_5)
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief Set a GPIO pin to a level
 *
 * @param port GPIO port instance (for example, GPIOA, GPIOB)
 * @param pin GPIO pin mask (for example, GPIO_PIN_5)
 * @param state Output state to write: GPIO_PIN_SET or GPIO_PIN_RESET
 */
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);

/**
 * @brief Toggle a GPIO pin level
 *
 * @param port GPIO port instance (for example, GPIOA, GPIOB)
 * @param pin GPIO pin mask (for example, GPIO_PIN_5)
 */
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin);
