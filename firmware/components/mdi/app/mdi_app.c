/**
 * @file    mdi_app.c
 * @brief   MDI application wrappers for motor command flow.
 */
#include "mdi_app.h"

void MdiAppSetMotorCommand(const MdiMotorCommand *command)
{
    MdiDriverSetMotorCommand(command);
}

void MdiAppParseMotorCommand(const uint8_t *buffer, MdiMotorCommand *command)
{
    MdiDriverParseMotorCommand(buffer, command);
}

void MdiAppStopMotor(void)
{
    MdiDriverStopMotor();
}
