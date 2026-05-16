/**
 * @file hvc_fsm_states.c
 * @brief HVC FSM state implementations
 */

#include "hvc_fsm_private.h"

/*============================================================================*/
/* STATES */

/**
 * @brief Opens all contactors and disables load outputs.
 *
 * Exit State: HVC_STATE_HV_CONNECT
 */
void HVC_State_Reset(void)
{
    DEBUG_IO_print("HVC: STATE_RESET\r\n");

    open_all_contactors();
    GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_RESET);
    GPIO_Write(FAN_CTRL_GPIO_Port,             FAN_CTRL_Pin,             GPIO_PIN_RESET);
    GPIO_Write(MPPT_CTRL_GPIO_Port,            MPPT_CTRL_Pin,            GPIO_PIN_RESET);
    GPIO_Write(DIST_CTRL_GPIO_Port,            DIST_CTRL_Pin,            GPIO_PIN_RESET);
    GPIO_Write(IMD_CTRL_GPIO_Port,             IMD_CTRL_Pin,             GPIO_PIN_SET);

    check_supp_voltage();

    ticks.generic = HAL_GetTick();
    hvc_state = HVC_STATE_HV_CONNECT;
}

/**
 * @brief Closes NEG then POS contactors with a settling delay between each step.
 *
 * Exit Condition: POS closed and settling delay elapsed.
 * Exit State: HVC_STATE_MOTOR_PRECHARGE
 */
void HVC_State_HVConnect(void)
{
    DEBUG_IO_print("HVC: STATE_HV_CONNECT\r\n");

    static bool neg_closed = false;
    static bool pos_closed = false;

    if (!neg_closed && timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(NEG_CTRL_GPIO_Port, NEG_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        neg_closed = true;
        ticks.neg_contactor = HAL_GetTick();
    }

    if (neg_closed && !pos_closed && timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.neg_contactor)) {
        GPIO_Write(POS_CTRL_GPIO_Port, POS_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pos_closed = true;
        ticks.pos_contactor = HAL_GetTick();
    }

    if (pos_closed && timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.pos_contactor)) {
        neg_closed = false;
        pos_closed = false;
        ticks.generic = HAL_GetTick();
        hvc_state = HVC_STATE_MOTOR_PRECHARGE;
    }
}

/**
 * @brief Closes motor precharge contactor and waits for bus voltage to reach threshold.
 *
 * Exit Condition: motor_precharge ADC >= HVC_PC_COMPLETE_RATIO% of HV bus voltage.
 * Exit State: HVC_STATE_CLOSE_MOTOR_BUS
 *
 * Exit Condition: Timeout (HVC_MOTOR_PC_TIMEOUT_MS).
 * Exit State: HVC_STATE_FAULT
 */
