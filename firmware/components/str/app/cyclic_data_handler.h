/**
 * @file    cyclic_data_handler.h
 * @brief   Cyclic Data Handler for the STR Module
 *
 * Declares getters and setters for cyclic datatypes used by the steering wheel.
 * Currently only vehicle speed is tracked for the hex display.
 *
 * @author  Martin W
 * @date    Jul 15 2026
 */

#ifndef CYCLIC_DATA_HANDLER_H
#define CYCLIC_DATA_HANDLER_H

#include <stdint.h>

/* User Defines */
#define MAX_CYCLE_TIME 1000 // Maximum cycle time in milliseconds

/* CYCLIC DATA SETTERS */
void CyclicDataSetSpeed(uint32_t speed);
void CyclicDataSetCanRxTimestamp(uint32_t timestamp_ms);

/* CYCLIC DATA GETTERS */
uint32_t* CyclicDataGetSpeed(void);
uint32_t* CyclicDataGetCanRxTimestamp(void);

#endif // CYCLIC_DATA_HANDLER_H
