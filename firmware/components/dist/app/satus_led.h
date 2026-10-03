/**
 * @file status_led.c
 * @brief Status LED control for the distribution board.
 *
 * This file contains the code for turning on and off the status LEDs on the distribution board.
 * @author Christopher De Lazzari
 * @date 2026-10-02
 */

#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <stdbool.h>

 typdef enum {
    HLIM,
    LLIM,
    NEG,
    DIST_FAULT,
    ESTOP,
    CAN_FAULT,
    POS, 
    MOTOR_PC,
    MPPT_PC,
    IMD, 
    MPPT,
    DCH_ON,
    DCH_OFF,
    FANS,
    SUPP_ACTIVE,
    SUPP_LOW,
    DCDC_ACTIVE,
    CONTACTOR_FAULT,
    STATUS_LED_COUNT
 } StatusLedId;

 bool StatusLedsInit(void);
 void StatusLedsSet(StatusLedId led, bool on);
 void status_leds_toggle(StatusLedId led);
 void StatusLedsUpdate(void);

 #endif // STATUS_LEDS_H