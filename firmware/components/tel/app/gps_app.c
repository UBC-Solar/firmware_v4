#include "gps_app.h"

#include "diagnostics.h"
#include "i2c.h"

uint8_t g_gps_raw_buffer[GPS_RAW_BUFFER_SIZE];
volatile uint16_t g_gps_raw_buffer_length = 0U;
static bool g_gps_read_failed = false;
static GPSDriverResult g_gps_init_result = GPS_DRIVER_NOT_READY;

static void GpsAppUpdateReadDiagnostic(GPSDriverResult result)
{
    bool read_failed = (result != GPS_DRIVER_OK);

    /*
     * The GPS task polls continuously. Only notify diagnostics when the
     * failure state changes; otherwise a successful poll needlessly calls the
     * fail-flag setter every 250 ms and makes a breakpoint there look like a
     * recurring failure.
     */
    if (read_failed != g_gps_read_failed)
    {
        DiagnosticsSetGpsReadFailFlag(read_failed);
        g_gps_read_failed = read_failed;
    }
}

GPSDriverResult GpsAppInit(void)
{
    GpsParserInit();
    GPSDriverResult result = GpsDriverInit(&hi2c2);

    g_gps_init_result = result;
    GpsAppUpdateReadDiagnostic(result);
    DiagnosticsSetGpsWriteFailFlag((result == GPS_DRIVER_I2C_ERROR) ||
                                   (result == GPS_DRIVER_CONFIG_REJECTED) ||
                                   (result == GPS_DRIVER_ACK_TIMEOUT) ||
                                   (result == GPS_DRIVER_FIRMWARE_NOT_RUNNING));

    g_gps_debug.init_result = result;
    g_gps_debug.init_complete = true;

    return result;
}

GPSDriverResult GpsAppReadRawData(void)
{
    uint16_t bytes_read = 0U;
    GPSDriverResult result;

    if (g_gps_init_result != GPS_DRIVER_OK)
    {
        g_gps_debug.last_read_result = g_gps_init_result;
        GpsAppUpdateReadDiagnostic(g_gps_init_result);
        return g_gps_init_result;
    }

    result = GpsDriverReadRawData(&hi2c2,
                                  g_gps_raw_buffer,
                                  GPS_RAW_BUFFER_SIZE,
                                  &bytes_read);

    /* Keep the last complete read visible between GPS output intervals. */
    if ((result == GPS_DRIVER_OK) && (bytes_read > 0U))
    {
        g_gps_raw_buffer_length = bytes_read;
        GpsParserConsume(g_gps_raw_buffer, bytes_read);
    }

    GpsAppUpdateReadDiagnostic(result);
    return result;
}
