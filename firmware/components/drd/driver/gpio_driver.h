#ifndef __DRIVERS_H__
#define __DRIVERS_H__

/* INCLUDES */
#include <stdlib.h>

#include "adc.h"

/* DEFINES */
#define BRAKE_INPUT_PIN         BRK_IN_Pin
#define BRAKE_INPUT_PORT        BRK_IN_GPIO_Port

#define DRIVE_NEXT_PIN          DRIVE_STATE_NEXT_Pin
#define DRIVE_NEXT_PORT         DRIVE_STATE_NEXT_GPIO_Port

#define DRIVE_PREV_PIN          DRIVE_STATE_PREV_Pin
#define DRIVE_PREV_PORT         DRIVE_STATE_PREV_GPIO_Port

#define ECO_POWER_PIN           ECO_POWER_Pin
#define ECO_POWER_PORT          ECO_POWER_GPIO_Port

#define DEBUG_LED0_PIN          DEBUG_LED_Pin
#define DEBUG_LED0_PORT         DEBUG_LED_GPIO_Port

#define BRAKE_LED_PIN           BRK_OUT_Pin
#define BRAKE_LED_PORT          BRK_OUT_GPIO_Port

/* DRIVERS FUNCTION PROTOTYPES */
uint8_t ReadBrakePin(GPIO_TypeDef* port, uint16_t pin);
uint8_t ReadEcoPin(GPIO_TypeDef* port, uint16_t pin);
void ToggleLedPin(GPIO_TypeDef* port, uint16_t pin);
void ToggleBrakeLedPin(GPIO_TypeDef* port, uint16_t pin);

#endif //__DRIVERS_H__