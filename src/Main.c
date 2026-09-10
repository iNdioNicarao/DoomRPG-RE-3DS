// Using SDL and standard IO
#ifdef __3DS__
  #include <3ds.h>
  #include <SDL/SDL.h>        // SDL 1.2 для 3DS-порта
  #include <SDL/SDL_mixer.h>
#else
  #include <SDL.h>            // SDL2
  #include <SDL_mixer.h>
#endif
#include <stdio.h>
#include <zlib.h>

#include "Z_Zone.h"
#include "DoomRPG.h"
#include "DoomCanvas.h"
#include "Player.h"
#include "Game.h"
#include "Hud.h"
#include "MenuSystem.h"
#include "SDL_Video.h"
#include "Z_Zip.h"

extern DoomRPG_t* doomRpg;

// 3DS: libctru's initSystem creates the main thread with this many bytes of stack.
// Default 32KB is exhausted by the map-load call chain (Render_loadTexels
// frame + locals), causing data aborts near the stack ceiling.
#if defined(__3DS__)
unsigned int __stacksize__ = 0x40000; // 256KB
#endif

// Build identity - bump on every build so we can verify what is actually running.
static const char* BUILD_VERSION = "DOOMRPG-3DS v1.1.0";

int main(int argc, char* args[])
{
#ifdef __3DS__
    /* Enable 804MHz CPU clock + L2 cache on New 3DS (safe no-op on Old 3DS) */
    osSetSpeedupEnable(true);
#endif
    Game_ensureSaveDir();
    printf("\n=== %s ===\n", BUILD_VERSION);
    {
        FILE* bv = fopen("sdmc:/3ds/doomrpg/version.log", "w");
        if (bv) { fprintf(bv, "%s\n", BUILD_VERSION); fclose(bv); }
    }
    Z_Init();
    SDL_InitVideo();
    SDL_InitAudio();
    SDL_ShowCursor(SDL_DISABLE);

    if (DoomRPG_Init() == 0) {
        DoomRPG_Error("Failed to initialize Doom Rpg\n");
    }

    // Состояние клавиатуры
#ifdef __3DS__
    const Uint8* state = SDL_GetKeyState(NULL);   // SDL 1.2 API
#else
    const Uint8* state = SDL_GetKeyboardState(NULL);
#endif

    SDL_Event ev;
    int UpTime = 0;
    int mouseTime = 0;
    int key = 0, oldKey = -1;
    int mouse_Button = MOUSE_BUTTON_INVALID;
    while (aptMainLoop() && !doomRpg->closeApplet)
    {
#ifdef __3DS__
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp   = hidKeysUp();

        if (kHeld & KEY_TOUCH) {
            g_botScreenDirty = true;
            touchPosition touch;
            hidTouchRead(&touch);
            int touchX = (touch.px * 400) / 320;
            int touchY = 240 + touch.py;
            DoomCanvas_handleTouchHeld(doomRpg->doomCanvas, touchX, touchY, (kDown & KEY_TOUCH) != 0);
        } else if (kUp & KEY_TOUCH) {
            g_botScreenDirty = true;
            DoomCanvas_handleTouchUp(doomRpg->doomCanvas);
        }

        /* Instant text reveal on dialogs when tapping A or Touch */
        if ((kDown & (KEY_A | KEY_TOUCH)) &&
            (doomRpg->doomCanvas->state == ST_DIALOG || doomRpg->doomCanvas->state == ST_DIALOGPASSWORD)) {
            if (doomRpg->doomCanvas->dialogTypeLineIdx < doomRpg->doomCanvas->numDialogLines) {
                doomRpg->doomCanvas->dialogTypeLineIdx = doomRpg->doomCanvas->numDialogLines;
                if (doomRpg->doomCanvas->state == ST_DIALOGPASSWORD && doomRpg->doomCanvas->numDialogLines > 4) {
                    doomRpg->doomCanvas->currentDialogLine = doomRpg->doomCanvas->numDialogLines - 4;
                }
            }
        }

        /* Right Nub (C-Stick) scrolling across dialogs, terminals, and menus */
        circlePosition cstick;
        hidCstickRead(&cstick);
        if (doomRpg->doomCanvas->state == ST_DIALOG || doomRpg->doomCanvas->state == ST_DIALOGPASSWORD) {
            static int s_cstickScrollTime = 0;
            int curTime = DoomRPG_GetUpTimeMS();
            if (curTime > s_cstickScrollTime) {
                if (cstick.dy > 40 || (kDown & KEY_CSTICK_UP)) {
                    if (doomRpg->doomCanvas->currentDialogLine > 0) {
                        doomRpg->doomCanvas->currentDialogLine--;
                        s_cstickScrollTime = curTime + 120;
                    }
                } else if (cstick.dy < -40 || (kDown & KEY_CSTICK_DOWN)) {
                    if (doomRpg->doomCanvas->currentDialogLine + 4 < doomRpg->doomCanvas->numDialogLines) {
                        doomRpg->doomCanvas->currentDialogLine++;
                        s_cstickScrollTime = curTime + 120;
                    }
                }
            }
        } else if (doomRpg->doomCanvas->state == ST_MENU) {
            static int s_cstickMenuScrollTime = 0;
            int curTime = DoomRPG_GetUpTimeMS();
            if (curTime > s_cstickMenuScrollTime) {
                if (cstick.dy > 40 || (kDown & KEY_CSTICK_UP)) {
                    MenuSystem_scrollUp(doomRpg->menuSystem);
                    s_cstickMenuScrollTime = curTime + 120;
                } else if (cstick.dy < -40 || (kDown & KEY_CSTICK_DOWN)) {
                    MenuSystem_scrollDown(doomRpg->menuSystem);
                    s_cstickMenuScrollTime = curTime + 120;
                }
            }
        }
#endif
        int currentTimeMillis = DoomRPG_GetUpTimeMS();
        mouse_Button = MOUSE_BUTTON_INVALID;

        while (SDL_PollEvent(&ev))
        {
            switch (ev.type)
            {
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    Uint32 buttons = SDL_GetMouseState(NULL, NULL);
                    if (buttons & SDL_BUTTON_LMASK)      mouse_Button = MOUSE_BUTTON_LEFT;
                    else if (buttons & SDL_BUTTON_MMASK) mouse_Button = MOUSE_BUTTON_MIDDLE;
                    else if (buttons & SDL_BUTTON_RMASK) mouse_Button = MOUSE_BUTTON_RIGHT;
                    else if (buttons & SDL_BUTTON_X1MASK)mouse_Button = MOUSE_BUTTON_X1;
                    else if (buttons & SDL_BUTTON_X2MASK)mouse_Button = MOUSE_BUTTON_X2;
                } break;

#ifndef __3DS__
                case SDL_MOUSEWHEEL: {
                    if (currentTimeMillis > mouseTime) {
                        mouseTime = currentTimeMillis + 128;
                        if (ev.wheel.y > 0)      mouse_Button = MOUSE_BUTTON_WHELL_UP;
                        else if (ev.wheel.y < 0) mouse_Button = MOUSE_BUTTON_WHELL_DOWN;
                    }
                } break;
#endif

                case SDL_MOUSEMOTION: {
                    if (!doomRpg->menuSystem->setBind && currentTimeMillis > mouseTime) {
                        mouseTime = currentTimeMillis + 128;
                        int x = 0, y = 0;
                        SDL_GetRelativeMouseState(&x, &y);
                        int sensivity = (doomRpg->doomCanvas->mouseSensitivity * 1000) / 100;
                        if (x <= -sensivity) mouse_Button = MOUSE_BUTTON_MOTION_LEFT;
                        else if (x >= sensivity) mouse_Button = MOUSE_BUTTON_MOTION_RIGHT;

                        if (doomRpg->doomCanvas->mouseYMove) {
                            if (y <= -sensivity) mouse_Button = MOUSE_BUTTON_MOTION_UP;
                            else if (y >= sensivity) mouse_Button = MOUSE_BUTTON_MOTION_DOWN;
                        }
                    }
                } break;

#ifndef __3DS__
                case SDL_WINDOWEVENT: {
                    if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
                        // мягко выходим: ставим флаг, цикл сам завершится
                        doomRpg->closeApplet = true;
                    }
                    else {
                        int w, h; SDL_GetWindowSize(sdlVideo.window, &w, &h);
                        SDL_WarpMouseInWindow(sdlVideo.window, w/2, h/2);
                        SDL_GetRelativeMouseState(NULL, NULL);
                    }
                } break;

                case SDL_QUIT: {
                    doomRpg->closeApplet = true; // не exit(0)!
                } break;
#endif
            }

            key = DoomRPG_getEventKey(mouse_Button, state);
            if (key != oldKey) {
                oldKey = key;
                if (!doomRpg->menuSystem->setBind) {
                    DoomCanvas_keyPressed(doomRpg->doomCanvas, key);
                } else {
                    DoomRPG_setBind(doomRpg, mouse_Button, state);
                }
            } else if (key == 0 && doomRpg->menuSystem->setBind) {
                DoomRPG_setBind(doomRpg, mouse_Button, state);
            }
        }
        key = DoomRPG_getEventKey(mouse_Button, state);
        if (key != oldKey) {
            oldKey = key;
            if (!doomRpg->menuSystem->setBind) {
                DoomCanvas_keyPressed(doomRpg->doomCanvas, key);
            } else {
                DoomRPG_setBind(doomRpg, mouse_Button, state);
            }
        } else if (key == 0 && doomRpg->menuSystem->setBind) {
            DoomRPG_setBind(doomRpg, mouse_Button, state);
        }
#ifdef __3DS__
        /* Attack Buffering / Hold-to-Fire: holding A or R continuously executes weapon strike */
        if (g_attackBuffer || g_turboCombat) {
            static int s_lastHoldAttackTime = 0;
            if ((kHeld & (KEY_A | KEY_R)) && !doomRpg->menuSystem->setBind) {
                if (doomRpg->doomCanvas->state == ST_PLAYING &&
                    doomRpg->doomCanvas->animFrameCount == 0 &&
                    currentTimeMillis > s_lastHoldAttackTime + 180) {
                    DoomCanvas_keyPressed(doomRpg->doomCanvas, AVK_SELECT);
                    s_lastHoldAttackTime = currentTimeMillis;
                }
            }
        }
#endif
        if (currentTimeMillis > UpTime) {
            UpTime = currentTimeMillis + 15;
            DoomRPG_loopGame(doomRpg);
        }
    }
#ifdef MIX_MAJOR_VERSION
    Mix_HaltChannel(-1);
    Mix_CloseAudio();
    Mix_Quit();
#endif
    // closeZipFile(&zipFile);
    //DoomRPG_FreeAppData(doomRpg);
#ifdef __3DS__
    SDL_Close();
#else
    SDL_Quit();
#endif

    return 0;
}
