/**
 * @file    can_app.h
 * @brief   MDI CAN application interface.
 */

#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdbool.h>
#include <stdint.h>

#include "mdi_driver.h"

/**
 * @brief Initializes CAN application state and RX handling.
 */
void CanAppInit(void);

/**
 * @brief Attempts to fetch the latest received motor command.
 * @param command Output pointer populated when a new command is available.
 * @retval true A new command was copied to @p command.
 * @retval false No new command or invalid argument.
 */
bool CanAppTryGetMotorCommand(MdiMotorCommand *command);

/**
 * @brief Gets the system tick of the last valid motor command reception.
 * @return Tick timestamp of the most recent command frame.
 */
uint32_t CanAppGetLastCommandTick(void);

#endif /* __CAN_APP_H__ */
