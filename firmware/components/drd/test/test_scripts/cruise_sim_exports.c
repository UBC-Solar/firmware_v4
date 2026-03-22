/**
 * cruise_sim_exports.c
 * Global variables Python injects into + wrapper functions Python calls.
 */
#include "cruise_sim_stubs.h"

/* ── Hardware mocks ────────────────────────────────────────────────────────── */
float             g_sim_velocity_ms = 6.94f;
uint16_t          g_sim_adc1        = 1475;
ADC_HandleTypeDef hadc1             = {0};

/* ── Physics parameter globals (defaults match cruise_control.c #defines) ─── */
float g_sim_mass         = 300.0f;
float g_sim_power        = 2000.0f;
float g_sim_efficiency   = 1.0f;
float g_sim_cd           = 0.116f;
float g_sim_rolling      = 0.01f;
float g_sim_frontal_area = 1.18f;
float g_sim_air_density  = 1.26714f;

/* ── CAN headers ───────────────────────────────────────────────────────────── */
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t DLC; } CAN_TxHeaderTypeDef;
CAN_TxHeaderTypeDef drive_control_header = {0};
CAN_TxHeaderTypeDef mdu_request_header   = {0};

/* ── Set all physics params at once ───────────────────────────────────────── */
void sim_set_params(float mass, float power, float efficiency,
                    float cd, float rolling, float frontal, float air_density) {
    g_sim_mass         = mass;
    g_sim_power        = power;
    g_sim_efficiency   = efficiency;
    g_sim_cd           = cd;
    g_sim_rolling      = rolling;
    g_sim_frontal_area = frontal;
    g_sim_air_density  = air_density;
}

/* ── IMU injection ─────────────────────────────────────────────────────────── */
void ImuStateXCanMsgHandler(uint8_t* data);
void ImuStateZCanMsgHandler(uint8_t* data);

void sim_inject_imu(float accel_x, float accel_z, float gyro_y) {
    uint8_t buf[8];
    FloatToBytes fb;
    fb.f = accel_x;
    for (int i = 0; i < 4; i++) buf[i]   = fb.bytes[i];
    fb.f = 0.0f;
    for (int i = 0; i < 4; i++) buf[i+4] = fb.bytes[i];
    ImuStateXCanMsgHandler(buf);
    fb.f = accel_z;
    for (int i = 0; i < 4; i++) buf[i]   = fb.bytes[i];
    fb.f = gyro_y;
    for (int i = 0; i < 4; i++) buf[i+4] = fb.bytes[i];
    ImuStateZCanMsgHandler(buf);
}

/* ── Core wrappers ─────────────────────────────────────────────────────────── */
void VelocitySetMs(float dt);

float sim_velocity_set_ms(float dt) {
    VelocitySetMs(dt);
    return g_cruise_data.est_cruise_velocity_ms;
}

void sim_set_cruise_setpoint(float setpoint_ms) {
    g_cruise_data.set_cruise_velocity_ms = setpoint_ms;
}

float    sim_get_accel(void)        { return g_cruise_data.accel; }
float    sim_get_est_velocity(void) { return g_cruise_data.est_cruise_velocity_ms; }
float    sim_get_f_net(void)        { return g_cruise_data.f_net; }

uint16_t AccelCruiseNormalizeToDac(float accel);
uint16_t sim_cruise_dac(void)       { return AccelCruiseNormalizeToDac(g_cruise_data.accel); }

uint16_t AccelDriverReadThrottle(void);
uint16_t sim_pedal_dac(uint16_t adc_value) {
    g_sim_adc1 = adc_value;
    return AccelDriverReadThrottle();
}

void sim_reset(float initial_velocity_ms, float setpoint_ms) {
    volatile CruiseData *d = &g_cruise_data;
    d->accel                   = 0.0f;
    d->set_cruise_velocity_ms  = setpoint_ms;
    d->est_cruise_velocity_ms  = initial_velocity_ms;
    d->prev_cruise_velocity_ms = initial_velocity_ms;
    d->f_net                   = 0.0f;
    g_sim_velocity_ms          = initial_velocity_ms;
}