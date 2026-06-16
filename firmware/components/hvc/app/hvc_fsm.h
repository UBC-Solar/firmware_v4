/**
 * @file hvc_fsm.h
 * @brief Public interface for the HVC finite state machine
 */

#pragma once

#include "gpio_driver.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*============================================================================*/
/* STATE ENUM */

typedef enum
{
    HVC_RESET = 0,
    MVP_LV_POWERUP,
    MST_READY,
    MST_CHECK,
    FANS_POWERUP,
    HV_CONNECT,
    MOTOR_DISCHARGE,
    MOTOR_PRECHARGE,
    MPPT_PRECHARGE,
    CLOSE_LLIM,
    CLOSE_HLIM,
    LV_POWERUP,
    MONITORING,
    FAULT,

    UNKNOWN,
} HVC_State_t;

/*============================================================================*/
/* CONSTANTS — TODO: Verify all constants*/

#define HVC_CONTACTOR_DELAY_MS      200U
#define HVC_MOTOR_PC_TIMEOUT_MS     20000U
#define HVC_MPPT_PC_TIMEOUT_MS      2000U
#define HVC_CAN_TX_INTERVAL_MS      200U
#define HVC_FAULT_LED_BLINK_MS      200U
#define MVP_LV_POWERUP_TIMEOUT_MS   5000U
#define MST_READY_TIMEOUT_MS        5000U
#define LV_POWERUP_MAX_RETRY        5U
#define LV_POWERUP_INTERVAL_MS      500U
#define Thermistor_MAX_THRESHOLD_MV 5000U
#define LV_POWERUP_TIMEOUT_MS       5000U
#define MC_DC_WAIT_TIME_MS          100U
#define FANS_FULL_SPEED             65535U
#define FANS_HALF_SPEED             32767U
#define FANS_FULL_SPEED_DURATION_MS 2000U

#define HVC_SUPP_LOW_THRESHOLD_MV   10500
#define HVC_PC_COMPLETE_RATIO          90   // % of HV bus voltage for precharge complete
#define HVC_MOTOR_PC_SCALE       56

/* CONTACTOR ACTIVE LEVELS — TODO: verify polarity from schematic */
#define HVC_CONTACTOR_CLOSE         GPIO_PIN_SET
#define HVC_CONTACTOR_OPEN          GPIO_PIN_RESET

/* CAN Message ID's — TODO: Set to specific message decided by user*/
#define TEL_HEARTBEAT_ID            0x300          
#define HVC_HEARTBEAT_ID            0x302
#define LV_POWERUP_ID               0x303
#define MST_HEARTBEAT_ID            0x622
#define DIST_FAULT_ID               0x324

#define MAX_STATE_NAME_LEN          20U

/*============================================================================*/
/* INTERNAL TYPE DEFS */

typedef struct
{
    uint32_t generic;
    uint32_t fault_led;
    uint32_t neg_contactor;
    uint32_t pos_contactor;
    uint32_t lv_msg;
} HVC_Ticks_t;

typedef struct {
    bool estop;
    bool imd_fault;
    bool masterboard_fault;
    bool dist_fault;
    bool overcurrent;
    bool undercurrent;
} HVC_FaultFlags_t;
/*============================================================================*/
/* SHARED STATE — defined in hvc_fsm.c */

extern volatile HVC_State_t hvc_state;
extern HVC_Ticks_t ticks;
extern bool startup_complete;
extern bool tel_heartbeat_received;
extern bool mst_status_healthy;
extern int32_t mst_pack_voltage_mv;
extern bool lv_powerup_received;
extern HVC_FaultFlags_t fault_flags;

/*============================================================================*/
/* INTERNAL HELPERS — implemented in hvc_fsm.c */

bool   timer_elapsed(uint32_t interval, uint32_t *last_tick);
void   open_all_contactors(void);
void   check_supp_voltage(void);
const char* state_to_string(HVC_State_t state_in);
void    log_state_change(HVC_State_t new_state, HVC_State_t old_state);

/*============================================================================*/
/* STATE FUNCTION PROTOTYPES — implemented in hvc_fsm_states.c */

void Reset(void);
void MvpLvPowerup(void);
void MST_Ready(void);
void MST_Check(void);
void Fans_Powerup(void);
void HV_Connect(void);
void MotorPrecharge(void);
void MpptPrecharge(void);
void MotorDischarge(void);
void CloseLLIM(void);
void CloseHLIM(void);
void LvPowerup(void);
void Monitoring(void);
void Fault(void);

/*============================================================================*/
/* PUBLIC API */

void HVC_FSM_Init(void);
void HVC_FSM_Run(void);

void ESTOPCallback(void);
void IMDFaultCallback(void);
void MasterboardFaultCallback(void);
void HVCurrentAlertCallback(void);