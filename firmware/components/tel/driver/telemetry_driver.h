#ifndef __TELEMETRY_DRIVER__H__
#define __TELEMETRY_DRIVER__H__

#include "usart.h"
#include "stdlib.h"
#include "cmsis_os2.h"

// ONLY CHANGE CELLULAR TO 0 IF FLASHING WITH ST-LINK OR POWERING WITH POWER SUPPLY
// J-LINK DOES NOT SUPPLY ENOUGH CURRENT FOR THE RADIO MODULE. ONLY USE ST-LINK FOR THIS.
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


/* TODO: SEND THIS OVER CAN AND SEE ON GRAFANA NUMBER */
extern telemetry_diagnostics_t telemetry_diagnostic;
extern osSemaphoreId_t usart1_tx_semaphore;

/**
  * @brief Transmits a telemetry message via UART
  * @param can_tel_msg Pointer to the telemetry message to be transmitted
  */
void UART_telemetry_transmit(TEL_Msg_TypeDef* can_tel_msg);

#endif /* __TELEMETRY_DRIVER__H__ */