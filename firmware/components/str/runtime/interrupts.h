/**
 * @file    interrupts.h
 * @brief   Interrupt callback interface for the UBC Solar STR board.
 */

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

/* INCLUDES */
#include <stdint.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief GPIO external interrupt callback handler.
 *
 * Handles external GPIO interrupts and dispatches to appropriate handlers.
 * @param GPIO_Pin The pin number that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* __INTERRUPTS_H__ */
