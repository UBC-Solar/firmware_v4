/**
 * @file pin_runtime.h
 * @brief Levels of the input pins the FSM checks itself, built on gpio_driver.
 *
 * PinRuntimeRead() gives the level of every polled input pin (PinLevels). HvcMainRun() calls
 * it once per pass and stores the result in g_shared.pins.
 */

#pragma once

// HAL and CubeMX
#include "stm32f1xx_hal.h" // GPIO_PinState

/**
 * @brief Every polled input pin's level, from one PinRuntimeRead().
 */
typedef struct
{
    GPIO_PinState estop;            // GPIO_PIN_SET = ESTOP pressed
    GPIO_PinState imd_fault;        // GPIO_PIN_SET = IMD reports a fault
    GPIO_PinState hv_current_alert; // GPIO_PIN_SET = shunt monitor alert
    GPIO_PinState mst_fault;        // GPIO_PIN_SET = MST fault (also held high at MST startup)
    GPIO_PinState dcdc_active;      // Active-low: GPIO_PIN_RESET = DCDC in use
    GPIO_PinState supp_active;      // Active-low: GPIO_PIN_RESET = supp in use
} PinLevels;

PinLevels PinRuntimeRead(void);
