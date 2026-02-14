/*
 *  drive_state.c
 * 
 *  Created: Jan 28, 2026
 *  Author:  Tony Chen
*/

#ifndef __DRIVE_STATE_H__
#define __DRIVE_STATE_H__

/* INCLUDES */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "drivers.h"
#include "adc_driver.h"

/* DRIVE STATE DEFINES */
#define MC_DAC_MAX            1023    // Note: This gets capped by MDI to 920 anyways for safety
#define DRIVE_STATE_FSM_DELAY 25
#define REGEN_DAC_ON          1023
#define REGEN_DAC_OFF         0
#define ACCEL_DAC_OFF         0

#define WHEEL_RADIUS          0.283
#define M_PI                  3.14159
#define VELOCITY_THRESHOLD    0.5

/* DRIVE STATE DATA TYPES */

typedef struct {
    volatile bool brake_on;
    volatile bool regen_on;
    volatile bool cruise_on;
    volatile bool velocity_under_threshold;
    volatile bool next_state_request;
    volatile bool prev_state_request;
    volatile bool eco_mode_on;
} drive_flags_t;

typedef struct {
    uint16_t accel_DAC_value;
    uint16_t regen_DAC_value;
    uint8_t motor_control_flags;
} motor_control_t;

typedef enum {
    INVALID = (uint8_t) 0x00,
    FORWARD = (uint8_t) 0x01,
    PARK = (uint8_t) 0x02,
    REVERSE = (uint8_t) 0x03,
    CRUISE = (uint8_t) 0x04
} drive_state_t;

/* GLOBAL VARIABLES */
extern volatile drive_state_t g_drive_state;
extern volatile drive_flags_t g_drive_flags;
extern volatile uint32_t g_velocity_kmh;

/* FUNCTION PROTOTYPES */
void DriveStateFsmHandler(void);
void DriveStateInterruptHandler(uint16_t toggle);

#endif /* __DRIVE_STATE_H_ */