#ifndef __RTC__DRIVER__H__
#define __RTC__DRIVER__H__

#include <stdbool.h>

#define MILLIS_TO_SECONDS(milliseconds) ( (double)(milliseconds * 0.001) )
#define MILLISECONDS_IN_SECONDS         1000

typedef enum {
    TIMETYPEDEF_SECONDS_IDX = 0,
    TIMETYPEDEF_MINUTES_IDX = 1,
    TIMETYPEDEF_HOURS_IDX = 2,
    TIMETYPEDEF_DAY_IDX = 3,
    TIMETYPEDEF_MONTH_IDX = 4,
    TIMETYPEDEF_YEAR_IDX = 5
} TIMETYPEDEF_IDX_t;

/**
 * @brief Initializes the RTC driver and hardware if not already initialized.
 * @param data Data from the memorator to set the RTC Date
 */
void RtcDriverSetDate(uint8_t *data);
/**
 * @brief Initializes the RTC driver and hardware if not already initialized.
 * @param data Data from the memorator to set the RTC Time
 */
void RtcDriverSetTime(uint8_t *data);

/**
 * @brief Get the current timestamp in seconds with milliseconds precision
 * @return double The current timestamp in seconds.
 */
double RtcDriverGetTimeStamp();

#endif /* __RTC__DRIVER__H__ */