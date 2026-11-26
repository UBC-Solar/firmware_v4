/**
 * @file    cruise_control.c
 * @brief   Cruise control logic implementation for UBC Solar DRD board
 *
 * This file implements the cruise control logic, velocity estimation, and related calculations
 * for the DRD board. It handles PI control, force calculations, IMU data processing, and CAN message handling.
 *
 * @author  Tony Chen
 * @date    Mar 22, 2026
 * 
 * 
 * 
 * CHANGES MADE post:
 * restructured PI controller to add feedforward + cleared up anti-windup logic and misc. unit issues
 */

 /* INCLUDES */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "drive_state.h"
#include "cruise_control.h"







/* DEFINES */


//make all these values floats
#define VEHICLE_MASS_KG 370
#define POWER_WATTS 2000  //cascadia will have 5kW max power. 
#define EFFICIENCY_FACTOR 0.95
#define DRAG_COEFFICIENT 0.39568 
#define AIR_DENSITY 1.26714
#define FRONTAL_VEHICLE_AREA 1.1853
#define ROLLING_RESISTANCE 0.0234 //marwan's update says 2.3% so seems fair
#define GRAVITY 9.81

#define KI 0.00009 * VEHICLE_MASS_KG
#define KP 0.5 * VEHICLE_MASS_KG



//force limits: force_motor_max, force_regen_max, maybe hardcode if required

//change force clamping once we have better numbers for min/max force required for regen vs cruise. 


//integral limits Fmax/Ki for integral clamping in anti-windup. 
#define CRUISE_INTEGRAL_MAX_N 


#define Q_ANGLE   0.001f    // Gyro trust factor
#define Q_BIAS    0.003f    // Rate of change for the gyro bias
#define R_MEASURE 0.03f     // Accelerometer at rest trust factor
#define IMU_ACCEL_MAX (15.5f * 9.80665f)
#define IMU_GYRO_MAX (3900.0f * M_PI / 180.0f)
#define ACCEL_DELTA_MAX 20.0f
#define GYRO_DELTA_MAX 10.0f

#define MAX_SLOPE_RAD 0.35f
#define MAX_DELTA_RAD 0.05f

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* ENUMS */
typedef enum { IMU_AXIS_X = 0, IMU_AXIS_Y, IMU_AXIS_Z, IMU_AXIS_COUNT } ImuAxis;

/* STRUCTS */


typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float prev_pitch;
    float prev_accel[IMU_AXIS_COUNT];
    float prev_gyro[IMU_AXIS_COUNT];
    bool prev_imu_data[IMU_AXIS_COUNT];
} CruisePitchData;

typedef struct {
    float prev_radian;
    bool radian_checked;
} CruiseRadianData;

typedef struct {
    float prev_force_hill;
    float prev_force_output;
} CruiseForceData;

typedef struct {
    float angle;
    float bias;
    float P[2][2];
    bool  initialised;
} KalmanState;


typedef struct{
    float integral; //integral of the error v_Ref - v wrt , units are in metres.
    float v_ref; //reference speed i.e.speed we want to cruise at
    
    bool active; //true only when in CRUISE state
    } ControllerState;


/* FUNCTION PROTOTYPES */
static float Clamp(float value, float min, float max);
static float Error(float target_velocity_ms, float current_velocity_ms);
static float MeasuredSpeedMs(void);
static float SetpointMs(void);
static void ReferenceUpdate (float v_set, float dt);  //v_Set is the speed requested i.e. input setpoint, v_ref is the prefilter output, v_cruise is the actual estimated vehicle speed from the mdi


static void ControlStep(float dt, float pitch_rad); //pitch_rad


static float NominalForce(void);
static float ForceLowerLimit(float f_load);
static float ForceDrag(float speed_ms);
static float ForceRolling(void);
static float ComputePitch(float accel_forward, float accel_vertical);
static float UpdatePitch(float accel_x, float accel_z, float gyro_y, float dt);
static float ForceHill(float radian);
static bool ImuDataValidation(float accel, float gyro, ImuAxis axis);

/* GLOBAL VARIABLES */
volatile CruiseData g_cruise_data = {0};

static CruisePitchData g_cruise_pitch_data = {0};
static CruiseRadianData g_cruise_radian_data = {0};
static CruiseForceData g_cruise_force_data = {0};
static ControllerState g_ctrl = {0};

static KalmanState g_kalman = {
    .angle       = 0.0f,
    .bias        = 0.0f,
    .P           = {{Q_ANGLE, 0.0f}, {0.0f, Q_BIAS}},
    .initialised = false,
};

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




//latest measured speed from the mdi. also stores the last usable value in 
//todo - replace all getvelocityms with measuredspeedms within the pi loop
static float MeasuredSpeedMs(void){
    float v = GetVelocityMs();

    if(!isfinite(v) || v <0.0f){
        return g_cruise_data.prev_cruise_velocity_ms;
    }
    g_cruise_data.prev_cruise_velocity_ms = v; //store the last usable value in CruiseData. 
    return v;

}

