/**
 * @file    fault_handler.h
 * @brief   Fault handling API for UBC Solar DRD board
 *
 * This header declares the function prototypes and data structures for the fault handling module.
 * It provides an interface for detecting, reporting, and recovering from various fault conditions.
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#ifndef FAULT_HANDLER_H
#define FAULT_HANDLER_H

#include <stdint.h>

void FaultHandlerParseMotorFaults(uint8_t* can_rx_data);

void FaultHandlerParseECUFaults(uint8_t* can_rx_data);

void FaultHandlerParseBatteryFaults(uint8_t* can_rx_data);

void FaultHandlerParseTemperatures(uint8_t* can_rx_data);


#endif /* FAULT_HANDLER_H */