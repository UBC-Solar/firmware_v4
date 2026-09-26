/**
 * @file    gpio_driver.h
 * @brief   GPIO driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes for the GPIO driver functions for the TEL board. Application
 * code should use these wrappers instead of calling the HAL directly; the GPIO ports and pins
 * themselves are private to the driver.
 *
 * @author  Gregory Bian
 * @date    Aug 25 2026
 */
#ifndef __GPIO__DRIVER__H__
#define __GPIO__DRIVER__H__

/* FUNCTION PROTOTYPES */
/**
 * @brief Toggles the state of the debug LED.
 */
void GpioDriverToggleDebugLed(void);

#endif /* __GPIO__DRIVER__H__ */
