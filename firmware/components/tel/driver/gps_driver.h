#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include "stm32f1xx_hal.h"

#define GPS_I2C_ADDRESS_7BIT       0x42U
#define GPS_I2C_ADDRESS_HAL        (GPS_I2C_ADDRESS_7BIT << 1)

#define GPS_REG_BYTES_AVAILABLE    0xFDU
#define GPS_REG_STREAM             0xFFU

typedef enum
{
    GPS_DRIVER_OK = 0,
    GPS_DRIVER_INVALID_ARGUMENT,
    GPS_DRIVER_NOT_READY,
    GPS_DRIVER_I2C_ERROR
} GPSDriverResult;

/**
 * @brief Wait for the GPS to become available and apply its startup UBX configuration.
 * @param hi2c I2C peripheral connected to the GPS module.
 * @return GPS_DRIVER_OK if the module acknowledged its address and the configuration
 *         frame was transmitted successfully. This does not parse UBX-ACK-ACK yet.
 */
GPSDriverResult GpsDriverInit(I2C_HandleTypeDef *hi2c);

/**
 * @brief Read currently available bytes from the GPS I2C stream.
 * @param hi2c I2C peripheral connected to the GPS module.
 * @param buffer Destination for raw UBX/NMEA stream bytes.
 * @param buffer_capacity Maximum number of bytes that can be written to buffer.
 * @param bytes_read Number of bytes placed in buffer; zero when no data is available.
 * @return GPS driver result.
 */
GPSDriverResult GpsDriverReadRawData(I2C_HandleTypeDef *hi2c,
                                     uint8_t *buffer,
                                     uint16_t buffer_capacity,
                                     uint16_t *bytes_read);

#endif /* GPS_DRIVER_H */
