/**
 * @file    drivers.c
 * @brief   Hardware driver implementations for DRD board peripherals
 * 
 * This file contains the implementation of all hardware drivers for this board component of UBC Solar
 * firmware. Drivers provide abstraction layers between high-level application code and low-level
 * hardware peripherals, enabling consistent interfaces and easier code maintenance.
 * 
 * Drivers implemented here include:
 * - DRD board-specific peripherals
 * 
 * @author  UBC Solar
 * @date    Feb 7 2026
 */

#include "drivers.h"
#include "drive_state.h"

/* DRIVE STATE DRIVERS */

void HAL_GPIO_EXT1_Callback(uint16_t GPIO_Pin) {
    drive_state_interrupt_handler(GPIO_Pin);
}