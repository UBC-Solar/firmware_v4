/**
 * @file    tick_driver.h
 * @brief   MDI header file for the driver to get the ticks.
 *
 * @author  Shlok Lande
 * @date    Nov 2, 2026
 */
#ifndef __TICK_DRIVER_H__
#define __TICK_DRIVER_H__

#include <stdint.h>

/**
 * @brief Returns milliseconds since boot.
 * @return Current system tick in ms.
 */
uint32_t TickDriverGetMs(void);

#endif 