/**
 * @file    hex_driver.h
 * @brief   AS1115 hex display driver interface for UBC Solar STR board
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

#ifndef __HEX__DRIVER__H__
#define __HEX__DRIVER__H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

/* DEFINES */
#define AS1115_REG_DIGIT0      0x01
#define AS1115_REG_DIGIT1      0x02

/* FUNCTION PROTOTYPES */
/**
 * @brief Initializes the AS1115 display driver.
 * @return True if all configuration writes succeed, false otherwise.
 */
bool HexDisplayInit(void);

/**
 * @brief Writes a byte to an AS1115 register over I2C.
 * @param reg AS1115 register address.
 * @param data Register value to write.
 * @return HAL status from the I2C transmit operation.
 */
HAL_StatusTypeDef HexDisplayWriteReg(uint8_t reg, uint8_t data);

#endif /* __HEX__DRIVER__H__ */
