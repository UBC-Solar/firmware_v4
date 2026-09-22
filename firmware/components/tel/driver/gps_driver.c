#include "gps_driver.h"

#include <stdbool.h>

#define GPS_READY_ATTEMPTS          20U
#define GPS_READY_TRIALS            1U
#define GPS_READY_RETRY_DELAY_MS    50U
#define GPS_I2C_TIMEOUT_MS          100U
#define GPS_CONFIG_ACK_TIMEOUT_MS   1000U
#define GPS_CONFIG_ACK_POLL_MS      5U
#define GPS_POLL_RESPONSE_ATTEMPTS  10U
#define GPS_POLL_RESPONSE_DELAY_MS  5U
#define GPS_MON_VER_ATTEMPTS        50U
#define GPS_CONFIG_FRAME_MAX_SIZE   18U
#define GPS_ACK_FRAME_SIZE          10U
#define GPS_ACK_READ_CHUNK_SIZE     64U

#define GPS_CFG_I2CINPROT_UBX             0x10710001UL
#define GPS_CFG_I2COUTPROT_UBX            0x10720001UL
#define GPS_CFG_I2COUTPROT_NMEA           0x10720002UL
#define GPS_CFG_I2C_EXTENDEDTIMEOUT        0x10510002UL
#define GPS_CFG_I2C_ENABLED                0x10510003UL
#define GPS_CFG_RATE_MEAS                  0x30210001UL
#define GPS_CFG_RATE_NAV                   0x30210002UL
#define GPS_CFG_MSGOUT_NAV_PVAT_I2C        0x2091062AUL
#define GPS_CFG_MSGOUT_NAV_PVT_I2C         0x20910006UL
#define GPS_CFG_MSGOUT_ESF_STATUS_I2C      0x20910105UL

volatile GPSDebugInfo g_gps_debug = {0};
uint8_t g_gps_mon_ver_buffer[GPS_MON_VER_BUFFER_SIZE] = {0U};

static bool GpsDriverBufferContains(const uint8_t *buffer,
                                    uint16_t buffer_length,
                                    const char *text)
{
    uint16_t text_length = 0U;
    uint16_t index;
    uint16_t text_index;

    while (text[text_length] != '\0')
    {
        text_length++;
    }

    if ((text_length == 0U) || (text_length > buffer_length))
    {
        return false;
    }

    for (index = 0U; index <= (uint16_t)(buffer_length - text_length); index++)
    {
        for (text_index = 0U; text_index < text_length; text_index++)
        {
            if (buffer[index + text_index] != (uint8_t)text[text_index])
            {
                break;
            }
        }

        if (text_index == text_length)
        {
            return true;
        }
    }

    return false;
}

static bool GpsDriverContainsCompleteFrame(const uint8_t *buffer,
                                           uint16_t length,
                                           uint8_t message_class,
                                           uint8_t message_id)
{
    uint16_t index;

    for (index = 0U; (index + 6U) <= length; index++)
    {
        if ((buffer[index] == 0xB5U) &&
            (buffer[index + 1U] == 0x62U) &&
            (buffer[index + 2U] == message_class) &&
            (buffer[index + 3U] == message_id))
        {
            uint16_t payload_length = (uint16_t)buffer[index + 4U] |
                                      ((uint16_t)buffer[index + 5U] << 8U);
            uint32_t frame_end = (uint32_t)index + 8UL + payload_length;

            if (frame_end <= length)
            {
                return true;
            }
        }
    }

    return false;
}

