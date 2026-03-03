/**
 * @file    interrupts.c
 * @brief   Interrupt Service Routines (ISRs) for hardware events
 *
 * This file contains the implementation of all hardware interrupt service routines across
 * all of UBC Solar's boards.
 */

#include "interrupts.h"
#include "drive_state.h"
#include "gpio_driver.h"

/* GPIO INTERRUPT CALLBACKS */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    DriveStateInterruptHandler(GPIO_Pin);
}