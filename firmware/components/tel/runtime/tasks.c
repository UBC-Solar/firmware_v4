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
 * GPS FIX VERIFICATION -- temporary bring-up code, not a driver.
 *
 * Turns NMEA off on I2C (RAM layer only, reverts on power cycle) and polls
 * three UBX messages once a second. Watch the globals below in Live Watch.
 *   NAV-PVT : fix type, gnssFixOK, satellites used    (IM 3.1.5.3, p.12-13)
 *   NAV-SAT : per-satellite signal strength (C/N0)
 *   MON-RF  : antenna status, jamming indicator, AGC
 * I2C registers: 0xFD/0xFE = bytes waiting, 0xFF = data stream (IM 3.6.2.1).
 * ========================================================================== */

#define GPS_ADDR        (0x42 << 1)   /* 7-bit addr 0x42, shifted for the HAL */
#define GPS_TIMEOUT     100U
#define GPS_CHUNK       128U          /* bytes per stream read               */
#define GPS_DRAIN_MAX   4096U         /* max bytes drained per pass          */
#define GPS_POLL_MS     1000U
#define UBX_MAX_PAYLOAD 1024U         /* NAV-SAT is 8 + 12 * numSvs bytes    */

/* Link health */
volatile uint8_t  gps_ready;          /* 1 = module ACKed its I2C address                */
volatile uint8_t  gps_cfg_ack;        /* NMEA-off VALSET: 0 no reply, 1 ACK, 2 NAK       */
volatile uint32_t gps_loops;          /* poll count, proves the task is running          */
volatile uint32_t gps_rx_bytes;       /* total bytes read from the receiver              */
volatile uint32_t gps_frames;         /* UBX frames with a good checksum                 */
volatile uint32_t gps_ck_err;         /* UBX frames with a bad checksum                  */
volatile HAL_StatusTypeDef gps_hal;   /* last HAL status: 0=OK 1=ERR 2=BUSY 3=TMO        */

/* NAV-PVT: is there a fix? */
volatile uint8_t  gps_fix_type;       /* 0 none, 1 DR, 2 2D, 3 3D, 4 GNSS+DR, 5 time    */
volatile uint8_t  gps_fix_ok;         /* gnssFixOK: 1 = valid fix, use this (IM p.13)    */
volatile uint8_t  gps_num_sv;         /* satellites used in the solution                 */
volatile uint8_t  gps_valid;          /* bit0 validDate, bit1 validTime                  */
volatile uint32_t gps_hms;            /* UTC time as hhmmss                              */
volatile int32_t  gps_lat_e7;         /* latitude,  degrees * 1e7                        */
volatile int32_t  gps_lon_e7;         /* longitude, degrees * 1e7                        */
volatile uint32_t gps_hacc_mm;        /* horizontal accuracy estimate, mm                */

/* NAV-SAT: can it hear satellites? */
volatile uint8_t  gps_sv_total;       /* satellites in the receiver's list               */
volatile uint8_t  gps_sv_signal;      /* with any signal (C/N0 > 0)                      */
volatile uint8_t  gps_sv_locked;      /* code locked or better (qualityInd >= 4)         */
volatile uint8_t  gps_sv_used;        /* used in navigation                              */
volatile uint8_t  gps_cno_max;        /* strongest C/N0 in dBHz, want >= 35 (IM p.18)    */

/* MON-RF: antenna and interference */
volatile uint8_t  gps_ant_status;     /* 0 INIT, 1 DONTKNOW, 2 OK, 3 SHORT, 4 OPEN       */
volatile uint8_t  gps_ant_power;      /* 0 OFF, 1 ON, 2 DONTKNOW                         */
volatile uint8_t  gps_jam_state;      /* 0 unknown, 1 OK, 2 warning, 3 critical          */
volatile uint8_t  gps_jam_ind;        /* CW jamming, 0 none .. 255 strong                */
volatile uint16_t gps_agc_cnt;        /* AGC monitor, 0 .. 8191                          */
volatile uint16_t gps_noise;          /* noise level per ms                              */

