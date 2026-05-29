#ifndef __RTC__APP__H__
#define __RTC__APP__H__

#include <stdint.h>

/**
 * @brief Synchronizes the RTC memory with the telemetry data.
 * @param data Pointer to the data to synchronize.
 */
void RtcAppSyncMemoratorToTel(uint8_t *data);

/**
 * @brief Handles received RTC-related CAN messages.
 * @param msg_id The CAN message ID.
 * @param data Pointer to the received CAN message data.
 */
void RtcAppRxHandler(uint32_t msg_id, uint8_t* data);

#endif /* __RTC__APP__H__ */