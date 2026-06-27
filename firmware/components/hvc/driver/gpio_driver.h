#pragma once

#include "stm32f1xx_hal.h"

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
<<<<<<< HEAD
void GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);
=======
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin);
>>>>>>> Cdella778-hvc-fsm