static uint8_t  ubx_payload[UBX_MAX_PAYLOAD];
static uint8_t  ubx_state;
static uint8_t  ubx_cls;
static uint8_t  ubx_id;
static uint8_t  ubx_cka;
static uint8_t  ubx_ckb;
static uint16_t ubx_len;
static uint16_t ubx_idx;

static uint16_t UbxU2(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t UbxU4(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Frame and send a UBX message (payload <= 24 bytes). */
static void UbxSend(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
{
    static uint8_t frame[32];
    uint8_t a = 0U;
    uint8_t b = 0U;

    if (len > (sizeof(frame) - 8U))
    {
        return;
    }

    frame[0] = 0xB5U;
    frame[1] = 0x62U;
    frame[2] = cls;
    frame[3] = id;
    frame[4] = (uint8_t)(len & 0xFFU);
    frame[5] = (uint8_t)(len >> 8);
    if (len > 0U)
    {
        memcpy(&frame[6], payload, len);
    }
    for (uint16_t i = 2U; i < (6U + len); i++)
    {
        a += frame[i];
        b += a;
    }
    frame[6U + len] = a;
    frame[7U + len] = b;

    gps_hal = HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDR, frame, (uint16_t)(8U + len), GPS_TIMEOUT);
}

static void UbxHandle(uint8_t cls, uint8_t id, const uint8_t *p, uint16_t len)
{
    if ((cls == 0x01U) && (id == 0x07U) && (len >= 92U))            /* NAV-PVT */
    {
        gps_hms      = (uint32_t)p[8] * 10000U + (uint32_t)p[9] * 100U + p[10];
        gps_valid    = p[11] & 0x03U;
        gps_fix_type = p[20];
        gps_fix_ok   = p[21] & 0x01U;
        gps_num_sv   = p[23];
        gps_lon_e7   = (int32_t)UbxU4(&p[24]);
        gps_lat_e7   = (int32_t)UbxU4(&p[28]);
        gps_hacc_mm  = UbxU4(&p[40]);
    }
    else if ((cls == 0x01U) && (id == 0x35U) && (len >= 8U))        /* NAV-SAT */
    {
        uint8_t signal = 0U;
        uint8_t locked = 0U;
        uint8_t used   = 0U;
        uint8_t cmax   = 0U;
        uint8_t n      = p[5];

        for (uint16_t i = 0U; (i < n) && ((8U + 12U * (i + 1U)) <= len); i++)
        {
            const uint8_t *sv = &p[8U + 12U * i];
            uint8_t  cno   = sv[2];
            uint32_t flags = UbxU4(&sv[8]);

            if (cno > 0U)             { signal++; }
            if ((flags & 0x07U) >= 4U) { locked++; }
            if ((flags & 0x08U) != 0U) { used++; }
            if (cno > cmax)           { cmax = cno; }
        }
        gps_sv_total  = n;
        gps_sv_signal = signal;
        gps_sv_locked = locked;
        gps_sv_used   = used;
        gps_cno_max   = cmax;
    }
    else if ((cls == 0x0AU) && (id == 0x38U) && (len >= 28U))       /* MON-RF, first block */
    {
        const uint8_t *blk = &p[4];

        gps_jam_state  = blk[1] & 0x03U;
        gps_ant_status = blk[2];
        gps_ant_power  = blk[3];
        gps_noise      = UbxU2(&blk[12]);
        gps_agc_cnt    = UbxU2(&blk[14]);
        gps_jam_ind    = blk[16];
    }
    else if ((cls == 0x05U) && (len >= 2U) && (p[0] == 0x06U) && (p[1] == 0x8AU))  /* ACK for VALSET */
    {
        gps_cfg_ack = (id == 0x01U) ? 1U : 2U;
    }
}

/* Byte-wise UBX framer; frames may span reads, NMEA text is ignored. */
static void UbxFeed(uint8_t c)
{
    switch (ubx_state)
    {
        case 0U:
            if (c == 0xB5U) { ubx_state = 1U; }
            break;
        case 1U:
            ubx_state = (c == 0x62U) ? 2U : ((c == 0xB5U) ? 1U : 0U);
            break;
        case 2U:
            ubx_cls = c; ubx_cka = c; ubx_ckb = c;
            ubx_state = 3U;
            break;
        case 3U:
            ubx_id = c; ubx_cka += c; ubx_ckb += ubx_cka;
            ubx_state = 4U;
            break;
        case 4U:
            ubx_len = c; ubx_cka += c; ubx_ckb += ubx_cka;
            ubx_state = 5U;
            break;
        case 5U:
            ubx_len |= (uint16_t)c << 8; ubx_cka += c; ubx_ckb += ubx_cka;
            ubx_idx = 0U;
            if (ubx_len > UBX_MAX_PAYLOAD) { ubx_state = 0U; }
            else                           { ubx_state = (ubx_len == 0U) ? 7U : 6U; }
            break;
        case 6U:
            ubx_payload[ubx_idx++] = c; ubx_cka += c; ubx_ckb += ubx_cka;
            if (ubx_idx >= ubx_len) { ubx_state = 7U; }
            break;
        case 7U:
            if (c == ubx_cka) { ubx_state = 8U; }
            else              { gps_ck_err++; ubx_state = 0U; }
            break;
        default:
            if (c == ubx_ckb)
            {
                gps_frames++;
                UbxHandle(ubx_cls, ubx_id, ubx_payload, ubx_len);
            }
            else
            {
                gps_ck_err++;
            }
            ubx_state = 0U;
            break;
    }
}

/* Read everything the receiver has queued (up to GPS_DRAIN_MAX). */
static void GpsDrain(void)
{
    static uint8_t chunk[GPS_CHUNK];   /* static: TasksGPS stack is only 512 bytes */
    uint8_t  cnt[2];
    uint16_t budget = GPS_DRAIN_MAX;

    while (budget > 0U)
    {
        gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFD, I2C_MEMADD_SIZE_8BIT, cnt, 2, GPS_TIMEOUT);
        if (gps_hal != HAL_OK)
        {
            return;
        }

        uint16_t avail = ((uint16_t)cnt[0] << 8) | cnt[1];
        if (avail == 0U)
        {
            return;
        }

        uint16_t n = (avail > GPS_CHUNK) ? GPS_CHUNK : avail;
        if (n > budget)
        {
            n = budget;
        }

        gps_hal = HAL_I2C_Mem_Read(&hi2c2, GPS_ADDR, 0xFF, I2C_MEMADD_SIZE_8BIT, chunk, n, GPS_TIMEOUT);
        if (gps_hal != HAL_OK)
        {
            return;
        }

        gps_rx_bytes += n;
        for (uint16_t i = 0U; i < n; i++)
        {
            UbxFeed(chunk[i]);
        }
        budget -= n;
    }
}

void TasksGPS(void *argument)
{
    (void)argument; // Unused parameter

    /* CFG-VALSET: CFG-I2COUTPROT-NMEA (0x10720002) = 0 in RAM, so only UBX replies arrive */
    static const uint8_t nmea_off[9] = {
        0x00, 0x01, 0x00, 0x00,     /* version 0, layer RAM, reserved */
        0x02, 0x00, 0x72, 0x10,     /* key, little-endian             */
        0x00 };                     /* value: false                   */

    gps_hal = HAL_I2C_IsDeviceReady(&hi2c2, GPS_ADDR, 5, GPS_TIMEOUT);
    gps_ready = (gps_hal == HAL_OK) ? 1U : 0U;

    UbxSend(0x06U, 0x8AU, nmea_off, sizeof(nmea_off));
    osDelay(100);
    GpsDrain();

    for (;;)
    {
        gps_loops++;

        UbxSend(0x01U, 0x07U, NULL, 0U);    /* poll NAV-PVT */
        UbxSend(0x01U, 0x35U, NULL, 0U);    /* poll NAV-SAT */
        UbxSend(0x0AU, 0x38U, NULL, 0U);    /* poll MON-RF  */
        osDelay(100);
        GpsDrain();

        osDelay(GPS_POLL_MS - 100U);
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
