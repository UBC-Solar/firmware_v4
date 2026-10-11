/**
 * @file    rtc_driver.h
 * @brief   RTC driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the RTC driver functions for the TEL board.
 *
 * Time is kept as UTC epoch milliseconds counted on HAL_GetTick(), which runs off the HSE
 * crystal. The STM32 RTC peripheral isn't used for timestamps: on this board it is clocked by
 * the LSI, which the F103 datasheet only specifies to 30-60 kHz, and its seconds and HAL_GetTick()
 * milliseconds rolled over at different moments, so timestamps went backwards once a second.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
#ifndef __RTC__DRIVER__H__
#define __RTC__DRIVER__H__

#include <stdbool.h>
#include <stdint.h>

/** @brief 2000-01-01T00:00:00Z. The clock counts up from this at boot until a source sets it. */
#define RTC_DRIVER_DEFAULT_EPOCH_MS     946684800000ULL

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

} RtcDriverTimeData;

typedef struct {
    uint8_t day;
    uint8_t month;
    uint8_t year;       // years since 2000
} RtcDriverDateData;

typedef enum {
    TIMETYPEDEF_SECONDS_IDX = 0,
    TIMETYPEDEF_MINUTES_IDX = 1,
    TIMETYPEDEF_HOURS_IDX = 2,
    TIMETYPEDEF_DAY_IDX = 3,
    TIMETYPEDEF_MONTH_IDX = 4,
    TIMETYPEDEF_YEAR_IDX = 5
} RtcDriverTimeIndexes;

/**
 * @brief Converts a UTC date and time to epoch milliseconds.
 * @param date Date, year counted from 2000
 * @param time Time of day
 * @param epoch_ms Set to the epoch milliseconds if the date and time are in range
 * @return true if the date and time were in range, false otherwise
 */
bool RtcDriverDateTimeToEpochMs(const RtcDriverDateData* date,
                                const RtcDriverTimeData* time,
                                uint64_t* epoch_ms);

/**
 * @brief Sets the clock. Timestamps jump to the new time, backwards if it is earlier.
 *        Safe to call from any task.
 * @param epoch_ms UTC milliseconds since 1970-01-01
 */
void RtcDriverSetEpochMs(uint64_t epoch_ms);

/**
 * @brief Moves the clock by a small amount. Timestamps never go backwards: after a negative
 *        adjustment the clock holds its last value until the new time catches up.
 *        Safe to call from any task.
 * @param delta_ms Milliseconds to add, negative to pull the clock back
 */
void RtcDriverAdjustEpochMs(int32_t delta_ms);

/**
 * @brief Gets the current time. Never less than the previous call's result unless
 *        RtcDriverSetEpochMs() moved the clock back. Safe to call from any task.
 * @return UTC milliseconds since 1970-01-01
 */
uint64_t RtcDriverGetEpochMs(void);

/**
 * @brief Get the current timestamp in seconds with milliseconds precision
 * @return double The current timestamp in seconds.
 */
double RtcDriverGetTimeStamp(void);

#endif /* __RTC__DRIVER__H__ */
