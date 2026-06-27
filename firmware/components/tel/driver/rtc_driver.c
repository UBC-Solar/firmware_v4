#include <sys/types.h>
#include "rtc_driver.h"
#include "rtc.h"
#include "stm32f1xx_hal.h"
#include <time.h>

#define MILLIS_TO_SECONDS(milliseconds) ( (double)(milliseconds * 0.001) )
#define MILLISECONDS_IN_SECONDS         1000

static uint32_t start_of_second;

void RtcDriverSetDate(RtcDriverDateData *data) {
    /* Initialize Date Object */
    RTC_DateTypeDef sDate = {0};

    /* Manually parsing the date, month, and year */
    sDate.Date  = data->day;
    sDate.Month = data->month;
    sDate.Year  = data->year;

    /* Set the RTC Date with these settings */
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void RtcDriverSetTime(RtcDriverTimeData *data) {
    // Store the start of the second for timestamp calculations
    start_of_second = HAL_GetTick();

    /* Initialize Time Object */
    RTC_TimeTypeDef sTime = {0};

    /* Manually parsing the seconds minutes hours */
    sTime.Seconds = data->seconds;
    sTime.Minutes = data->minutes;
    sTime.Hours   = data->hours;

    /* Set the RTC time with these settings */
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

double RtcDriverGetTimeStamp() {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    struct tm t;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);              /* RTC hours, mins, seconds */
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);              /* RTC years, months, days */

    t.tm_year = date.Year + 100;  // Year since 1900, so add 100 to the year from the RTC (which starts at 2000).
    t.tm_mon = date.Month - 1;    // tm_mon is 0-11, so subtract 1 from the RTC month (which starts at 1).
    t.tm_mday = date.Date;
    t.tm_hour = time.Hours;
    t.tm_min = time.Minutes;
    t.tm_sec = time.Seconds;
    t.tm_isdst = 0;                // Disable daylight saving time adjustments.
    long int epoch_secs = (long int) mktime(&t);

    double milliseconds = MILLIS_TO_SECONDS(((HAL_GetTick() - start_of_second) % MILLISECONDS_IN_SECONDS));  // Get Milliseconds

    return (double)epoch_secs + milliseconds;  
}
