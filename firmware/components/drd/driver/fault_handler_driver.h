/**
 * @file    fault_handler_driver.h
 * @brief   Fault handling API for UBC Solar DRD board
 *
 * This header declares the function prototypes and data structures for the fault handling module.
 * It provides an interface for detecting, reporting, and recovering from various fault conditions.
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#ifndef FAULT_HANDLER_DRIVER_H
#define FAULT_HANDLER_DRIVER_H

#include "main.h"
#include "stdbool.h"

/**
 * @brief   Flash debug LED for fault handling
 */
void FaultHandlerDriverFlashDebug();

/**
 * @brief   Triggers the emergency stop functionality.
 *
 * @param estop A boolean value indicating whether to engage (true) or disengage (false) the emergency stop.
 */
void FaultHandlerDriverEStop(bool estop);

#endif /* FAULT_HANDLER_DRIVER_H */