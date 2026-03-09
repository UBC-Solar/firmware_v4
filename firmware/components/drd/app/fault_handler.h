/**
 * @file    fault_handler.h
 * @brief   Fault handling API for UBC Solar DRD board
 *
 * This header declares the function prototypes and data structures for the fault handling module.
 * It provides an interface for detecting, reporting, and recovering from various fault conditions.
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#ifndef FAULT_HANDLER_H
#define FAULT_HANDLER_H

#include <stdint.h>
#include <sys/types.h>

#define PACK_VOLTAGE_DIVISOR	468
#define MAX_PACK_VOLTAGE 	 	134.4
#define MIN_PACK_VOLTAGE 	 	86.72

#define FAULT_LIGHT_FLASH_DELAY 100

/**
 * @brief Flashes the debug LED based on the current fault status.
 */
void FaultHandlerFlashLED();
/**
 * @brief Triggers the emergency stop functionality.
 */
void FaultHandlerEStop(uint8_t* can_rx_data);
/**
 * @brief Parses battery fault data from received CAN messages and updates the cyclic data handler with the latest battery fault information.
 *
 * @param can_rx_data Pointer to the received CAN message data containing the battery fault information.
 */
void FaultHandlerParseBatteryFaults(uint8_t* can_rx_data);
/**
 * @brief Parses battery fault data received from CAN messages from the ECU
 *
 * @param can_rx_data Pointer to the received CAN message data containing the temperature information.
 */
void FaultHandlerParseECUFaults(uint8_t* can_rx_data);
/**
 * @brief Parses battery fault data for overvoltage from received CAN messages and updates the cyclic data handler with the latest battery fault information.
 *
 * @param can_rx_data Pointer to the received CAN message data containing the battery fault information.
 */
void FaultHandlerParsePackVoltageFaults(uint8_t* can_rx_data);
/**
 * @brief Parses motor fault data received from CAN messages from the motor and MDI
 *
 * @param can_rx_data Pointer to the received CAN message data containing the temperature information.
 */
void FaultHandlerParseMotorFaults(uint8_t* can_rx_data);
/**
 * @brief Parses battery warning data from received CAN messages and updates the cyclic data handler with the latest battery warning information.
 *
 * @param can_rx_data Pointer to the received CAN message data containing the battery warning information.
 */
void FaultHandlerParseBatteryWarnings(uint8_t* can_rx_data);
/**
 * @brief Parses ECU warning data from received CAN messages and updates the cyclic data handler with the latest ECU warning information.
 *
 * @param can_rx_data Pointer to the received CAN message data containing the ECU warning information.
 */
void FaultHandlerParseECUWarnings(uint8_t* can_rx_data);
/**
 * @brief Parses temperature data from received CAN messages and updates the cyclic data handler with the latest temperatures.
 *
 * @param msg_id The CAN message ID to identify which temperature data is being parsed.
 * @param can_rx_data Pointer to the received CAN message data containing the temperature information.
 */
void FaultHandlerParseTemperatures(uint32_t msg_id, uint8_t* can_rx_data);

#endif /* FAULT_HANDLER_H */