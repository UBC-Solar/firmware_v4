#include "test_drd_services.h"
#include "can_app.h"
#include "lcd_handler.h"
#include "lcd_app.h"
#include "unity.h"

TestDrdServices test_drd;

void TestDrdServicesReset(void)
{
    test_drd = (TestDrdServices){.speed_units = LCD_APP_KPH};
}

void MotorCommandPackAndSend(DriveStateMotorControl *command, bool isr)
{
    TEST_ASSERT_NOT_NULL(command);
    test_drd.motor_command = *command;
    test_drd.command_from_isr = isr;
    ++test_drd.motor_command_count;
}
uint8_t LcdHandlerGetSpeedUnits(void) { return test_drd.speed_units; }

void DiagnosticSetRawADC1(uint16_t value) { test_drd.diagnostics.raw_adc1 = value; }
void DiagnosticSetRawADC2(uint16_t value) { test_drd.diagnostics.raw_adc2 = value; }
void DiagnosticSetMechBrakePressed(bool value) { test_drd.diagnostics.flags.mech_brake_pressed = value; }
void DiagnosticSetRegenEnabled(bool value) { test_drd.diagnostics.flags.regen_enabled = value; }
void DiagnosticSetThrottleADCOutOfRange(bool value) { test_drd.diagnostics.flags.throttle_ADC_out_of_range = value; }
void DiagnosticSetThrottleADCMismatch(bool value) { test_drd.diagnostics.flags.throttle_ADC_mismatch = value; }
void DiagnosticSetSpeedTimeout(bool value) { test_drd.diagnostics.cyclic_flags.speed_timeout = value; }
void DiagnosticSetDriveStateTimeout(bool value) { test_drd.diagnostics.cyclic_flags.drive_state_timeout = value; }
void DiagnosticSetSocTimeout(bool value) { test_drd.diagnostics.cyclic_flags.soc_timeout = value; }
void DiagnosticSetVoltageTimeout(bool value) { test_drd.diagnostics.cyclic_flags.voltage_timeout = value; }
void DiagnosticSetCurrentTimeout(bool value) { test_drd.diagnostics.cyclic_flags.current_timeout = value; }
