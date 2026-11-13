/**
 * @file test_ceedling_drive_state.c
 * @brief DRD drive-state and ADC timeout unit tests with local service stubs.
 */

#include "accel_driver.h"
#include "can_app.h"
#include "cyclic_data_handler.h"
#include "diagnostic.h"
#include "drive_state.h"
#include "gpio_driver.h"
#include "lcd_app.h"
#include "lcd_handler.h"
#include "test_hal.h"
#include "unity.h"

/**
 * @brief Captured CAN, LCD, and diagnostic state for the drive component test.
 */
typedef struct
{
    DriveStateMotorControl motor_command;
    unsigned motor_command_count;
    bool command_from_isr;
    uint8_t speed_units;
    DiagnosticDRD diagnostics;
} TestDrdServices;

static TestDrdServices test_drd;

/**
 * @brief Capture the motor command without transmitting CAN frames.
 * @param command Motor command to copy.
 * @param isr Whether the caller is in interrupt context.
 */
void MotorCommandPackAndSend(DriveStateMotorControl* command, bool isr)
{
    TEST_ASSERT_NOT_NULL(command);
    test_drd.motor_command = *command;
    test_drd.command_from_isr = isr;
    ++test_drd.motor_command_count;
}
/**
 * @brief Return the configured LCD speed units.
 * @return LCD_APP_KPH or the test-configured unit selection.
 */
uint8_t LcdHandlerGetSpeedUnits(void) { return test_drd.speed_units; }

/**
 * @brief Record the raw ADC1 sample in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetRawADC1(uint16_t value) { test_drd.diagnostics.raw_adc1 = value; }
/**
 * @brief Record the raw ADC2 sample in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetRawADC2(uint16_t value) { test_drd.diagnostics.raw_adc2 = value; }
/**
 * @brief Record the mechanical brake status in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetMechBrakePressed(bool value)
{
    test_drd.diagnostics.flags.mech_brake_pressed = value;
}
/**
 * @brief Record the regen status in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetRegenEnabled(bool value) { test_drd.diagnostics.flags.regen_enabled = value; }
/**
 * @brief Record the throttle range fault in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetThrottleADCOutOfRange(bool value)
{
    test_drd.diagnostics.flags.throttle_ADC_out_of_range = value;
}
/**
 * @brief Record the throttle mismatch fault in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetThrottleADCMismatch(bool value)
{
    test_drd.diagnostics.flags.throttle_ADC_mismatch = value;
}
/**
 * @brief Record the speed timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetSpeedTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.speed_timeout = value;
}
/**
 * @brief Record the drive-state timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetDriveStateTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.drive_state_timeout = value;
}
/**
 * @brief Record the state-of-charge timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetSocTimeout(bool value) { test_drd.diagnostics.cyclic_flags.soc_timeout = value; }
/**
 * @brief Record the voltage timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetVoltageTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.voltage_timeout = value;
}
/**
 * @brief Record the current timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetCurrentTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.current_timeout = value;
}

/**
 * @brief Record the accelerator timeout flag in the captured diagnostics.
 * @param value Value to record.
 */
void DiagnosticSetAccelTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.accel_timeout = value;
}

// Reset the existing context without changing the production API.
extern volatile DriveStateCtx g_drive_state_ctx;

/**
 * @brief Reset test inputs and captured outputs before each test.
 */
void setUp(void)
{
    TestHalReset();
    test_drd = (TestDrdServices){.speed_units = LCD_APP_KPH};
    g_drive_state_ctx = (DriveStateCtx){
        .state = PARK,
        .flags = {.regen_on = true, .velocity_under_threshold = true},
    };
    test_hal.adc_value = ADC_LOWEST_VALID;
    CyclicDataSetSpeed(0);
    CyclicDataSetDriveState(PARK);
}
/**
 * @brief Complete the test; no additional cleanup is required.
 */
void tearDown(void) {}

/**
 * @brief Verify a braked forward request publishes FORWARD with zero motor output.
 */
void test_braked_forward_request_changes_state_and_publishes_safe_output(void)
{
    TestHalSetPin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN, GPIO_PIN_SET);

    DriveStateInterruptHandler(DRIVE_NEXT_PIN);
    DriveStateFsmHandler();

    TEST_ASSERT_EQUAL(FORWARD, DriveStateGetDriveState());
    uint8_t* published = CyclicDataGetDriveState();
    TEST_ASSERT_NOT_NULL(published);
    TEST_ASSERT_EQUAL_UINT8(FORWARD, *published);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, TestHalGetPin(BRAKE_LED_PORT, BRAKE_LED_PIN));
    TEST_ASSERT_EQUAL_UINT(1, test_hal.adc_reads);
    TEST_ASSERT_EQUAL_UINT32(10, test_hal.adc_poll_timeout);
    TEST_ASSERT_FALSE(test_drd.diagnostics.cyclic_flags.accel_timeout);
    TEST_ASSERT_TRUE(test_drd.diagnostics.flags.mech_brake_pressed);
    TEST_ASSERT_EQUAL_UINT(1, test_drd.motor_command_count);
    TEST_ASSERT_FALSE(test_drd.command_from_isr);
    TEST_ASSERT_EQUAL_UINT16(0, test_drd.motor_command.accel_DAC_value);
    TEST_ASSERT_EQUAL_UINT16(0, test_drd.motor_command.regen_DAC_value);
    TEST_ASSERT_EQUAL_HEX8(0x05, test_drd.motor_command.motor_control_flags);
}

/**
 * @brief Verify an ADC timeout sets its diagnostic and returns zero throttle.
 */
void test_adc_timeout_sets_diagnostic_and_returns_zero_throttle(void)
{
    test_hal.adc_poll_status = HAL_TIMEOUT;
    test_hal.adc_value = ADC_HIGHEST_VALID;

    TEST_ASSERT_EQUAL_UINT16(0, AccelDriverReadThrottle());
    TEST_ASSERT_EQUAL_UINT32(10, test_hal.adc_poll_timeout);
    TEST_ASSERT_TRUE(test_drd.diagnostics.cyclic_flags.accel_timeout);
    TEST_ASSERT_EQUAL_UINT(0, test_hal.adc_reads);
}
