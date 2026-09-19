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
static LedMode_t s_timedMode = LED_MODE_OFF;
static int s_timedFlashEndTime = 0;
static int s_lastSecretsFound = -1;
static int s_lastKeys = -1;
static int s_lastLevel = -1;
static int s_lastWeapons = -1;
static int s_lastCanvasState = -1;

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

// Rapid urgent crimson double-strobe for critical health (< 12% or <= 15 HP)
static const InfoLedPattern s_patCriticalHealth = {
    .delay = 0x01,
    .smoothing = 0x80,
    .loopDelay = 0x02,
    .blinkSpeed = 0,
    .redPattern = {
        255, 255, 120, 0, 255, 255, 120, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
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

// Celebratory emerald green triple-pulse for secret discovery (3.0 seconds: 24 steps @ 125ms)
static const InfoLedPattern s_patSecret = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF, // Play once
    .blinkSpeed = 0,
    .redPattern = { 0 },
    .greenPattern = {
        40, 180, 255, 255, 180, 40, 0,
        40, 200, 255, 255, 200, 40, 0,
        50, 220, 255, 255, 255, 180, 100, 40, 10, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        5, 20, 30, 30, 20, 5, 0,
        5, 20, 30, 30, 20, 5, 0,
        5, 25, 35, 35, 35, 25, 15, 5, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Radiant Sunburst Gold fanfare for Level Up / Rank Up (3.0 seconds)
static const InfoLedPattern s_patLevelUp = {
    .delay = 0x02,
    .smoothing = 0xA0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        80, 220, 255, 255, 200, 150, 130, 160,
        220, 255, 255, 240, 190, 160, 210, 255,
        255, 255, 220, 160, 100, 50, 15, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        60, 170, 200, 200, 150, 110, 95, 120,
        170, 200, 200, 180, 140, 120, 160, 200,
        200, 200, 170, 120, 75, 35, 10, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        10, 35, 45, 45, 25, 10, 5, 10,
        25, 45, 45, 35, 20, 15, 25, 45,
        45, 45, 35, 20, 10, 5, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Green Keycard acquired - Vivid Lime / Emerald security chime (3.0 seconds)
static const InfoLedPattern s_patKeyGreen = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = { 0 },
    .greenPattern = {
        30, 120, 255, 255, 160, 60, 20, 0,
        40, 150, 255, 255, 255, 220, 170, 120,
        80, 50, 25, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        5, 20, 40, 40, 25, 10, 0, 0,
        5, 25, 40, 40, 40, 35, 25, 15,
        10, 5, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Yellow Keycard acquired - Brilliant Yellow / Amber Gold chime (3.0 seconds)
static const InfoLedPattern s_patKeyYellow = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        30, 120, 255, 255, 160, 60, 20, 0,
        40, 150, 255, 255, 255, 220, 170, 120,
        80, 50, 25, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        25, 100, 220, 220, 140, 50, 15, 0,
        35, 130, 220, 220, 220, 190, 145, 100,
        70, 40, 20, 8, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = { 0 }
};

// Blue Keycard acquired - Deep Electric Sapphire Blue chime (3.0 seconds)
static const InfoLedPattern s_patKeyBlue = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = { 0 },
    .greenPattern = {
        5, 20, 50, 50, 30, 10, 0, 0,
        10, 30, 60, 60, 60, 50, 35, 20,
        10, 5, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        30, 120, 255, 255, 160, 60, 20, 0,
        40, 150, 255, 255, 255, 220, 170, 120,
        80, 50, 25, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Red Keycard acquired - Pure Crimson Ruby Red chime (3.0 seconds)
static const InfoLedPattern s_patKeyRed = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        30, 120, 255, 255, 160, 60, 20, 0,
        40, 150, 255, 255, 255, 220, 170, 120,
        80, 50, 25, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = { 0 },
    .bluePattern = { 0 }
};

// Play Coin Exchange - Sparkling Electric Cyan / Turquoise cascade (3.0 seconds)
static const InfoLedPattern s_patCoinExchange = {
    .delay = 0x02,
    .smoothing = 0x80,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = { 0 },
    .greenPattern = {
        0, 160, 240, 60, 0, 180, 255, 80,
        0, 200, 255, 90, 0, 220, 255, 255,
        190, 140, 90, 50, 25, 10, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        0, 180, 255, 80, 0, 200, 255, 100,
        0, 220, 255, 110, 0, 240, 255, 255,
        220, 160, 110, 60, 30, 15, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Soul Sphere / Megasphere - Mystical Violet / Purple power surge (3.0 seconds)
static const InfoLedPattern s_patSoulSphere = {
    .delay = 0x02,
    .smoothing = 0xB0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        20, 80, 180, 220, 180, 100, 50, 30,
        60, 140, 220, 255, 255, 220, 170, 120,
        80, 50, 30, 15, 5, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        0, 5, 15, 25, 20, 10, 0, 0,
        5, 15, 30, 40, 40, 30, 20, 10,
        5, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        30, 100, 220, 255, 220, 140, 70, 40,
        80, 180, 255, 255, 255, 255, 200, 150,
        100, 65, 40, 20, 10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// New Weapon Acquired - Fiery High-Tech Orange arming surge (3.0 seconds)
static const InfoLedPattern s_patNewWeapon = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        40, 150, 255, 255, 220, 180, 160, 180,
        220, 255, 255, 255, 230, 190, 150, 110,
        80, 55, 35, 20, 10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        10, 40, 80, 80, 65, 50, 45, 55,
        70, 85, 85, 85, 75, 60, 45, 30,
        20, 15, 10, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = { 0 }
};

// Game Saved / Checkpoint - Calming Soft Sky Blue / Cyan pulse (3.0 seconds)
static const InfoLedPattern s_patGameSaved = {
    .delay = 0x02,
    .smoothing = 0xC0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        0, 5, 15, 20, 15, 5, 0, 0,
        5, 15, 25, 25, 20, 15, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        10, 40, 120, 160, 120, 50, 15, 0,
        20, 60, 140, 170, 160, 130, 90, 55,
        30, 15, 5, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        20, 70, 200, 255, 200, 90, 30, 5,
        35, 100, 220, 255, 240, 200, 150, 95,
        55, 30, 10, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Dog Familiar Tamed - Warm Amber Gold bond pulse (3.0 seconds)
static const InfoLedPattern s_patDogTamed = {
    .delay = 0x02,
    .smoothing = 0xA0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        30, 100, 220, 255, 180, 80, 30, 10,
        40, 120, 240, 255, 255, 210, 160, 110,
        70, 40, 20, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        15, 55, 120, 140, 100, 45, 15, 5,
        20, 65, 135, 145, 145, 120, 90, 60,
        40, 20, 10, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        0, 5, 10, 15, 10, 0, 0, 0,
        0, 5, 15, 20, 20, 15, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Player Death - Ominous Blood Red fade-to-black (3.0 seconds)
static const InfoLedPattern s_patDeath = {
    .delay = 0x02,
    .smoothing = 0xB0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        255, 255, 240, 210, 180, 150, 120, 95,
        75, 55, 40, 30, 20, 15, 10, 6,
        3, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = { 0 },
    .bluePattern = { 0 }
};

// Boss Defeated (Cyberdemon / Kronos) - Majestic Victory Royal Purple / Gold flare (3.0 seconds)
static const InfoLedPattern s_patBossDefeated = {
    .delay = 0x02,
    .smoothing = 0x90,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        80, 255, 255, 150, 255, 255, 200, 120,
        255, 255, 255, 200, 255, 255, 255, 220,
        180, 140, 100, 60, 30, 10, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        20, 120, 200, 60, 180, 200, 80, 30,
        100, 180, 200, 120, 180, 200, 180, 140,
        100, 65, 40, 20, 10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        80, 255, 255, 30, 50, 255, 180, 40,
        20, 80, 255, 40, 60, 255, 200, 140,
        90, 50, 25, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Level / Sector Completed - Radiant Starlight White / Cyan celebratory double flare (3.0 seconds)
static const InfoLedPattern s_patLevelComplete = {
    .delay = 0x02,
    .smoothing = 0xA0,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        60, 180, 255, 255, 200, 140, 100, 140,
        200, 255, 255, 255, 240, 190, 150, 100,
        60, 30, 15, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        100, 220, 255, 255, 220, 160, 120, 160,
        220, 255, 255, 255, 255, 210, 170, 120,
        75, 40, 20, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        140, 240, 255, 255, 240, 180, 140, 180,
        240, 255, 255, 255, 255, 220, 180, 130,
        85, 45, 20, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Armor Broken / Gone - Electric Hazard Yellow warning snap & decay (3.0 seconds)
static const InfoLedPattern s_patArmorBroken = {
    .delay = 0x02,
    .smoothing = 0x60,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        255, 255, 0, 255, 255, 0, 200, 220,
        160, 100, 0, 160, 180, 120, 60, 0,
        100, 120, 60, 20, 5, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        180, 180, 0, 180, 180, 0, 140, 150,
        100, 60, 0, 110, 120, 80, 35, 0,
        60, 75, 35, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = { 0 }
};

// Capacity Full / Denied Pickup - Warning Coral-Magenta strobe (3.0 seconds)
static const InfoLedPattern s_patCapacityFull = {
    .delay = 0x02,
    .smoothing = 0x70,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        255, 255, 180, 40, 0, 0, 255, 255,
        180, 40, 0, 0, 160, 160, 80, 20,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        20, 20, 10, 0, 0, 0, 20, 20,
        10, 0, 0, 0, 10, 10, 5, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .bluePattern = {
        100, 100, 60, 15, 0, 0, 100, 100,
        60, 15, 0, 0, 60, 60, 30, 10,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    }
};

// Dog Familiar Attack / Guard Action - Fierce Amber-Orange Canine Strike (3.0 seconds)
static const InfoLedPattern s_patDogAttack = {
    .delay = 0x02,
    .smoothing = 0x80,
    .loopDelay = 0xFF,
    .blinkSpeed = 0,
    .redPattern = {
        120, 255, 255, 80, 0, 140, 255, 255,
        220, 180, 140, 100, 70, 45, 25, 10,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    },
    .greenPattern = {
        50, 140, 140, 35, 0, 60, 140, 140,
        110, 80, 55, 35, 20, 10, 5, 0,
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
        case LED_MODE_CRITICAL_HEALTH:
            MCUHWC_SetInfoLedPattern(&s_patCriticalHealth);
            break;
        case LED_MODE_BERSERK:
            MCUHWC_SetInfoLedPattern(&s_patBerserk);
            break;
        case LED_MODE_SECRET:
            MCUHWC_SetInfoLedPattern(&s_patSecret);
            break;
        case LED_MODE_LEVEL_UP:
            MCUHWC_SetInfoLedPattern(&s_patLevelUp);
            break;
        case LED_MODE_KEY_GREEN:
            MCUHWC_SetInfoLedPattern(&s_patKeyGreen);
            break;
        case LED_MODE_KEY_YELLOW:
            MCUHWC_SetInfoLedPattern(&s_patKeyYellow);
            break;
        case LED_MODE_KEY_BLUE:
            MCUHWC_SetInfoLedPattern(&s_patKeyBlue);
            break;
        case LED_MODE_KEY_RED:
            MCUHWC_SetInfoLedPattern(&s_patKeyRed);
            break;
        case LED_MODE_COIN_EXCHANGE:
            MCUHWC_SetInfoLedPattern(&s_patCoinExchange);
            break;
        case LED_MODE_SOUL_SPHERE:
            MCUHWC_SetInfoLedPattern(&s_patSoulSphere);
            break;
        case LED_MODE_NEW_WEAPON:
            MCUHWC_SetInfoLedPattern(&s_patNewWeapon);
            break;
        case LED_MODE_GAME_SAVED:
            MCUHWC_SetInfoLedPattern(&s_patGameSaved);
            break;
        case LED_MODE_DOG_TAMED:
            MCUHWC_SetInfoLedPattern(&s_patDogTamed);
            break;
        case LED_MODE_DEATH:
            MCUHWC_SetInfoLedPattern(&s_patDeath);
            break;
        case LED_MODE_BOSS_DEFEATED:
            MCUHWC_SetInfoLedPattern(&s_patBossDefeated);
            break;
        case LED_MODE_LEVEL_COMPLETE:
            MCUHWC_SetInfoLedPattern(&s_patLevelComplete);
            break;
        case LED_MODE_ARMOR_BROKEN:
            MCUHWC_SetInfoLedPattern(&s_patArmorBroken);
            break;
        case LED_MODE_CAPACITY_FULL:
            MCUHWC_SetInfoLedPattern(&s_patCapacityFull);
            break;
        case LED_MODE_DOG_ATTACK:
            MCUHWC_SetInfoLedPattern(&s_patDogAttack);
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
    s_timedMode = LED_MODE_OFF;
    s_timedFlashEndTime = 0;
    s_lastSecretsFound = -1;
    s_lastKeys = -1;
    s_lastLevel = -1;
    s_lastWeapons = -1;
    s_lastCanvasState = -1;
}

void Hardware_exit(void)
{
    if (s_mcuHwcAvailable) {
        applyLedMode(LED_MODE_OFF);
        mcuHwcExit();
        s_mcuHwcAvailable = false;
    }
}

void Hardware_triggerTimedLed(LedMode_t mode, int durationMs)
{
    if (durationMs <= 0) durationMs = 3000;
    s_timedMode = mode;
    s_timedFlashEndTime = DoomRPG_GetUpTimeMS() + durationMs;
    // Reset s_currentMode so applyLedMode forces re-submitting pattern even if same mode
    if (s_mcuHwcAvailable) {
        s_currentMode = LED_MODE_OFF;
        applyLedMode(mode);
    }
}

void Hardware_triggerSecretLed(void)
{
    Hardware_triggerTimedLed(LED_MODE_SECRET, 3000);
}

void Hardware_resetSecretCounter(DoomRPG_t* doomRpg)
{
    if (!doomRpg) return;
    if (doomRpg->player) {
        int found = 0, total = 0;
        Player_fillSecretStats(doomRpg->player, &found, &total);
        s_lastSecretsFound = found;
        s_lastKeys = doomRpg->player->keys;
        s_lastLevel = doomRpg->player->level;
        s_lastWeapons = doomRpg->player->weapons;
    }
    if (doomRpg->doomCanvas) {
        s_lastCanvasState = doomRpg->doomCanvas->state;
    }
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

    int now = DoomRPG_GetUpTimeMS();

    // Baseline initialization on first frame
    if (s_lastLevel < 0) s_lastLevel = player->level;
    if (s_lastKeys < 0) s_lastKeys = player->keys;
    if (s_lastWeapons < 0) s_lastWeapons = player->weapons;
    if (s_lastCanvasState < 0) s_lastCanvasState = canvas->state;

    // 1. Detect Secret Discovery
    int found = 0, total = 0;
    Player_fillSecretStats(player, &found, &total);
    if (s_lastSecretsFound >= 0 && found > s_lastSecretsFound) {
        Hardware_triggerTimedLed(LED_MODE_SECRET, 3000);
    }
    s_lastSecretsFound = found;

    // 2. Detect Level Up
    if (player->level > s_lastLevel) {
        Hardware_triggerTimedLed(LED_MODE_LEVEL_UP, 3000);
    }
    s_lastLevel = player->level;

    // 3. Detect Keycard Acquisition
    if (player->keys != s_lastKeys) {
        int newKeys = player->keys & ~s_lastKeys;
        if (newKeys & 1) {
            Hardware_triggerTimedLed(LED_MODE_KEY_GREEN, 3000);
        } else if (newKeys & 2) {
            Hardware_triggerTimedLed(LED_MODE_KEY_YELLOW, 3000);
        } else if (newKeys & 4) {
            Hardware_triggerTimedLed(LED_MODE_KEY_BLUE, 3000);
        } else if (newKeys & 8) {
            Hardware_triggerTimedLed(LED_MODE_KEY_RED, 3000);
        }
        s_lastKeys = player->keys;
    }

    // 4. Detect New Weapon Acquired
    if (player->weapons != s_lastWeapons) {
        if ((player->weapons & ~s_lastWeapons) != 0) {
            Hardware_triggerTimedLed(LED_MODE_NEW_WEAPON, 3000);
        }
        s_lastWeapons = player->weapons;
    }

    // 5. Detect Canvas State Transitions (Death & Dog Taming)
    if (canvas->state == ST_DYING && s_lastCanvasState != ST_DYING) {
        Hardware_triggerTimedLed(LED_MODE_DEATH, 3000);
    } else if (canvas->state == ST_CAPTUREDOG && s_lastCanvasState != ST_CAPTUREDOG) {
        Hardware_triggerTimedLed(LED_MODE_DOG_TAMED, 3000);
    }
    s_lastCanvasState = canvas->state;

    // Priority 1: Timed one-shot flash active (3 seconds)
    if (now < s_timedFlashEndTime && s_timedMode != LED_MODE_OFF) {
        applyLedMode(s_timedMode);
        return;
    } else {
        s_timedMode = LED_MODE_OFF;
    }

    // Only active during gameplay states for continuous ambient modes
    if (canvas->state != ST_PLAYING && canvas->state != ST_COMBAT) {
        applyLedMode(LED_MODE_OFF);
        return;
    }

    // Priority 2: Berserk active
    if (player->berserkerTics > 0) {
        applyLedMode(LED_MODE_BERSERK);
        return;
    }

    // Priority 3: Critical health (< 12% max health or <= 15 HP)
    int hp = CombatEntity_getHealth(&player->ce);
    int maxHp = CombatEntity_getMaxHealth(&player->ce);
    if (hp > 0 && maxHp > 0) {
        if (hp <= (maxHp / 8) || hp <= 15) {
            applyLedMode(LED_MODE_CRITICAL_HEALTH);
            return;
        }
        // Priority 4: Low health (< 25% max health)
        if (hp <= (maxHp / 4)) {
            applyLedMode(LED_MODE_LOW_HEALTH);
            return;
        }
    }

    // Default: Off
    applyLedMode(LED_MODE_OFF);
}

#else

void Hardware_init(void) {}
void Hardware_exit(void) {}
void Hardware_updateLed(DoomRPG_t* doomRpg) { (void)doomRpg; }
void Hardware_triggerTimedLed(LedMode_t mode, int durationMs) { (void)mode; (void)durationMs; }
void Hardware_triggerSecretLed(void) {}
void Hardware_resetSecretCounter(DoomRPG_t* doomRpg) { (void)doomRpg; }

#endif
