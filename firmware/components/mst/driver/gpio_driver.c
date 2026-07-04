#include "gpio_driver.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"
#include "main.h"

/**
 * @brief Read the current logic state of a GPIO pin.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET.
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief Write a logic state to a GPIO pin.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
 * @param state Output state to write: GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}

/**
 * @brief Toggle the logic state of a GPIO pin. ON --> OFF, or OFF --> ON.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
 */
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_PinState state = HAL_GPIO_ReadPin(port, pin);
    HAL_GPIO_WritePin(
        port, 
        pin, 
        (state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET );
}

/**
 * @brief HAL callback invoked on external interrupt line events.
 * @param GPIO_Pin The pin number that triggered the interrupt (for example, GPIO_PIN_5).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    GPIO_PinState level;

    UNUSED(level);
    // Interrupts are triggered on rising or falling edges, so we read the pin state to determine the current level

    // switch (GPIO_Pin) {
    //     case ESTOP_Pin:
    //         level = HAL_GPIO_ReadPin(ESTOP_GPIO_Port, ESTOP_Pin);
    //         if (level == GPIO_PIN_SET) {
    //             // ESTOP is high (rising edge or already high when sampled)
    //         } else {
    //             // ESTOP is low (falling edge or already low when sampled)
    //         }
    //         break;

    //     default:
    //         break;
    // }
}