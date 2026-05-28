#include "gpio_app.h"

#include "gpio_driver.h"

static volatile uint32_t s_current_velocity_kmh = 0U;

void StrState(void)
{
    LightState();
    CruiseState(ReadCurrentVelocity());
}

uint32_t GetVelocity(uint32_t velocity)
{
    s_current_velocity_kmh = velocity;
    return s_current_velocity_kmh;
}

uint32_t ReadCurrentVelocity(void)
{
    return s_current_velocity_kmh;
}