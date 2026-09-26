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

/**
 * @brief Toggles the state of the debug LED.
 */
void GpioDriverToggleDebugLed(void);

#endif /* INC_GPIO_DRIVER_H_ */
