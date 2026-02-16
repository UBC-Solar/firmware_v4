/**
 * @file    interrupts.c
 * @brief   Interrupt Service Routines (ISRs) for board-specific hardware events
 *
 * This file contains the implementation of all hardware interrupt service routines specific to this
 * board component of UBC Solar firmware. ISRs provide immediate response to hardware events and
 * peripherals that require real-time handling with minimal latency.
 *
 * Interrupt handlers defined here include the following boards:
 * - DRD
 *
 * @author  UBC Solar
 * @date    Feb 4 2026
 */

#include "interrupts.h"
#include "drive_state.h"

/* GPIO INTERRUPT CALLBACKS */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { DriveStateInterruptHandler(GPIO_Pin); }