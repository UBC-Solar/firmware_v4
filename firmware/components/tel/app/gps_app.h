/**
 * @file    gps_app.h
 * @brief   GPS application header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the GPS application, which reads the
 * receiver's position solution and sends it over CAN and telemetry.
 *
 * CAN messages (all little-endian):
 *   0x758 GPS_LONG_LAT           [0:3] int32 longitude deg * 1e7, [4:7] int32 latitude deg * 1e7
 *   0x759 GPS_ALT_SPEED_HEADING  [0:3] int32 height above MSL mm, [4:5] uint16 ground speed cm/s,
 *                                [6:7] uint16 heading of motion deg * 100
 *   0x75A GPS_STATUS             [0] fix type, [1] flags (GPS_APP_STATUS_*), [2] satellites used,
 *                                [3] satellites heard (any signal), [4:5] uint16 PDOP * 100,
 *                                [6:7] uint16 horizontal accuracy cm
 *   0x75B GPS_UTC_TIME           [0] sec, [1] min, [2] hour, [3] day, [4] month, [5] year - 2000
 *
 * 0x758 and 0x759 are only sent with a valid fix, 0x75B only with a valid UTC date and time.
 *
 * @author  Raymond Shen
 * @date    Oct 3 2026
 */

#ifndef __GPS__APP__H__
#define __GPS__APP__H__

/* GPS_STATUS flags byte */
#define GPS_APP_STATUS_FIX_OK           0x01U
#define GPS_APP_STATUS_DIFF_SOLN        0x02U
#define GPS_APP_STATUS_DATE_VALID       0x04U
#define GPS_APP_STATUS_TIME_VALID       0x08U
#define GPS_APP_STATUS_FULLY_RESOLVED   0x10U

/**
 * @brief Initializes the GPS receiver. Retried by GpsAppTask if it fails.
 * @retval None
 */
void GpsAppInit(void);

/**
 * @brief Polls the receiver for a new solution and sends it over CAN and telemetry.
 *        Blocks for GPS_APP_RESPONSE_DELAY while the receiver prepares its reply.
 * @retval None
 */
void GpsAppTask(void);

#endif /* __GPS__APP__H__ */