//update CruiseData with the set cruise velocity. if this is not within accepted range, use Clamp()
static float SetpointMs(void){

    
    float v_set = g_cruise_data.set_cruise_velocity_ms;
    if(!(isfinite(v_set)) || v_set<0.0f){
        return g_ctrl.v_ref;


    }
    return Clamp(v_set, (float) CRUISE_SPEED_MIN_MS, (float) CRUISE_SPEED_MAX_MS  );

}


//not adding prefilter right now  


static void ControlStep(float dt, float pitch_rad){



    //feedofrward+pi -> clamp force and then add anti-windup logic

    float v = MeasuredSpeedMs(); //current measured speed from MDI. 
    g_ctrl.v_ref = SetpointMs(); //pick up the latest setpoint every step (steering wheel / debug CAN)
   
    
    //calculate all load forces for feefdorward. 
    
    float f_hill = ForceHill(pitch_rad);
    float f_ff = ForceDrag(g_ctrl.v_ref) + ForceRolling();
    

    //PI on the reference error
    float error_value = Error(g_ctrl.v_ref, v); //error = v_ref - v
    float f_unsat = f_ff + KP * error_value + KI * g_ctrl.integral; //calculates the unsaturated force request to be sent to the throttle i.e. f_Ff + f_pi

    //currently upper limit is set by nominal force. 
    float f_hi = NominalForce();
    float f_lo = ForceLowerLimit(f_ff);
    float f_cmd; //commanded force request. 
    bool saturated = false;

    if (isfinite(f_unsat)) {
        f_cmd = Clamp(f_unsat, f_lo, f_hi);
        if(f_cmd!=f_unsat){saturated = true;}
       


        //anti windup logic based on conditional integration. freeze the integral only when unclamped command is beyond the limit and depending on whether adding the error pushes it towards the saturation limit vs away. 
        //unwind as soon as the eror flips. 

        bool upper_limit = (f_unsat >= f_hi) && (error_value > 0.0f); //error pushes towards the higher end
        bool lower_limit = (f_unsat <= f_lo) && (error_value < 0.0f); //error pushes towards the lower end

        if(!upper_limit && !lower_limit){ //if the error is not pushing towards saturation, let the integral accumulate
            g_ctrl.integral+=error_value * dt;
            
        }
    }
    else {
        //holds last input, doesnt change the integral. 
        f_cmd = Clamp(g_cruise_force_data.prev_force_output, f_lo, f_hi);
        saturated = true;
    }

    g_cruise_force_data.prev_force_output = f_cmd;


    //outputs of the control step:
    g_cruise_data.accel = Clamp((f_cmd - f_ff) / VEHICLE_MASS_KG, ACCEL_MIN, ACCEL_MAX); //send accel based on f_cmd - f_load = ma
    g_cruise_data.est_cruise_velocity_ms = g_ctrl.v_ref; //set the cruise velocity sent by the request. 
    //we dont necessarily need to store earlier forces? but if required can be easily added to the struct. 
    


}



//this is essentially force_max
static float NominalForce(void) {
    return (EFFICIENCY_FACTOR * POWER_WATTS) / MeasuredSpeedMs();
}

//set min force based on earlier ACCEL_MIN

static float ForceLowerLimit(float f_load){
    return VEHICLE_MASS_KG *ACCEL_MIN + f_load;
}

static float ForceDrag(float speed_ms) {
    return 0.5f * AIR_DENSITY * DRAG_COEFFICIENT * FRONTAL_VEHICLE_AREA * speed_ms * speed_ms;
}

static float ForceRolling(void) {
    return ROLLING_RESISTANCE * VEHICLE_MASS_KG * GRAVITY;
}

static float ComputePitch(float accel_forward, float accel_vertical) {
    return atan2f(accel_forward, accel_vertical); // needs to adjust based on IMU setup
}

