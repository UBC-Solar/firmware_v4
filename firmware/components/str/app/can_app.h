#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdbool.h>
#include <stdint.h>

#define STR_LCD_PAGE_CAN_ID 0x580U
#define FRAME0 0x08850225

void CanAppInit(void);
void CanAppTransmitNextPage(void);
void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data);
void TransmitDriveControlState(void);

#endif /* __CAN_APP_H__ */