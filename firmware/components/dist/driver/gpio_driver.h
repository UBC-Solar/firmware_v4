/**
 * @file gpio_driver.h
 * @brief GPIO pin access and the eFuse FAULT pin interrupts.
 */

#pragma once

// HAL and CubeMX
#include "stm32f1xx_hal.h"

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin);

void GPIO_InitEfuseFaultInterrupts(void);
