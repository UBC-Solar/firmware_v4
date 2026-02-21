/**
 * @file    drivers.c
 * @brief   Hardware driver implementations for DRD board peripherals
 *
 * This file contains the implementation of all hardware drivers for this board component of UBC
 * Solar firmware. Drivers provide abstraction layers between high-level application code and
 * low-level hardware peripherals, enabling consistent interfaces and easier code maintenance.
 *
 * Drivers implemented here include:
 * - DRD board-specific peripherals
 *
 * @author  UBC Solar
 * @date    Feb 7 2026
 */

#include "gpio_driver.h"

/* FUNCTION DECLARATIONS */

/* DRIVE STATE DRIVERS */
/**
 * @brief Reads the state of the brake input pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @return Pin state (0 or 1).
 */
uint8_t ReadBrakePin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin); }

/**
 * @brief Reads the state of the eco power mode input pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @return Pin state (0 or 1).
 */
uint8_t ReadEcoPowerPin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin); }

/**
 * @brief Toggles the state of an LED pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 */
void ToggleLedPin(GPIO_TypeDef* port, uint16_t pin) { HAL_GPIO_TogglePin(port, pin); }

void SetBrakeLedPin(GPIO_TypeDef* port, uint16_t pin, uint8_t brake_on)
{
    if (brake_on)
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET); // LED ON
    else
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET); // LED OFF
}