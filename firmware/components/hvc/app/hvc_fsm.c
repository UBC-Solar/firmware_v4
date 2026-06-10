/**
 * @file hvc_fsm.c
 * @brief HVC FSM core — init, dispatch, helpers, and interrupt callbacks
 */

#include "hvc_fsm.h"
#include "main.h"


/*============================================================================*/
/* SHARED VARIABLES */

volatile HVC_State_t hvc_state;
HVC_Ticks_t ticks;
bool startup_complete = false;
bool tel_heartbeat_received = false;

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
        hvc_state = RESET;
    }
}

/**
 * @brief Dispatches to the current state handler. Call from HVC_Main().
 */
void HVC_FSM_Run(void)
{
    switch (hvc_state) {
        case HVC_RESET:
            Reset();
            break;
        case MVP_LV_POWERUP:
            MvpLvPowerup();
            break;
        case MST_READY:
            MSTready();
            break;
        case MST_CHECK:
            MSTcheck();
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
}

/*============================================================================*/
/* INTERRUPT CALLBACKS */

void HVC_ESTOPCallback(void)
{
    GPIO_Write(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin, GPIO_PIN_SET);
    // TODO: set ESTOP flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void HVC_IMDFaultCallback(void)
{
    // TODO: set IMD fault flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void HVC_MasterboardFaultCallback(void)
{
    // TODO: set masterboard fault flag in CAN data struct
    hvc_state = FAULT;
    HVC_FSM_Run();
}

void HVC_HVCurrentAlertCallback(void)
{
    // TODO: read INA228 DIAG_ALRT register to distinguish over/under current
    hvc_state = FAULT;
    HVC_FSM_Run();
}
void HVC_DistFaultCallback(void){
    //TODO: Send CAN message to HVC
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
