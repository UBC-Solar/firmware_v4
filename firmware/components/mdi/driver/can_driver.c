/**
 * @file    can_driver.c
 * @brief   CAN driver implementation for the MDI board.
 */
#include "can_driver.h"

static void CanFilterInit(CAN_FilterTypeDef *filter);

static CanDriverRxCallback s_rx_callback = NULL;

void CanDriverInit(void)
{
    CAN_FilterTypeDef filter = {0};

    CanFilterInit(&filter);
    HAL_CAN_ConfigFilter(&hcan, &filter);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Start(&hcan);
}

bool CanDriverSend(const CAN_TxHeaderTypeDef *header, const uint8_t *data)
{
    uint32_t mailbox = 0;

    if (header == NULL || data == NULL)
    {
        return false;
    }

    return (HAL_CAN_AddTxMessage(&hcan, header, (uint8_t *)data, &mailbox) == HAL_OK);
}

void CanDriverRegisterRxCallback(CanDriverRxCallback callback)
{
    s_rx_callback = callback;
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

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *can_handle)
{
    uint8_t rx_data[8] = {0};
    CAN_RxHeaderTypeDef rx_header;

    if (HAL_CAN_GetRxMessage(can_handle, CAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK)
    {
        return;
    }

    if (s_rx_callback != NULL)
    {
        s_rx_callback(&rx_header, rx_data);
    }
}
