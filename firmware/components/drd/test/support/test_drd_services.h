#ifndef TEST_DRD_SERVICES_H
#define TEST_DRD_SERVICES_H
#include "drive_state.h"
#include "diagnostic.h"

// Boundaries outside the drive component: CAN transport, UI and telemetry.
// These record outputs; they do not implement CAN packing or LCD rendering.
typedef struct {
    DriveStateMotorControl motor_command;
    unsigned motor_command_count;
    bool command_from_isr;
    uint8_t speed_units;
    DiagnosticDRD diagnostics;
} TestDrdServices;

extern TestDrdServices test_drd;
void TestDrdServicesReset(void);
#endif
