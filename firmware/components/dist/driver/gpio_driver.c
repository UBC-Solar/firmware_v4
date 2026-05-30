#include "gpio_driver.h"
#include "stm32f1xx_hal_gpio.h"
#include "main.h"

/**
 * @brief Read the current logic state of a GPIO pin.
 * @param port GPIO port instance (for example, ).
 * @param pin GPIO pin mask (for example, DIST_CTRL_Pin).
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET.
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief Write a logic state to a GPIO pin.
 * @param port GPIO port instance (for example, DIST_CTRL_GPIO_Port).
 * @param pin GPIO pin mask (for example, DIST_CTRL_Pin).
 * @param state Output state to write: GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}

/**
 * @brief Toggle the logic state of a GPIO pin.
 * @param port GPIO port instance (for example, DIST_CTRL_GPIO_Port).
 * @param pin GPIO pin mask (for example, DIST_CTRL_Pin).
 */
void GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}


void SPARE_MUX_CTRL_Toggle(void) {
    GPIO_TogglePin(SPARE_MUX_CTRL_GPIO_Port, SPARE_MUX_CTRL_Pin);
}

void SPARE_CTRL_Toggle(void) {
    GPIO_TogglePin(SPARE_CTRL_GPIO_Port, SPARE_CTRL_Pin);
}

void DRD_Toggle(void) {
    GPIO_TogglePin(DRD_CTRL_GPIO_Port, DRD_CTRL_Pin);
}

void MDI_CTRL_Toggle(void) {
    GPIO_TogglePin(MDI_CTRL_GPIO_Port, MDI_CTRL_Pin);
}

GPIO_PinState DRD_FUSE_Read(void) {
    return GPIO_Read(DRD_FUSE_GPIO_Port, DRD_FUSE_Pin);
}

GPIO_PinState MDI_FUSE_Read(void) {
    return GPIO_Read(MDI_FUSE_GPIO_Port, MDI_FUSE_Pin);
}

GPIO_PinState SPARE_FUSE_Read(void) {
    return GPIO_Read(SPARE_FUSE_GPIO_Port, SPARE_FUSE_Pin);
}

GPIO_PinState SPARE_CTRL_FUSE_Read(void) {
    return GPIO_Read(SPARE_CTRL_FUSE_GPIO_Port, SPARE_CTRL_FUSE_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        default:
            break;
    }
}