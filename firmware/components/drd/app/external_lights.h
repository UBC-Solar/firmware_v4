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
#define EXTERNAL_LIGHTS_STATE_MACHINE_DELAY_MS 50
#define EXTERNAL_LIGHTS_STATE_PERIOD_MS        650  // period of signal (on -> off -> on)
#define EXTERNAL_LIGHTS_FLIP_COUNT             ((EXTERNAL_LIGHTS_STATE_PERIOD_MS) / (EXTERNAL_LIGHTS_STATE_MACHINE_DELAY_MS))

/*	Typedefs 	*/
typedef enum {
    EXTERNAL_LIGHTS_IDLE_STATE  = 0,
    EXTERNAL_LIGHTS_HAZARD_STATE,
    EXTERNAL_LIGHTS_LTS_STATE,
    EXTERNAL_LIGHTS_RTS_STATE,
    EXTERNAL_LIGHTS_BRAKE_STATE,
} ExternalLightsState_t;

/*	Function Prototypes	*/
void ExternalLightsStateMachine();
void ExternalLightsCanRxHandle(uint32_t can_id, uint8_t* data);

#endif /* INC_EXTERNAL_LIGHTS_H_ */
