#include "unity.h"
#include "drive_state.h"
#include "gpio_driver.h"
#include "accel_driver.h"
#include "cyclic_data_handler.h"
#include "test_hal.h"
#include "test_drd_services.h"

// Reset the existing context without changing the production API.
extern volatile DriveStateCtx g_drive_state_ctx;

void setUp(void)
{
    TestHalReset();
    TestDrdServicesReset();
    g_drive_state_ctx = (DriveStateCtx){
        .state = PARK,
        .flags = {.regen_on = true, .velocity_under_threshold = true},
    };
    test_hal.adc_value = ADC_LOWEST_VALID;
    CyclicDataSetSpeed(0);
    CyclicDataSetDriveState(PARK);
}
void tearDown(void) {}

void test_braked_forward_request_changes_state_and_publishes_safe_output(void)
{
    TestHalSetPin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN, GPIO_PIN_SET);

    DriveStateInterruptHandler(DRIVE_NEXT_PIN);
    DriveStateFsmHandler();

    TEST_ASSERT_EQUAL(FORWARD, DriveStateGetDriveState());
    uint8_t *published = CyclicDataGetDriveState();
    TEST_ASSERT_NOT_NULL(published);
    TEST_ASSERT_EQUAL_UINT8(FORWARD, *published);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, TestHalGetPin(BRAKE_LED_PORT, BRAKE_LED_PIN));
    TEST_ASSERT_EQUAL_UINT(1, test_hal.adc_reads);
    TEST_ASSERT_TRUE(test_drd.diagnostics.flags.mech_brake_pressed);
    TEST_ASSERT_EQUAL_UINT(1, test_drd.motor_command_count);
    TEST_ASSERT_FALSE(test_drd.command_from_isr);
    TEST_ASSERT_EQUAL_UINT16(0, test_drd.motor_command.accel_DAC_value);
    TEST_ASSERT_EQUAL_UINT16(0, test_drd.motor_command.regen_DAC_value);
    TEST_ASSERT_EQUAL_HEX8(0x05, test_drd.motor_command.motor_control_flags);
}
