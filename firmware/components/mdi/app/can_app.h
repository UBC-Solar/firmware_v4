/**
 * @file    can_app.h
 * @brief   MDI CAN application interface.
 */
#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdbool.h>
#include <stdint.h>

#include "mdi_driver.h"

void CanAppInit(void);
bool CanAppTryGetMotorCommand(MdiMotorCommand *command);
uint32_t CanAppGetLastCommandTick(void);

#endif /* __CAN_APP_H__ */
