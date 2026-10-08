/**
 * @file    gps_app.c
 * @brief   GPS application implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the GPS application for the TEL board.
 *
 * @author  Raymond Shen
 * @date    Oct 3 2026
 */

#include "gps_app.h"
#include "gps_driver.h"
#include "can_driver.h"
#include "telemetry_app.h"
#include "diagnostics.h"
#include "cmsis_os2.h"
#include <string.h>

/* DEFINES */
#define GPS_APP_RESPONSE_DELAY          100     // ms for the receiver to queue its NAV-PVT and NAV-SAT replies
#define GPS_APP_CAN_TX_DELAY            3       // ms between CAN queue and radio, as in diagnostics
#define GPS_APP_YEAR_OFFSET             2000U

/* STATIC VARIABLES */
static bool gps_initialized = false;
static GpsDriverPvtData gps_pvt;                // static: keeps it off the GPS task stack

/* STATIC FUNCTION PROTOTYPES */
/**
 * @brief  Queues a message on CAN and sends it over telemetry
 * @param  msg: Message to send
 * @retval None
 */
static void GpsAppSend(CAN_comms_Tx_msg_t* msg);
/**
 * @brief  Sends 0x758 longitude and latitude
 * @param  pvt: Solution to send
 * @retval None
 */
static void GpsAppSendLongLat(const GpsDriverPvtData* pvt);
/**
 * @brief  Sends 0x759 altitude, ground speed and heading
 * @param  pvt: Solution to send
 * @retval None
 */
static void GpsAppSendAltSpeedHeading(const GpsDriverPvtData* pvt);
/**
 * @brief  Sends 0x75A fix status
 * @param  pvt: Solution to send
 * @retval None
 */
static void GpsAppSendStatus(const GpsDriverPvtData* pvt);
/**
 * @brief  Sends 0x75B UTC date and time
 * @param  pvt: Solution to send
 * @retval None
 */
static void GpsAppSendUtcTime(const GpsDriverPvtData* pvt);
/**
 * @brief  Writes a 16-bit value little-endian
 * @param  dst: Destination of 2 bytes
 * @param  value: Value to write
 * @retval None
 */
static void PutU16(uint8_t* dst, uint16_t value);
/**
 * @brief  Writes a 32-bit value little-endian
 * @param  dst: Destination of 4 bytes
 * @param  value: Value to write
 * @retval None
 */
static void PutU32(uint8_t* dst, uint32_t value);
/**
 * @brief  Clamps a value into the uint16 range
 * @param  value: Value to clamp
 * @retval The clamped value
 */
static uint16_t ClampU16(int32_t value);

/* FUNCTION DEFINITIONS */
void GpsAppInit(void)
{
    GpsDriverStatus status = GpsDriverInit();

    DiagnosticsSetGpsReadFailFlag(status == GPS_DRIVER_READ_FAIL);
    DiagnosticsSetGpsWriteFailFlag(status == GPS_DRIVER_WRITE_FAIL);
    gps_initialized = (status == GPS_DRIVER_OK);
}

void GpsAppTask(void)
{
    bool new_pvt = false;

    if (!gps_initialized)
    {
        GpsAppInit();
        if (!gps_initialized)
        {
            return;
        }
    }

    bool write_ok = (GpsDriverRequestPvt() == GPS_DRIVER_OK);
    write_ok = (GpsDriverRequestSat() == GPS_DRIVER_OK) && write_ok;
    DiagnosticsSetGpsWriteFailFlag(!write_ok);

    osDelay(GPS_APP_RESPONSE_DELAY);

    GpsDriverStatus read_status = GpsDriverRead(&gps_pvt, &new_pvt);
    DiagnosticsSetGpsReadFailFlag((read_status != GPS_DRIVER_OK) || !new_pvt);

    if (!new_pvt)
    {
        return;
    }

    GpsAppSendStatus(&gps_pvt);

    if ((gps_pvt.flags & GPS_DRIVER_FLAG_GNSS_FIX_OK) != 0U)
    {
        GpsAppSendLongLat(&gps_pvt);
        GpsAppSendAltSpeedHeading(&gps_pvt);
    }

    if ((gps_pvt.valid & (GPS_DRIVER_VALID_DATE | GPS_DRIVER_VALID_TIME)) == (GPS_DRIVER_VALID_DATE | GPS_DRIVER_VALID_TIME))
    {
        GpsAppSendUtcTime(&gps_pvt);
    }
}

