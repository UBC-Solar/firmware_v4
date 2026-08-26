/**
 * @file    gpio_driver.c
 * @brief   GPIO driver implementation for UBC Solar MDI board
 */

#include "gpio_driver.h"

#include "main.h"

void GpioDriverToggleDebugLed(void) { HAL_GPIO_TogglePin(MC_LED_GPIO_Port, MC_LED_Pin); }
