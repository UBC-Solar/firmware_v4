/**
* @file satus_led.c
* @brief Status LED control for the distribution board.
* This file contains the code for turning on and off the status LEDs on the distribution board.
* @author Christopher De Lazzari
* @date 2026-10-02
*/

#include "status_led.h"
#include "is31fl3236_driver.h"
#include "hi2c1.h"
#include "dist_main.h"

