/**
 * @file    gpio_driver.c
 * @brief   GPIO driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the GPIO driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Aug 25 2026
 */

/* INCLUDES */
#include "gpio_driver.h"

#include "main.h"

void GpioDriverToggleDebugLed(void) { HAL_GPIO_TogglePin(DEBUG_LED_1_GPIO_Port, DEBUG_LED_1_Pin); }
