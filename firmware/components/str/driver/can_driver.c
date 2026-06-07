/**
 * @file    can_driver.c
 * @brief   CAN driver implementation for the UBC Solar STR board.
 */

/* INCLUDES */
#include "can_driver.h"

/* GLOBAL VARIABLES */
const CAN_TxHeaderTypeDef steering_header = {
	.StdId = STR_CAN_MSG_ID,
	.ExtId = 0x0000,
	.IDE = CAN_ID_STD,
	.RTR = CAN_RTR_DATA,
	.DLC = 8U,
};

/* CAN FILTERS */
void CanFilterInit(CAN_FilterTypeDef* can_filter)
{
	can_filter->FilterIdHigh = 0x0000;
	can_filter->FilterMaskIdHigh = 0x0000;
	can_filter->FilterIdLow = 0x0000;
	can_filter->FilterMaskIdLow = 0x0000;
	can_filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
	can_filter->FilterBank = 0U;
	can_filter->FilterMode = CAN_FILTERMODE_IDMASK;
	can_filter->FilterScale = CAN_FILTERSCALE_32BIT;
	can_filter->FilterActivation = CAN_FILTER_ENABLE;
}
