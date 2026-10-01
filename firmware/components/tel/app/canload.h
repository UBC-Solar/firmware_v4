/*
* @file    canload.h
* @brief   function headers used by canload.c
*
* This header contains function declarations for the canload
*
* Monday Update explaining the IDs and high level understanding of how CAN laod works: https://ubcsolar26.monday.com/boards/9565346490/pulses/13086882321/posts/5590202228
*
* @author Shlok Lande
* @date Sep 21 2026
*/

#ifndef INC_CANBUSLOAD_H_
#define INC_CANBUSLOAD_H_

#include "main.h"
#define CANLOAD_MSG_RATE 100

#define BIT_RATE 500000
#define SOF_BITS 1
#define STANDARD_ID_BITS 11
#define EXTENDED_ID_BITS 29
#define SRR_BITS 1
#define RESERVED_BIT_R1 1
#define RTR_BITS 1
#define IDE_BITS 1
#define RESERVED_BIT_R0 1
#define DLC_BITS 4
#define CRC_BITS 15
#define CRC_DELIMITER_BITS 1
#define ACK_SLOT_BITS 1
#define ACK_DELIMITER_BITS 1
#define EOF_BITS 7
#define IFS_BITS 3
#define CANLOAD_MSG_ID 0x763
#define CANLOAD_DATA_LENGTH 1

/**
 * @brief Calculates the total number of CAN bus bits in the sliding window.
 *
 * This function iterates over the circular buffer to calculate the sum of
 * CAN bus data bits currently in the sliding window. The result is returned
 * as a floating-point value.
 *
 * This function does not modify any global variables or have any side effects.
 *
 * @return The total number of CAN bus bits in the sliding window as a float.
 */
static float CanloadCalculateTotalBits();

  /**
 * @brief Calculates the current bus load as a percentage.
 *
 * This function calculates the current bus load as a percentage based on the
 * total number of CAN bus bits in the sliding window and the bit rate of the
 * CAN bus. The result is returned as a floating-point value.
 *
 * This function does not modify any global variables or have any side effects.
 *
 * @return The current bus load as a percentage as a float.
 */
static float CanloadCalculateBusLoad();

/**
 * @brief Returns the current bus load as a percentage.
 *
 * This function returns the current bus load as a percentage based on the
 * total number of CAN bus bits in the sliding window and the bit rate of the
 * CAN bus. The result is returned as a floating-point value.
 *
 * @return The current bus load as a percentage as a float.
 */
float CanloadGetBusLoad();

 /**
 * @brief Updates the sliding window with the total number of CAN bus bits.
 *
 * This function updates the sliding window with the total number of CAN bus
 * bits. The sliding window is implemented as a circular buffer with a fixed
 * size. The oldest element in the buffer is removed and replaced with the
 * new value.
 *
 * This function modifies the global variables `currentIdx`, `circularBuffer`,
 * and `can_total_bits`. It does not return any value.
 */
void CanloadUpdateSlidingWindow();

/**
 * @brief Calculates the number of bits in a CAN message.
 *
 * This function calculates the number of bits in a CAN message based on the
 * data length code (DLC) of the message. The result is stored in the global
 * variable `can_total_bits`.
 *
 * This function modifies the global variable `can_total_bits`. It does not
 * return any value.
 *
 * @param DLC The data length code of the CAN message.
 * @param IDE The identifier extension bit of the CAN message.
 */
void CanloadCalculateMessageBits(uint32_t DLC, uint32_t IDE);
void CAN_tx_canload_msg();

#endif /* INC_CANBUSLOAD_H_ */