static void GpsAppSend(CAN_comms_Tx_msg_t* msg)
{
    CAN_comms_Add_Tx_message(msg);
    osDelay(GPS_APP_CAN_TX_DELAY);
    TelAppTransmitInternalMsg(msg);
}

static void GpsAppSendLongLat(const GpsDriverPvtData* pvt)
{
    CAN_comms_Tx_msg_t msg = {
        .header = gps_long_lat_can_header,
    };

    PutU32(&msg.data[0], (uint32_t)pvt->lon_e7);
    PutU32(&msg.data[4], (uint32_t)pvt->lat_e7);
    GpsAppSend(&msg);
}

static void GpsAppSendAltSpeedHeading(const GpsDriverPvtData* pvt)
{
    CAN_comms_Tx_msg_t msg = {
        .header = gps_alt_speed_heading_can_header,
    };

    PutU32(&msg.data[0], (uint32_t)pvt->height_msl_mm);
    PutU16(&msg.data[4], ClampU16(pvt->ground_speed_mm_s / 10));   // mm/s -> cm/s
    PutU16(&msg.data[6], ClampU16(pvt->heading_e5 / 1000));        // deg * 1e5 -> deg * 100
    GpsAppSend(&msg);
}

static void GpsAppSendStatus(const GpsDriverPvtData* pvt)
{
    uint8_t flags = 0U;

    if ((pvt->flags & GPS_DRIVER_FLAG_GNSS_FIX_OK) != 0U) { flags |= GPS_APP_STATUS_FIX_OK; }
    if ((pvt->flags & GPS_DRIVER_FLAG_DIFF_SOLN) != 0U)   { flags |= GPS_APP_STATUS_DIFF_SOLN; }
    if ((pvt->valid & GPS_DRIVER_VALID_DATE) != 0U)       { flags |= GPS_APP_STATUS_DATE_VALID; }
    if ((pvt->valid & GPS_DRIVER_VALID_TIME) != 0U)       { flags |= GPS_APP_STATUS_TIME_VALID; }
    if ((pvt->valid & GPS_DRIVER_FULLY_RESOLVED) != 0U)   { flags |= GPS_APP_STATUS_FULLY_RESOLVED; }

    CAN_comms_Tx_msg_t msg = {
        .data[0] = pvt->fix_type,
        .data[1] = flags,
        .data[2] = pvt->num_sv,
        .data[3] = pvt->num_sv_heard,
        .header = gps_status_can_header,
    };

    PutU16(&msg.data[4], pvt->pdop_e2);
    PutU16(&msg.data[6], (pvt->h_acc_mm / 10U > UINT16_MAX) ? UINT16_MAX : (uint16_t)(pvt->h_acc_mm / 10U));  // mm -> cm
    GpsAppSend(&msg);
}

static void GpsAppSendUtcTime(const GpsDriverPvtData* pvt)
{
    CAN_comms_Tx_msg_t msg = {
        .data[0] = pvt->second,
        .data[1] = pvt->minute,
        .data[2] = pvt->hour,
        .data[3] = pvt->day,
        .data[4] = pvt->month,
        .data[5] = (uint8_t)(pvt->year - GPS_APP_YEAR_OFFSET),
        .header = gps_utc_time_can_header,
    };

    GpsAppSend(&msg);
}

static void PutU16(uint8_t* dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)(value >> 8);
}

static void PutU32(uint8_t* dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8) & 0xFFU);
    dst[2] = (uint8_t)((value >> 16) & 0xFFU);
    dst[3] = (uint8_t)(value >> 24);
}

static uint16_t ClampU16(int32_t value)
{
    if (value < 0)
    {
        return 0U;
    }
    if (value > (int32_t)UINT16_MAX)
    {
        return UINT16_MAX;
    }
    return (uint16_t)value;
}
