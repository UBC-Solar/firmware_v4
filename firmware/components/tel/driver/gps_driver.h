/**
 * @file    gps_driver.h
 * @brief   GPS driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the u-blox GPS I2C/UBX driver for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Sep 22 2026
 */

#ifndef GPS_DRIVER_H_
#define GPS_DRIVER_H_

/**
 * @brief   Initialize the GPS receiver over I2C.
 * @return  None
 */
void GpsDriverInit(void);

#endif /* GPS_DRIVER_H_ */
