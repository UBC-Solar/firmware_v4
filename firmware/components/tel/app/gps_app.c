#include "gps_app.h"

#include "diagnostics.h"
#include "i2c.h"

uint8_t g_gps_raw_buffer[GPS_RAW_BUFFER_SIZE];
volatile uint16_t g_gps_raw_buffer_length = 0U;

GPSDriverResult GpsAppInit(void)
{
    GPSDriverResult result = GpsDriverInit(&hi2c2);

    DiagnosticsSetGpsReadFailFlag(result == GPS_DRIVER_NOT_READY);
    DiagnosticsSetGpsWriteFailFlag(result == GPS_DRIVER_I2C_ERROR);

    return result;
}

GPSDriverResult GpsAppReadRawData(void)
{
    uint16_t bytes_read = 0U;
    GPSDriverResult result;

    g_gps_raw_buffer_length = 0U;
    result = GpsDriverReadRawData(&hi2c2,
                                  g_gps_raw_buffer,
                                  GPS_RAW_BUFFER_SIZE,
                                  &bytes_read);

    if (result == GPS_DRIVER_OK)
    {
        g_gps_raw_buffer_length = bytes_read;
    }

    DiagnosticsSetGpsReadFailFlag(result != GPS_DRIVER_OK);
    return result;
}
