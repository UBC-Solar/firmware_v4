/**
 * @file hvc_fsm.h
 * @brief Public interface for the HVC finite state machine
 */

#pragma once

#include "gpio_driver.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "can_driver.h"
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
/* CAN ID's*/
#define HVC_FAULT_ID                0x301U
#define TEL_HEARTBEAT_ID            0x750U                   
#define HVC_HEARTBEAT_ID            0x302U
#define LV_POWERUP_SENT_ID          0x303U
#define HVC_STATUS_ID               0x304U
#define SHUNT_CURRENT_ID            0x305U
#define DIST_HEARTBEAT_ID           0x306U 
#define DIST_FAULT_ID               0x307U
#define LV_POWERUP_RECIEVED_ID      0x308U
#define HVC_LV_CURRENT_ID           0x309U
#define MST_HEARTBEAT_ID            0x310U
#define MST_VOLT_SUMMARY_ID         0x311U
#define SUPP_VOLTAGE_ID             0x317U
#define DCDC_TEMP_VOLTAGE_ID        0x319U
#define HVC_PC_MONITORING_ID        0x320U
/*============================================================================*/
/* TIMEOUT CONSTANTS */

#define MOTOR_PC_TIMEOUT_MS         3000U
#define MPPT_PC_TIMEOUT_MS          3000U
#define MVP_LV_POWERUP_TIMEOUT_MS   3000U
#define MST_READY_TIMEOUT_MS        3000U
#define MST_CHECK_TIMEOUT_MS        3000U
#define FAN_POWERUP_TIMEOUT_MS      3000U
#define HV_CONNECT_TIMEOUT_MS       3000U
#define MOTOR_DISCHARGE_TIMEOUT_MS  3000U
#define LV_POWERUP_TIMEOUT_MS       3000U
#define HEARTBEAT_TIMEOUT_MS        3000U

/*============================================================================*/
/* Delay/Interval CONSTANTS */

#define CONTACTOR_DELAY_MS          200U
#define CAN_TX_INTERVAL_MS          200U
#define MOTOR_DISCHARGE_DELAY_MS    250U
#define FAULT_LED_BLINK_MS          200U
#define LV_POWERUP_RETRY_MS         500U
#define HVC_HEARTBEAT_INTERVAL_MS   1000U

#define MPPT_PC_DELAY_MS            5000U
#define MOTOR_PC_DELAY_MS           5000U



/*============================================================================*/
/* FAN CRTL CONSTANTS */
#define FANS_FULL_SPEED             65535U
#define FANS_HALF_SPEED             32767U
#define FANS_FULL_SPEED_DURATION_MS 1000U

/*============================================================================*/
/* THRESHOLD CONSTANTS */
#define HVC_SUPP_LOW_THRESHOLD_MV   10500
#define Thermistor_MAX_THRESHOLD_MV 5000U
#define DISCHARGE_COMPLETE_THRESHOLD_MV 100000U

/*============================================================================*/
/* RATIOS & SCALES */

#define HVC_PC_COMPLETE_RATIO       90 // % of HV bus voltage for precharge complete
#define HVC_MOTOR_PC_SCALE          56
#define SUPP_SENSE_DIVIDER_NUM      5700  // (R13.4 + R13.5)
#define SUPP_SENSE_DIVIDER_DEN      1000  // R13.5
#define LV_CURRENT_SENSOR_VOFFSET_MV   330    // VS × 0.1, VS = 3.3V, A3U variant
#define LV_CURRENT_SENSOR_MV_PER_A     200    // A3 sensitivity (both B/U variants)

/*============================================================================*/
/* CONTACTOR ACTIVE LEVELS*/
#define HVC_CONTACTOR_CLOSE         GPIO_PIN_SET
#define HVC_CONTACTOR_OPEN          GPIO_PIN_RESET

/*============================================================================*/
/* INTERNAL TYPE DEFS */

typedef struct
{
    uint32_t generic;
    uint32_t fault_led;
    uint32_t neg_contactor;
    uint32_t pos_contactor;
    uint32_t lv_msg;
    uint32_t startup;
    uint32_t retry;
    uint32_t supp_voltage;
} HVC_Ticks_t;

typedef struct {
    bool estop;
    bool imd_fault;
    bool masterboard_fault;
    bool dist_fault;
    bool dcdc_fault;
    bool overcurrent;
    bool undercurrent;
    bool tel_heartbeat_timeout;
    bool mst_heartbeat_timeout;
    bool dist_heartbeat_timeout;
} HVC_FaultFlags_t;
/*============================================================================*/
/* SHARED STATE — defined in hvc_fsm.c */

extern volatile HVC_State_t hvc_state;
extern HVC_Ticks_t ticks;
extern HVC_FaultFlags_t fault_flags;
extern bool startup_complete;
extern bool tel_heartbeat_received;
extern bool mst_status_healthy;
extern bool tel_dist_heartbeat_check_enabled;
extern bool mst_heartbeat_check_enabled;
extern bool lv_powerup_received;
extern bool mst_irq_armed;
extern int32_t mst_pack_voltage_mv;
extern uint32_t last_tel_heartbeat_ms;
extern uint32_t last_mst_heartbeat_ms;
extern uint32_t last_dist_heartbeat_ms;

/*============================================================================*/
/* INTERNAL HELPERS — implemented in hvc_fsm.c */

bool   timer_elapsed(uint32_t interval, uint32_t *last_tick);
void   open_all_contactors(void);
void   check_supp_voltage(void);
const char* state_to_string(HVC_State_t state_in);
void   log_state_change(HVC_State_t new_state, HVC_State_t old_state);
void   log_fault_cause();
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
void MasterboardFaultCallback(bool if_fault);
void HVCurrentAlertCallback(void);
void DCDCFaultCallback(void);
