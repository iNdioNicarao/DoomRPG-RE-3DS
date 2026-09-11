#include "Hardware3DS.h"
#include "CombatEntity.h"
#include "Render.h"
#include "Player.h"
#include "DoomCanvas.h"

#ifdef __3DS__
#include <3ds.h>
#include <3ds/services/mcuhwc.h>

static bool s_mcuHwcAvailable = false;
static LedMode_t s_currentMode = LED_MODE_OFF;
static int s_secretFlashEndTime = 0;
static int s_lastSecretsFound = 0;

static const InfoLedPattern s_patOff = {
    .delay = 0,
    .smoothing = 0,
    .loopDelay = 0,
    .blinkSpeed = 0,
    .redPattern = { 0 },
    .greenPattern = { 0 },
    .bluePattern = { 0 }
};

// Smooth crimson heartbeat pulse for low health (< 25%)
static const InfoLedPattern s_patLowHealth = {
    .delay = 0x02,
    .smoothing = 0xFF,
    .loopDelay = 0x06,
    .blinkSpeed = 0,
    .redPattern = {
        0, 15, 35, 70, 120, 180, 240, 255,
        255, 240, 180, 120, 70, 35, 15, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = { 0 },
    .bluePattern = { 0 }
};

// Rapid energetic fiery amber/orange throb for Berserk mode
static const InfoLedPattern s_patBerserk = {
    .delay = 0x01,
    .smoothing = 0xFF,
    .loopDelay = 0x02,
    .blinkSpeed = 0,
    .redPattern = {
        0, 40, 90, 160, 220, 255, 255, 220,
        160, 90, 40, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        0, 10, 25, 45, 65, 80, 80, 65,
        45, 25, 10, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = { 0 }
};

// Celebratory emerald green double-flash for secret discovery (one-shot)
static const InfoLedPattern s_patSecret = {
    .delay = 0x02,
    .smoothing = 0x00,
    .loopDelay = 0xFF, // Play once
    .blinkSpeed = 0x02,
    .redPattern = { 0 },
    .greenPattern = {
        255, 255, 255, 0, 0, 255, 255, 255,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = { 0 }
};

static void applyLedMode(LedMode_t mode)
{
    if (!s_mcuHwcAvailable) return;
    if (s_currentMode == mode) return;

    s_currentMode = mode;
    switch (mode) {
        case LED_MODE_LOW_HEALTH:
            MCUHWC_SetInfoLedPattern(&s_patLowHealth);
            break;
        case LED_MODE_BERSERK:
            MCUHWC_SetInfoLedPattern(&s_patBerserk);
            break;
        case LED_MODE_SECRET:
            MCUHWC_SetInfoLedPattern(&s_patSecret);
            break;
        case LED_MODE_OFF:
        default:
            MCUHWC_SetInfoLedPattern(&s_patOff);
            break;
    }
}

void Hardware_init(void)
{
    Result res = mcuHwcInit();
    s_mcuHwcAvailable = R_SUCCEEDED(res);
    if (!s_mcuHwcAvailable) {
        printf("mcuHwcInit failed: 0x%08lX (running without LED notification)\n", (unsigned long)res);
    }
    s_currentMode = LED_MODE_OFF;
    s_secretFlashEndTime = 0;
    s_lastSecretsFound = 0;
}

void Hardware_exit(void)
{
    if (s_mcuHwcAvailable) {
        applyLedMode(LED_MODE_OFF);
        mcuHwcExit();
        s_mcuHwcAvailable = false;
    }
}

void Hardware_triggerSecretLed(void)
{
    s_secretFlashEndTime = DoomRPG_GetUpTimeMS() + 1500;
    applyLedMode(LED_MODE_SECRET);
}

void Hardware_resetSecretCounter(DoomRPG_t* doomRpg)
{
    if (!doomRpg || !doomRpg->player) return;
    int found = 0, total = 0;
    Player_fillSecretStats(doomRpg->player, &found, &total);
    s_lastSecretsFound = found;
}

void Hardware_updateLed(DoomRPG_t* doomRpg)
{
    if (!s_mcuHwcAvailable || !doomRpg) return;

    Player_t* player = doomRpg->player;
    DoomCanvas_t* canvas = doomRpg->doomCanvas;
    if (!player || !canvas) {
        applyLedMode(LED_MODE_OFF);
        return;
    }

    // Only active during gameplay states
    if (canvas->state != ST_PLAYING && canvas->state != ST_COMBAT) {
        applyLedMode(LED_MODE_OFF);
        return;
    }

    int now = DoomRPG_GetUpTimeMS();

    // Check secrets
    int found = 0, total = 0;
    Player_fillSecretStats(player, &found, &total);
    if (found > s_lastSecretsFound) {
        s_lastSecretsFound = found;
        Hardware_triggerSecretLed();
    }

    // Priority 1: Secret flash active
    if (now < s_secretFlashEndTime) {
        applyLedMode(LED_MODE_SECRET);
        return;
    }

    // Priority 2: Berserk active
    if (player->berserkerTics > 0) {
        applyLedMode(LED_MODE_BERSERK);
        return;
    }

    // Priority 3: Low health (< 25% max health)
    int hp = CombatEntity_getHealth(&player->ce);
    int maxHp = CombatEntity_getMaxHealth(&player->ce);
    if (hp > 0 && maxHp > 0 && hp <= (maxHp / 4)) {
        applyLedMode(LED_MODE_LOW_HEALTH);
        return;
    }

    // Default: Off
    applyLedMode(LED_MODE_OFF);
}

#else

void Hardware_init(void) {}
void Hardware_exit(void) {}
void Hardware_updateLed(DoomRPG_t* doomRpg) { (void)doomRpg; }
void Hardware_triggerSecretLed(void) {}
void Hardware_resetSecretCounter(DoomRPG_t* doomRpg) { (void)doomRpg; }

#endif
