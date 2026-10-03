#pragma once

#include <stdint.h>

typedef enum {
    FSM_STATE_STARTUP,
    FSM_STATE_ACTIVATE_CTRL,
    FSM_STATE_NORMAL,
    FSM_STATE_FAULT,
} FsmState_t;

void FSM_Init(void);
void FSM_Run(void);
