#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdint.h>

#define STR_LCD_PAGE_CAN_ID 0x580U
#define FRAME0 0x08850225

void CanAppInit(void);
void CanAppTransmitNextPage(void);
void SteeringCanRxHandler(uint32_t msg_id, uint8_t* data);
void TurnSignalHornPtt(bool turn_left, bool turn_right, bool horn, bool ptt, bool regen, bool next_page, bool cruise, uint16_t set_velocity_kmh);

#endif /* __CAN_APP_H__ */