/*
 * rtd.h
 *
 *  Created on: Nov 1, 2025
 *      Author: Luke Santosham & Martin Wu
 */

#ifndef INC_RTD_H_
#define INC_RTD_H_
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RtdStatusOk,
    RtdStatusFault,
    RtdStatusHalError
} RtdStatus;

// PUBLIC FUNCTIONS

/*
 * @brief:		Gets the temperature and status of the RTD
 * @param[out]:	temperature; a uint32_t of the temperature read from the RTD
 * 				converted from the resistance ratio.
 * @returns		status of the RTD as an enumerator (RtdStatusOk, RtdStatusFault, or
 * RtdStatusHalError).
 */
RtdStatus RtdDriverGetTemp(uint32_t* temperature);

/*
 * @brief:      Initializes the MAX31865 RTD interface.
 * @details:    Writes the desired configuration settings (bias, conversion mode,
 *              wiring type, and filter selection) to the configuration register.
 * @param:      None.
 * @returns:    None.
 */
void RtdDriverInit();

#endif /* INC_RTD_H_ */