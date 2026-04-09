#include "can_driver.h"
#include "can.h"
#include "CAN_comms.h"

/**
 * @brief Initialize CAN filter configuration
 * @param can_filter Pointer to CAN filter configuration structure
 */
static void CanDriverFilterInit(CAN_FilterTypeDef* can_filter);

/* CAN Driver initialization function */
CAN_comms_config_t CanDriverInit(){
    // Initialize CAN Comms configuration struct
    CAN_comms_config_t CAN_comms_config_tel = {0};
    CAN_FilterTypeDef can_filter = {0};
    CanDriverFilterInit(&can_filter);

    CAN_comms_config_tel.hcan = &hcan;
    CAN_comms_config_tel.CAN_Filter = can_filter;

    return CAN_comms_config_tel;
}

static void CanDriverFilterInit(CAN_FilterTypeDef* can_filter){
    // TODO: Figure out which CAN IDs need to be filtered.
    can_filter->FilterIdHigh = 0x0000;
    can_filter->FilterMaskIdHigh = 0x0000;

    can_filter->FilterIdLow = 0x0000;
    can_filter->FilterMaskIdLow = 0x0000;

    can_filter->FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter->FilterBank = (uint32_t) 0;
    can_filter->FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter->FilterScale = CAN_FILTERSCALE_16BIT;
    can_filter->FilterActivation = CAN_FILTER_ENABLE;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    // Handle CAN error callback if needed
}