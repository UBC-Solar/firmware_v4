#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include <stdint.h>

#define STR_LCD_PAGE_CAN_ID 0x580U

void CanAppInit(void);
void CanAppTransmitNextPage(void);

#endif /* __CAN_APP_H__ */