/**
 * @file    interrupts.c
 * @brief   Interrupt callback implementation for the UBC Solar STR board.
 */

/* INCLUDES */
#include "interrupts.h"

#include "gpio_driver.h"

/* INTERRUPT CALLBACKS */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) 
{
    StrInterruptHandler(GPIO_Pin);
}
