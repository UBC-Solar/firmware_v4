/**
 * @file    rtc_app.c
 * @brief   RTC application implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the RTC functions for the TEL board.
 *
 * GPS time discipline: TEL polls the receiver about once a second at its own phase, so each
 * solution's UTC second is 0-1.1 s old when read. That makes it a lower bound on the real time.
 * - Clock behind a solution: step forward to it.
 * - Clock ahead of every solution over a window long enough for the poll phase to sweep the
 *   whole GPS second: it's running fast, so slew it back 1 ms per solution. No backwards jumps.
 * - Clock more than RTC_APP_GPS_MAX_AHEAD_MS ahead: it's wrong, not slow to correct. Step back.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#include "rtc_app.h"
#include "can_app.h"
#include "rtc_driver.h"

#define RTC_APP_GPS_MAX_AHEAD_MS        2000    // more than a solution can be old (1 s poll + response delay)
#define RTC_APP_GPS_WINDOW_SAMPLES      90U     // ~1015 ms polls sweep the GPS second in about 70 s
#define RTC_APP_GPS_DEADBAND_MS         20      // allowed lead over the freshest solution in a window
#define RTC_APP_GPS_SLEW_MS             1       // pulled back per solution, so at most 1 ms/s

typedef enum {
    RTC_APP_SOURCE_NONE = 0,
    RTC_APP_SOURCE_MEMORATOR,
    RTC_APP_SOURCE_GPS,
} RtcAppSource;

static volatile RtcAppSource time_source = RTC_APP_SOURCE_NONE;

/* GPS discipline state, only touched by the GPS task */
static uint32_t window_samples = 0;
static int64_t window_min_ahead_ms = INT64_MAX;
static int64_t slew_remaining_ms = 0;

/**
 * @brief  Steps the clock to a GPS solution's time and restarts the slew window
 * @param  gps_ms: The solution's UTC time in epoch milliseconds
 * @retval None
 */
static void RtcAppStepToGps(uint64_t gps_ms);

void RtcAppSyncMemoratorToTel(uint8_t *data) {
    if (time_source != RTC_APP_SOURCE_NONE) {
        // Set once, and never over GPS time
        return;
    }
    RtcDriverTimeData rtc_time_data = {0};
    RtcDriverDateData rtc_date_data = {0};
    uint64_t epoch_ms = 0;

    rtc_time_data.seconds = data[TIMETYPEDEF_SECONDS_IDX];
    rtc_time_data.minutes = data[TIMETYPEDEF_MINUTES_IDX];
    rtc_time_data.hours = data[TIMETYPEDEF_HOURS_IDX];
    rtc_date_data.day = data[TIMETYPEDEF_DAY_IDX];
    rtc_date_data.month = data[TIMETYPEDEF_MONTH_IDX];
    rtc_date_data.year = data[TIMETYPEDEF_YEAR_IDX];

    if (RtcDriverDateTimeToEpochMs(&rtc_date_data, &rtc_time_data, &epoch_ms)) {
        RtcDriverSetEpochMs(epoch_ms);
        time_source = RTC_APP_SOURCE_MEMORATOR;
    }
}

void RtcAppSyncGpsToTel(const RtcDriverDateData* date, const RtcDriverTimeData* time)
{
    uint64_t gps_ms = 0;
    if (!RtcDriverDateTimeToEpochMs(date, time, &gps_ms))
    {
        return;
    }

    int64_t ahead_ms = (int64_t)(RtcDriverGetEpochMs() - gps_ms);
    if (time_source != RTC_APP_SOURCE_GPS || ahead_ms < 0 || ahead_ms > RTC_APP_GPS_MAX_AHEAD_MS)
    {
        RtcAppStepToGps(gps_ms);
        return;
    }

    if (ahead_ms < window_min_ahead_ms)
    {
        window_min_ahead_ms = ahead_ms;
    }
    if (++window_samples >= RTC_APP_GPS_WINDOW_SAMPLES)
    {
        // Even the freshest solution in the window was this far behind the clock
        if (window_min_ahead_ms > RTC_APP_GPS_DEADBAND_MS)
        {
            slew_remaining_ms = window_min_ahead_ms - RTC_APP_GPS_DEADBAND_MS;
        }
        window_samples = 0;
        window_min_ahead_ms = INT64_MAX;
    }
    if (slew_remaining_ms > 0)
    {
        RtcDriverAdjustEpochMs(-RTC_APP_GPS_SLEW_MS);
        slew_remaining_ms -= RTC_APP_GPS_SLEW_MS;
    }
}

static void RtcAppStepToGps(uint64_t gps_ms)
{
    RtcDriverSetEpochMs(gps_ms);
    time_source = RTC_APP_SOURCE_GPS;
    window_samples = 0;
    window_min_ahead_ms = INT64_MAX;
    slew_remaining_ms = 0;
}

void RtcAppRxHandler(uint32_t msg_id, uint8_t* data)
{
    switch (msg_id)
    {
    case RTC_TIMESTAMP_MSG_ID:
        RtcAppSyncMemoratorToTel(data);
        break;
    default:
        break;
    }
}
