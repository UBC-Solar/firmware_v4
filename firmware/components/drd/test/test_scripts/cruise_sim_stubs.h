/**
 * sim_stubs.h
 * Stubs for cruise_control.c + accel_driver.c on laptop.
 *
 * Physics constants (VEHICLE_MASS_KG etc.) are redefined as runtime globals
 * so Python can change them live via sliders without recompiling.
 */
#ifndef SIM_STUBS_H
#define SIM_STUBS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define HAL_MAX_DELAY 0xFFFFFFFF

/* ── HAL types ────────────────────────────────────────────────────────────── */
typedef struct { int dummy; } ADC_HandleTypeDef;
extern ADC_HandleTypeDef hadc1;

/* ── HAL ADC stubs ────────────────────────────────────────────────────────── */
extern uint16_t g_sim_adc1;
static inline void     HAL_ADC_Start(ADC_HandleTypeDef *h)                         { (void)h; }
static inline uint32_t HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t t) { (void)h;(void)t; return 0; }
static inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h)                      { (void)h; return g_sim_adc1; }

/* ── Diagnostic stubs ─────────────────────────────────────────────────────── */
static inline void DiagnosticSetRawADC1(uint16_t v)           { (void)v; }
static inline void DiagnosticSetRawADC2(uint16_t v)           { (void)v; }
static inline void DiagnosticSetThrottleADCOutOfRange(bool v) { (void)v; }
static inline void DiagnosticSetThrottleADCMismatch(bool v)   { (void)v; }

/* ── Velocity injection ───────────────────────────────────────────────────── */
#define KMH_TO_MS_CONVERSION 3.6f
extern float g_sim_velocity_ms;
static inline float GetVelocityMs(void) { return g_sim_velocity_ms; }

/* ── Physics parameter globals ────────────────────────────────────────────── 
 * These replace the #define constants in cruise_control.c so Python can
 * change them at runtime via sliders without recompiling.
 * Defaults match your current cruise_control.c defines exactly.
 */
extern float g_sim_mass;          /* VEHICLE_MASS_KG   = 300 placeholder  */
extern float g_sim_power;         /* POWER_WATTS       = 2000 placeholder */
extern float g_sim_efficiency;    /* EFFICIENCY_FACTOR = 1.0              */
extern float g_sim_cd;            /* DRAG_COEFFICIENT  = 0.116            */
extern float g_sim_rolling;       /* ROLLING_RESISTANCE = 0.01 placeholder*/
extern float g_sim_frontal_area;  /* FRONTAL_VEHICLE_AREA = 1.18          */
extern float g_sim_air_density;   /* AIR_DENSITY = 1.26714                */

/* Redefine the C constants to use runtime globals */
#define VEHICLE_MASS_KG      g_sim_mass
#define POWER_WATTS          g_sim_power
#define EFFICIENCY_FACTOR    g_sim_efficiency
#define DRAG_COEFFICIENT     g_sim_cd
#define ROLLING_RESISTANCE   g_sim_rolling
#define FRONTAL_VEHICLE_AREA g_sim_frontal_area
#define AIR_DENSITY          g_sim_air_density

/* ── cruise_control.h defines ─────────────────────────────────────────────── */
#define CRUISE_SPEED_MIN_MS  6.94f
#define CRUISE_SPEED_MAX_MS  22.22f
#define ACCEL_MAX            2.0f
#define ACCEL_MIN           -2.5f
#define CONTROL_FREQUENCY_HZ 10

/* ── accel_driver.h defines ───────────────────────────────────────────────── */
#define ADC_LOWEST_VALID      1000
#define ADC_HIGHEST_VALID     1950
#define ADC_LOWER_DEADZONE    10
#define ADC_UPPER_DEADZONE    4000
#define ADC_MAX_DIFFERENCE    99999
#define ADC_NO_THROTTLE_MAX   630
#define ADC_FULL_THROTTLE_MIN 1350
#define MC_DAC_MAX            1023
#define MC_DAC_MIN            0

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

typedef enum {
    ADC_FAULT_NONE         = 0x00,
    ADC1_SENSOR_FAULT      = 0x01,
    ADC2_SENSOR_FAULT      = 0x02,
    ADC_ERROR_DISAGREEMENT = 0x04,
} AdcError;

typedef union { float f; uint8_t bytes[4]; } FloatToBytes;

typedef struct {
    float accel;
    float set_cruise_velocity_ms;
    float est_cruise_velocity_ms;
    float prev_cruise_velocity_ms;
    float f_net;
} CruiseData;

extern volatile CruiseData g_cruise_data;

#endif /* SIM_STUBS_H */