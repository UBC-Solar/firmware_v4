/**
 * @file hvc_fsm_states.c
 * @brief HVC FSM state implementations
 */

#include "hvc_fsm.h"
#include "can_driver.h"
#include "stm32f1xx_hal.h"
#include "tim.h"

/*============================================================================*/
/* STATES */

/**
 * @brief Opens all contactors and disables load outputs.
 *
 * Exit State: HV_CONNECT
 */
void Reset(void)
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
    hvc_state = MVP_LV_POWERUP;
}

/**
 * @brief Powers DIST to bring up telemetry LV systems.
 *
 * Exit Condition: Telemetry CAN message received (TEL present build), or
 *                 immediate/alternate handling for TEL-absent build.
 * Exit State: MST_READY
 */
void MvpLvPowerup(void) {
    DEBUG_IO_print("HVC: STATE_MVP_LV_POWERUP\r\n");
    
    static bool dist_powered = false;
    if (!dist_powered) {
        GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);
        dist_powered = true;
        ticks.generic = HAL_GetTick();
    }

    if (tel_heartbeat_received) {
        tel_heartbeat_received = false;
        dist_powered = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MST_READY;
        return;
    }

    if (timer_elapsed(MVP_LV_POWERUP_TIMEOUT_MS, &ticks.generic)) {
        dist_powered = false;
        DEBUG_IO_print("HVC: MVP_LV_POWERUP timeout\r\n");
        hvc_state = FAULT;
        return;
    }
}

/**
 * @brief Waits for master board fault line handshake (HIGH then LOW).
 *
 * Exit Condition: MASTERBOARD_FAULT observed high then low.
 * Exit State: MST_CHECK
 */
void MST_Ready(void)
{
    DEBUG_IO_print("HVC: STATE_MST_READY\r\n");

    GPIO_PinState mst_fault = GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);

    if (mst_fault == GPIO_PIN_SET) {
        ticks.generic = HAL_GetTick();
        hvc_state = MST_CHECK;
        return;
    }

    if (timer_elapsed(MST_READY_TIMEOUT_MS, &ticks.generic)) {
        DEBUG_IO_print("HVC: MST_READY timeout\r\n");
        hvc_state = FAULT;
        return;
    }
}

/**
 * @brief Confirms CAN Communication with MST and Healthy Cell-State of Battery-Pack 

 *
 * Exit Condition: Recieves MST CAN message with healthy cell state
 * Exit State: FANS_POWERUP
 */
void MST_Check(void)
{
    DEBUG_IO_print("HVC: STATE_MST_CHECK\r\n");

    if (mst_status_healthy) {
        ticks.generic = HAL_GetTick();
        hvc_state = FANS_POWERUP;
        return;
    }

    if (timer_elapsed(MST_READY_TIMEOUT_MS, &ticks.generic)) {
        DEBUG_IO_print("HVC: MST_CHECK timeout\r\n");
        hvc_state = FAULT;
        return;
    }

#if (INT_TEST_JUNE_11TH == RUN)
    mst_status_healthy = true;
#endif // (INT_TEST_JUNE_11TH == RUN)
}   

/**
 * @brief Powers pack cooling fans prior to HV sequence.
 *
 * Exit Condition: Fan power-up complete.
 * Exit State: HV_CONNECT
 *
 * Exit Condition: Fault detected during fan power-up.
 * Exit State: FAULT
 */
void Fans_Powerup(void)
{
    DEBUG_IO_print("HVC: STATE_FANS_POWERUP\r\n");
    
    static bool full_speed_started = false;
    
    if(!full_speed_started) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, FANS_FULL_SPEED);
        full_speed_started = true;
        ticks.generic = HAL_GetTick();

    }

    if (timer_elapsed(FANS_FULL_SPEED_DURATION_MS, &ticks.generic)) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, FANS_HALF_SPEED);
        full_speed_started = false;
        ticks.generic = HAL_GetTick();
        hvc_state = HV_CONNECT;
        return;
    }
}

/**
 * @brief Closes NEG then POS contactors with a settling delay between each step.
 *
 * Exit Condition: POS closed and settling delay elapsed.
 * Exit State: MOTOR_PRECHARGE
 */
void HV_Connect(void)
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
        hvc_state = MOTOR_PRECHARGE;
    }
}



void MotorDischarge() 
{

}
/**
 * @brief Closes motor precharge contactor and waits for bus voltage to reach threshold.
 *
 * Exit Condition: motor_precharge ADC >= HVC_PC_COMPLETE_RATIO% of HV bus voltage.
 * Exit State: MPPT_PRECHARGE
 *
 * Exit Condition: Timeout (HVC_MOTOR_PC_TIMEOUT_MS).
 * Exit State: FAULT
 */
