#include "rtc_app.h"
#include "rtc_driver.h"
#include <sys/types.h>

static bool rtc_initialized = false;

void RtcAppSyncMemoratorToTel(uint8_t *data) {
    if (rtc_initialized) {
        // If RTC is already initialized, we can ignore further sync messages
        return;
    }
    rtc_initialized = true; // Set the RTC initialized flag to true when we receive data to sync

    RtcDriverSetTime(data);
    RtcDriverSetDate(data);
}