void HVC_State_MotorPrecharge(void)
{
    DEBUG_IO_print("HVC: STATE_MOTOR_PRECHARGE\r\n");

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    if (timer_elapsed(HVC_MOTOR_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        hvc_state = HVC_STATE_FAULT;
        return;
    }

    // TODO: compare motor_precharge ADC voltage to HV bus voltage
    // ADC_Voltages adc = ADC_GetVoltages();
    // if (adc.motor_precharge >= hv_bus_mv * HVC_PC_COMPLETE_RATIO / 100) {
    //     pc_started = false;
    //     ticks.generic = HAL_GetTick();
    //     hvc_state = HVC_STATE_CLOSE_MOTOR_BUS;
    // }
}

/**
 * @brief Closes LLIM contactor then opens motor precharge contactor.
 *
 * Exit Condition: Settling delay elapsed.
 * Exit State: HVC_STATE_MPPT_PRECHARGE
 */
void HVC_State_CloseMotorBus(void)
{
    DEBUG_IO_print("HVC: STATE_CLOSE_MOTOR_BUS\r\n");

    GPIO_Write(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        ticks.generic = HAL_GetTick();
        hvc_state = HVC_STATE_MPPT_PRECHARGE;
    }
}

/**
 * @brief Closes MPPT precharge contactor and waits for bus voltage to reach threshold.
 *
 * Exit Condition: mppt_precharge ADC >= HVC_PC_COMPLETE_RATIO% of HV bus voltage.
 * Exit State: HVC_STATE_CLOSE_MPPT_BUS
 *
 * Exit Condition: Timeout (HVC_MPPT_PC_TIMEOUT_MS).
 * Exit State: HVC_STATE_FAULT
 */
void HVC_State_MpptPrecharge(void)
{
    DEBUG_IO_print("HVC: STATE_MPPT_PRECHARGE\r\n");

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    if (timer_elapsed(HVC_MPPT_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        hvc_state = HVC_STATE_FAULT;
        return;
    }

    // TODO: compare mppt_precharge ADC voltage to HV bus voltage
    // ADC_Voltages adc = ADC_GetVoltages();
    // if (adc.mppt_precharge >= hv_bus_mv * HVC_PC_COMPLETE_RATIO / 100) {
    //     pc_started = false;
    //     ticks.generic = HAL_GetTick();
    //     hvc_state = HVC_STATE_CLOSE_MPPT_BUS;
    // }
}

/**
 * @brief Closes HLIM contactor, opens MPPT precharge, then enables MPPT and DIST.
 *
 * Exit Condition: Settling delay elapsed after HLIM close.
 * Exit State: HVC_STATE_MONITORING
 */
void HVC_State_CloseMpptBus(void)
{
    DEBUG_IO_print("HVC: STATE_CLOSE_MPPT_BUS\r\n");

    GPIO_Write(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        GPIO_Write(MPPT_CTRL_GPIO_Port,    MPPT_CTRL_Pin,    GPIO_PIN_SET);
        GPIO_Write(DIST_CTRL_GPIO_Port,    DIST_CTRL_Pin,    GPIO_PIN_SET);
        startup_complete = true;
        ticks.generic = HAL_GetTick();
        hvc_state = HVC_STATE_MONITORING;
    }
}

/**
 * @brief Normal operating state. Polls fault inputs and sends CAN heartbeat.
 *
 * Exit Condition: IMD fault, masterboard fault, or DCDC dropout.
 * Exit State: HVC_STATE_FAULT
 */
void HVC_State_Monitoring(void)
{
    if (GPIO_Read(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin) == GPIO_PIN_RESET) {
        DEBUG_IO_print("HVC: IMD fault\r\n");
        hvc_state = HVC_STATE_FAULT;
        return;
    }

    if (GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin) == GPIO_PIN_SET) {
        DEBUG_IO_print("HVC: masterboard fault\r\n");
        hvc_state = HVC_STATE_FAULT;
        return;
    }

    // TODO: check DCDC_ACTIVE dropout
    // TODO: check THERMISTOR over-temperature via ADC_GetVoltages()
    // TODO: send CAN status on interval
    // if (timer_elapsed(HVC_CAN_TX_INTERVAL_MS, &ticks.generic)) { CAN_SendStatusMsg(); }

    check_supp_voltage();
}

/**
 * @brief Safe fault state. Opens all contactors, disables loads, blinks FAULT_LED.
 *        Remains here until power cycle.
 */
void HVC_State_Fault(void)
{
    open_all_contactors();
    GPIO_Write(MPPT_CTRL_GPIO_Port, MPPT_CTRL_Pin, GPIO_PIN_RESET);
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_RESET);

    if (timer_elapsed(HVC_FAULT_LED_BLINK_MS, &ticks.fault_led)) {
        GPIO_Toggle(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    }

    // TODO: send CAN fault message on interval
    // if (timer_elapsed(HVC_CAN_TX_INTERVAL_MS, &ticks.generic)) { CAN_SendStatusMsg(); }

    check_supp_voltage();
}
