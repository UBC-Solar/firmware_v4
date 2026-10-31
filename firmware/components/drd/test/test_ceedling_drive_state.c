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

// Stubs for services outside this test.
// Boundaries outside the drive component: CAN transport, UI and telemetry.
// These record outputs; they do not implement CAN packing or LCD rendering.
typedef struct
{
    DriveStateMotorControl motor_command;
    unsigned motor_command_count;
    bool command_from_isr;
    uint8_t speed_units;
    DiagnosticDRD diagnostics;
} TestDrdServices;

static TestDrdServices test_drd;

void MotorCommandPackAndSend(DriveStateMotorControl* command, bool isr)
{
    TEST_ASSERT_NOT_NULL(command);
    test_drd.motor_command = *command;
    test_drd.command_from_isr = isr;
    ++test_drd.motor_command_count;
}
uint8_t LcdHandlerGetSpeedUnits(void) { return test_drd.speed_units; }

void DiagnosticSetRawADC1(uint16_t value) { test_drd.diagnostics.raw_adc1 = value; }
void DiagnosticSetRawADC2(uint16_t value) { test_drd.diagnostics.raw_adc2 = value; }
void DiagnosticSetMechBrakePressed(bool value)
{
    test_drd.diagnostics.flags.mech_brake_pressed = value;
}
void DiagnosticSetRegenEnabled(bool value) { test_drd.diagnostics.flags.regen_enabled = value; }
void DiagnosticSetThrottleADCOutOfRange(bool value)
{
    test_drd.diagnostics.flags.throttle_ADC_out_of_range = value;
}
void DiagnosticSetThrottleADCMismatch(bool value)
{
    test_drd.diagnostics.flags.throttle_ADC_mismatch = value;
}
void DiagnosticSetSpeedTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.speed_timeout = value;
}
void DiagnosticSetDriveStateTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.drive_state_timeout = value;
}
void DiagnosticSetSocTimeout(bool value) { test_drd.diagnostics.cyclic_flags.soc_timeout = value; }
void DiagnosticSetVoltageTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.voltage_timeout = value;
}
void DiagnosticSetCurrentTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.current_timeout = value;
}

void DiagnosticSetAccelTimeout(bool value)
{
    test_drd.diagnostics.cyclic_flags.accel_timeout = value;
}

// Reset the existing context without changing the production API.
extern volatile DriveStateCtx g_drive_state_ctx;

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
void tearDown(void) {}

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

void test_adc_timeout_sets_diagnostic_and_returns_zero_throttle(void)
{
    test_hal.adc_poll_status = HAL_TIMEOUT;
    test_hal.adc_value = ADC_HIGHEST_VALID;

    TEST_ASSERT_EQUAL_UINT16(0, AccelDriverReadThrottle());
    TEST_ASSERT_EQUAL_UINT32(10, test_hal.adc_poll_timeout);
    TEST_ASSERT_TRUE(test_drd.diagnostics.cyclic_flags.accel_timeout);
    TEST_ASSERT_EQUAL_UINT(0, test_hal.adc_reads);
}
