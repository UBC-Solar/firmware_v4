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
#include <stdbool.h>
#include <stdint.h>

#include "drive_state.h"

/* DEFINES */
#define KMH_TO_MS_CONVERSION 3.6
#define CRUISE_SPEED_MIN_MS 6.94 // Min speed so regen still work
#define CRUISE_SPEED_MAX_MS 22.22

#define ACCEL_CRUISE_DEADZONE 0.1f
#define ACCEL_MAX 2.0f  // to be changed once we get better numbers for force clamp
#define ACCEL_MIN -2.5f
#define CONTROL_FREQUENCY_HZ 10

/* STRUCTS */
typedef union {
    float f;
    uint8_t bytes[4];
} FloatToBytes;

typedef struct {
    float accel;                   // net acceleration the downstream DAC code uses, m/s^2
    float set_cruise_velocity_ms;  // driver setpoint, m/s
    float est_cruise_velocity_ms;  // setpoint used by the controller this step, m/s
    float prev_cruise_velocity_ms; // last valid measured speed from CAN, m/s


} CruiseData;

extern volatile CruiseData g_cruise_data;

/* FUNCTION PROTOTYPES */
void VelocitySetMs(float dt);
float GetCruiseAcceleration(void);
float GetCruiseForceN(void);
void CruiseControlEnter(void);
void CruiseControlExit(void);
bool CruiseControlActive(void);
void ImuStateXCanMsgHandler(uint8_t* data);
void ImuStateYCanMsgHandler(uint8_t* data);
void ImuStateZCanMsgHandler(uint8_t* data);

#ifdef DEBUG
void CruiseSetpointCanMsgHandler(uint8_t* data);
#endif

#endif // __CRUISE_CONTROL_H__
