/**
 * @file    fault_handler_driver.c
 * @brief   Fault handling driver for UBC Solar DRD board
 *
 * This file contains the implementation of the fault handling driver for the UBC Solar DRD board.
 * It manages the detection, reporting, and recovery from various fault conditions.
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#include "cmsis_os2.h"
#include "fault_handler_driver.h"

void FaultHandlerDriverFlashDebug(){
    HAL_GPIO_WritePin(FLT_MCU_GPIO_Port, FLT_MCU_Pin, GPIO_PIN_SET);
    osDelay(200);
    HAL_GPIO_WritePin(FLT_MCU_GPIO_Port, FLT_MCU_Pin, GPIO_PIN_RESET);
    osDelay(200);
}

void FaultHandlerDriverEStop(bool estop) {
    HAL_GPIO_WritePin(ESTOP_GPIO_Port, ESTOP_Pin, estop);
}