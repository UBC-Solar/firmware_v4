/**
 * cruise_sim_stubs.h
 * Minimal stubs so cruise_control_test.c + accel_driver_test.c compile on a laptop.
 * Only mocks what those two files actually need — nothing else.
 */
#ifndef CRUISE_SIM_STUBS_H
#define CRUISE_SIM_STUBS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── HAL types ─────────────────────────────────────────────────────────────── */
typedef struct { int dummy; } ADC_HandleTypeDef;
extern ADC_HandleTypeDef hadc1;

/* ── HAL ADC stubs ─────────────────────────────────────────────────────────── */
/* Python sets this before each tick to inject a mock ADC reading */
extern uint16_t g_sim_adc1;

static inline void     HAL_ADC_Start(ADC_HandleTypeDef *h)                         { (void)h; }
static inline uint32_t HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t t) { (void)h;(void)t; return 0; }
static inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h)                      { (void)h; return g_sim_adc1; }

/* ── Diagnostic stubs (accel_driver.c calls these) ─────────────────────────── */
static inline void DiagnosticSetRawADC1(uint16_t v)           { (void)v; }
static inline void DiagnosticSetRawADC2(uint16_t v)           { (void)v; }
static inline void DiagnosticSetThrottleADCOutOfRange(bool v) { (void)v; }
static inline void DiagnosticSetThrottleADCMismatch(bool v)   { (void)v; }

/* ── drive_state.h — only what cruise_control.c needs ──────────────────────── */
#define KMH_TO_MS_CONVERSION 3.6f

/* Python controls this to inject the current vehicle velocity */
extern float g_sim_velocity_ms;
static inline float GetVelocityMs(void) { return g_sim_velocity_ms; }

/* ── cruise_control.h defines ──────────────────────────────────────────────── */
#define CRUISE_SPEED_MIN_MS  6.94f
#define CRUISE_SPEED_MAX_MS  22.22f
#define ACCEL_MAX            2.0f
#define ACCEL_MIN           -2.5f
#define CONTROL_FREQUENCY_HZ 10

/* ── accel_driver.h defines ────────────────────────────────────────────────── */
#define ADC_LOWEST_VALID     1000
#define ADC_HIGHEST_VALID    1950
#define ADC_LOWER_DEADZONE   10
#define ADC_UPPER_DEADZONE   4000
#define ADC_MAX_DIFFERENCE   99999
#define ADC_NO_THROTTLE_MAX  630
#define ADC_FULL_THROTTLE_MIN 1350
#define MC_DAC_MAX           1023
#define MC_DAC_MIN           0

#define MAX(a,b) ((a)<(b)?(b):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))

#endif /* CRUISE_SIM_STUBS_H */
#define HAL_MAX_DELAY 0xFFFFFFFF