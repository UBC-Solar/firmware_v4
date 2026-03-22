/**
 * @file    cruise_control.h
 * @brief   Cruise control logic interface for UBC Solar DRD board
 *
 * This header provides the interface and data structures for cruise control, velocity estimation,
 * and IMU/CAN data handling for the DRD board.
 *
 * @author  Tony Chen
 * @date    Mar 22, 2026
 */

#ifndef __CRUISE_CONTROL_H__
#define __CRUISE_CONTROL_H__

/* INCLUDES */
#include <stdint.h>

#include "drive_state.h"

/* DEFINES */
#define KMH_TO_MS_CONVERSION 3.6
#define CRUISE_SPEED_MIN_MS 6.94 // Min speed so regen still work
#define CRUISE_SPEED_MAX_MS 22.22

#define ACCEL_MAX 2.0f
#define ACCEL_MIN -2.5f

#define CONTROL_FREQUENCY_HZ 10

/* STRUCTS */
typedef union {
    float f;
    uint8_t bytes[4];
} FloatToBytes;

typedef struct {
    float accel;
    float cruise_velocity_ms;
    float prev_cruise_velocity_ms;
} CruiseData;

/* FUNCTION PROTOTYPES */
void VelocitySetMs(float dt);
float GetCruiseAcceleration(void);

#endif // __CRUISE_CONTROL_H__