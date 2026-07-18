/**
 * @file    hex_app.h
 * @brief   Hex display application interface for UBC Solar STR board
 *
 * This file declares the application functions for parsing vehicle speed data and writing it
 * to the steering wheel hex display.
 *
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

#ifndef __HEX__APP__H__
#define __HEX__APP__H__

/* INCLUDES */
#include "hex_driver.h"

#include <stdint.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief Parses wheel speed data from an MDU velocity CAN frame.
 * @param data Pointer to the CAN payload bytes.
 */
void SteeringVelocityCanMsgHandler(uint8_t* data);

/**
 * @brief Writes a decimal value to the two-digit steering wheel display.
 * @param num Value to display. Values above the display range are clamped.
 */
void HexDisplayWriteDecimal(uint8_t num);

/**
 * @brief Writes "--" to the two-digit steering wheel display.
 *
 * Used when cyclic speed data is missing or stale so a real 0 km/h reading
 * is not confused with no data.
 */
void HexDisplayWriteDashes(void);

/**
 * @brief Updates the hex display with current speed or dashes if unavailable.
 *
 * Retrieves cyclic speed data and writes it to the display. If speed data is
 * unavailable or stale, displays dashes instead.
 */
void HexAppUpdate(void);

#endif /* __HEX__APP__H__ */
