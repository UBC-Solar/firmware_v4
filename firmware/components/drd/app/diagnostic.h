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
		volatile bool throttle_ADC_out_of_range : 1;
		volatile bool throttle_ADC_mismatch 	: 1;
		volatile bool watchdog_reset 			: 1; //done
		volatile bool motor_comm_fault 			: 1;
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


/*	Function Prototypes	*/
void DiagnosticTimeSinceBootup();
void DiagnosticTransmit(bool from_ISR);

void DiagnosticSetRawADC1(uint16_t raw_adc1);
void DiagnosticSetRawADC2(uint16_t raw_adc2);
void DiagnosticSetMechBrakePressed(bool pressed);
void DiagnosticSetRegenEnabled(bool enabled);
void DiagnosticSetThrottleADCOutOfRange(bool out_of_range);
void DiagnosticSetThrottleADCMismatch(bool mismatch);
void DiagnosticSetWatchdogReset(bool reset);
void DiagnosticSetMotorCommFault(bool fault);   
void DiagnosticSetSpeedTimeout(bool timeout);
void DiagnosticSetDriveStateTimeout(bool timeout);
void DiagnosticSetSocTimeout(bool timeout);
void DiagnosticSetVoltageTimeout(bool timeout);
void DiagnosticSetCurrentTimeout(bool timeout);

#endif /* INC_DIAGNOSTIC_H_ */
