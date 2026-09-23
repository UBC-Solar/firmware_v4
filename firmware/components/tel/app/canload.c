/******************************************************************************
* @file    canload.c
* @brief   Function to determine how much of the CAN bus is being occupied at a given time
*
* This file contains functions to calculate the percentage of CAN bus used at transmit that over CAN
*
* @author Shlok Lande
* @date Sep 21 2026
******************************************************************************/

#include "canload.h"

// Private defines
#ifndef WINDOW_SIZE
#define WINDOW_SIZE 5
#endif


#define BAUD_RATE 500000
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
#define CANLOAD_MSG_ID                      0x763
#define CANLOAD_DATA_LENGTH                 1

static uint32_t can_total_bits = 0;
uint64_t currentIdx = 0; // Always increments and needs large enough data type to avoid overflow
uint32_t circularBuffer[WINDOW_SIZE] = {0};