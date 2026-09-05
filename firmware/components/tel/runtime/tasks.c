/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementations for TEL board application logic
 *
 * This file contains the implementation of all FreeRTOS tasks for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

#include "tasks.h"
#include "CAN_comms.h"
#include "cmsis_os2.h"
#include "usart.h"
#include "rtc.h"
#include "telemetry_app.h"
#include "i2c.h"
#include <string.h>
#include "diagnostics.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        // TODO: Implement IMU data acquisition and processing
        osDelay(osWaitForever);
    }
}

/* ==========================================================================
 * GPS HARDWARE VERIFICATION -- temporary bring-up code, not a driver.
 *
 * Nothing is configured on the receiver. A NEO-M9V running its flash firmware
 * streams NMEA on I2C by default, so a healthy module produces ASCII '$'
 * sentences here with zero setup. Watch the globals below in the debugger.
 * ========================================================================== */

#define GPS_ADDR    (0x42 << 1)   /* 7-bit addr 0x42, shifted for the HAL */
#define GPS_TIMEOUT 100U

volatile uint8_t  gps_ready;     /* 1 = module ACKed its I2C address        */
volatile uint8_t  gps_ver_len;   /* bytes of the MON-VER reply captured     */
volatile uint16_t gps_avail;     /* byte count from registers 0xFD/0xFE     */
volatile uint16_t gps_len;       /* bytes placed in gps_buf this pass       */
volatile uint32_t gps_total;     /* cumulative bytes read since boot        */
volatile uint32_t gps_loops;     /* loop count, proves the task is running  */
volatile uint32_t gps_err;       /* last HAL_I2C_GetError() value           */
volatile HAL_StatusTypeDef gps_hal;  /* last HAL status: 0=OK 1=ERR 2=BUSY 3=TMO */
volatile HAL_StatusTypeDef gps_rst_hal;  /* status of the CFG-CFG reset write   */

uint8_t gps_ver[100];            /* MON-VER reply -- read the ASCII here    */
uint8_t gps_buf[200];            /* live stream bytes                       */

void TasksGPS(void *argument)
{
    (void)argument; // Unused parameter

    /* UBX-MON-VER poll, checksum precomputed */
    static const uint8_t mon_ver_poll[8] =
        { 0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34 };
    uint8_t cnt[2];
    uint16_t n;

    /* STEP 1: does the module ACK its address at all? */
    gps_hal = HAL_I2C_IsDeviceReady(&hi2c2, GPS_ADDR, 5, GPS_TIMEOUT);
    gps_err = HAL_I2C_GetError(&hi2c2);
    gps_ready = (gps_hal == HAL_OK) ? 1U : 0U;

    /* UBX-CFG-CFG: clear + load all config = revert to factory defaults */
    static const uint8_t cfg_reset[21] = {
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
        0xFF, 0xFB, 0x00, 0x00,   /* clearMask  */
        0x00, 0x00, 0x00, 0x00,   /* saveMask   */
        0xFF, 0xFB, 0x00, 0x00,   /* loadMask   */
        0x17,                     /* deviceMask */
        0x27, 0x6E };
    gps_rst_hal = HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDR,
                                          (uint8_t *)cfg_reset, 21, GPS_TIMEOUT);
    osDelay(2000);

    /* STEP 2: poll MON-VER once and capture the reply */
    gps_hal = HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDR,
                                      (uint8_t *)mon_ver_poll, 8, GPS_TIMEOUT);
    gps_err = HAL_I2C_GetError(&hi2c2);
    osDelay(200);

    gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFD,
                               I2C_MEMADD_SIZE_8BIT, cnt, 2, GPS_TIMEOUT);
    gps_err = HAL_I2C_GetError(&hi2c2);
    gps_avail = ((uint16_t)cnt[0] << 8) | cnt[1];

    if ((gps_hal == HAL_OK) && (gps_avail > 0U))
    {
        n = (gps_avail > sizeof(gps_ver)) ? sizeof(gps_ver) : gps_avail;
        gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFF,
                                   I2C_MEMADD_SIZE_8BIT, gps_ver, n, GPS_TIMEOUT);
        gps_err = HAL_I2C_GetError(&hi2c2);
        if (gps_hal == HAL_OK)
        {
            gps_ver_len = (uint8_t)n;
        }
    }

    /* STEP 3: read whatever the module streams on its own, forever */
    for (;;)
    {
        gps_loops++;
        gps_len = 0U;

        /* Re-poll MON-VER every pass so a slow boot cannot hide the answer. */
        HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDR,
                                (uint8_t *)mon_ver_poll, 8, GPS_TIMEOUT);
        osDelay(200);

        gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFD,
                                   I2C_MEMADD_SIZE_8BIT, cnt, 2, GPS_TIMEOUT);
        gps_err = HAL_I2C_GetError(&hi2c2);
        gps_avail = ((uint16_t)cnt[0] << 8) | cnt[1];

        if ((gps_hal == HAL_OK) && (gps_avail > 0U))
        {
            n = (gps_avail > sizeof(gps_buf)) ? sizeof(gps_buf) : gps_avail;
            memset(gps_buf, 0, sizeof(gps_buf));
            gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFF,
                                       I2C_MEMADD_SIZE_8BIT, gps_buf, n, GPS_TIMEOUT);
            gps_err = HAL_I2C_GetError(&hi2c2);
            if (gps_hal == HAL_OK)
            {
                gps_len = n;
                gps_total += n;
            }
        }

        osDelay(500);
    }
}

/* DIAGNOSTICS TASK */
void TasksDiagnostics(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        DiagnosticsSendTelFlags();
        osDelay(DIAGNOSTICS_TASK_DELAY);
    }
}

/* TEL HEARTBEAT TASK */
void TimeSinceStartup(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        DiagnosticsTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY); // Delay for specified time
    }
}
