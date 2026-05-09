#include "can_driver.h"

static void CanFilterInit(CAN_FilterTypeDef *filter);

void CanDriverInit(void)
{
    CAN_FilterTypeDef filter = {0};

    CanFilterInit(&filter);
    HAL_CAN_ConfigFilter(&hcan, &filter);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Start(&hcan);
}

static void CanFilterInit(CAN_FilterTypeDef *filter)
{
    if (filter == NULL)
    {
        return;
    }

    filter->FilterBank = 0;
    filter->FilterMode = CAN_FILTERMODE_IDMASK;
    filter->FilterScale = CAN_FILTERSCALE_32BIT;
    filter->FilterIdHigh = (uint16_t)((DRD_MOTOR_COMMAND_CAN_ID << 5) >> 16);
    filter->FilterIdLow = (uint16_t)((DRD_MOTOR_COMMAND_CAN_ID << 5) & 0xFFFFU);
    filter->FilterMaskIdHigh = (uint16_t)((0x7FFU << 5) >> 16);
    filter->FilterMaskIdLow = (uint16_t)((0x7FFU << 5) & 0xFFFFU);
    filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter->FilterActivation = ENABLE;
}
