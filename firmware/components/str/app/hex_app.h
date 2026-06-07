/**
 * @file    hex_app.h
 * @brief   Hex display application interface for the UBC Solar STR board.
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

#endif /* __HEX__APP__H__ */
