/*
 * rtd_driver.h
 *
 *  Created on: Nov 1, 2025
 *      Author: Luke Santosham & Martin Wu
 */

#ifndef INC_RTD_H_
#define INC_RTD_H_
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Return status for RTD temperature reads.
 */
typedef enum
{
    RtdStatusOk,    
    RtdStatusFault,   
    RtdStatusHalError, 
} RtdStatus;

/**
 * @brief Raw MAX31865 Fault Status register bits (D7 through D2).
 */
typedef uint8_t RtdFaultFlags;

/**
 * @brief Reads the latched MAX31865 Fault Status register.
 *
 * @param[out] faults Raw fault bits from register 0x07. Unused bits D1 and D0
 *                    are masked off.
 * @return RtdStatusOk on success, RtdStatusFault on invalid input, or
 *         RtdStatusHalError on an SPI failure.
 */
RtdStatus RtdDriverReadFaults(RtdFaultFlags* faults);

/**
 * @brief Gets the debounced MAX31865 fault flags.
 *
 * @return Fault bits after the configured number of consecutive faulted
 *         readings, or zero when no debounced fault is active.
 */
RtdFaultFlags RtdDriverGetFaults(void);

/**
 * @brief Reads motor temperature from the MAX31865 RTD interface.
 *
 * Converts the 15-bit RTD ADC code to degrees Celsius using the PT1000
 * resistance curve. 
 *
 * @param[out] temperature Signed temperature in degrees Celsius (integer).
 * @return RtdStatusOk on success, RtdStatusFault on sensor fault,
 *         or RtdStatusHalError on SPI failure.
 */
RtdStatus RtdDriverGetTemp(int32_t* temperature);

/**
 * @brief Initializes breakout board for temperature measurement.
 *
 * Configures 3-wire PT1000, auto-conversion, VBIAS enabled, and 60 Hz filter.
 * Clears any fault latched during power-up or VBIAS settling.
 *
 * Call once after SPI1 is initialized.
 */
void RtdDriverInit(void);

#endif /* INC_RTD_H_ */
