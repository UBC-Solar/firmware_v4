#ifndef SUNLITE_OTA_CAN_APP_H
#define SUNLITE_OTA_CAN_APP_H

#include "stm32f1xx_hal.h"

#include <stdbool.h>

void SunliteOtaCanAppInit(CAN_HandleTypeDef *handle);
void SunliteOtaCanAppPoll(void);

/* Override on a board that has a real safe-state interlock. */
bool SunliteOtaCanBoardUpdateAllowed(void);

#endif /* SUNLITE_OTA_CAN_APP_H */
