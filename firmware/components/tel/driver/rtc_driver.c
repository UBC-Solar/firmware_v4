/**
 * @file    rtc_driver.c
 * @brief   RTC driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the RTC driver functions for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */
#include "rtc_driver.h"
#include "stm32f1xx_hal.h"

#define MILLISECONDS_PER_SECOND         1000ULL
#define SECONDS_PER_MINUTE              60U
#define SECONDS_PER_HOUR                3600U
#define SECONDS_PER_DAY                 86400ULL
#define RTC_DRIVER_YEAR_OFFSET          2000
#define MAX_MONTH                       12U
#define MAX_DAY                         31U
#define MAX_HOUR                        23U
#define MAX_MINUTE                      59U
#define MAX_SECOND                      60U     // 60 during a leap second

/* Epoch milliseconds at HAL tick base_tick. Starts at 2000-01-01 at boot (tick 0). */
static uint64_t base_epoch_ms = RTC_DRIVER_DEFAULT_EPOCH_MS;
static uint32_t base_tick = 0;
/* Last time returned. Reads never go below it, so RtcDriverAdjustEpochMs() can't go backwards. */
static uint64_t last_epoch_ms = 0;

/**
 * @brief  Counts days from 1970-01-01 to a date in the proleptic Gregorian calendar.
 *         Howard Hinnant's days_from_civil, so no mktime() and no time zone handling.
 * @param  year: Full year, e.g. 2026
 * @param  month: 1-12
 * @param  day: 1-31
 * @retval Days since 1970-01-01
 */
static int32_t RtcDriverDaysFromCivil(int32_t year, uint32_t month, uint32_t day)
{
    year -= (month <= 2U) ? 1 : 0;
    const int32_t era = ((year >= 0) ? year : (year - 399)) / 400;
    const uint32_t year_of_era = (uint32_t)(year - era * 400);
    const uint32_t day_of_year = (153U * ((month > 2U) ? (month - 3U) : (month + 9U)) + 2U) / 5U + day - 1U;
    const uint32_t day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U + day_of_year;
    return era * 146097 + (int32_t)day_of_era - 719468;
}

bool RtcDriverDateTimeToEpochMs(const RtcDriverDateData* date,
                                const RtcDriverTimeData* time,
                                uint64_t* epoch_ms)
{
    if (date->month < 1U || date->month > MAX_MONTH || date->day < 1U || date->day > MAX_DAY ||
        time->hours > MAX_HOUR || time->minutes > MAX_MINUTE || time->seconds > MAX_SECOND)
    {
        return false;
    }

    int32_t days = RtcDriverDaysFromCivil(RTC_DRIVER_YEAR_OFFSET + date->year, date->month, date->day);
    uint64_t seconds = (uint64_t)days * SECONDS_PER_DAY + time->hours * SECONDS_PER_HOUR +
                       time->minutes * SECONDS_PER_MINUTE + time->seconds;
    *epoch_ms = seconds * MILLISECONDS_PER_SECOND;
    return true;
}

void RtcDriverSetEpochMs(uint64_t epoch_ms)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();        // the 64-bit base and the tick have to change together
    base_epoch_ms = epoch_ms;
    base_tick = HAL_GetTick();
    last_epoch_ms = 0;
    __set_PRIMASK(primask);
}

void RtcDriverAdjustEpochMs(int32_t delta_ms)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    base_epoch_ms = (uint64_t)((int64_t)base_epoch_ms + delta_ms);
    __set_PRIMASK(primask);
}

uint64_t RtcDriverGetEpochMs(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t now = HAL_GetTick();
    base_epoch_ms += (uint32_t)(now - base_tick);   // unsigned difference survives the tick wrap
    base_tick = now;                                // re-base so it never gets near 49.7 days
    uint64_t epoch_ms = base_epoch_ms;
    if (epoch_ms < last_epoch_ms)
    {
        epoch_ms = last_epoch_ms;                   // being slewed back: hold instead
    }
    last_epoch_ms = epoch_ms;
    __set_PRIMASK(primask);
    return epoch_ms;
}

double RtcDriverGetTimeStamp(void)
{
    return (double)RtcDriverGetEpochMs() / (double)MILLISECONDS_PER_SECOND;
}
