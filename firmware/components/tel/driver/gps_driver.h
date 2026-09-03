#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <stdbool.h>

#include "stm32f1xx_hal.h"

#define GPS_I2C_ADDRESS_7BIT       0x42U
#define GPS_I2C_ADDRESS_HAL        (GPS_I2C_ADDRESS_7BIT << 1)

#define GPS_REG_BYTES_AVAILABLE    0xFDU
#define GPS_REG_STREAM             0xFFU
#define GPS_MON_VER_BUFFER_SIZE    256U

typedef enum
{
    GPS_DRIVER_OK = 0,
    GPS_DRIVER_INVALID_ARGUMENT,
    GPS_DRIVER_NOT_READY,
    GPS_DRIVER_I2C_ERROR,
    GPS_DRIVER_CONFIG_REJECTED,
    GPS_DRIVER_ACK_TIMEOUT,
    GPS_DRIVER_FIRMWARE_NOT_RUNNING
} GPSDriverResult;

/** Persistent state intended for debugger live-watch and fault diagnosis. */
typedef struct
{
    volatile bool init_complete;
    volatile GPSDriverResult init_result;
    volatile uint32_t read_call_count;
    volatile GPSDriverResult last_read_result;
    volatile uint32_t last_hal_error;
    volatile uint32_t last_config_key;
    volatile GPSDriverResult last_config_result;
    volatile uint8_t configured_nav_message_id;
    volatile uint16_t last_available_count;
    volatile uint16_t last_bytes_read;
    volatile uint16_t max_available_count;
    volatile uint32_t poll_request_count;
    volatile uint32_t nonempty_read_count;
    volatile uint32_t total_bytes_read;
    volatile GPSDriverResult mon_ver_result;
    volatile uint16_t mon_ver_length;
    volatile bool mon_ver_rom_boot;
} GPSDebugInfo;

extern volatile GPSDebugInfo g_gps_debug;
extern uint8_t g_gps_mon_ver_buffer[GPS_MON_VER_BUFFER_SIZE];

/**
 * @brief Wait for the GPS to become available and apply its startup UBX configuration.
 * @param hi2c I2C peripheral connected to the GPS module.
 * @return GPS_DRIVER_OK if the module acknowledged its address and each required
 *         configuration item; otherwise the specific readiness, bus, NAK, or timeout result.
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
