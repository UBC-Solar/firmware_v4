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

/* DRIVE STATE DEFINES */
#define MC_DAC_MAX     1023        // Note: This gets capped by MDI to 920 anyways for safety
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define DRIVE_STATE_FSM_DELAY 25
#define REGEN_DAC_ON          1023
#define REGEN_DAC_OFF         0
#define ACCEL_DAC_OFF         0

#define HIGHEST_ADC           1950    
#define LOWEST_ADC            800

#define ADC_LOWER_DEADZONE    10
#define ADC_NO_THROTTLE_MAX   630                 // https://ubcsolar26.monday.com/boards/7524367653/pulses/8891936447/posts/4032506875
#define ADC_FULL_THROTTLE_MIN 1350

#define ADC_UPPER_DEADZONE    4000 
#define ADC_MAX_DIFFERENCE    99999

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
void drive_state_fsm_handler(void);
void drive_state_interrupt_handler(uint16_t toggle);

#endif /* __DRIVE_STATE_H_ */