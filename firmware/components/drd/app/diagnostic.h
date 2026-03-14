/**
 * @file    diagnostic.h
 * @brief   Diagnostic header file for UBC Solar's DRD Module
 *
 * This file is used to declare datatypes for the diagnostic data and function prototypes for UBC Solar's
 * DRD so that it can be sent over CAN for other boards to process. 
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#ifndef INC_DIAGNOSTIC_H_
#define INC_DIAGNOSTIC_H_

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

/*	DATA TYPES 	*/
typedef union {
	struct {
		volatile bool mech_brake_pressed 		: 1; // done
		volatile bool regen_enabled 			: 1; // done
		volatile bool throttle_ADC_out_of_range : 1; // done
		volatile bool throttle_ADC_mismatch 	: 1; // done
		volatile bool watchdog_reset 			: 1; // done
		volatile bool motor_comm_fault 			: 1; // never set in v3
	};
	uint8_t all_flags;
} DiagnosticDRDAllFlags;

// TODO : Add more flags as needed (TEMPERATURE, FAULTS, WARNINGS, etc.)
typedef union {
	struct {
		volatile bool speed_timeout 		: 1; //done
		volatile bool drive_state_timeout 	: 1; //done
		volatile bool soc_timeout 			: 1; //done
		volatile bool voltage_timeout 		: 1; //done
		volatile bool current_timeout 		: 1; //done
	};
	uint8_t cyclic_data_all_flags;
} DiagnosticCyclicDataFlags;

typedef struct {
	volatile uint16_t raw_adc1; // done
	volatile uint16_t raw_adc2; // done
	DiagnosticDRDAllFlags flags;
	DiagnosticCyclicDataFlags cyclic_flags;
} DiagnosticDRD;


/*	DIAGNOSTIC TX FUNCTION PROTOTYPES	*/

/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticTimeSinceBootup();

/**
 * @brief  Transmits DRD Diagnostic Messages over CAN
 * @param from_ISR Indicates if the function is called from an ISR context
 */
void DiagnosticTransmit(bool from_ISR);


/* DIAGNOSTIC SETTERS FUNCTION PROTOTYPES */

/*
 * @brief Sets the raw ADC1 value
 * @param raw_adc1 The raw ADC1 value
 */
void DiagnosticSetRawADC1(uint16_t raw_adc1);
/**
 * @brief Sets the raw ADC2 value
 * @param raw_adc2 The raw ADC2 value
 */
void DiagnosticSetRawADC2(uint16_t raw_adc2);
/**
 * @brief Sets the mechanical brake pressed status
 * @param pressed The mechanical brake pressed status
 */
void DiagnosticSetMechBrakePressed(bool pressed);
/**
 * @brief Sets the regen enabled status
 * @param enabled The regen enabled status
 */
void DiagnosticSetRegenEnabled(bool enabled);
/**
 * @brief Sets the throttle ADC out of range status
 * @param out_of_range The throttle ADC out of range status
 */
void DiagnosticSetThrottleADCMismatch(bool mismatch);
/**
 * @brief Sets the throttle ADC mismatch status
 * @param mismatch The throttle ADC mismatch status
 */
 void DiagnosticSetThrottleADCOutOfRange(bool out_of_range);
/**
 * @brief Sets the watchdog reset status
 * @param reset The watchdog reset status
 */
void DiagnosticSetWatchdogReset(bool reset);
/**
 * @brief Sets the motor communication fault status
 * @param fault The motor communication fault status
 */
void DiagnosticSetMotorCommFault(bool fault);   
/**
 * @brief Sets the speed timeout status
 * @param timeout The speed timeout status
 */
void DiagnosticSetSpeedTimeout(bool timeout);
/**
 * @brief Sets the drive state timeout status
 * @param timeout The drive state timeout status
 */
void DiagnosticSetDriveStateTimeout(bool timeout);
/**
 * @brief Sets the SOC timeout status
 * @param timeout The SOC timeout status
 */
void DiagnosticSetSocTimeout(bool timeout);
/**
 * @brief Sets the voltage timeout status
 * @param timeout The voltage timeout status
 */
void DiagnosticSetVoltageTimeout(bool timeout);
/**
 * @brief Sets the current timeout status
 * @param timeout The current timeout status
 */
void DiagnosticSetCurrentTimeout(bool timeout);

#endif /* INC_DIAGNOSTIC_H_ */
