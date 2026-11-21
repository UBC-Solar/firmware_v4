/**
 * @file gpio_runtime.h
 * @brief Levels of the input pins the FSM and debug output check, built on gpio_driver.
 *
 * GPIO_Runtime_Read() gives the level of every polled input pin (PinLevels).
 * DIST_Main() calls it once per pass and stores the result in g_shared.pins.
 */

#pragma once

// HAL and CubeMX
#include "stm32f1xx_hal.h" // GPIO_PinState

/**
 * @brief Every polled input pin's level, from one GPIO_Runtime_Read().
 *
 * The eFuse FAULT pins are also read here for the debug report; trips are caught by their pin
 * interrupts (gpio_driver.c), not by polling.
 */
typedef struct {
    GPIO_PinState estop;           // Active-low: GPIO_PIN_RESET = ESTOP pressed
    GPIO_PinState mux_status;      // 12 V mux status
    GPIO_PinState drd_fuse;        // Active-low: GPIO_PIN_RESET = DRD eFuse tripped
    GPIO_PinState mdi_fuse;        // Active-low: GPIO_PIN_RESET = MDI eFuse tripped
    GPIO_PinState spare_fuse;      // Active-low: GPIO_PIN_RESET = SPARE eFuse tripped
    GPIO_PinState spare_ctrl_fuse; // Active-low: GPIO_PIN_RESET = SPARE_CTRL eFuse tripped
} PinLevels;

PinLevels GPIO_Runtime_Read(void);
