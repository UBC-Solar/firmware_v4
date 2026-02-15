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
uint8_t ReadBrakePin(GPIO_TypeDef* port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET);
}

uint8_t ReadEcoPin(GPIO_TypeDef* port, uint16_t pin) { 
    return HAL_GPIO_ReadPin(port, pin); 
}

void ToggleLedPin(GPIO_TypeDef* port, uint16_t pin) { 
    HAL_GPIO_TogglePin(port, pin); 
}

void ToggleBrakeLedPin(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}