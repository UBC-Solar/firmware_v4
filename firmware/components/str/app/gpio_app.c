/**
 * @file    gpio_app.c
 * @brief   GPIO application state implementation for UBC Solar STR board
 *
 * This file implements STR application state updates for GPIO inputs, vehicle speed, and cruise
 * control set speed.
 *
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

/* INCLUDES */
#include "gpio_app.h"

/* PRIVATE VARIABLES */
static volatile uint32_t s_current_velocity_kmh = 0U;
static volatile uint32_t s_cruise_set_velocity_kmh = 0U;

uint32_t GPIOAppSetVehicleVelocity (uint32_t velocity)
{
    s_current_velocity_kmh = velocity;
    return s_current_velocity_kmh;
}

uint32_t GPIOAppGetVehicleVelocity (void)
{
    return s_current_velocity_kmh;
}

uint32_t GPIOAppSetCruiseVelocity (uint32_t velocity)
{
    s_cruise_set_velocity_kmh = velocity;
    return s_cruise_set_velocity_kmh;
}

uint32_t GPIOAppGetCruiseVelocity (void)
{
    return s_cruise_set_velocity_kmh;
}
