/**
 * @file hvc_fsm.c
 * @brief HVC FSM core — init, dispatch, helpers, and interrupt callbacks
 */

#include "hvc_fsm.h"
#include "debug_io.h"
#include "main.h"
#include "stm32f1xx.h"



/*============================================================================*/
/* SHARED VARIABLES */

volatile HVC_State_t hvc_state;
HVC_Ticks_t ticks;
bool startup_complete = false;
bool tel_heartbeat_received = false;
bool mst_heartbeat_received = false;
bool mst_status_healthy = false;
int32_t mst_pack_voltage_mv = 0;
bool lv_powerup_received = false;

/*============================================================================*/
/* PUBLIC API */

/**
 * @brief Initializes the FSM. Transitions to FAULT on watchdog-triggered reset.
 */
void HVC_FSM_Init(void)
{
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        DEBUG_IO_print("HVC: watchdog reset\r\n");
        hvc_state = FAULT;
    } else {
        hvc_state = HVC_RESET;
    }

    #if (INT_TEST_JUNE_11TH == RUN)
    mst_pack_voltage_mv = 115 * 1000;
    mst_heartbeat_received = true;

    #endif // (INT_TEST_CAN == RUN)

}

/**
 * @brief Dispatches to the current state handler. Call from HVC_Main().
 */
void HVC_FSM_Run(void)
{
    static HVC_State_t prev_hvc_state = HVC_RESET;
    log_state_change(hvc_state, prev_hvc_state);

    switch (hvc_state) {
        case HVC_RESET:
            Reset();
            break;
        case MVP_LV_POWERUP:
            MvpLvPowerup();
            break;
        case MST_READY:
            MST_Ready();
            break;
        case MST_CHECK:
            MST_Check();
            break;
        case FANS_POWERUP:
            Fans_Powerup();
            break;
        case HV_CONNECT:
            HV_Connect();
            break;
        case MOTOR_DISCHARGE:
            MotorDischarge();
            break;
        case MOTOR_PRECHARGE:
            MotorPrecharge();
            break;
        case MPPT_PRECHARGE:
            MpptPrecharge();
            break;
        case CLOSE_LLIM:
            CloseLLIM();
            break;  
        case CLOSE_HLIM:
            CloseHLIM();
            break;
        case LV_POWERUP:
            LvPowerup();
            break;
        case MONITORING:
            Monitoring();
            break;
        case FAULT:
        default:
            Fault();
            break;
    }

    prev_hvc_state = hvc_state;
}

/*============================================================================*/
/* INTERRUPT CALLBACKS */

void ESTOPCallback(void)
{
    GPIO_Write(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin, GPIO_PIN_SET);
    // TODO: set ESTOP flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void IMDFaultCallback(void)
{
    // TODO: set IMD fault flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void MasterboardFaultCallback(void)
{
    // TODO: set masterboard fault flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void HVCurrentAlertCallback(void)
{
    // TODO: read INA228 DIAG_ALRT register to distinguish over/under current
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void DistFaultCallback(void){
    //TODO: Send CAN message to HVC
    hvc_state = FAULT;
    HVC_FSM_Run();
}

/*============================================================================*/
/* HELPERS */

bool timer_elapsed(uint32_t interval, uint32_t *last_tick)
{
    if (HAL_GetTick() - *last_tick >= interval) {
        *last_tick = HAL_GetTick();
        return true;
    }
    return false;
}

void open_all_contactors(void)
{
    GPIO_Write(NEG_CTRL_GPIO_Port,      NEG_CTRL_Pin,      HVC_CONTACTOR_OPEN);
    GPIO_Write(POS_CTRL_GPIO_Port,      POS_CTRL_Pin,      HVC_CONTACTOR_OPEN);
    GPIO_Write(LLIM_CTRL_GPIO_Port,     LLIM_CTRL_Pin,     HVC_CONTACTOR_OPEN);
    GPIO_Write(HLIM_CTRL_GPIO_Port,     HLIM_CTRL_Pin,     HVC_CONTACTOR_OPEN);
    GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
    GPIO_Write(MPPT_PC_CTRL_GPIO_Port,  MPPT_PC_CTRL_Pin,  HVC_CONTACTOR_OPEN);
}

void check_supp_voltage(void)
{
    // TODO: convert ADC supp_sense to mV and drive SUPP_LOW_LED
    ADC_Voltages adc = ADC_GetVoltages();
    GPIO_PinState state = (adc.supp_sense < HVC_SUPP_LOW_THRESHOLD_MV)
                           ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, state);
}

// Note that char pointers returned from this function will always point to string literals
// stored inside flash memory (where the program itself is stored). We're not using heap space here.
const char* state_to_string(HVC_State_t state_in) {
    switch (state_in) {
        case HVC_RESET:         return "HVC_RESET";
        case MVP_LV_POWERUP:    return "MVP_LV_POWERUP";
        case MST_READY:         return "MST_READY";
        case MST_CHECK:         return "MST_CHECK";
        case FANS_POWERUP:      return "FANS_POWERUP";
        case HV_CONNECT:        return "HV_CONNECT";
        case MOTOR_DISCHARGE:   return "MOTOR_DISCHARGE";
        case MOTOR_PRECHARGE:   return "MOTOR_PRECHARGE";
        case MPPT_PRECHARGE:    return "MPPT_PRECHARGE";
        case CLOSE_LLIM:        return "CLOSE_LLIM";
        case CLOSE_HLIM:        return "CLOSE_HLIM";
        case LV_POWERUP:        return "LV_POWERUP";
        case MONITORING:        return "MONITORING";
        case FAULT:             return "FAULT";
        default:                return "UNKNOWN";
    }
}

void log_state_change(HVC_State_t new_state, HVC_State_t old_state) {
#ifndef DEBUG
    return;
#endif // DEBUG

    if (new_state == old_state) return;

    const char *new_state_str = state_to_string(new_state);
    const char *old_state_str = state_to_string(old_state);

    DEBUG_IO_PRINT("HVC State change: new=%s, old=%s", new_state_str, old_state_str);
}
