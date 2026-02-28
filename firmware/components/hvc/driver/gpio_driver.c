#include "gpio_driver.h"

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}