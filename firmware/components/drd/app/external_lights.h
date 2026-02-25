/*
 * external_lights.h
 *
 *  Created on: Feb 12, 2026
 *      Author: Martin Wu
 */

#ifndef INC_EXTERNAL_LIGHTS_H_
#define INC_EXTERNAL_LIGHTS_H_

/*	Includes	*/
#include "CAN_comms.h"
#include "can_driver.h"
#include "main.h"


/*	Symbolic Constants	*/
#define EXTERNAL_LIGHTS_STATE_MACHINE_DELAY 50
#define EXTERNAL_LIGHTS_STATE_PERIOD        650  // period of signal (on -> off -> on)
#define EXTERNAL_LIGHTS_FLIP_COUNT          ((EXTERNAL_LIGHTS_STATE_PERIOD) / (EXTERNAL_LIGHTS_STATE_MACHINE_DELAY))

/*	Typedefs 	*/

/*	Function Prototypes	*/
void ExternalLightsStateMachine();
void ExternalLightsCANRxHandle(uint32_t can_id, uint8_t* data);

#endif /* INC_EXTERNAL_LIGHTS_H_ */
