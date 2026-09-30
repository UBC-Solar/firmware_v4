/******************************************************************************
* @file    canload.c
* @brief   Function to determine how much of the CAN bus is being occupied at a given time
*
* This file contains functions to calculate the percentage of CAN bus used and transmit that over CAN
*
* @author Shlok Lande
* @date Sep 21 2026
******************************************************************************/

#include "canload.h"
#include "can_driver.h"
#include "main.h"


// Private defines
#ifndef WINDOW_SIZE
#define WINDOW_SIZE 5
#endif

static uint32_t can_total_bits = 0;
static uint64_t currentIdx = 0; // Always increments and needs large enough data type to avoid overflow
static uint32_t circularBuffer[WINDOW_SIZE] = {0};

/** CAN HEADER DEFINITION */
CAN_TxHeaderTypeDef CANLOAD_busload = {
    .StdId = CANLOAD_MSG_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = CANLOAD_DATA_LENGTH,
};

 void CanloadCalculateMessageBits(uint32_t DLC, uint32_t IDE)
 {
     uint32_t bits = 0;
     bits += SOF_BITS;
     if (IDE == CAN_ID_STD)
     {
         bits += STANDARD_ID_BITS;
     }
     else
     {
         bits += EXTENDED_ID_BITS;
         bits += SRR_BITS;
         bits += RESERVED_BIT_R1;
     }
     bits += RTR_BITS;
     bits += IDE_BITS;
     bits += RESERVED_BIT_R0;
     bits += DLC_BITS;
     bits += DLC * 8;
     bits += CRC_BITS;
     bits += CRC_DELIMITER_BITS;
     bits += ACK_SLOT_BITS;
     bits += ACK_DELIMITER_BITS;
     bits += EOF_BITS;
     bits += IFS_BITS;
 
     can_total_bits += bits;
 }


void CanloadUpdateSlidingWindow()
{
    uint8_t removeIdx = currentIdx % WINDOW_SIZE;
    circularBuffer[removeIdx] = can_total_bits;
    can_total_bits = 0;
    currentIdx++;
}


 float CanloadCalculateTotalBits()
 {
     uint32_t sliding_sum = 0;
     for (uint8_t i = 0; i < WINDOW_SIZE; i++)
     {
         sliding_sum += circularBuffer[i];
     }
     return (float) sliding_sum;
 }


float CanloadCalculateBusLoad()
{
	  float total_bits = (float) CanloadCalculateTotalBits();
	  float window_duration_seconds = (float) WINDOW_SIZE * (float) (CANLOAD_MSG_RATE / 1000.0f);
	  float max_bits_in_window = window_duration_seconds * (float) BIT_RATE;
	  float load_percentage = (total_bits / max_bits_in_window) * 100.0f;

	  return load_percentage;
}


 float CanloadGetBusLoad()
 {
     return CanloadCalculateBusLoad();
 }
 
 void CAN_tx_canload_msg() {
     CAN_comms_Tx_msg_t CAN_comms_Tx_msg = {
         .data[0] = (uint8_t) CanloadGetBusLoad(),
         .header = CANLOAD_busload
     };  
 
   CanloadCalculateMessageBits(CAN_comms_Tx_msg.header.DLC, CAN_comms_Tx_msg.header.IDE);
   CAN_comms_Add_Tx_message(&CAN_comms_Tx_msg);
 }