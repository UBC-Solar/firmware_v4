/**
 * @file    gpio_driver.h
 * @brief   Hardware driver declarations for DRD board peripherals
 *
 * This header declares the interface for hardware drivers for this board component of UBC
 * Solar firmware. Drivers provide abstraction layers between high-level application code and
 * low-level hardware peripherals, enabling consistent interfaces and easier code maintenance.
 *
 * @author  UBC Solar
 * @date    Feb 7 2026
 */

#ifndef __DRIVERS_H__
#define __DRIVERS_H__

/* INCLUDES */
#include <stdlib.h>
#include "adc.h"

/* DEFINES */
#define BRAKE_INPUT_PIN BRK_IN_Pin
#define BRAKE_INPUT_PORT BRK_IN_GPIO_Port

#define DRIVE_NEXT_PIN DRIVE_STATE_NEXT_Pin
#define DRIVE_NEXT_PORT DRIVE_STATE_NEXT_GPIO_Port

#define DRIVE_PREV_PIN DRIVE_STATE_PREV_Pin
#define DRIVE_PREV_PORT DRIVE_STATE_PREV_GPIO_Port

#define ECO_POWER_PIN ECO_POWER_Pin
#define ECO_POWER_PORT ECO_POWER_GPIO_Port

#define DEBUG_LED0_PIN DEBUG_LED_Pin
#define DEBUG_LED0_PORT DEBUG_LED_GPIO_Port

#define DEBUG_LEDA1_PIN DEBUG_LEDA1_Pin
#define DEBUG_LEDA1_PORT DEBUG_LEDA1_GPIO_Port

#define BRAKE_LED_PIN BRK_OUT_Pin
#define BRAKE_LED_PORT BRK_OUT_GPIO_Port

#define HAZARD_PIN HAZARD_Pin
#define HAZARD_PORT HAZARD_GPIO_Port

/* DRIVERS FUNCTION PROTOTYPES */
/**
 * @brief Reads the state of the brake input pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @return Pin state (0 or 1).
 */
uint8_t ReadBrakePin(GPIO_TypeDef* port, uint16_t pin);
/**
 * @brief Reads the state of the eco power mode input pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @return Pin state (0 or 1).
 */
uint8_t ReadEcoPowerPin(GPIO_TypeDef* port, uint16_t pin);
/**
 * @brief Toggles the state of an LED pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 */
void ToggleLedPin(GPIO_TypeDef* port, uint16_t pin);
/**
 * @brief Sets the brake LED pin according to the brake status.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @param brake_on Non-zero to turn on, zero to turn off.
 */
void SetBrakeLedPin(GPIO_TypeDef* port, uint16_t pin, uint8_t brake_on);
/**
 * @brief Reads the state of the hazard switch input pin.
 * @param port GPIO port.
 * @param pin GPIO pin number.
 * @return Pin state (0 or 1).
 */
uint8_t ReadHazardPin(GPIO_TypeDef* port, uint16_t pin);

#endif //__DRIVERS_H__