/**
 * @file    cruise_control.h
 * @brief   Header file for functions related to cruise control states.
 *
 * This file contains headers and defines for functions that handle the entry and exit
 * conditions for cruise control.
 *
 * @author Shlok Lande
 * @date Nov 25 2026
 */

 #ifndef __CRUISE_CONTROL_H__
 #define __CRUISE_CONTROL_H__
 
 /* INCLUDES */
 #include <stdbool.h>
 #include <stdint.h>
 
 /* DEFINES */
 #define CRUISE_ENGAGE_MIN_KMH 30.0f  
 #define CRUISE_EXIT_MIN_KMH 25.0f
 #define CRUISE_SETPOINT_MIN_KMH 30.0f
 #define CRUISE_SETPOINT_MAX_KMH 80.0f
 #define CRUISE_DAC_OFF 0U
 

 typedef enum{
     CRUISE_OFF = 0,
     CRUISE_ACTIVE = 1
 } CruiseState;
 
 typedef struct
 {
     bool     cruise_flag;     // STR byte 0 bit 1
     float    setpoint_kmh;    // STR bytes 1-2
     float    speed_ms;        // from motor RPM
     bool     speed_stale;     // true after 2 missed RPM polls
     bool     brake_on;
     bool     fault_or_estop;
     uint16_t pedal_dac;       // driver pedal, to temporarily accelerate
 } CruiseInputs;
 
 typedef struct
 {
     CruiseState state;
     uint16_t    accel_dac;    // never both non-zero
     uint16_t    regen_dac;
 } CruiseOutputs;
 
 /**
  * @brief Runs one cycle of the cruise state machine. Call every drive-state tick.
  * @param in Pointer to this cycle's inputs.
  * @return Cruise state and the accel/regen DAC values to command.
  */
 CruiseOutputs CruiseControlStep(const CruiseInputs* in);
 
 /**
  * @brief Resets cruise internals.
  */
 void CruiseControlReset(void);
 
 #endif /* __CRUISE_CONTROL_H__ */