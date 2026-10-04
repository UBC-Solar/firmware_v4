#include "unity.h"
#include "hex_app.h"
#include "hex_driver.h"
#include "cyclic_data_handler.h"
#include "test_hal.h"

void setUp(void)
{
    TestHalReset();
    HexAppSetSpeedUnits(STR_SPEED_UNITS_KPH);
    CyclicDataSetSpeed(0);
}
void tearDown(void) {}

void test_speed_is_formatted_and_sent_to_display_over_i2c(void)
{
    CyclicDataSetSpeed(42);
    HexAppUpdate();

    TEST_ASSERT_EQUAL_UINT(2, test_hal.i2c_count);
    uint8_t tens[] = {AS1115_REG_DIGIT0, 4};
    uint8_t ones[] = {AS1115_REG_DIGIT1, 2};
    for (unsigned i = 0; i < 2; ++i) {
        TEST_ASSERT_EQUAL_PTR(&hi2c1, test_hal.i2c[i].bus);
        TEST_ASSERT_EQUAL_UINT16(0, test_hal.i2c[i].address);
        TEST_ASSERT_EQUAL_UINT16(2, test_hal.i2c[i].size);
        TEST_ASSERT_EQUAL_UINT32(HAL_MAX_DELAY, test_hal.i2c[i].timeout);
    }
    TEST_ASSERT_EQUAL_HEX8_ARRAY(tens, test_hal.i2c[0].data, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(ones, test_hal.i2c[1].data, 2);
}
