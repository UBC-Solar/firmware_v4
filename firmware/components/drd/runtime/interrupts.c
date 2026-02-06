/**
 * @file    interrupts.c
 * @brief   Interrupt Service Routines (ISRs) for board-specific hardware events
 * 
 * This file contains the implementation of all hardware interrupt service routines specific to this
 * board component of UBC Solar firmware. ISRs provide immediate response to hardware events and
 * peripherals that require real-time handling with minimal latency.
 * 
 * Interrupt handlers defined here include the following boards:
 * - DRD
 * 
 * @author  UBC Solar
 * @date    Feb 4 2026
 */

 #include "interrupts.h"

 void drive_state_interrupt_handler(uint16_t toggle) { // in progress
    if (toggle == BRK_IN_Pin) {
        break_on_handler();
    }

    if (toggle == DRIVE_STATE_NEXT_Pin) {

    }

    if (toggle == DRIVE_STATE_PREV_Pin) {

    }
 }