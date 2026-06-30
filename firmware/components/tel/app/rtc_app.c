/**
 * @file    rtc_app.c
 * @brief   RTC application implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the RTC functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#include "rtc_app.h"
#include "can_app.h"
#include "rtc_driver.h"

static bool rtc_initialized = false;

void RtcAppSyncMemoratorToTel(uint8_t *data) {
    if (rtc_initialized) {
        // If RTC is already initialized, we can ignore further sync messages
        return;
    }
    RtcDriverTimeData rtc_time_data = {0};
    RtcDriverDateData rtc_date_data = {0};

    rtc_time_data.seconds = data[TIMETYPEDEF_SECONDS_IDX];
    rtc_time_data.minutes = data[TIMETYPEDEF_MINUTES_IDX];
    rtc_time_data.hours = data[TIMETYPEDEF_HOURS_IDX];
    rtc_date_data.day = data[TIMETYPEDEF_DAY_IDX];
    rtc_date_data.month = data[TIMETYPEDEF_MONTH_IDX];
    rtc_date_data.year = data[TIMETYPEDEF_YEAR_IDX];

    RtcDriverSetTime(&rtc_time_data);
    RtcDriverSetDate(&rtc_date_data);
    rtc_initialized = true; // Set the RTC initialized flag to true when we receive data to sync
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