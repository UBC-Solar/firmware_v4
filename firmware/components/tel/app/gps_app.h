#ifndef GPS_APP_H
#define GPS_APP_H

#include "gps_driver.h"

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
 * @brief Replace the raw GPS buffer with the currently available I2C stream bytes.
 * @return GPS driver result. g_gps_raw_buffer_length identifies the valid bytes.
 */
GPSDriverResult GpsAppReadRawData(void);

#endif /* GPS_APP_H */
