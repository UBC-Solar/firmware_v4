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

/* DEFINES */
/* Speed unit encoding carried in Motor Command (0x401) bit 34.
 * These values must stay in sync with LCD_APP_MPH / LCD_APP_KPH on DRD. */
#define STR_SPEED_UNITS_MPH 0U
#define STR_SPEED_UNITS_KPH 1U

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
 * @brief Stores the driver-selected speed units received from DRD.
 * @param speed_units STR_SPEED_UNITS_MPH or STR_SPEED_UNITS_KPH.
 * @return Stored speed unit selection.
 */
uint8_t HexAppSetSpeedUnits(uint8_t speed_units);

/**
 * @brief Reads the driver-selected speed units.
 * @return STR_SPEED_UNITS_MPH or STR_SPEED_UNITS_KPH.
 */
uint8_t HexAppGetSpeedUnits(void);

/**
 * @brief Updates the hex display with current speed or dashes if unavailable.
 *
 * Retrieves cyclic speed data, converts it to the driver-selected units, and
 * writes it to the display. If speed data is unavailable or stale, displays
 * dashes instead.
 */
void HexAppUpdate(void);

#endif /* __HEX__APP__H__ */
