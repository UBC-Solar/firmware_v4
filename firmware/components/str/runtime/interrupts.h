#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

/**
 * @brief GPIO external interrupt callback handler.
 *
 * Handles external GPIO interrupts and dispatches to appropriate handlers.
 * @param GPIO_Pin The pin number that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* __INTERRUPTS_H__ */