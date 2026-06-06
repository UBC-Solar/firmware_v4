#ifndef GPIO_APP_H
#define GPIO_APP_H

#include <stdint.h>

void StrState(void);
uint32_t GetVelocity(uint32_t velocity_kmh);
uint32_t ReadCurrentVelocity(void);
uint32_t GetCruiseSetVelocity(uint32_t velocity_kmh);
uint32_t ReadCruiseSetVelocity(void);

#endif // GPIO_APP_H