static float UpdatePitch(float accel_x, float accel_z, float gyro_y, float dt)
{
    float gyro_pitch_rate = gyro_y;
    gyro_pitch_rate = gyro_pitch_rate / 1000.0f;          // mdps → deg/s
    gyro_pitch_rate = (gyro_pitch_rate * M_PI) / 180.0f;  // deg/s → rad/s

    float pitch_acc = ComputePitch(accel_x, accel_z);     // accelerometer angle

    // On first call, seed the filter with the accelerometer reading
    if (!g_kalman.initialised) {
        g_kalman.angle       = pitch_acc;
        g_kalman.initialised = true;
    }

    // If accel_z deviates from gravity or bump or vibration, then distrust accel
    float deviation = fabsf(accel_z - 9.81f);
    float R_adaptive = (deviation > 1.5f) ? R_MEASURE * 10.0f :  // bump
                       (deviation > 0.5f) ? R_MEASURE * 3.0f  :  // vibration
                                            R_MEASURE;            // smooth

    // Project angle forward using gyro, subtract estimated bias
    g_kalman.angle += dt * (gyro_pitch_rate - g_kalman.bias);

    // Update covariance — uncertainty grows as we integrate
    g_kalman.P[0][0] += dt * (dt * g_kalman.P[1][1]
                             - g_kalman.P[0][1]
                             - g_kalman.P[1][0]
                             + Q_ANGLE);
    g_kalman.P[0][1] -= dt * g_kalman.P[1][1];
    g_kalman.P[1][0] -= dt * g_kalman.P[1][1];
    g_kalman.P[1][1] += Q_BIAS * dt;

    // Correction based on accelerometer reading
    float S    = g_kalman.P[0][0] + R_adaptive;
    float K[2] = { g_kalman.P[0][0] / S,
                   g_kalman.P[1][0] / S };

    // Difference between accel and prediction
    float y = pitch_acc - g_kalman.angle;

    // Correct angle and bias estimates
    g_kalman.angle += K[0] * y;
    g_kalman.bias  += K[1] * y;

    // Update covariance (save originals before mutating)
    float P00 = g_kalman.P[0][0];
    float P01 = g_kalman.P[0][1];
    g_kalman.P[0][0] -= K[0] * P00;
    g_kalman.P[0][1] -= K[0] * P01;
    g_kalman.P[1][0] -= K[1] * P00;
    g_kalman.P[1][1] -= K[1] * P01;

    g_cruise_pitch_data.prev_pitch = -g_kalman.angle;

    return -g_kalman.angle; // negate to match ForceHill() convention
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

static bool ImuDataValidation(float accel, float gyro, ImuAxis axis) {

    if (!isfinite(accel) || !isfinite(gyro)) return false;
    if (fabsf(accel) > IMU_ACCEL_MAX) return false;
    if (fabsf(gyro) > IMU_GYRO_MAX) return false;
    if (g_cruise_pitch_data.prev_imu_data[axis]) {
        if (fabsf(accel - g_cruise_pitch_data.prev_accel[axis]) > ACCEL_DELTA_MAX) return false;
        if (fabsf(gyro - g_cruise_pitch_data.prev_gyro[axis]) > GYRO_DELTA_MAX) return false;
    }

    g_cruise_pitch_data.prev_accel[axis] = accel;
    g_cruise_pitch_data.prev_gyro[axis] = gyro;
    g_cruise_pitch_data.prev_imu_data[axis] = true;

    return true;
}

void ImuStateXCanMsgHandler(uint8_t* data) {

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

    if (ImuDataValidation(accel_x, gyro_x, IMU_AXIS_X)) {
        g_cruise_pitch_data.accel_x = accel_x;
        g_cruise_pitch_data.gyro_x = gyro_x;
    }
}

void ImuStateYCanMsgHandler(uint8_t* data) {

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

    if (ImuDataValidation(accel_y, gyro_y, IMU_AXIS_Y)) {
        g_cruise_pitch_data.accel_y = accel_y;
        g_cruise_pitch_data.gyro_y = gyro_y;
    }
}

void ImuStateZCanMsgHandler(uint8_t* data) {

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

    if (ImuDataValidation(accel_z, gyro_z, IMU_AXIS_Z)) {
        g_cruise_pitch_data.accel_z = accel_z;
        g_cruise_pitch_data.gyro_z = gyro_z;
    }
}

void VelocitySetMs(float dt) {
    static float velocity = 0.0f;



    //filter needs to run in every drive state so that the pitch estimate has settled when cruise is engaged. 
    float pitch_rad = UpdatePitch(g_cruise_pitch_data.accel_x, g_cruise_pitch_data.accel_z, g_cruise_pitch_data.gyro_y, dt);


    if(DriveStateGetDriveState() == CRUISE){
        if(!g_ctrl.active){
            //enter cruise control
            float measured_v = MeasuredSpeedMs();

            //setpoint is the speed at engagement. 
            g_cruise_data.set_cruise_velocity_ms = Clamp(measured_v, (float) CRUISE_SPEED_MIN_MS, (float) CRUISE_SPEED_MAX_MS);
            g_ctrl.v_ref = g_cruise_data.set_cruise_velocity_ms;

            //integral starts at zero, the feedforward carries the force load terms
            g_ctrl.integral = 0.0f;
            g_ctrl.active = true;
            ControlStep(0.0f, g_cruise_pitch_data.prev_pitch);
        }
        ControlStep(dt, pitch_rad);
    }
    else if(g_ctrl.active){
        //cruise is off.
        g_ctrl.active = false;
        g_cruise_data.accel = 0.0f;
        //night need to add a return statement here
    }


}

float GetCruiseAcceleration(void) {
    return g_cruise_data.accel;
}

#ifdef DEBUG
void CruiseSetpointCanMsgHandler(uint8_t* data) {
    if (data == NULL) return;

    FloatToBytes fb;
    for (int i = 0; i < 4; i++) {
        fb.bytes[i] = data[i];
    }

    float setpoint_ms = fb.f;
    if (!isfinite(setpoint_ms)) return;
    if (setpoint_ms < CRUISE_SPEED_MIN_MS || setpoint_ms > CRUISE_SPEED_MAX_MS) return;

    g_cruise_data.set_cruise_velocity_ms = setpoint_ms;
}
#endif
