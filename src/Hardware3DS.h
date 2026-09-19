#ifndef HARDWARE_3DS_H__
#define HARDWARE_3DS_H__

#include "DoomRPG.h"

typedef enum {
    LED_MODE_OFF = 0,
    // Ambient / Continuous modes
    LED_MODE_LOW_HEALTH,
    LED_MODE_CRITICAL_HEALTH,
    LED_MODE_BERSERK,
    // One-shot timed flashes (3 seconds)
    LED_MODE_SECRET,
    LED_MODE_LEVEL_UP,
    LED_MODE_KEY_GREEN,
    LED_MODE_KEY_YELLOW,
    LED_MODE_KEY_BLUE,
    LED_MODE_KEY_RED,
    LED_MODE_SOUL_SPHERE,
    LED_MODE_COIN_EXCHANGE,
    LED_MODE_NEW_WEAPON,
    LED_MODE_GAME_SAVED,
    LED_MODE_DOG_TAMED,
    LED_MODE_DEATH,
    LED_MODE_BOSS_DEFEATED
} LedMode_t;

void Hardware_init(void);
void Hardware_exit(void);
void Hardware_updateLed(DoomRPG_t* doomRpg);
void Hardware_triggerTimedLed(LedMode_t mode, int durationMs);
void Hardware_triggerSecretLed(void);
void Hardware_resetSecretCounter(DoomRPG_t* doomRpg);

#endif // HARDWARE_3DS_H__
