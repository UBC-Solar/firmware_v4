#ifndef FW_UPDATE_APP_H
#define FW_UPDATE_APP_H

#include <stdbool.h>

void FirmwareUpdateAppPoll(void);
bool FirmwareUpdateApplicationSessionActive(void);
bool FirmwareUpdateBoardUpdateAllowed(void);
void FirmwareUpdateBoardTransmitAborted(void);
void FirmwareUpdateBoardYield(void);

#endif /* FW_UPDATE_APP_H */
