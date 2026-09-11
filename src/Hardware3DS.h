#ifndef HARDWARE_3DS_H__
#define HARDWARE_3DS_H__

#include "DoomRPG.h"

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_LOW_HEALTH,
    LED_MODE_BERSERK,
    LED_MODE_SECRET
} LedMode_t;

void Hardware_init(void);
void Hardware_exit(void);
void Hardware_updateLed(DoomRPG_t* doomRpg);
void Hardware_triggerSecretLed(void);
void Hardware_resetSecretCounter(DoomRPG_t* doomRpg);

#endif // HARDWARE_3DS_H__
