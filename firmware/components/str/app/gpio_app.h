/**
 * @file    gpio_app.h
 * @brief   GPIO application state interface for UBC Solar STR board
 *
 * This file declares the application functions for tracking current vehicle speed and cruise
 * control set speed from STR board state.
 *
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

#ifndef GPIO_APP_H
#define GPIO_APP_H

/* INCLUDES */
#include <stdint.h>

/* FUNCTION PROTOTYPES */
/**
 * @brief Stores the latest vehicle velocity.
 * @param velocity_kmh Vehicle velocity in km/h.
 * @return Stored vehicle velocity in km/h.
 */
uint32_t VehicleSetVelocity(uint32_t velocity_kmh);

/**
 * @brief Reads the latest stored vehicle velocity.
 * @return Current vehicle velocity in km/h.
 */
uint32_t VehicleGetVelocity(void);

/**
 * @brief Stores the current cruise control set velocity.
 * @param velocity_kmh Cruise set velocity in km/h.
 * @return Stored cruise set velocity in km/h.
 */
uint32_t CruiseSetVelocity(uint32_t velocity_kmh);

/**
 * @brief Reads the current cruise control set velocity.
 * @return Cruise set velocity in km/h.
 */
uint32_t CruiseGetVelocity(void);

#endif // GPIO_APP_H
