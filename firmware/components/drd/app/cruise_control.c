/**
 * @file    cruise_control.c
 * @brief   Cruise control logic implementation for UBC Solar DRD board
 *
 * This file implements the cruise control logic, velocity estimation, and related calculations
 * for the DRD board. It handles PI control, force calculations, IMU data processing, and CAN message handling.
 *
 * @author  Tony Chen
 * @date    Mar 22, 2026
 */

 /* INCLUDES */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "drive_state.h"
#include "cruise_control.h"

/* DEFINES */
#define VEHICLE_MASS_KG 300 // Placeholder
#define POWER_WATTS 2000 // Placeholder
#define EFFICIENCY_FACTOR 1.0 // Placeholder
#define DRAG_COEFFICIENT 0.116
#define AIR_DENSITY 1.26714
#define FRONTAL_VEHICLE_AREA 1.18
#define ROLLING_RESISTANCE 0.01 // Placeholder
#define GRAVITY 9.81

#define KI 0.00009 * VEHICLE_MASS_KG
#define KP 0.5 * VEHICLE_MASS_KG

#define IMU_ACCEL_MAX (15.5f * 9.80665f)
#define IMU_GYRO_MAX (3900.0f * M_PI / 180.0f)
#define ACCEL_DELTA_MAX 20.0f
#define GYRO_DELTA_MAX 10.0f

#define MAX_SLOPE_RAD 0.35f
#define MAX_DELTA_RAD 0.05f

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* FUNCTION PROTOTYPES */
static float Clamp(float value, float min, float max);
static float Error(float target_velocity_ms, float current_velocity_ms);
static float PiControllerForce(float dt, float nominal_force);
static float NominalForce(void);
static float ForceOutput(float dt);
static float ForceDrag(void);
static float ForceRolling(void);
static float ComputePitch(float accel_forward, float accel_vertical);
static float ComputeGyro(float pitch, float gyro_pitch_rate, float dt);
static float UpdatePitch(float accel_x, float accel_z, float gyro_y, float dt);
static float ForceHill(float radian);
static bool ImuDataValidation(float accel, float gyro);

/* STRUCTS */
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float prev_pitch;
    float prev_accel;
    float prev_gyro;
    bool prev_imu_data;
} CruisePitchData;

typedef struct {
    float prev_radian;
    bool radian_checked;
} CruiseRadianData;

typedef struct {
    float prev_force_hill;
    float prev_force_output;
} CruiseForceData;

/* GLOBAL VARIABLES */
volatile CruiseData g_cruise_data = {0};

static CruisePitchData g_cruise_pitch_data = {0};
static CruiseRadianData g_cruise_radian_data = {0};
static CruiseForceData g_cruise_force_data = {0};

static float Clamp(float value, float min, float max) {
    if (value > max) {
        return max;
    }

    else if (value < min) {
        return min;
    }

    return value;
}

static float Error(float target_velocity_ms, float current_velocity_ms) {
    return target_velocity_ms - current_velocity_ms;
}

static float PiControllerForce(float dt, float nominal_force) {
    static float integral = 0.0f;

    float error_value = Error(g_cruise_data.cruise_velocity_ms, GetVelocityMs());

    bool saturated = isfinite(nominal_force) && (g_cruise_force_data.prev_force_output >= nominal_force);
    if (!saturated) {
        integral += error_value * dt;
    }

    return KP * error_value + KI * integral;
}

static float NominalForce(void) {
    return (EFFICIENCY_FACTOR * POWER_WATTS) / GetVelocityMs();
}

static float ForceOutput(float dt) {

    float force_output;

    bool valid_nominal_force = true;
    bool valid_pi_controller_force = true;

    float nominal_force = NominalForce();
    float pi_controller_force = PiControllerForce(dt, nominal_force);
    if (!isfinite(nominal_force)) {
        valid_nominal_force = false;
    }

    if(!isfinite(pi_controller_force)) {
        valid_pi_controller_force = false;
    }

    if (valid_nominal_force && valid_pi_controller_force) {
        force_output = MIN(pi_controller_force, nominal_force);
        g_cruise_force_data.prev_force_output = force_output;
    } else if (valid_nominal_force) {
        force_output = nominal_force;
    } else if (valid_pi_controller_force) {
        force_output = pi_controller_force;
    } else {
        force_output = g_cruise_force_data.prev_force_output;
    }

    if (valid_nominal_force || valid_pi_controller_force) {
        g_cruise_force_data.prev_force_output = force_output;
    }

    return force_output;
}

static float ForceDrag(void) {
    return 0.5f * AIR_DENSITY * DRAG_COEFFICIENT * FRONTAL_VEHICLE_AREA * powf(GetVelocityMs(), 2);
}

static float ForceRolling(void) {
    return ROLLING_RESISTANCE * VEHICLE_MASS_KG * GRAVITY;
}

static float ComputePitch(float accel_forward, float accel_vertical) {
    return atan2f(accel_forward, accel_vertical);
}

static float ComputeGyro(float pitch, float gyro_pitch_rate, float dt) {
    return pitch + gyro_pitch_rate * dt;
}

