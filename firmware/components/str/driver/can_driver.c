#include "can_driver.h"

void CanDriverInit(void)
{
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