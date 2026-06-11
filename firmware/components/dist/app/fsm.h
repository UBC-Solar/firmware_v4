#pragma once

#include <stdint.h>

typedef enum {
    FSM_STATE_STARTUP,
    FSM_STATE_ACTIVATE_CTRL,
    FSM_STATE_NORMAL,
    FSM_STATE_FAULT,
} FsmState_t;

void FSM_Init(uint8_t led_driver_available);
void FSM_Run(void);