static GPSDriverResult GpsDriverReadMonVer(I2C_HandleTypeDef *hi2c)
{
    static const uint8_t mon_ver_poll[] =
    {
        0xB5U, 0x62U, 0x0AU, 0x04U, 0x00U, 0x00U, 0x0EU, 0x34U
    };
    uint8_t available_bytes[2];
    uint16_t available_count;
    uint16_t read_count;
    uint16_t total_length = 0U;
    uint8_t attempt;

    if (HAL_I2C_Master_Transmit(hi2c,
                                GPS_I2C_ADDRESS_HAL,
                                (uint8_t *)mon_ver_poll,
                                (uint16_t)sizeof(mon_ver_poll),
                                GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
        return GPS_DRIVER_I2C_ERROR;
    }

    for (attempt = 0U; attempt < GPS_MON_VER_ATTEMPTS; attempt++)
    {
        HAL_Delay(GPS_POLL_RESPONSE_DELAY_MS);
        if (HAL_I2C_Mem_Read(hi2c,
                             GPS_I2C_ADDRESS_HAL,
                             GPS_REG_BYTES_AVAILABLE,
                             I2C_MEMADD_SIZE_8BIT,
                             available_bytes,
                             (uint16_t)sizeof(available_bytes),
                             GPS_I2C_TIMEOUT_MS) != HAL_OK)
        {
            g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
            return GPS_DRIVER_I2C_ERROR;
        }

        available_count = ((uint16_t)available_bytes[0] << 8U) |
                          (uint16_t)available_bytes[1];
        if (available_count == 0U)
        {
            continue;
        }

        read_count = available_count;
        if (read_count > (uint16_t)(GPS_MON_VER_BUFFER_SIZE - total_length))
        {
            read_count = (uint16_t)(GPS_MON_VER_BUFFER_SIZE - total_length);
        }
        if (read_count == 0U)
        {
            break;
        }

        if (HAL_I2C_Mem_Read(hi2c,
                             GPS_I2C_ADDRESS_HAL,
                             GPS_REG_STREAM,
                             I2C_MEMADD_SIZE_8BIT,
                             &g_gps_mon_ver_buffer[total_length],
                             read_count,
                             GPS_I2C_TIMEOUT_MS) != HAL_OK)
        {
            g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
            return GPS_DRIVER_I2C_ERROR;
        }

        total_length = (uint16_t)(total_length + read_count);
        g_gps_debug.mon_ver_length = total_length;
        if (GpsDriverContainsCompleteFrame(g_gps_mon_ver_buffer,
                                           total_length,
                                           0x0AU,
                                           0x04U))
        {
            return GPS_DRIVER_OK;
        }
    }

    return GPS_DRIVER_ACK_TIMEOUT;
}

/*
 * Send each UBX-CFG-VALSET item separately. This prevents one unsupported
 * configuration key from rejecting every otherwise-valid setting.
 */
static GPSDriverResult GpsDriverWaitForValsetAck(I2C_HandleTypeDef *hi2c)
{
    static const uint8_t ack_prefix[] =
    {
        0xB5U, 0x62U, 0x05U, 0x01U, 0x02U, 0x00U, 0x06U, 0x8AU
    };
    static const uint8_t nak_prefix[] =
    {
        0xB5U, 0x62U, 0x05U, 0x00U, 0x02U, 0x00U, 0x06U, 0x8AU
    };
    uint8_t available_bytes[2];
    uint8_t read_buffer[GPS_ACK_READ_CHUNK_SIZE];
    uint8_t window[GPS_ACK_FRAME_SIZE] = {0U};
    uint16_t available_count;
    uint16_t read_count;
    uint16_t read_index;
    uint8_t window_length = 0U;
    uint32_t start_tick = HAL_GetTick();

    while ((HAL_GetTick() - start_tick) < GPS_CONFIG_ACK_TIMEOUT_MS)
    {
        if (HAL_I2C_Mem_Read(hi2c,
                             GPS_I2C_ADDRESS_HAL,
                             GPS_REG_BYTES_AVAILABLE,
                             I2C_MEMADD_SIZE_8BIT,
                             available_bytes,
                             (uint16_t)sizeof(available_bytes),
                             GPS_I2C_TIMEOUT_MS) != HAL_OK)
        {
            g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
            return GPS_DRIVER_I2C_ERROR;
        }

        available_count = ((uint16_t)available_bytes[0] << 8U) |
                          (uint16_t)available_bytes[1];

        while (available_count > 0U)
        {
            read_count = available_count;
            if (read_count > (uint16_t)sizeof(read_buffer))
            {
                read_count = (uint16_t)sizeof(read_buffer);
            }

            if (HAL_I2C_Mem_Read(hi2c,
                                 GPS_I2C_ADDRESS_HAL,
                                 GPS_REG_STREAM,
                                 I2C_MEMADD_SIZE_8BIT,
                                 read_buffer,
                                 read_count,
                                 GPS_I2C_TIMEOUT_MS) != HAL_OK)
            {
                g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
                return GPS_DRIVER_I2C_ERROR;
            }

            for (read_index = 0U; read_index < read_count; read_index++)
            {
                uint8_t index;
                bool ack_match = true;
                bool nak_match = true;

                if (window_length < GPS_ACK_FRAME_SIZE)
                {
                    window[window_length++] = read_buffer[read_index];
                }
                else
                {
                    for (index = 1U; index < GPS_ACK_FRAME_SIZE; index++)
                    {
                        window[index - 1U] = window[index];
                    }
                    window[GPS_ACK_FRAME_SIZE - 1U] = read_buffer[read_index];
                }

                if (window_length == GPS_ACK_FRAME_SIZE)
                {
                    for (index = 0U; index < (uint8_t)sizeof(ack_prefix); index++)
                    {
                        ack_match = ack_match && (window[index] == ack_prefix[index]);
                        nak_match = nak_match && (window[index] == nak_prefix[index]);
                    }

                    if (ack_match)
                    {
                        return GPS_DRIVER_OK;
                    }
                    if (nak_match)
                    {
                        return GPS_DRIVER_CONFIG_REJECTED;
                    }
                }
            }

            available_count = (uint16_t)(available_count - read_count);
        }

        HAL_Delay(GPS_CONFIG_ACK_POLL_MS);
    }

    return GPS_DRIVER_ACK_TIMEOUT;
}

static GPSDriverResult GpsDriverSetConfig(I2C_HandleTypeDef *hi2c,
                                           uint32_t key,
                                           uint16_t value,
                                           uint8_t value_size)
{
    uint8_t frame[GPS_CONFIG_FRAME_MAX_SIZE];
    uint16_t payload_length = (uint16_t)(8U + value_size);
    uint16_t frame_length = (uint16_t)(payload_length + 8U);
    uint16_t index;
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    GPSDriverResult result;

    g_gps_debug.last_config_key = key;

    frame[0] = 0xB5U;
    frame[1] = 0x62U;
    frame[2] = 0x06U;
    frame[3] = 0x8AU;
    frame[4] = (uint8_t)payload_length;
    frame[5] = 0U;
    frame[6] = 0U; /* version */
    frame[7] = 1U; /* RAM layer */
    frame[8] = 0U; /* transaction */
    frame[9] = 0U; /* reserved */
    frame[10] = (uint8_t)key;
    frame[11] = (uint8_t)(key >> 8U);
    frame[12] = (uint8_t)(key >> 16U);
    frame[13] = (uint8_t)(key >> 24U);
    frame[14] = (uint8_t)value;
    if (value_size == 2U)
    {
        frame[15] = (uint8_t)(value >> 8U);
    }

    for (index = 2U; index < (uint16_t)(frame_length - 2U); index++)
    {
        checksum_a = (uint8_t)(checksum_a + frame[index]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }
    frame[frame_length - 2U] = checksum_a;
    frame[frame_length - 1U] = checksum_b;

    if (HAL_I2C_Master_Transmit(hi2c,
                                GPS_I2C_ADDRESS_HAL,
                                frame,
                                frame_length,
                                GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
        g_gps_debug.last_config_result = GPS_DRIVER_I2C_ERROR;
        return GPS_DRIVER_I2C_ERROR;
    }

    result = GpsDriverWaitForValsetAck(hi2c);
    g_gps_debug.last_config_result = result;
    return result;
}

static GPSDriverResult GpsDriverPollNavMessage(I2C_HandleTypeDef *hi2c)
{
    uint8_t frame[8] =
    {
        0xB5U, 0x62U, 0x01U, g_gps_debug.configured_nav_message_id,
        0x00U, 0x00U, 0x00U, 0x00U
    };
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint8_t index;

    for (index = 2U; index < 6U; index++)
    {
        checksum_a = (uint8_t)(checksum_a + frame[index]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }
    frame[6] = checksum_a;
    frame[7] = checksum_b;

    if (HAL_I2C_Master_Transmit(hi2c,
                                GPS_I2C_ADDRESS_HAL,
                                frame,
                                (uint16_t)sizeof(frame),
                                GPS_I2C_TIMEOUT_MS) != HAL_OK)
    {
        g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
        return GPS_DRIVER_I2C_ERROR;
    }

    g_gps_debug.poll_request_count++;
    return GPS_DRIVER_OK;
}

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

    g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
    return GPS_DRIVER_NOT_READY;
}

GPSDriverResult GpsDriverInit(I2C_HandleTypeDef *hi2c)
{
    GPSDriverResult result;
    static const struct
    {
        uint32_t key;
        uint16_t value;
        uint8_t value_size;
    } startup_config[] =
    {
        { GPS_CFG_I2C_ENABLED,             1U,   1U },
        { GPS_CFG_I2C_EXTENDEDTIMEOUT,     1U,   1U },
        { GPS_CFG_RATE_MEAS,              200U, 2U },
        { GPS_CFG_RATE_NAV,               1U,   2U },
        { GPS_CFG_I2CINPROT_UBX,          1U,   1U },
        { GPS_CFG_I2COUTPROT_UBX,         1U,   1U },
        /* NAV-PVAT is the application data source; disable the additional
         * NMEA stream so the I2C output queue cannot outrun the GPS task. */
        { GPS_CFG_I2COUTPROT_NMEA,        0U,   1U }
    };
    uint8_t index;

    g_gps_debug.mon_ver_result = GPS_DRIVER_NOT_READY;
    g_gps_debug.mon_ver_length = 0U;
    g_gps_debug.mon_ver_rom_boot = false;

    result = GpsDriverReadyCheck(hi2c);

    if (result != GPS_DRIVER_OK)
    {
        return result;
    }

    /* Probe the actual receiver firmware before modifying its output stream. */
    g_gps_debug.mon_ver_result = GpsDriverReadMonVer(hi2c);
    if (g_gps_debug.mon_ver_result != GPS_DRIVER_OK)
    {
        return g_gps_debug.mon_ver_result;
    }

    /* A receiver in ROM BOOT responds over I2C and accepts some CFG keys,
     * but no GNSS/navigation engine is running, so NAV output remains empty.
     * Stop here instead of reporting a successful initialization. */
    g_gps_debug.mon_ver_rom_boot =
        GpsDriverBufferContains(g_gps_mon_ver_buffer,
                                g_gps_debug.mon_ver_length,
                                "ROM BOOT");
    if (g_gps_debug.mon_ver_rom_boot)
    {
        return GPS_DRIVER_FIRMWARE_NOT_RUNNING;
    }

    /* Prefer the NEO-M9V attitude message, but retain basic navigation data
     * on receivers/firmware that only support UBX-NAV-PVT. */
    result = GpsDriverSetConfig(hi2c, GPS_CFG_MSGOUT_NAV_PVAT_I2C, 1U, 1U);
    if (result == GPS_DRIVER_CONFIG_REJECTED)
    {
        result = GpsDriverSetConfig(hi2c, GPS_CFG_MSGOUT_NAV_PVT_I2C, 1U, 1U);
        if (result == GPS_DRIVER_OK)
        {
            g_gps_debug.configured_nav_message_id = 0x07U;
        }
    }
    else if (result == GPS_DRIVER_OK)
    {
        g_gps_debug.configured_nav_message_id = 0x17U;
    }
    if (result != GPS_DRIVER_OK)
    {
        return result;
    }

    /* ESF status is optional; some non-M9V firmware does not implement it. */
    result = GpsDriverSetConfig(hi2c, GPS_CFG_MSGOUT_ESF_STATUS_I2C, 1U, 1U);
    if ((result != GPS_DRIVER_OK) && (result != GPS_DRIVER_CONFIG_REJECTED))
    {
        return result;
    }

    for (index = 0U; index < (uint8_t)(sizeof(startup_config) / sizeof(startup_config[0])); index++)
    {
        result = GpsDriverSetConfig(hi2c,
                                    startup_config[index].key,
                                    startup_config[index].value,
                                    startup_config[index].value_size);
        if (result != GPS_DRIVER_OK)
        {
            return result;
        }
    }

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
    uint8_t poll_attempt;

    g_gps_debug.read_call_count++;
    g_gps_debug.last_available_count = 0U;
    g_gps_debug.last_bytes_read = 0U;

    if ((hi2c == NULL) || (buffer == NULL) ||
        (buffer_capacity == 0U) || (bytes_read == NULL))
    {
        g_gps_debug.last_read_result = GPS_DRIVER_INVALID_ARGUMENT;
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
        g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
        g_gps_debug.last_read_result = GPS_DRIVER_I2C_ERROR;
        return GPS_DRIVER_I2C_ERROR;
    }

    available_count = ((uint16_t)available_bytes[0] << 8U) |
                      (uint16_t)available_bytes[1];
    g_gps_debug.last_available_count = available_count;
    if (available_count > g_gps_debug.max_available_count)
    {
        g_gps_debug.max_available_count = available_count;
    }

    if (available_count == 0U)
    {
        if (g_gps_debug.configured_nav_message_id == 0U)
        {
            g_gps_debug.last_read_result = GPS_DRIVER_OK;
            return GPS_DRIVER_OK;
        }

        if (GpsDriverPollNavMessage(hi2c) != GPS_DRIVER_OK)
        {
            g_gps_debug.last_read_result = GPS_DRIVER_I2C_ERROR;
            return GPS_DRIVER_I2C_ERROR;
        }

        for (poll_attempt = 0U;
             (poll_attempt < GPS_POLL_RESPONSE_ATTEMPTS) && (available_count == 0U);
             poll_attempt++)
        {
            HAL_Delay(GPS_POLL_RESPONSE_DELAY_MS);
            if (HAL_I2C_Mem_Read(hi2c,
                                 GPS_I2C_ADDRESS_HAL,
                                 GPS_REG_BYTES_AVAILABLE,
                                 I2C_MEMADD_SIZE_8BIT,
                                 available_bytes,
                                 (uint16_t)sizeof(available_bytes),
                                 GPS_I2C_TIMEOUT_MS) != HAL_OK)
            {
                g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
                g_gps_debug.last_read_result = GPS_DRIVER_I2C_ERROR;
                return GPS_DRIVER_I2C_ERROR;
            }

            available_count = ((uint16_t)available_bytes[0] << 8U) |
                              (uint16_t)available_bytes[1];
            g_gps_debug.last_available_count = available_count;
            if (available_count > g_gps_debug.max_available_count)
            {
                g_gps_debug.max_available_count = available_count;
            }
        }

        if (available_count == 0U)
        {
            g_gps_debug.last_read_result = GPS_DRIVER_OK;
            return GPS_DRIVER_OK;
        }
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
        g_gps_debug.last_hal_error = HAL_I2C_GetError(hi2c);
        g_gps_debug.last_read_result = GPS_DRIVER_I2C_ERROR;
        return GPS_DRIVER_I2C_ERROR;
    }

    *bytes_read = read_count;
    g_gps_debug.last_bytes_read = read_count;
    g_gps_debug.nonempty_read_count++;
    g_gps_debug.total_bytes_read += read_count;
    g_gps_debug.last_read_result = GPS_DRIVER_OK;
    return GPS_DRIVER_OK;
}
