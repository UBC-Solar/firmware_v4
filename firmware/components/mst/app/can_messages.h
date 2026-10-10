#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "stm32f1xx.h"


/**
 * @brief Sends the heartbeat counter plus packed faults, warnings, and enable
 * states
 */
void CAN_SendHeartbeatMessage(void);

/**
 * @brief Publish total pack voltage and outlier modules
 */
void CAN_SendVoltageSummaryMessage(void);

/**
 * @brief Publish average pack temperature and outlier modules
 */
void CAN_SendTempSummaryMessage(void);

/**
 * @brief Sends per-module voltage data
 */
void CAN_SendModuleVoltMessage(void);

/**
 * @brief Sends per-module temperature data
 */
void CAN_SendModuleTempMessage(void);

/**
 * @brief Sends per-module status flags
 */
void CAN_SendModuleStatusMessage(void);

/**
 * @brief Publish which modules are currently balancing
 */
void CAN_SendBalanceStatusMessage(void);
