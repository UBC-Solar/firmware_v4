/**
 * @file    can_app.c
 * @brief   CAN application implementation for UBC Solar TEL board
 *
 * This file contains the initialization and callback functions for the CAN application, 
 * which handles the reception and processing of CAN messages.
 *
 * @author  Gregory Bian
 * @date    Jun 30 2026
 */

#include "can_driver.h"
#include "can_app.h"
#include "CAN_comms.h"
#include "cmsis_os2.h"
#include "rtc_app.h"
#include "telemetry_app.h"
#include "tel_ota_safety.h"

#ifndef TEL_OTA_ENABLE_TEST_BYPASS
#define TEL_OTA_ENABLE_TEST_BYPASS 0
#endif

static TelOtaSafetyState ota_safety_state;

/**
 * @brief Can Comms Callback Function for processing received CAN messages.
 * @param CAN_comms_Rx_msg Pointer to the received CAN message structure.
 */
static void CanAppRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);


void CanAppInit(){
    TelOtaSafetyInit(&ota_safety_state);

    // Initialize CAN Comms RX Callback function 
    CAN_comms_config_t CAN_comms_config_tel = CanDriverInit();
    CAN_comms_config_tel.CAN_comms_Rx_callback = CanAppRxCallback;

    CAN_comms_init(&CAN_comms_config_tel);
}

static void CanAppRxCallback(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg){
	uint32_t CAN_ID = 0;
	bool is_extended_id = false;

	if (CAN_comms_Rx_msg == NULL)
	{
		return;
	}

	if(CAN_comms_Rx_msg->header.IDE == CAN_ID_EXT)
	{
		is_extended_id = true;
		CAN_ID = CAN_comms_Rx_msg->header.ExtId; // Get extended CAN ID
	}
	else
	{
		CAN_ID = CAN_comms_Rx_msg->header.StdId; // Get standard CAN ID
	}

	if (CAN_comms_Rx_msg->header.RTR == CAN_RTR_DATA)
	{
		taskENTER_CRITICAL();
		TelOtaSafetyObserveCanFrame(&ota_safety_state,
		                             CAN_ID,
		                             is_extended_id,
		                             (uint8_t)CAN_comms_Rx_msg->header.DLC,
		                             CAN_comms_Rx_msg->data,
		                             HAL_GetTick());
		taskEXIT_CRITICAL();
	}

    // RX Functions go here
	RtcAppRxHandler(CAN_ID, CAN_comms_Rx_msg->data);
	TelAppTransmitMsg(CAN_comms_Rx_msg);
}

/**
 * @brief Returns true only for an explicitly enabled Debug bench build.
 *
 * Configure TEL with -DTEL_OTA_ENABLE_TEST_BYPASS=ON to bypass
 * the vehicle-state gate while testing. CMake forces this off outside Debug.
 */
bool TelOtaSafetyTestBypassEnabled(void)
{
    return TEL_OTA_ENABLE_TEST_BYPASS != 0;
}

/**
 * @brief TEL OTA interlock used for both local and gateway-routed updates.
 */
bool SunliteOtaBoardUpdateAllowed(void)
{
    if (TelOtaSafetyTestBypassEnabled()) {
        return true;
    }

    uint32_t now_ms = HAL_GetTick();
    taskENTER_CRITICAL();
    bool update_allowed = TelOtaSafetyUpdateAllowed(&ota_safety_state, now_ms);
    taskEXIT_CRITICAL();
    return update_allowed;
}

/**
 * @brief Yield the dedicated OTA task while waiting for routed CAN traffic.
 */
void SunliteOtaBoardYield(void)
{
    osDelay(1U);
}
