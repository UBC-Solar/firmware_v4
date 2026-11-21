/**
 * @file i2c_driver.c
 * @brief I2C1 bus access for the DIST: owns the HAL handle, resets the bus, probes devices.
 */

#include "i2c_driver.h"

// Standard library
#include <stddef.h>

#define I2C_PROBE_TRIALS            1U  // Address attempts per I2C_IsDeviceReady()
#define I2C_PROBE_TIMEOUT_MS        10U // Timeout per I2C_IsDeviceReady()

static I2C_HandleTypeDef *hi2c = NULL;

/**
 * @brief Stores the HAL I2C handle every other function here uses.
 *
 * The peripheral itself is already set up by MX_I2C1_Init() in cube main.c.
 *
 * @param _hi2c HAL I2C handle (hi2c1).
 */
void I2C_Init(I2C_HandleTypeDef *_hi2c) {
    hi2c = _hi2c;
}

/**
 * @brief Gives the HAL I2C handle, for chip drivers that take one (is31fl3236_driver).
 * @return The handle passed to I2C_Init().
 */
I2C_HandleTypeDef *I2C_GetHandle(void) {
    return hi2c;
}

/**
 * @brief Turns the I2C peripheral off and on again, to clear a stuck bus or peripheral state.
 */
void I2C_Reset(void) {
    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}

/**
 * @brief Checks whether a device answers (ACKs) at an address.
 *
 * Blocking, up to I2C_PROBE_TIMEOUT_MS.
 *
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @return true if the device answered.
 */
bool I2C_IsDeviceReady(uint16_t dev_addr) {
    return HAL_I2C_IsDeviceReady(hi2c, dev_addr, I2C_PROBE_TRIALS, I2C_PROBE_TIMEOUT_MS) == HAL_OK;
}
