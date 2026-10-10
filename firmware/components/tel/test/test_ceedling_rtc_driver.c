#include "unity.h"
#include "rtc_driver.h"
#include "test_hal.h"

void setUp(void) { TestHalReset(); }
void tearDown(void) {}

void test_set_time_maps_hour_minute_second_in_binary_format(void)
{
    RtcDriverTimeData data = {.seconds = 56, .minutes = 34, .hours = 12};
    test_hal.tick = 1234;
    RtcDriverSetTime(&data);

    TEST_ASSERT_EQUAL_UINT(1, test_hal.rtc_time_writes);
    TEST_ASSERT_EQUAL_UINT8(12, test_hal.rtc_time.Hours);
    TEST_ASSERT_EQUAL_UINT8(34, test_hal.rtc_time.Minutes);
    TEST_ASSERT_EQUAL_UINT8(56, test_hal.rtc_time.Seconds);
}
