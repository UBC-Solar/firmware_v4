/**
 * @file gpio_driver.c
 * @brief GPIO pin access and the eFuse FAULT pin interrupts.
 */

#include "gpio_driver.h"

// HAL and CubeMX
#include "main.h" // Pin names

// DIST app
#include "fsm.h" // FSM_EfuseFaultCallback()

/**
 * @brief Read the current logic state of a GPIO pin.
 * @param port GPIO port instance (for example, ESTOP_GPIO_GPIO_Port).
 * @param pin GPIO pin mask (for example, ESTOP_GPIO_Pin).
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET.
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief Write a logic state to a GPIO pin.
 * @param port GPIO port instance (for example, DRD_CTRL_GPIO_Port).
 * @param pin GPIO pin mask (for example, DRD_CTRL_Pin).
 * @param state Output state to write: GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}

/**
 * @brief Toggle the output state of a GPIO pin.
 * @param port GPIO port instance (for example, DEBUG_GPIO_Port).
 * @param pin GPIO pin mask (for example, DEBUG_Pin).
 */
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}

/**
 * @brief Turns the eFuse FAULT pins into falling-edge interrupts. Call once from DIST_Init().
 *
 * The TPS259631 FAULT pin is open-drain and active-low (pulled up on the board), so a falling edge
 * means the eFuse shut its output off.
 * All four pins are on GPIOB (PB12-PB15), which share EXTI15_10_IRQn; its handler is in the
 * USER CODE section of cube stm32f1xx_it.c.
 * Edges are ignored until the FSM arms them (FSM_EfuseFaultCallback()).
 *
 * TODO: set these pins up as EXTI in dist.ioc instead, so cube gpio.c and stm32f1xx_it.c generate
 * this (as the HVC does).
 */
void GPIO_InitEfuseFaultInterrupts(void) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = DRD_FUSE_Pin | MDI_FUSE_Pin | SPARE_CTRL_FUSE_Pin | SPARE_FUSE_Pin;
    gpio_init.Mode = GPIO_MODE_IT_FALLING;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DRD_FUSE_GPIO_Port, &gpio_init);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief HAL callback for the pin interrupts: passes an eFuse FAULT edge on to the FSM.
 *
 * Only the eFuse FAULT pins interrupt (falling edge, GPIO_InitEfuseFaultInterrupts()).
 *
 * Side effects: may set one of the g_shared *_fuse_tripped flags (through FSM_EfuseFaultCallback()).
 *
 * @param GPIO_Pin Pin that triggered the interrupt (for example, DRD_FUSE_Pin).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case DRD_FUSE_Pin:
        case MDI_FUSE_Pin:
        case SPARE_FUSE_Pin:
        case SPARE_CTRL_FUSE_Pin:
            FSM_EfuseFaultCallback(GPIO_Pin);
            break;

        default:
            break;
    }
}
