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

#include "gpio_driver.h"

/* PRIVATE VARIABLES */
static volatile uint32_t s_current_velocity_kmh = 40U;
static volatile uint32_t s_cruise_set_velocity_kmh = 0U;

/* GPIO STATE */
void StrState(void)
{
    LightState();
}

uint32_t VehicleSetVelocity(uint32_t velocity)
{
    s_current_velocity_kmh = velocity;
    return s_current_velocity_kmh;
}

uint32_t VehicleGetVelocity(void)
{
    return s_current_velocity_kmh;
}

uint32_t CruiseSetVelocity(uint32_t velocity)
{
    s_cruise_set_velocity_kmh = velocity;
    return s_cruise_set_velocity_kmh;
}

uint32_t CruiseGetVelocity(void)
{
    return s_cruise_set_velocity_kmh;
}
