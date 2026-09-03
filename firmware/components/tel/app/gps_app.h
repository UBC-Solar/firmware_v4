#ifndef GPS_APP_H
#define GPS_APP_H

#include "gps_driver.h"
#include "gps_parser.h"

#define GPS_DELAY 250
#define GPS_RAW_BUFFER_SIZE 500U

extern uint8_t g_gps_raw_buffer[GPS_RAW_BUFFER_SIZE];
extern volatile uint16_t g_gps_raw_buffer_length;

/**
 * @brief Initialize and configure the NEO-M9V connected to TEL I2C2.
 * @return GPS driver initialization result.
 */
GPSDriverResult GpsAppInit(void);

/**
 * @brief Replace the raw GPS buffer when new I2C stream bytes are available.
 * @return GPS driver result. g_gps_raw_buffer_length identifies the most recent
 *         valid bytes and remains unchanged when no new bytes are waiting.
 */
GPSDriverResult GpsAppReadRawData(void);

#endif /* GPS_APP_H */