static float UpdatePitch(float accel_x, float accel_z, float gyro_y, float dt) 
{
    float accel_forward = accel_x;
    float accel_vertical = accel_z;
    float gyro_pitch_rate = gyro_y;

    gyro_pitch_rate = gyro_pitch_rate / 1000.0; // mdps -> deg/s
    gyro_pitch_rate = (gyro_pitch_rate * M_PI) / 180; // deg/s -> rad/s

    float pitch_acc = ComputePitch(accel_forward, accel_vertical);
    float pitch_gyro = ComputeGyro(g_cruise_pitch_data.prev_pitch, gyro_pitch_rate, dt);

    float alpha = 0.98; // Reliance on gyro vs accel
    float pitch = alpha * pitch_gyro + (1 - alpha) * pitch_acc;

    g_cruise_pitch_data.prev_pitch = pitch;

    return pitch;
}

static float ForceHill(float radian) {
    if (!isfinite(radian)) return g_cruise_force_data.prev_force_hill;
    if (fabsf(radian) > MAX_SLOPE_RAD) return g_cruise_force_data.prev_force_hill;
    
    if (g_cruise_radian_data.radian_checked) {
        if (fabsf(radian - g_cruise_radian_data.prev_radian) > MAX_DELTA_RAD) return g_cruise_force_data.prev_force_hill;
    }

    float force_hill = VEHICLE_MASS_KG * GRAVITY * sinf(radian);

    g_cruise_radian_data.prev_radian = radian;
    g_cruise_force_data.prev_force_hill = force_hill;

    g_cruise_radian_data.radian_checked = true;

    return force_hill;
}

static bool ImuDataValidation(float accel, float gyro) {

    if (!isfinite(accel) || !isfinite(gyro)) return false;
    if (fabsf(accel) > IMU_ACCEL_MAX) return false;
    if (fabsf(gyro) > IMU_GYRO_MAX) return false;
    if (g_cruise_pitch_data.prev_imu_data) {
        if (fabsf(accel - g_cruise_pitch_data.prev_accel) > ACCEL_DELTA_MAX) return false;
        if (fabsf(gyro - g_cruise_pitch_data.prev_gyro) > GYRO_DELTA_MAX) return false;
    }

    g_cruise_pitch_data.prev_accel = accel;
    g_cruise_pitch_data.prev_gyro = gyro;
    g_cruise_pitch_data.prev_imu_data = true;

    return true;
}

static void ImuStateXCanMsgHandler(uint8_t* data) {

    if (data == NULL) return;

    FloatToBytes float_bytes_x;

    for (int i = 0; i < 4; i++) {
        float_bytes_x.bytes[i] = data[i];
    }

    float accel_x = float_bytes_x.f;

    for (int i = 0; i < 4; i++) {
        float_bytes_x.bytes[i] = data[i + 4];
    }

    float gyro_x = float_bytes_x.f;

    if (ImuDataValidation(accel_x, gyro_x)) {
        g_cruise_pitch_data.accel_x = accel_x;
        g_cruise_pitch_data.gyro_x = gyro_x;
    }
}

static void ImuStateYCanMsgHandler(uint8_t* data) {

    if (data == NULL) return;

    FloatToBytes float_bytes_y;

    for (int i = 0; i < 4; i++) {
        float_bytes_y.bytes[i] = data[i];
    }

    float accel_y = float_bytes_y.f;

    for (int i = 0; i < 4; i++) {
        float_bytes_y.bytes[i] = data[i + 4];
    }

    float gyro_y = float_bytes_y.f;

    if (ImuDataValidation(accel_y, gyro_y)) {
        g_cruise_pitch_data.accel_y = accel_y;
        g_cruise_pitch_data.gyro_y = gyro_y;
    }
}

static void ImuStateZCanMsgHandler(uint8_t* data) {

    if (data == NULL) return;

    FloatToBytes float_bytes_z;

    for (int i = 0; i < 4; i++) {
        float_bytes_z.bytes[i] = data[i];
    }

    float accel_z = float_bytes_z.f;

    for (int i = 0; i < 4; i++) {
        float_bytes_z.bytes[i] = data[i + 4];
    }

    float gyro_z = float_bytes_z.f;

    if (ImuDataValidation(accel_z, gyro_z)) {
        g_cruise_pitch_data.accel_z = accel_z;
        g_cruise_pitch_data.gyro_z = gyro_z;
    }
}

void VelocitySetMs(float dt) {
    static float velocity = 0.0f;

    float pitch_degree = UpdatePitch(g_cruise_pitch_data.accel_x, g_cruise_pitch_data.accel_z, g_cruise_pitch_data.gyro_y, dt);

    float f_net = ForceOutput(dt) - (ForceDrag() + ForceRolling() + ForceHill(pitch_degree));
    if (!isfinite(f_net)) {
        g_cruise_data.cruise_velocity_ms = g_cruise_data.prev_cruise_velocity_ms;
    }

    float accel = f_net / VEHICLE_MASS_KG;
    accel = Clamp(accel, ACCEL_MIN, ACCEL_MAX);
    g_cruise_data.accel = accel;

    velocity += accel * dt;

    velocity = Clamp(velocity, CRUISE_SPEED_MIN_MS, CRUISE_SPEED_MAX_MS);

    g_cruise_data.prev_cruise_velocity_ms = velocity;

    g_cruise_data.cruise_velocity_ms = velocity;
}

float GetCruiseAcceleration(void) {
    return g_cruise_data.accel;
}