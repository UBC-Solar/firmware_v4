/**
 * @file    gpio_driver.h
 * @brief   GPIO driver header file for UBC Solar MDI board
 *
 * This file contains the prototypes for the GPIO driver functions for the MDI board. Application
 * code should use these wrappers instead of calling the HAL directly; the GPIO ports and pins
 * themselves are private to the driver.
 */
#ifndef INC_GPIO_DRIVER_H_
#define INC_GPIO_DRIVER_H_
#include <stdbool.h>

/**
 * @brief Toggles the state of the debug LED.
 */
void GpioDriverToggleDebugLed(void);

/**
 * @brief Toggles the state of the RTD fault LED.
 */
 void GpioDriverToggleRtdFaultLed(bool fault);

/**
 * @brief Toggles the state of the forward direction LEDs.
 */
 void GpioDriverToggleFwdLeds(void);

 /**
  * @brief Toggles the state of the reverse direction LEDs.
  */
 void GpioDriverToggleRevLeds(void);
 
 /**
  * @brief Turns on the regen braking LED.
  */
 void GpioDriverEnableRegenLed(void);
 
 /**
  * @brief Turns off the regen braking LED.
  */
 void GpioDriverDisableRegenLed(void);
 
 /**
  * @brief Turns on the eco mode LEDs.
  */
 void GpioDriverEnableEcoLeds(void);
 
 /**
  * @brief Turns off the eco mode LEDs.
  */
 void GpioDriverDisableEcoLeds(void);

#endif /* INC_GPIO_DRIVER_H_ */
