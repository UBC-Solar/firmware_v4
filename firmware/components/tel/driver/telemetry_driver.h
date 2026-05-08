#ifndef __TELEMETRY_DRIVER__H__
#define __TELEMETRY_DRIVER__H__

#include "usart.h"
#include "stdlib.h"

#define CELLULAR 1

#define TEL_DATA_LENGTH                           8U
#define TEL_MSG_TYPEDEF_SIZE                      sizeof(RADIO_Msg_TypeDef)
#define TEL_QUEUE_SIZE                            60

typedef struct {
    uint64_t timestamp;
    char ID_DELIMETER;
    uint32_t can_id;
    uint8_t data[TEL_DATA_LENGTH];
    uint8_t data_len;
    char CARRIAGE_RETURN;
    char NEW_LINE;
} __attribute__((packed)) TEL_Msg_TypeDef;

typedef struct {
   uint32_t dropped_telemetry_msg;
   uint32_t telemetry_hal_transmit_failures;
   uint32_t successful_telemetry_tx;
} telemetry_diagnostics_t;

extern telemetry_diagnostics_t telemetry_diagnostic;

osSemaphoreId_t usart1_tx_semaphore;

void UART_telemetry_transmit(TEL_Msg_TypeDef* can_tel_msg);

#endif /* __TELEMETRY_DRIVER__H__ */