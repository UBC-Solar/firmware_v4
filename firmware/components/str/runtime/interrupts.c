#include "interrupts.h"

#include "gpio_driver.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) 
{
    StrInterruptHandler(GPIO_Pin);
}