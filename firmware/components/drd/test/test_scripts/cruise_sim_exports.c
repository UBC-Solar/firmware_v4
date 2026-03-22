/**
 * cruise_sim_exports.c
 *
 * Provides:
 *  1. Global variables Python controls (velocity injection, ADC injection)
 *  2. Wrapper functions exported to Python via ctypes
 *
 */
#include "cruise_sim_stubs.h"
#include "cruise_control_test.h"

/* ── Globals Python injects into ────────────────────────────────────────────── */
float    g_sim_velocity_ms = 6.94f;   /* injected by Python each tick          */
uint16_t g_sim_adc1        = 1475;    /* injected by Python for pedal position  */
ADC_HandleTypeDef hadc1    = {0};

/* ── IMU injection ───────────────────────────────────────────────────────────  */
/* Python calls sim_inject_imu(ax, az, gy) to send realistic IMU values         */
void ImuStateXCanMsgHandler(uint8_t* data);
void ImuStateZCanMsgHandler(uint8_t* data);

void sim_inject_imu(float accel_x, float accel_z, float gyro_y) {
    uint8_t buf[8];
    FloatToBytes fb;

    /* X message: accel_x in bytes[0..3], gyro_x in bytes[4..7] */
    fb.f = accel_x;
    for (int i = 0; i < 4; i++) buf[i]   = fb.bytes[i];
    fb.f = 0.0f;                           /* gyro_x not used for pitch */
    for (int i = 0; i < 4; i++) buf[i+4] = fb.bytes[i];
    ImuStateXCanMsgHandler(buf);

    /* Z message: accel_z in bytes[0..3], gyro_z in bytes[4..7]            */
    /* gyro_y goes into the Y handler but pitch only uses gyro_y from Y     */
    fb.f = accel_z;
    for (int i = 0; i < 4; i++) buf[i]   = fb.bytes[i];
    fb.f = gyro_y;                         /* store gyro_y in Z's gyro slot  */
    for (int i = 0; i < 4; i++) buf[i+4] = fb.bytes[i];
    ImuStateZCanMsgHandler(buf);
}

/* ── VelocitySetMs wrapper ───────────────────────────────────────────────────  */
void VelocitySetMs(float dt);

float sim_velocity_set_ms(float dt) {
    VelocitySetMs(dt);
    return g_cruise_data.est_cruise_velocity_ms;
}

/* ── Setpoint setter ─────────────────────────────────────────────────────────  */
void sim_set_cruise_setpoint(float setpoint_ms) {
    g_cruise_data.set_cruise_velocity_ms = setpoint_ms;
}

/* ── Getters for all CruiseData fields ───────────────────────────────────────  */
float sim_get_accel(void)          { return g_cruise_data.accel; }
float sim_get_est_velocity(void)   { return g_cruise_data.est_cruise_velocity_ms; }

/* ── AccelCruiseNormalizeToDac ───────────────────────────────────────────────  */
uint16_t AccelCruiseNormalizeToDac(float accel);
uint16_t sim_cruise_dac(void) {
    return AccelCruiseNormalizeToDac(g_cruise_data.accel);
}

/* ── AccelDriverReadThrottle (pedal → DAC) ───────────────────────────────────  */
uint16_t AccelDriverReadThrottle(void);
uint16_t sim_pedal_dac(uint16_t adc_value) {
    g_sim_adc1 = adc_value;
    return AccelDriverReadThrottle();
}

/* ── Reset all static state inside cruise_control.c ─────────────────────────  */
/* Needed between test runs so the integral/velocity static vars reset          */
void sim_reset(float initial_velocity_ms, float setpoint_ms) {
    /* Zero the entire CruiseData */
    volatile CruiseData *d = &g_cruise_data;
    d->accel                   = 0.0f;
    d->set_cruise_velocity_ms  = setpoint_ms;
    d->est_cruise_velocity_ms  = initial_velocity_ms;
    d->prev_cruise_velocity_ms = initial_velocity_ms;
    g_sim_velocity_ms          = initial_velocity_ms;
}