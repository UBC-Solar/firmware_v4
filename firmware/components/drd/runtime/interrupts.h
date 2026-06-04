/**
 * @file    interrupts.h
 * @brief   Interrupt Service Routine (ISR) declarations for hardware events
 *
 * This header declares all hardware interrupt service routine prototypes across
 * all of UBC Solar's boards.
 */

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

/* INCLUDES */
#include "drive_state.h"

/* INTERRUPTS FUNCTION PROTOTYPES */
/**
 * @brief GPIO external interrupt callback handler.
 *
 * Handles external GPIO interrupts and dispatches to appropriate handlers.
 * @param GPIO_Pin The pin number that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif //__INTERRUPTS_H__