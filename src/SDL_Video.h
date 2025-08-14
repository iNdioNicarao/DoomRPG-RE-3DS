#ifndef SDL_VIDEO_H__
#define SDL_VIDEO_H__
#include "Render.h"
typedef struct SDLVideo_s
{
#ifdef __3DS__
	SDL_Surface* screenSurface;
	SDL_Surface* backbuffer;
	int screenW;
	int screenH;
#else
	SDL_Window* window;
	SDL_Renderer* renderer;
	int rendererW;
	int rendererH;
	boolean fullScreen;
	boolean vSync;
	boolean integerScaling;
	boolean displaySoftKeys;
	int resolutionIndex;
#endif
} SDLVideo_t;


extern SDLVideo_t sdlVideo;

void SDL_InitVideo(void);
void SDL_Close(void);
SDLVideo_t* SDL_GetVideo(void);
void SDL_RenderDrawFillCircle(RenderTarget* target, int x, int y, int r);
void SDL_RenderDrawCircle(RenderTarget* target, int x, int y, int r);

//-----

#ifdef __3DS__
#elif __aarch64__
#else
#include <fluidsynth.h>
typedef struct FluidSynth_s
{
	fluid_settings_t* settings;
	fluid_synth_t* synth;
	fluid_audio_driver_t* adriver;
} FluidSynth_t;

extern FluidSynth_t fluidSynth;
#endif

void SDL_InitAudio(void);
void SDL_CloseAudio(void);

//-----
typedef struct SDLController_s
{
	//Game controller handler with force feedback
#ifdef __3DS__
#else
	SDL_GameController* gGameController;
#endif

	//Joystick handler with haptic
	SDL_Joystick* gJoystick;
	#ifdef __3DS__
#else
	SDL_Haptic* gJoyHaptic;
#endif
	int deadZoneLeft;
	int deadZoneRight;
} SDLController_t;

extern SDLController_t sdlController;

typedef struct SDLVidModes_s
{
	int width, height;
} SDLVidModes_t;

extern SDLVidModes_t sdlVideoModes[14];

//Analog joystick dead zone
#define JOYSTICK_DEAD_ZONE 8000
int SDL_GameControllerGetButtonID(void);
char* SDL_GameControllerGetNameButton(int id);

char* SDL_MouseGetNameButton(int id);

int SDL_JoystickGetButtonID(void);
void SDL_RenderSetClipRect(SDL_Surface *surface, const SDL_Rect *rect);
void SDL_SetRenderDrawColor(SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void SDL_RenderPresent(SDL_Surface *surface);
void SDL_RenderClear(SDL_Surface *surface);
void SDL_RenderDrawRect(SDL_Surface *surface, const SDL_Rect *rect);
void SDL_RenderFillRect(SDL_Surface *surface, const SDL_Rect *rect);
static void put_pixel_unlocked(SDL_Surface* surface, int x, int y, Uint32 color);
void SDL_RenderDrawLine(SDL_Surface *surface, int x1, int y1, int x2, int y2);

#endif