void MotorPrecharge(void)
{
    DEBUG_IO_print("HVC: STATE_MOTOR_PRECHARGE\r\n");

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    // TODO: compare motor_precharge ADC voltage to HV bus voltage
    ADC_Voltages adc = ADC_GetVoltages();
    if (adc.motor_precharge >= mst_pack_voltage_mv * HVC_PC_COMPLETE_RATIO / 100) {
        pc_started = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MPPT_PRECHARGE;
        return;
    }

    if (timer_elapsed(HVC_MOTOR_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        hvc_state = FAULT;
        return;
    }
}

/**
 * @brief Closes MPPT precharge contactor and waits for bus voltage to reach threshold.
 *
 * Exit Condition: mppt_precharge ADC >= HVC_PC_COMPLETE_RATIO% of HV bus voltage.
 * Exit State: CLOSE_LLIM
 *
 * Exit Condition: Timeout (HVC_MPPT_PC_TIMEOUT_MS).
 * Exit State: FAULT
 */
void MpptPrecharge(void)
{
    DEBUG_IO_print("HVC: STATE_MPPT_PRECHARGE\r\n");

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    // TODO: compare mppt_precharge ADC voltage to HV bus voltage
    ADC_Voltages adc = ADC_GetVoltages();
    if (adc.mppt_precharge >= mst_pack_voltage_mv * HVC_PC_COMPLETE_RATIO / 100) {
        pc_started = false;
        ticks.generic = HAL_GetTick();
        hvc_state = CLOSE_LLIM;
        return;
    }

    if (timer_elapsed(HVC_MPPT_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        hvc_state = FAULT;
        return;
    }
}

/**
 * @brief Closes LLIM contactor then opens motor precharge contactor.
 *
 * Exit Condition: Settling delay elapsed.
 * Exit State: CLOSE_HLIM
 */
void CloseLLIM(void)
{
    DEBUG_IO_print("HVC: STATE_CLOSE_MOTOR_BUS\r\n");

    GPIO_Write(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        ticks.generic = HAL_GetTick();
        hvc_state = CLOSE_HLIM;
    }
}


/**
 * @brief Closes HLIM contactor, opens MPPT precharge, then enables MPPT and DIST.
 *
 * Exit Condition: Settling delay elapsed after HLIM close.
 * Exit State: MONITORING
 */
void CloseHLIM(void)
{
    DEBUG_IO_print("HVC: STATE_CLOSE_HLIM\r\n");

    GPIO_Write(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(HVC_CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        GPIO_Write(MPPT_CTRL_GPIO_Port,    MPPT_CTRL_Pin,    GPIO_PIN_SET);
        GPIO_Write(DIST_CTRL_GPIO_Port,    DIST_CTRL_Pin,    GPIO_PIN_SET);
        startup_complete = true;
        ticks.generic = HAL_GetTick();
        hvc_state = MONITORING;
    }
}

/**
 * @brief Powers remaining LV systems after HV path is established.
 *
 * Exit Condition: LV power-up sequence complete.
 * Exit State: MONITORING
 */
void LvPowerup(void)
{
    DEBUG_IO_print("HVC: STATE_LV_POWERUP\r\n");

    static bool msg_sent = false;

    if (!msg_sent) {
        CAN_SendMessage323();
        msg_sent = true;
        ticks.generic = HAL_GetTick();
    }

    if (lv_powerup_received) {
        lv_powerup_received = false;
        msg_sent = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MONITORING;
        return;
    }

    if (timer_elapsed(LV_POWERUP_TIMEOUT_MS, &ticks.generic)) {
        msg_sent = false;
        DEBUG_IO_print("HVC: LV_POWERUP timeout\r\n");
        hvc_state = FAULT;
        return;
    }
}

/**
 * @brief Normal operating state. Polls fault inputs and sends CAN heartbeat.
 *
 * Exit Condition: IMD fault, masterboard fault, or DCDC dropout.
 * Exit State: FAULT
 */
void Monitoring(void)
{
    if (GPIO_Read(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin) == GPIO_PIN_RESET) {
        DEBUG_IO_print("HVC: IMD fault\r\n");
        hvc_state = FAULT;
        return;
    }

    if (GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin) == GPIO_PIN_SET) {
        DEBUG_IO_print("HVC: masterboard fault\r\n");
        hvc_state = FAULT;
        return;
    }
    // TODO: check DCDC_ACTIVE dropout
    if (GPIO_Read(DCDC_ACTIVE_GPIO_Port, DCDC_ACTIVE_Pin) == GPIO_PIN_RESET) {
        DEBUG_IO_print("HVC: DCDC dropout fault\r\n");
        hvc_state = FAULT;
        return;
    }
    // TODO: check THERMISTOR over-temperature via ADC_GetVoltages()
    ADC_Voltages adc = ADC_GetVoltages();
    if (adc.dcdc_thermistor > Thermistor_MAX_THRESHOLD_MV) {
        DEBUG_IO_print("HVC: thermistor over-temperature\r\n");
        hvc_state = FAULT;
        return;
    }
    //TODO: Send CAN Status Message
    if (timer_elapsed(HVC_CAN_TX_INTERVAL_MS, &ticks.generic)) { CAN_SendStatusMsg(); }

    check_supp_voltage();
}

/**
 * @brief Safe fault state. Opens all contactors, disables loads, blinks FAULT_LED.
 *        Remains here until power cycle.
 */
void Fault(void)
{
    open_all_contactors();
    GPIO_Write(MPPT_CTRL_GPIO_Port, MPPT_CTRL_Pin, GPIO_PIN_RESET);
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_RESET);

    if (timer_elapsed(HVC_FAULT_LED_BLINK_MS, &ticks.fault_led)) {
        GPIO_Toggle(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    }

    if (timer_elapsed(HVC_CAN_TX_INTERVAL_MS, &ticks.generic)) { CAN_SendStatusMsg(); }

    check_supp_voltage();
}
