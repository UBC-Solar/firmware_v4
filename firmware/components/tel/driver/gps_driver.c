#include "gps_driver.h"

#define GPS_READY_ATTEMPTS          20U
#define GPS_READY_TRIALS            1U
#define GPS_READY_RETRY_DELAY_MS    50U
#define GPS_I2C_TIMEOUT_MS          100U

/*
 * UBX-CFG-VALSET
 *
 * Configuration:
 *   CFG-I2CINPROT-UBX             = 1
 *   CFG-I2COUTPROT-UBX            = 1
 *   CFG-I2COUTPROT-NMEA           = 0
 *   CFG-RATE-MEAS                 = 200 ms (5 Hz)
 *   CFG-RATE-NAV                  = 1
 *   CFG-MSGOUT-UBX_NAV_PVAT_I2C   = 1
 *   CFG-MSGOUT-UBX_ESF_STATUS_I2C = 1
 *
 * UBX frame:
 *   Sync:     B5 62
 *   Class/ID: 06 8A
 *   Length:   29 00
 *   Payload:  41 bytes
 *   Checksum: 4D F7
 */
static const uint8_t g_gps_startup_config[] =
{
    0xB5, 0x62,
    0x06, 0x8A,
    0x29, 0x00,

    /* CFG-VALSET header: version 0, RAM layer */
    0x00, 0x01, 0x00, 0x00,

    /* CFG-I2CINPROT-UBX = 1 */
    0x01, 0x00, 0x71, 0x10,
    0x01,

    /* CFG-I2COUTPROT-UBX = 1 */
    0x01, 0x00, 0x72, 0x10,
    0x01,

    /* CFG-I2COUTPROT-NMEA = 0 */
    0x02, 0x00, 0x72, 0x10,
    0x00,

    /* CFG-RATE-MEAS = 200 ms */
    0x01, 0x00, 0x21, 0x30,
    0xC8, 0x00,

    /* CFG-RATE-NAV = 1 */
    0x02, 0x00, 0x21, 0x30,
    0x01, 0x00,

    /* CFG-MSGOUT-UBX_NAV_PVAT_I2C = 1 */
    0x2A, 0x06, 0x91, 0x20,
    0x01,

    /* CFG-MSGOUT-UBX_ESF_STATUS_I2C = 1 */
    0x05, 0x01, 0x91, 0x20,
    0x01,

    /* UBX checksum */
    0x4D, 0xF7
};

static GPSDriverResult GpsDriverReadyCheck(I2C_HandleTypeDef *hi2c)
{
    uint32_t attempt;

    if (hi2c == NULL)
    {
        return GPS_DRIVER_INVALID_ARGUMENT;
    }

    for (attempt = 0U; attempt < GPS_READY_ATTEMPTS; attempt++)
    {
        if (HAL_I2C_IsDeviceReady(hi2c,
                                  GPS_I2C_ADDRESS_HAL,
                                  GPS_READY_TRIALS,
                                  GPS_I2C_TIMEOUT_MS) == HAL_OK)
        {
            return GPS_DRIVER_OK;
        }

        HAL_Delay(GPS_READY_RETRY_DELAY_MS);
    }

    return GPS_DRIVER_NOT_READY;
}

GPSDriverResult GpsDriverInit(I2C_HandleTypeDef *hi2c)
{
    GPSDriverResult result = GpsDriverReadyCheck(hi2c);

    if (result != GPS_DRIVER_OK)
    {
        return result;
    }

    if (HAL_I2C_Master_Transmit(hi2c,
                                GPS_I2C_ADDRESS_HAL,
                                (uint8_t *)g_gps_startup_config,
                                (uint16_t)sizeof(g_gps_startup_config),
                                GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return GPS_DRIVER_I2C_ERROR;
    }

    /* UBX-ACK-ACK validation will be added with the UBX stream parser. */
    return GPS_DRIVER_OK;
}

GPSDriverResult GpsDriverReadRawData(I2C_HandleTypeDef *hi2c,
                                     uint8_t *buffer,
                                     uint16_t buffer_capacity,
                                     uint16_t *bytes_read)
{
    uint8_t available_bytes[2];
    uint16_t available_count;
    uint16_t read_count;

    if ((hi2c == NULL) || (buffer == NULL) ||
        (buffer_capacity == 0U) || (bytes_read == NULL))
    {
        return GPS_DRIVER_INVALID_ARGUMENT;
    }

    *bytes_read = 0U;

    if (HAL_I2C_Mem_Read(hi2c,
                         GPS_I2C_ADDRESS_HAL,
                         GPS_REG_BYTES_AVAILABLE,
                         I2C_MEMADD_SIZE_8BIT,
                         available_bytes,
                         (uint16_t)sizeof(available_bytes),
                         GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return GPS_DRIVER_I2C_ERROR;
    }

    available_count = ((uint16_t)available_bytes[0] << 8U) |
                      (uint16_t)available_bytes[1];

    if (available_count == 0U)
    {
        return GPS_DRIVER_OK;
    }

    read_count = (available_count < buffer_capacity) ? available_count : buffer_capacity;

    if (HAL_I2C_Mem_Read(hi2c,
                         GPS_I2C_ADDRESS_HAL,
                         GPS_REG_STREAM,
                         I2C_MEMADD_SIZE_8BIT,
                         buffer,
                         read_count,
                         GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return GPS_DRIVER_I2C_ERROR;
    }

    *bytes_read = read_count;
    return GPS_DRIVER_OK;
}
