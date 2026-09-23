/**
 * @file    gps_app.h
 * @brief   GPS application header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the GPS functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Sep 22 2026
 */

#ifndef GPS_APP_H_
#define GPS_APP_H_

#define GPS_TASK_DELAY 1000 // Delay in milliseconds for the GPS task

/**
 * @brief   Initialize the GPS driver and configure the receiver.
 * @return  None
 */
void GpsAppInit(void);

/**
 * @brief   Poll the GPS and process any received data.
 * @return  None
 */
void GpsAppTask(void);

#endif /* GPS_APP_H_ */
