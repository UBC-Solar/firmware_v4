/**
 * @file led_runtime.h
 * @brief The DIST's status LED panel: an IS31FL3236 LED driver on I2C, built on is31fl3236_driver.
 *
 * LED_Init() finds the chip and sets it up; LED_Toggle() flips one LED.
 * The panel is optional: if LED_Init() does not find it, LED_Toggle() does nothing.
 */

#pragma once

// Standard library
#include <stdbool.h>

/**
 * @brief Each panel LED, by the IS31FL3236 OUT pin it is wired to (1-indexed, as on the silk).
 *
 * Only LED_OUT_DIST_FAULT is used so far.
 * TODO: check this mapping against the board schematic.
 */
typedef enum {
    LED_OUT_HLIM        = 3,
    LED_OUT_LLIM        = 4,
    LED_OUT_NEG         = 5,
    LED_OUT_DIST_FAULT  = 6,
    LED_OUT_ESTOP       = 7,
    LED_OUT_CAN_FAULT   = 8,
    LED_OUT_POS         = 9,
    LED_OUT_MOTOR_PC    = 10,
    LED_OUT_MPPT_PC     = 11,
    LED_OUT_IMD         = 12,
    LED_OUT_MPPT        = 13,
    LED_OUT_DCH_ON      = 14,
    LED_OUT_DCH_OFF     = 15,
    LED_OUT_FANS        = 17,
    LED_OUT_SUPP_ACTIVE = 18,
    LED_OUT_SUPP_LOW    = 19,
    LED_OUT_DCDC_ACTIVE = 20,
    LED_OUT_CONTACTOR   = 21,
} LedOut;

bool LED_Init(void);
void LED_Toggle(LedOut out);
