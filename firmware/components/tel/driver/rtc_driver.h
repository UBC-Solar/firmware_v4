#ifndef __RTC__DRIVER__H__
#define __RTC__DRIVER__H__

#include <stdbool.h>

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

} RtcDriverTimeData;

typedef struct {
    uint8_t day;
    uint8_t month;
    uint8_t year;
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
 * @brief Initializes the RTC driver and hardware if not already initialized.
 * @param data Data from the memorator to set the RTC Date
 */
void RtcDriverSetDate(RtcDriverDateData *data);
/**
 * @brief Initializes the RTC driver and hardware if not already initialized.
 * @param data Data from the memorator to set the RTC Time
 */
void RtcDriverSetTime(RtcDriverTimeData *data);

/**
 * @brief Get the current timestamp in seconds with milliseconds precision
 * @return double The current timestamp in seconds.
 */
double RtcDriverGetTimeStamp();

#endif /* __RTC__DRIVER__H__ */