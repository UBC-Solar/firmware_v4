#ifndef SUNLITE_OTA_APP_H
#define SUNLITE_OTA_APP_H

#include <stdbool.h>

void SunliteOtaAppPoll(void);
bool SunliteOtaApplicationSessionActive(void);
bool SunliteOtaBoardUpdateAllowed(void);
void SunliteOtaBoardTransmitAborted(void);
void SunliteOtaBoardYield(void);

#endif /* SUNLITE_OTA_APP_H */
