/**
 * @file    mdi_app.h
 * @brief   MDI application helpers for motor commands.
 */
#ifndef __MDI_APP_H__
#define __MDI_APP_H__

#include "mdi_driver.h"

void MdiAppSetMotorCommand(const MdiMotorCommand *command);
void MdiAppParseMotorCommand(const uint8_t *buffer, MdiMotorCommand *command);
void MdiAppStopMotor(void);

#endif /* __MDI_APP_H__ */
