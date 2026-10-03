#ifndef FW_UPDATE_CAN_APP_H
#define FW_UPDATE_CAN_APP_H

#include "stm32f1xx_hal.h"

#include <stdbool.h>

void FirmwareUpdateCanAppInit(CAN_HandleTypeDef *handle);
void FirmwareUpdateCanAppPoll(void);

/* Override on a board that has a real safe-state interlock. */
bool FirmwareUpdateCanBoardUpdateAllowed(void);

#endif /* FW_UPDATE_CAN_APP_H */
