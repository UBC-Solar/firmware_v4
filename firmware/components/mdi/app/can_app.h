/**
 * @file    can_app.h
 * @brief   MDI CAN application interface.
 */
#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdbool.h>
#include <stdint.h>

#include "mdi_driver.h"

extern MdiMotorCommand g_mdi_motor_command;
extern volatile bool g_mdi_motor_command_received;
extern volatile uint32_t g_mdi_last_command_tick;

void CanAppInit(void);
void CanAppSendTimeSinceBootup(void);
void CanAppSendDiagnosticFlags(void);

#endif /* __CAN_APP_H__ */
