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
#include "Hud.h"
#include "MenuSystem.h"
#include "SDL_Video.h"
#include "Z_Zip.h"

extern DoomRPG_t* doomRpg;

int main(int argc, char* args[])
{
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
    SDL_Quit();

    return 0;
}
