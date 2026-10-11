/**
 * @file    rtc_app.h
 * @brief   RTC application header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the RTC application, 
 * which handles synchronization and message processing for the RTC module.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#ifndef __RTC__APP__H__
#define __RTC__APP__H__

#include <stdint.h>
#include "rtc_driver.h"

/**
 * @brief Synchronizes the RTC memory with the telemetry data.
 * @param data Pointer to the data to synchronize.
 */
void RtcAppSyncMemoratorToTel(uint8_t *data);

/**
 * @brief Sets the clock from a GPS solution's UTC time. Call only when the receiver reports the
 *        date and time valid and fully resolved. Overrides the memorator's time.
 * @param date UTC date, year counted from 2000
 * @param time UTC time of day
 */
void RtcAppSyncGpsToTel(const RtcDriverDateData* date, const RtcDriverTimeData* time);

/**
 * @brief Handles received RTC-related CAN messages.
 * @param msg_id The CAN message ID.
 * @param data Pointer to the received CAN message data.
 */
void RtcAppRxHandler(uint32_t msg_id, uint8_t* data);

#endif /* __RTC__APP__H__ */