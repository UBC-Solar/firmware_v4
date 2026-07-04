/**
 * @file hvc_fsm_states.c
 * @brief HVC FSM state implementations
 */

#include "debug_io.h"
#include "gpio_driver.h"
#include "hvc_fsm.h"
#include "can_driver.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
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

    open_all_contactors();
    GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_SET);
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
    GPIO_PinState mst_fault = GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
    #if (INT_TEST_JUNE_11TH == RUN) 
    mst_fault = GPIO_PIN_SET;
#endif

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
    GPIO_PinState mst_fault = GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
    #if (INT_TEST_JUNE_11TH == RUN) 
    mst_fault = GPIO_PIN_RESET;
#endif
    if (mst_fault == GPIO_PIN_RESET) {
        mst_irq_armed = true;
        if (mst_status_healthy) {
            ticks.generic = HAL_GetTick();
            hvc_state = FANS_POWERUP;
            return;
        }
    }
    

    if (timer_elapsed(MST_CHECK_TIMEOUT_MS, &ticks.generic)) {
        DEBUG_IO_print("HVC: MST_CHECK timeout\r\n");
        hvc_state = FAULT;
        return;
    }
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
    
    static bool full_speed_started = false;
    
    if(!full_speed_started) {
        GPIO_Write(FAN_CTRL_GPIO_Port, FAN_CTRL_Pin, GPIO_PIN_SET);
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

    static bool neg_closed = false;
    static bool pos_closed = false;

    if (!neg_closed && timer_elapsed(CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(NEG_CTRL_GPIO_Port, NEG_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        neg_closed = true;
        ticks.neg_contactor = HAL_GetTick();
    }

    if (neg_closed && !pos_closed && timer_elapsed(CONTACTOR_DELAY_MS, &ticks.neg_contactor)) {
        GPIO_Write(POS_CTRL_GPIO_Port, POS_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pos_closed = true;
        ticks.pos_contactor = HAL_GetTick();
    }

    if (pos_closed && timer_elapsed(CONTACTOR_DELAY_MS, &ticks.pos_contactor)) {
        neg_closed = false;
        pos_closed = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MOTOR_DISCHARGE;
    }
}


void MotorDischarge() 
{
    static bool discharge_started = false;
    ADC_Voltages adc = ADC_GetVoltages();

    if ((int64_t) adc.motor_precharge < DISCHARGE_COMPLETE_THRESHOLD_MV) {
        if (!discharge_started) {
            GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_SET);
            ticks.generic = HAL_GetTick();
            discharge_started = true;
        }
        if (timer_elapsed(MOTOR_DISCHARGE_DELAY_MS, &ticks.generic)) {
            GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_RESET);
            ticks.generic = HAL_GetTick();
            hvc_state = MOTOR_PRECHARGE;
        }
    }
    if (timer_elapsed(MOTOR_DISCHARGE_TIMEOUT_MS, &ticks.generic)) {
        DEBUG_IO_print("HVC: MotorDischarge timeout\r\n");
        hvc_state = FAULT;
        return;
    }
}


/**
 * @brief Closes motor precharge contactor and waits for bus voltage to reach threshold.
 *
 * Exit Condition: motor_precharge ADC >= HVC_PC_COMPLETE_RATIO% of HV bus voltage.
 * Exit State: MPPT_PRECHARGE
 *
 * Exit Condition: Timeout (MOTOR_PC_TIMEOUT_MS).
 * Exit State: FAULT
 */
void MotorPrecharge(void)
{

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    ADC_Voltages adc = ADC_GetVoltages();
    if ((int64_t) adc.motor_precharge >= 
        (int64_t) mst_pack_voltage_mv / HVC_MOTOR_PC_SCALE * HVC_PC_COMPLETE_RATIO / 100)
    {
        pc_started = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MPPT_PRECHARGE;
        return;
    }

    if (timer_elapsed(MOTOR_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        DEBUG_IO_print("Motor-Precharge timeout\r\n");
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
 * Exit Condition: Timeout (MPPT_PC_TIMEOUT_MS).
 * Exit State: FAULT
 */
void MpptPrecharge(void)
{

    static bool pc_started = false;

    if (!pc_started) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_CLOSE);
        pc_started = true;
        ticks.generic = HAL_GetTick();
    }

    ADC_Voltages adc = ADC_GetVoltages();
    if ((int64_t) adc.mppt_precharge >= 
        (int64_t) mst_pack_voltage_mv / HVC_MOTOR_PC_SCALE * HVC_PC_COMPLETE_RATIO / 100)
    {
        pc_started = false;
        ticks.generic = HAL_GetTick();
        hvc_state = CLOSE_LLIM;
        return;
    }

    if (timer_elapsed(MPPT_PC_TIMEOUT_MS, &ticks.generic)) {
        pc_started = false;
        DEBUG_IO_print("MPPT-Precharge timeout\r\n");
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

    GPIO_Write(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        ticks.generic = HAL_GetTick();
        hvc_state = CLOSE_HLIM;
    }
}


/**
 * @brief Closes HLIM contactor, opens MPPT precharge, then enables MPPT and DIST.
 *
 * Exit Condition: Settling delay elapsed after HLIM close.
 * Exit State: LV_POWERUP
 */
void CloseHLIM(void)
{

    GPIO_Write(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin, HVC_CONTACTOR_CLOSE);

    if (timer_elapsed(CONTACTOR_DELAY_MS, &ticks.generic)) {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
        GPIO_Write(MPPT_CTRL_GPIO_Port,    MPPT_CTRL_Pin,    GPIO_PIN_SET);
        startup_complete = true;
        ticks.generic = HAL_GetTick();
        hvc_state = LV_POWERUP;
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

    if (timer_elapsed(LV_POWERUP_RETRY_MS, &ticks.retry)) {
        CAN_LV_PowerupMessage();
    }

    if (lv_powerup_received) {
        lv_powerup_received = false;
        ticks.generic = HAL_GetTick();
        hvc_state = MONITORING;
        return;
    }

    if (timer_elapsed(LV_POWERUP_TIMEOUT_MS, &ticks.generic)) {
        DEBUG_IO_print("LV_POWERUP timeout\r\n");
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
    if (GPIO_Read(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin) == GPIO_PIN_SET) {
        DEBUG_IO_print("Monitoring: IMD Fault\r\n");
        hvc_state = FAULT;
        return;
    }

    if (GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin) == GPIO_PIN_SET) {
        DEBUG_IO_print("Monitoring: Masterboard Fault\r\n");
        hvc_state = FAULT;
        return;
    }

    if (GPIO_Read(DCDC_ACTIVE_GPIO_Port, DCDC_ACTIVE_Pin) == GPIO_PIN_SET) {
        DEBUG_IO_print("Monitoring: DCDC dropout Fault\r\n");
        hvc_state = FAULT;
        return;
    }

    ADC_Voltages adc = ADC_GetVoltages();
    if (adc.dcdc_thermistor > Thermistor_MAX_THRESHOLD_MV) {
        DEBUG_IO_print("Monitoring: DCDC Thermistor Over-Temperature Fault\r\n");
        hvc_state = FAULT;
        return;
    }

    if (fault_flags.estop == GPIO_PIN_SET) {
        DEBUG_IO_PRINT("Monitoring: ESTOP Pressed\r\n");
        hvc_state = FAULT;
        return;
    }

    if (fault_flags.dist_fault == GPIO_PIN_SET) {
        DEBUG_IO_PRINT("Monitoring: DIST Fault\r\n");
        hvc_state = FAULT;
        return;
    }

    if (HAL_GetTick() - last_tel_heartbeat_ms > HEARTBEAT_TIMEOUT_MS) {
        DEBUG_IO_print("TEL heartbeat timeout\r\n");
        fault_flags.tel_heartbeat_timeout = true;
        hvc_state = FAULT;
    }
    if (HAL_GetTick() - last_mst_heartbeat_ms > HEARTBEAT_TIMEOUT_MS) {
        DEBUG_IO_print("MST heartbeat timeout\r\n");
        fault_flags.mst_heartbeat_timeout = true;
        hvc_state = FAULT;
    }
    if (HAL_GetTick() - last_dist_heartbeat_ms > HEARTBEAT_TIMEOUT_MS) {
        DEBUG_IO_print("DIST heartbeat timeout\r\n");
        fault_flags.dist_heartbeat_timeout = true;
        hvc_state = FAULT;
    }

    if (timer_elapsed(CAN_TX_INTERVAL_MS, &ticks.generic)) {
        CAN_SendStatusMsg();
        CAN_SendMessage_ShuntCurrent();
    }
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
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);

    if (timer_elapsed(FAULT_LED_BLINK_MS, &ticks.fault_led)) {
        GPIO_Toggle(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    }

    if (timer_elapsed(CAN_TX_INTERVAL_MS, &ticks.generic)) { CAN_SendStatusMsg(); }

    check_supp_voltage();
}
