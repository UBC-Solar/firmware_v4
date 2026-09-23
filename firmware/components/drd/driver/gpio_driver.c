/**
 * @file    drivers.c
 * @brief   Hardware driver implementations for DRD board peripherals
 *
 * This file contains the implementation of all hardware drivers for this board component of UBC
 * Solar firmware.
 */

/* INCLUDES */
#include "gpio_driver.h"

/* DRIVE STATE DRIVERS */
uint8_t ReadBrakePin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin); }

uint8_t ReadEcoPowerPin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin); }

void ToggleLedPin(GPIO_TypeDef* port, uint16_t pin) { HAL_GPIO_TogglePin(port, pin); }

uint8_t ReadHazardPin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin); }

void SetBrakeLedPin(GPIO_TypeDef* port, uint16_t pin, uint8_t brake_on)
{
    if (brake_on)
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET); // LED ON
    else
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET); // LED OFF
}

void GpioDriverToggleDebugLed(void)
{
    HAL_GPIO_TogglePin(DEBUG_LEDA1_PORT, DEBUG_LEDA1_PIN);
}