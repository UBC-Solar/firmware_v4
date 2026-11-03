/**
 * @file    tick_driver.c
 * @brief   MDI file for the driver to get the ticks.
 *
 * @author  Shlok Lande
 * @date    Nov 2, 2026
 */
#include "tick_driver.h"
#include "main.h"

uint32_t TickDriverGetMs(void)
{
    return HAL_GetTick();
}