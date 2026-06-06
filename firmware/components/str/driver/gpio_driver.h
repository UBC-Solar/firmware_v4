#ifndef __GPIO_DRIVER_H__
#define __GPIO_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile bool rts_en;
    volatile bool lts_en;
} LightsCtx;

typedef struct {
    volatile bool cruise_en;
    volatile bool cruise_inc;
    volatile bool cruise_dec;
} CruiseCtx;

typedef struct {
    volatile bool horn_en;
    volatile bool ptt_en;
    volatile bool next_page;
    volatile bool regen_en;
    LightsCtx lights_state;
    CruiseCtx cruise_state;
} StrGpioCtx;

extern volatile StrGpioCtx gpio_pin_state;

void LightState(void);
void CruiseState(uint32_t velocity);
void GpioPollState(void);
void StrInterruptHandler(uint16_t toggle);

#endif /* __GPIO_DRIVER_H__ */