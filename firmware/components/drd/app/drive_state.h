/*
 *  drive_state.c
 * 
 *  Created: Jan 28, 2026
 *  Author:  Tony Chen
*/

#ifndef __DRIVE_STATE_H__
#define __DRIVE_STATE_H__

/* DRIVE STATE DATA TYPES */

typedef struct {
    volatile bool brake_on;
    volatile bool regen_on;
    volatile bool velocity_under_threshold;
    volatile bool forward_request;
    volatile bool reverse_request;
    volatile bool park_request;
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

extern volatile drive_state_t g_drive_state;

#endif /* __DRIVE_STATE_H_ */