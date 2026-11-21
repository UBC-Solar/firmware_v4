/**
 * @file i2c_driver.h
 * @brief I2C1 bus access for the DIST: owns the HAL handle, resets the bus, probes devices.
 *
 * Only the IS31FL3236 LED panel (led_runtime.c) is on this bus.
 * The register writes themselves go through is31fl3236_driver, which takes the handle from
 * I2C_GetHandle().
 */

#pragma once

// Standard library
#include <stdbool.h>
#include <stdint.h>

// HAL and CubeMX
#include "stm32f1xx_hal.h"

void I2C_Init(I2C_HandleTypeDef *_hi2c);
I2C_HandleTypeDef *I2C_GetHandle(void);
void I2C_Reset(void);
bool I2C_IsDeviceReady(uint16_t dev_addr);
