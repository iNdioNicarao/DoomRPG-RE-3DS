

//Using SDL and standard IO

#ifdef __3DS__
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <3ds.h>
#include <3ds/console.h>
#include <citro3d.h>
#include <citro2d.h>
//#include <SDL/SDL_opengl.h>
#include <SDL/SDL_audio.h>
#include <stdio.h>
#else
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <fluidsynth.h>
#endif

#include "DoomRPG.h"
#include "Game.h"
#include "SDL_Video.h"

/* Global instances (declared extern in SDL_Video.h). */
#ifdef __3DS__
#define MARK(s) ((void)0)
#endif
SDLVideo_t sdlVideo;
SDLController_t sdlController;

#ifdef __3DS__
/* Raw gfx framebuffers for BOTH screens (we own gfx directly; SDL video does
   NOT claim any screen). Top = stereoscopic (both eyes); bottom = raw. */
static u8* g_topFbL = NULL;   /* GFX_TOP/GFX_LEFT  (left eye)  */
static u8* g_topFbR = NULL;   /* GFX_TOP/GFX_RIGHT (right eye) */
static u8* g_botFb  = NULL;   /* GFX_BOTTOM/GFX_LEFT */
static u8* g_botTmp = NULL;   /* off-screen scratch for the bottom; present = render here, then memcpy once */
static u32 g_topW = 240, g_topH = 400;  /* real top-eye stride (from gfx) */
static u32 g_botW = 240, g_botH = 320;  /* real bottom framebuffer size (portrait 240x320) */
static volatile int g_gfx_suspended = 0;  /* set by APT hook; skip present (no gfx flush) during HOME */
static aptHookCookie g_apt_cookie;

/* STEREO 3D (real hardware auto-stereoscopic, top screen).
   The top framebuffer is allocated 240x800 (GSP_SCREEN_HEIGHT_TOP_2X) by gfxInitDefault.
   In MODE_2D (gfxSet3D(false)) GFX_LEFT == GFX_RIGHT pointer and only the first 400 rows
   are shown. In MODE_3D (gfxSet3D(true)) GFX_LEFT = rows 0..399, GFX_RIGHT = rows 400..799,
   each a full 240x400 portrait eye; the parallax barrier shows the two as one 3D image.
   We render the scene twice (angles viewAngle +/- sep) into two 400x240 capture buffers
   (g_topEyeL/R), then transpose each into its eye half of the top framebuffer.
   gfxSet3D is toggled ONLY on the main thread (never in the APT hook) to stay crash-free. */
static Uint32* g_topEyeL = NULL;  /* 400x240 scene capture, left eye  */
static Uint32* g_topEyeR = NULL;  /* 400x240 scene capture, right eye */
SDL_Surface* g_stereoRight = NULL;  /* 400x240 RGBA32 right-eye capture (legacy, unused now) */
int g_stereoRightValid = 0;
int g_top3D = 0;          /* 1 => stereo enabled this frame (slider > 0) */
float g_stereoSep = 0.0f;

/* citro2d top-screen present (suspend-safe stereo path). The TOP screen is handed to citro2d
   render targets; citro3d owns the present + suspend lifecycle (devkitPro stereoscopic_2d example
   survives aptMainLoop+HOME). BOTTOM stays raw-gfx (separate screen, no conflict). */
static C3D_Tex      g_topTex;        /* 512x256 RGB565 texture (Left eye) */
static C3D_Tex      g_topTexR;       /* 512x256 RGB565 texture (Right eye) */
static C2D_Image    g_topImg;        /* citro2d image wrapper around g_topTex */
static C2D_Image    g_topImgR;       /* citro2d image wrapper around g_topTexR */
static C3D_RenderTarget* g_topTargetL = NULL;  /* GFX_TOP LEFT eye */
static C3D_RenderTarget* g_topTargetR = NULL;  /* GFX_TOP RIGHT eye */
u16*         g_topScratch = NULL;    /* 512x256 RGB565 top region (Left eye) */
u16*         g_topScratchR = NULL;   /* 512x256 RGB565 top region (Right eye) */
Tex3DS_SubTexture g_topSub;    /* subtexture region (400x240 within 512x256 tex) */
bool         g_topCitroInited = false;
int          g_topProbed = 0;        /* one-shot measurement probe fired (in Main loop, not present) */
volatile int g_topProbePending = 0;  /* set by present (safe: plain int write); drained in Main loop */

/* Flag-only APT hook: do NOT call any gfx function here (GPU state is invalid
   in the hook context and faults). Just record suspend so the present loop
   skips gfxFlushBuffers() while the applet owns the screen -- this stops our
   flush from racing libctru's GSP event thread (dump 98..105: gspEventThreadMain,
   FAR 0xf4). This is the exact setup that made build 711500 graceful on HOME. */
static void stereo_apt_gfx_reacquire(void) {
    /* On resume the OS hands back a DIFFERENT framebuffer address than the one
       cached at init (standard 3DS OS behavior). Re-fetch so we write live memory
       instead of a dead buffer (which would show stale garbage = the bar). */
    u16 tw=0, th=0, bw=0, bh=0;
    g_topFbL = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
    g_topFbR = g_topFbL;  /* gfxSet3D(false): right eye unused; alias LEFT so we never
                             hand the GSP display-transfer thread a bogus RIGHT pointer
                             (that triggers FAR 0xf4 Data Abort at HOME suspend, dump 122).
                             ctrWolfen (crash-free) writes LEFT only. */
    g_botFb  = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
    g_topW = tw ? tw : 240;  g_topH = th ? th : 400;
    g_botW = bw ? bw : 240;  g_botH = bh ? bh : 320;
}

static void stereo_apt_hook(APT_HookType hook, void* param) {
    (void)param;
    /* SAFE hook (71218-style): do NOT call any gfx function here. The 71221/71222 gspWaitForVBlank()
       in this hook faulted at suspend (dump 130, FAR 0xf4). Just flag suspend so the present loop
       force-flats before aptMainLoop blocks. */
    if (hook == APTHOOK_ONSUSPEND) g_gfx_suspended = 1;
    else if (hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP) {
        g_gfx_suspended = 0;
        stereo_apt_gfx_reacquire();
    } else if (hook == APTHOOK_ONEXIT) {
        g_gfx_suspended = 1;
    }
}

#endif /* __3DS__ */

void SDL_InitVideo(void) {
#ifdef __3DS__
	putenv("SDL_N3DS_CONSOLE=");
	SDL_memset(&sdlVideo, 0, sizeof(sdlVideo));
	/* NO SDL_INIT_VIDEO. We OWN gfx raw via gfxInit() below. SDL_Init with VIDEO
	   calls gfxInitDefault() internally and claims the screens (SDL_DUALSCR
	   blue-collision) -- that is what we must NOT do. SDL is input/audio only. */
	gfxInitDefault();           /* default top=BGR8, bottom=BGR8; libctru owns the GSP/suspend lifecycle (ctrWolfen method) */
	gfxSetDoubleBuffering(GFX_TOP, true);    /* TOP is citro2d-owned -> must be double-buffered so C3D_FrameEnd swaps the
	                                           stereo eye buffers to the display (the stereoscopic_2d reference leaves this
	                                           at default ON; forcing false left the GPU target undisplayed = black top). */
	gfxSetDoubleBuffering(GFX_BOTTOM, false); /* bottom stays raw-gfx single-buffer (its flush is direct, works). */
	gfxSet3D(true);              /* stereo ON at the gfx layer; citro2d render targets present both
	                            eyes and own the suspend-safe stereo transfer (dumps 122-134 were
	                            caused by raw gfxSet3D + gfxSwapBuffers 800-tall transfers). */
	/* citro2d top-screen present: software scene -> RGB565 texture -> GPU dual-eye targets.
	   This is the devkitPro stereoscopic_2d pattern and is suspend-safe. */
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	g_topTargetL = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	g_topTargetR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	/* 400x240 RGB565 texture padded to POT 512x256 (tex3ds requires POT). */
	C3D_TexInit(&g_topTex, 512, 256, GPU_RGB565);
	C3D_TexSetFilter(&g_topTex, GPU_NEAREST, GPU_NEAREST);
	C3D_TexSetWrap(&g_topTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
	C3D_TexInit(&g_topTexR, 512, 256, GPU_RGB565);
	C3D_TexSetFilter(&g_topTexR, GPU_NEAREST, GPU_NEAREST);
	C3D_TexSetWrap(&g_topTexR, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
	g_topScratch = (u16*)linearAlloc(512 * 256 * 2);
	memset(g_topScratch, 0, 512 * 256 * 2);
	g_topScratchR = (u16*)linearAlloc(512 * 256 * 2);
	memset(g_topScratchR, 0, 512 * 256 * 2);
	/* PICA200 texture coordinates: V=1.0 is row 0 (top of texture), V=0.0 is row 255.
	   Rows 0..239 contain our active image, rows 240..255 are 16px of POT padding.
	   Subtexture top is 1.0f (row 0), bottom is (256-240)/256 = 16/256 = 0.0625f (row 239). */
	g_topSub = (Tex3DS_SubTexture){ 400, 240, 0.0f, 1.0f, 400.0f/512.0f, (256.0f - 240.0f)/256.0f };
	g_topImg.tex = &g_topTex; g_topImg.subtex = &g_topSub;
	g_topImgR.tex = &g_topTexR; g_topImgR.subtex = &g_topSub;
	g_topCitroInited = (g_topTargetL && g_topTargetR && g_topScratch && g_topScratchR);
	/* Stereo eye-capture buffers: 400x240 scene per eye (matches the 3D view region). */
	g_topEyeL = (Uint32*)SDL_calloc(1, (size_t)400 * 240 * sizeof(Uint32));
	g_topEyeR = (Uint32*)SDL_calloc(1, (size_t)400 * 240 * sizeof(Uint32));
	g_stereoRight = SDL_CreateRGBSurface(SDL_SWSURFACE, 400, 240, 16,
		0xF800, 0x07E0, 0x001F, 0);  /* RGB565, matches piDIB 1:1 (blit = memcpy, no convert) */
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		DoomRPG_Error("Could not initialize SDL: %s", SDL_GetError());
	}
	/* Software offscreen draw surface ONLY (400x480). SDL does NOT claim any
	   hardware screen (no SDL_DUALSCR / no SDL_SetVideoMode) -- we own gfx raw.
	   This is the 705472 config that rendered the top NORMALLY. A separate
	   surface avoids the 768KB double-alloc vs SDL_SetVideoMode; RAM stays at
	   the single-alloc level. The game composites 3D view + HUD + automap here. */
	sdlVideo.screenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 400, 480, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!sdlVideo.screenSurface) {
		DoomRPG_Error("Could not create video surface: %s", SDL_GetError());
	}
	sdlVideo.screenW = sdlVideo.screenSurface->w;
	sdlVideo.screenH = sdlVideo.screenSurface->h;
		/* Grab raw framebuffers (RGB565, pitch = 240*2 for top eyes / 240*2 bottom). */
		{ u16 tw=0, th=0, bw=0, bh=0;
		  g_topFbL = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
		  g_topFbR = g_topFbL;  /* alias LEFT; 3D off => right eye unused (see reacquire note) */
		  g_botFb  = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
	  g_topW = tw ? tw : 240;  g_topH = th ? th : 400;
	  g_botW = bw ? bw : 240;  g_botH = bh ? bh : 320;
	  }
	  if (!g_topFbL || !g_topFbR || !g_botFb) DoomRPG_Error("Could not get framebuffers");
	  /* Off-screen scratch for the bottom blit: we render the rotated map into g_botTmp,
	     then memcpy it to the live framebuffer in ONE contiguous copy. This keeps the
	     GSP-happy "full contiguous buffer write" each frame (no memcpy+transpose-dotted
	     gaps) while never showing a half-black intermediate frame -> no CRT flash. */
	  if (!g_botTmp) g_botTmp = (u8*)malloc((size_t)g_botW * g_botH * 3);
	  if (g_botFb) SDL_memset(g_botFb, 0, (size_t)g_botW * g_botH * 3);
	  if (g_botTmp) SDL_memset(g_botTmp, 0, (size_t)g_botW * g_botH * 3);
	  printf("3DS gfx video: top=raw stereo BGR8, bottom=raw BGR8\n");
	  printf("3DS video initialized: %dx%d\n", sdlVideo.screenW, sdlVideo.screenH);
	  aptHook(&g_apt_cookie, stereo_apt_hook, NULL);
	  #else
	Uint32 flags;
	int video_w, video_h;

	SDL_memset(&sdlVideo, 0, sizeof(sdlVideo));
	SDL_memset(&sdlController, 0, sizeof(sdlController));

	// Default
	sdlVideo.fullScreen = false;
	sdlVideo.vSync = false;
	sdlVideo.integerScaling = true;
	sdlVideo.resolutionIndex = 8;
	sdlVideo.displaySoftKeys = true;
	Game_loadConfig(NULL);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        DoomRPG_Error("Could not initialize SDL: %s", SDL_GetError());
    }

    flags = SDL_WINDOW_OPENGL| SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    video_w = sdlVideoModes[sdlVideo.resolutionIndex].width;
    video_h = sdlVideoModes[sdlVideo.resolutionIndex].height;

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_ShowCursor(SDL_DISABLE);

	if (sdlVideo.fullScreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	// Set the highdpi flags - this makes a big difference on Macs with
	// retina displays, especially when using small window sizes.
	flags |= SDL_WINDOW_ALLOW_HIGHDPI;

	sdlVideo.window = SDL_CreateWindow("DoomRPG", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, video_w, video_h, flags);

    if (!sdlVideo.window) {
		DoomRPG_Error("Could not set %dx%d video mode: %s", video_w, video_h, SDL_GetError());
    }

	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, sdlVideo.vSync ? "1" : "0");

	sdlVideo.renderer = SDL_CreateRenderer(sdlVideo.window, -1, SDL_RENDERER_ACCELERATED);

	sdlVideo.rendererW = sdlVideoModes[sdlVideo.resolutionIndex].width;
	sdlVideo.rendererH = sdlVideoModes[sdlVideo.resolutionIndex].height;

    // Since we are going to display a low resolution buffer,
    // it is best to limit the window size so that it cannot
    // be smaller than our internal buffer size.
    SDL_SetWindowMinimumSize(sdlVideo.window, sdlVideo.rendererW, sdlVideo.rendererH);
    SDL_RenderSetLogicalSize(sdlVideo.renderer, sdlVideo.rendererW, sdlVideo.rendererH);
    SDL_RenderSetIntegerScale(sdlVideo.renderer, sdlVideo.integerScaling);

	// Check for joysticks
	SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");

	sdlController.gGameController = NULL;
	sdlController.gJoystick = NULL;
	sdlController.gJoyHaptic = NULL;
	sdlController.deadZoneLeft = 25;
	sdlController.deadZoneRight = 25;

	if (SDL_NumJoysticks() < 1) {
		printf("Warning: No joysticks connected!\n");
	}
	else {
		printf("Joysticks connected: %d\n", SDL_NumJoysticks());

		// Open game controller and check if it supports rumble
		sdlController.gGameController = SDL_GameControllerOpen(0);
		if (sdlController.gGameController) {

			// Check if joystick supports Rumble
			if (!SDL_GameControllerHasRumble(sdlController.gGameController)) {
				printf("Warning: Game controller does not have rumble! SDL Error: %s\n", SDL_GetError());
			}
		}

		// Load joystick if game controller could not be loaded
		if (sdlController.gGameController == NULL) {
			// Open first joystick
			sdlController.gJoystick = SDL_JoystickOpen(0);
			if (sdlController.gJoystick == NULL) {
				printf("Warning: Unable to open joystick! SDL Error: %s\n", SDL_GetError());
			}
			else
			{
				// Check if joystick supports haptic
				if (!SDL_JoystickIsHaptic(sdlController.gJoystick)) {
					printf("Warning: Controller does not support haptics! SDL Error: %s\n", SDL_GetError());
				}
				else
				{
					// Get joystick haptic device
					sdlController.gJoyHaptic = SDL_HapticOpenFromJoystick(sdlController.gJoystick);
					if (sdlController.gJoyHaptic == NULL) {
						printf("Warning: Unable to get joystick haptics! SDL Error: %s\n", SDL_GetError());
					}
					else
					{
						// Initialize rumble
						if (SDL_HapticRumbleInit(sdlController.gJoyHaptic) < 0) {
							printf("Warning: Unable to initialize haptic rumble! SDL Error: %s\n", SDL_GetError());
						}
					}
				}
			}
		}
	}
#endif
}
void SDL_Close(void)
{

#ifdef __3DS__

	if (sdlController.gJoystick) {
		SDL_JoystickClose(sdlController.gJoystick);
		sdlController.gJoystick = NULL;
	}
#else
	if (sdlController.gGameController) {
		SDL_GameControllerClose(sdlController.gGameController);
	}

	if (sdlController.gJoyHaptic) {
		SDL_HapticClose(sdlController.gJoyHaptic);
	}

	if (sdlController.gJoystick) {
		SDL_JoystickClose(sdlController.gJoystick);
	}

	if (sdlVideo.window) {
		SDL_SetWindowFullscreen(sdlVideo.window, 0);
	}

	if (sdlVideo.renderer) {
		SDL_DestroyRenderer(sdlVideo.renderer);
	}

	if (sdlVideo.window) {
		SDL_DestroyWindow(sdlVideo.window);
	}
#endif

    MARK("Q0 QUIT_ENTRY\n");
#ifdef __3DS__
    MARK("C0 CLEANUP_START\n");
    /* We own gfx raw (no SDL_INIT_VIDEO), so SDL_Quit() will NOT tear down
       libctru's GSP/framebuffers for us. We MUST call C2D_Fini(), C3D_Fini(),
       and gfxExit() explicitly so the GPU and GSP event thread (gspEventThreadMain)
       are stopped before process memory is unmapped on exit (dumps 98..144). */
    g_topFbL = NULL; g_topFbR = NULL; g_botFb = NULL;
    if (g_topScratch) {
        linearFree(g_topScratch);
        g_topScratch = NULL;
    }
    if (g_topScratchR) {
        linearFree(g_topScratchR);
        g_topScratchR = NULL;
    }
    if (g_topEyeL) {
        SDL_free(g_topEyeL);
        g_topEyeL = NULL;
    }
    if (g_topEyeR) {
        SDL_free(g_topEyeR);
        g_topEyeR = NULL;
    }
    if (g_stereoRight) {
        SDL_FreeSurface(g_stereoRight);
        g_stereoRight = NULL;
    }
    if (g_botTmp) {
        SDL_free(g_botTmp);
        g_botTmp = NULL;
    }
    if (g_topCitroInited) {
        C2D_Fini();
        C3D_Fini();
        g_topCitroInited = false;
    }
    aptUnhook(&g_apt_cookie);
    gfxExit();
    MARK("C1 CLEANUP_DONE\n");
#endif
    //Quit SDL subsystems
    SDL_Quit();
}

SDLVideo_t* SDL_GetVideo(void)
{
	return &sdlVideo;
}
void SDL_RenderSetClipRect(SDL_Surface *surface, const SDL_Rect *rect)
{
    if (!surface) return;
    SDL_SetClipRect(surface, rect);
}
static int curColor = 0x0;
void SDL_SetRenderDrawColor(SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint32 color = SDL_MapRGB(surface->format, r, g, b);
    curColor = (uintptr_t)color;
}

/* PHASE 1: blit the offscreen 400x480 RGBA32 surface into the gfx
   top/bottom framebuffers. The 3DS gfx framebuffer is GSP_RGB565_OES: 16-bit,
   ONE u16 PER PIXEL, stride = width*2.
   gfx TOP is 240x400, BOTTOM is 240x320.
   The 3DS LCD rotates the buffer 90deg CCW on display, so our landscape
   offscreen must rotate into the portrait buffer. Standard 3DS mapping:
       fx = 239 - y   (logical y -> buffer x)
       fy = x         (logical x -> buffer y)
       idx = fx + fy * g_w */
/* Top/bottom framebuffers are GSP_BGR8_OES (3 bytes/px), allocated by
   SDL_Init(VIDEO)->gfxInitDefault(). GL_BGR byte order: byte0=B,byte1=G,byte2=R.
   Pack RGBA32 (R=0x00FF0000,G=0x0000FF00,B=0x000000FF) into that order. */
static inline Uint32 rgba32_to_bgr8(Uint32 px) {
	int r = (px >> 16) & 0xFF;
	int g = (px >> 8)  & 0xFF;
	int b = (px >> 0)  & 0xFF;
	return (Uint32)(((Uint32)b) | ((Uint32)g << 8) | ((Uint32)r << 16));
}
/* g_stereoRight holds RGB565 (matches piDIB 1:1). Convert RGB565 -> BGR8 for the top fb. */
static inline void put_bgr8_from565(u8* base, Uint16 px) {
	int r = (px >> 11) & 0x1F, g = (px >> 5) & 0x3F, b = px & 0x1F;
	r = (r << 3) | (r >> 2); g = (g << 2) | (g >> 4); b = (b << 3) | (b >> 2);
	base[0] = (u8)b; base[1] = (u8)g; base[2] = (u8)r;
}
static inline void put_bgr8(u8* base, Uint32 v) {
	base[0] = (u8)(v & 0xFF);
	base[1] = (u8)((v >> 8) & 0xFF);
	base[2] = (u8)((v >> 16) & 0xFF);
}

static inline u16 rgba32_to_rgb565(Uint32 px) {
    /* SDL surface mask 0x00FF0000/0x0000FF00/0x000000FF/0xFF000000 :
       little-endian => R at byte2, G byte1, B byte0. Swap so RGB565
       gets true R/G/B (previously R/B swapped -> blue/magenta). */
    int r = (px >> 16) & 0xFF;
    int g = (px >> 8)  & 0xFF;
    int b = (px >> 0)  & 0xFF;
    return (u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
/* BUILD 712029: STEREO via citro2d (suspend-safe). Top screen handed to citro2d dual-eye
   render targets (devkitPro stereoscopic_2d pattern); the software scene is rotated 90deg into a
   240x400 RGB565 texture and drawn to GFX_TOP LEFT/RIGHT. citro3d owns the present + suspend
   lifecycle, so HOME no longer faults (dumps 122-134 were raw-gfx 800-tall transfers). BOTTOM
   stays raw-gfx (separate screen). 3D gated on the real slider (hidScanInput refreshes HID mem,
   which is why the earlier probe read 0.0). HUD/weapon/status bar live in the top region -> both
   eyes get them (no disorienting half-3D). Parallax = per-eye horizontal offset (slider * sep). */
static void SDL_PresentGfx(SDL_Surface* surface) {
#ifdef __3DS__
    (void)surface;
    if (g_gfx_suspended) return;  /* HOME menu owns the GPU; skip present+flush */
    if (!g_topFbL || !g_topFbR || !g_botFb || !sdlVideo.screenSurface) return;
    const Uint32* src = (const Uint32*)sdlVideo.screenSurface->pixels;  /* 400x480 RGBA32 */

    /* Slider gate (main thread): enable 3D when the slider is pushed.
       Refreshed by hidScanInput() called once per frame in Main.c. */
    {
        float s = osGet3DSliderState();  /* 0.0 off .. 1.0 full */
        if (s > 0.0f) { g_top3D = 1; g_stereoSep = s; }
        else          { g_top3D = 0; g_stereoSep = 0.0f; }
    }

    /* Re-fetch the live framebuffers EVERY frame. The 3DS display buffer can
       differ from the one cached at init / after resume, so writing a cached
       pointer paints a buffer the display is NOT showing -> stale bar + flicker.
       Re-fetching guarantees we always write the currently-displayed buffer.
       Double-buffered top (citro2d); single-buffered bottom (raw-gfx). */
    {
        u16 tw = 0, th = 0, bw = 0, bh = 0;
        g_topFbL = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
        g_topW = tw ? tw : 240;  g_topH = th ? th : 400;
        g_botFb = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
        g_botW = bw ? bw : 240;  g_botH = bh ? bh : 320;
    }
    (void)g_topFbR;

    /* BOTTOM: 240x320 portrait framebuffer. The working SDL config (devkitPro
       3DS driver, SDL_DUALSCR 400x480) maps the surface LOWER HALF (rows 240..479)
       onto the bottom screen UPRIGHT (NO rotation). The LCD optically rotates the
       portrait buffer 90deg, so an upright landscape must be written rotated 90deg
       CW into the buffer. 3DS portrait rotation mapping:
           buffer_col = g_botW-1 - (src_row - 240)   (src_row 240..479 -> col 239..0)
           buffer_row = src_col * g_botH / 400         (src_col 0..399 -> row 0..319)
       The original bar was killed by clearing SOURCE rows 240..359 (the blank strip
       above the automap tiles, which start at y>=363) to opaque black EVERY frame
       -- the automap bg clear (SDL_FillRect) is a no-op on this surface, so without
       this the stale/garbage strip showed through. We replicate that here. */
    /* Blit the FULL lower half (rows 240..479) into g_botTmp, rotated 90deg so it
       appears UPRIGHT on screen. */
    {
        u8* dst = g_botTmp ? g_botTmp : g_botFb;
        for (int sy = 240; sy < 480; sy++) {
            int dx = g_botW - 1 - (sy - 240);
            const Uint32* row = src + sy * 400;
            for (int sx = 0; sx < 400; sx++) {
                int dy = (sx * g_botH) / 400;
                if (dy < 0) dy = 0; else if (dy >= g_botH) dy = g_botH - 1;
                int bi = (dy * g_botW + dx) * 3;
                put_bgr8(&dst[bi], rgba32_to_bgr8(row[sx]));
            }
        }
    }
    if (g_botTmp && g_botFb && !g_gfx_suspended) SDL_memcpy(g_botFb, g_botTmp, (size_t)g_botW * g_botH * 3);

    /* Clear bottom-screen rows 240..479 to opaque black for NEXT frame so stale pixels never linger */
    {
        Uint32* sp = (Uint32*)sdlVideo.screenSurface->pixels;
        for (int sy = 240; sy < 480; sy++)
            for (int sx = 0; sx < 400; sx++)
                sp[sy * 400 + sx] = 0xFF000000u;  /* A=255, RGB=0 */
    }

    /* TOP screen: hand to citro2d. Copy the top region (screenSurface rows 0..239, 400x240)
       directly into 512x256 RGB565 scratch texture, upload, and draw to BOTH stereo eye targets.
       citro2d's scene target automatically applies the 3DS screen rotation tilt via Mtx_OrthoTilt,
       so no manual software rotation is needed. citro3d owns the dual-eye present and suspend lifecycle. */
    if (g_topCitroInited && !g_gfx_suspended) {
        /* PICA200 GPU textures require 8x8 Morton (Z-order) tiled pixel data.
           We tile the 400x240 RGB565 source into 512x256 POT texture memory (rows 0..239).
           g_topSub has top=1.0f (row 0) and bottom=(256-240)/256=16/256 (row 239).
           In GPU texture space, mapping source row sy = by + py displays the
           entire 400x240 scene right-side up with status bar at top (y=0..20)
           and HUD at bottom (y=192..240) spanning the entire 240-height screen with no vertical offset. */
        static const u8 s_morton8x8[64] = {
             0,  1,  4,  5, 16, 17, 20, 21,
             2,  3,  6,  7, 18, 19, 22, 23,
             8,  9, 12, 13, 24, 25, 28, 29,
            10, 11, 14, 15, 26, 27, 30, 31,
            32, 33, 36, 37, 48, 49, 52, 53,
            34, 35, 38, 39, 50, 51, 54, 55,
            40, 41, 44, 45, 56, 57, 60, 61,
            42, 43, 46, 47, 58, 59, 62, 63
        };
        u16* dst = g_topScratch;
        for (int by = 0; by < 240; by += 8) {
            int ty = by / 8;
            for (int bx = 0; bx < 400; bx += 8) {
                int tx = bx / 8;
                u16* tileDst = dst + (ty * (512 / 8) + tx) * 64;
                for (int py = 0; py < 8; py++) {
                    int sy = by + py;
                    const Uint32* rowSrc = src + sy * 400 + bx;
                    const u8* mRow = &s_morton8x8[py * 8];
                    tileDst[mRow[0]] = rgba32_to_rgb565(rowSrc[0]);
                    tileDst[mRow[1]] = rgba32_to_rgb565(rowSrc[1]);
                    tileDst[mRow[2]] = rgba32_to_rgb565(rowSrc[2]);
                    tileDst[mRow[3]] = rgba32_to_rgb565(rowSrc[3]);
                    tileDst[mRow[4]] = rgba32_to_rgb565(rowSrc[4]);
                    tileDst[mRow[5]] = rgba32_to_rgb565(rowSrc[5]);
                    tileDst[mRow[6]] = rgba32_to_rgb565(rowSrc[6]);
                    tileDst[mRow[7]] = rgba32_to_rgb565(rowSrc[7]);
                }
            }
        }
        C3D_TexUpload(&g_topTex, g_topScratch);

        /* If 3D slider is active and Right Eye scene was rendered, construct Right Eye texture */
        if (g_top3D && g_stereoRightValid && g_stereoRight && g_topScratchR) {
            /* Copy status bar (0..19) and HUD (212..239) from Left Eye */
            memcpy(g_topScratchR, g_topScratch, 512 * 256 * 2);
            /* Overlay Right Eye 3D scene (rows 20..211) from g_stereoRight (RGB565) */
            const u16* rSrc = (const u16*)g_stereoRight->pixels;
            for (int by = 16; by < 216; by += 8) {
                int ty = by / 8;
                for (int bx = 0; bx < 400; bx += 8) {
                    int tx = bx / 8;
                    u16* tileDst = g_topScratchR + (ty * (512 / 8) + tx) * 64;
                    for (int py = 0; py < 8; py++) {
                        int sy = by + py;
                        if (sy < 20 || sy >= 212) continue;
                        const u16* rRow = rSrc + sy * 400 + bx;
                        const u8* mRow = &s_morton8x8[py * 8];
                        tileDst[mRow[0]] = rRow[0];
                        tileDst[mRow[1]] = rRow[1];
                        tileDst[mRow[2]] = rRow[2];
                        tileDst[mRow[3]] = rRow[3];
                        tileDst[mRow[4]] = rRow[4];
                        tileDst[mRow[5]] = rRow[5];
                        tileDst[mRow[6]] = rRow[6];
                        tileDst[mRow[7]] = rRow[7];
                    }
                }
            }
            C3D_TexUpload(&g_topTexR, g_topScratchR);
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            C2D_TargetClear(g_topTargetL, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(g_topTargetL);
            C2D_DrawImageAt(g_topImg, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);

            C2D_TargetClear(g_topTargetR, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(g_topTargetR);
            if (g_top3D && g_stereoRightValid) {
                C2D_DrawImageAt(g_topImgR, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
            } else {
                C2D_DrawImageAt(g_topImg, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
            }
        }
        C3D_FrameEnd(0);
        /* Consume stereo valid flag: if 3D scene was not rendered this frame
           (e.g. while in ST_MENU, ST_AUTOMAP, etc.), right eye falls back to 2D */
        g_stereoRightValid = 0;
    }
#endif
}

void SDL_RenderPresent(SDL_Surface *surface)
{
    SDL_PresentGfx(surface);
}

void SDL_RenderClear(SDL_Surface *surface)
{
    /* The game calls SDL_RenderClear(screenSurface) every frame expecting a full
       clear. SDL_FillRect is a no-op on this software surface, so the bottom region
       (and the automap interior) kept stale pixels -> the "bar". We clear ONLY the
       real screenSurface here, sized to its exact buffer (400x480x4) so it cannot
       overrun. Other surfaces (menu, etc.) keep the no-op FillRect -- an earlier
       unconditional memset overran THOSE surfaces' allocations and corrupted the heap
       (HOME crash, dumps 102/110). Guarding to screenSurface avoids that. */
    if (surface && surface == sdlVideo.screenSurface && surface->pixels) {
        int bpp = surface->format ? surface->format->BytesPerPixel : 4;
        SDL_memset(surface->pixels, 0, (size_t)surface->w * surface->h * bpp);
        return;
    }
    Uint32 color = (Uint32)(uintptr_t)curColor;
    SDL_FillRect(surface, NULL, color);
}

void SDL_RenderDrawRect(SDL_Surface *surface, const SDL_Rect *rect)
{
    if (!rect) return;
    SDL_Rect top = { rect->x, rect->y, rect->w, 1 };
    SDL_Rect bottom = { rect->x, rect->y + rect->h - 1, rect->w, 1 };
    SDL_Rect left = { rect->x, rect->y, 1, rect->h };
    SDL_Rect right = { rect->x + rect->w - 1, rect->y, 1, rect->h };

    Uint32 color = (Uint32)(uintptr_t)curColor;
    SDL_FillRect(surface, &top, color);
    SDL_FillRect(surface, &bottom, color);
    SDL_FillRect(surface, &left, color);
    SDL_FillRect(surface, &right, color);
}

void SDL_RenderFillRect(SDL_Surface *surface, const SDL_Rect *rect)
{
    Uint32 color = (Uint32)(uintptr_t)curColor;
    SDL_FillRect(surface, rect, color);
}
static void put_pixel_unlocked(SDL_Surface* surface, int x, int y, Uint32 color)
{
	if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) return;
	Uint32* pixels = (Uint32*)surface->pixels;
	pixels[y * surface->w + x] = color;
}
void put_pixel_safe(SDL_Surface *surface, int x, int y, Uint32 color)
{
	if (x >= 0 && x < 400 && y >= 0 && y < 480) {
		if (surface && surface->pixels) {
			Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
			*(Uint32 *)p = color;
		}
	}
}
void SDL_RenderDrawLine(SDL_Surface *surface, int x1, int y1, int x2, int y2)
{
	if (surface != NULL) {
		int dx = abs(x2 - x1);
		int dy = abs(y2 - y1);
		int sx = (x1 < x2) ? 1 : -1;
		int sy = (y1 < y2) ? 1 : -1;
		int err = dx - dy;
		int e2;

		Uint32 color = (Uint32)(uintptr_t)curColor;

		if (SDL_MUSTLOCK(surface)) {
			if (SDL_LockSurface(surface) < 0) {
				return;
			}
		}

		while (1) {
			put_pixel_safe(surface, x1, y1, color);

			if (x1 == x2 && y1 == y2) {
				break;
			}

			e2 = 2 * err;

			if (e2 > -dy) {
				err -= dy;
				x1 += sx;
			}

			if (e2 < dx) {
				err += dx;
				y1 += sy;
			}
		}

		if (SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}
void SDL_RenderDrawFillCircle(RenderTarget* target, int x, int y, int r)
{
	int dx, dy, accum;

	dx = r;
	dy = 0;
	accum = dx - (dy << 1) - 1;

	while (dy <= dx)
	{
		SDL_RenderDrawLine(target, dx + x, dy + y, -dx + x, dy + y);
		SDL_RenderDrawLine(target, dy + x, dx + y, -dy + x, dx + y);
		SDL_RenderDrawLine(target, -dx + x, -dy + y, dx + x, -dy + y);
		SDL_RenderDrawLine(target, -dy + x, -dx + y, dy + x, -dx + y);

		dy++;
		if ((accum -= (dy << 1) - 1) < 0)
		{
			dx--;
			accum += dx << 1;
		}
	}
}


//---------------
void SDL_InitAudio(void)
{
	printf("SDL_InitAudio\n");
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		DoomRPG_Error("Could not initialize SDL Mixer: %s", Mix_GetError());
	}
}
/*
void SDL_CloseAudio(void) {

	Mix_Quit();
}*/

//--------------------

int SDL_GameControllerGetButtonID(void)
{
#ifdef __3DS__
	{
		u32 keys = hidKeysDown();
		if (keys & KEY_A) return CONTROLLER_BUTTON_A;
		if (keys & KEY_B) return CONTROLLER_BUTTON_B;
		if (keys & KEY_X) return CONTROLLER_BUTTON_X;
		if (keys & KEY_Y) return CONTROLLER_BUTTON_Y;
		if (keys & KEY_SELECT) return CONTROLLER_BUTTON_BACK;
		if (keys & KEY_START) return CONTROLLER_BUTTON_START;
		if (keys & KEY_L) return CONTROLLER_BUTTON_LEFT_BUMPER;
		if (keys & KEY_R) return CONTROLLER_BUTTON_RIGHT_BUMPER;
		if (keys & KEY_DUP) return CONTROLLER_BUTTON_DPAD_UP;
		if (keys & KEY_DDOWN) return CONTROLLER_BUTTON_DPAD_DOWN;
		if (keys & KEY_DLEFT) return CONTROLLER_BUTTON_DPAD_LEFT;
		if (keys & KEY_DRIGHT) return CONTROLLER_BUTTON_DPAD_RIGHT;
		if (keys & KEY_ZL) return CONTROLLER_BUTTON_LEFT_TRIGGER;
		if (keys & KEY_ZR) return CONTROLLER_BUTTON_RIGHT_TRIGGER;
		return CONTROLLER_BUTTON_INVALID;
	}
#endif
	int deadZoneLeft, deadZoneRight;

	deadZoneLeft = (sdlController.deadZoneLeft * 32768) / 100;
	deadZoneRight = (sdlController.deadZoneRight * 32768) / 100;
#ifdef __3DS__
#else
	if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_A)) {
		return CONTROLLER_BUTTON_A;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_B)) {
		return CONTROLLER_BUTTON_B;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_X)) {
		return CONTROLLER_BUTTON_X;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_Y)) {
		return CONTROLLER_BUTTON_Y;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_BACK)) {
		return CONTROLLER_BUTTON_BACK;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_START)) {
		return CONTROLLER_BUTTON_START;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_LEFTSTICK)) {
		return CONTROLLER_BUTTON_LEFT_STICK;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) {
		return CONTROLLER_BUTTON_RIGHT_STICK;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
		return CONTROLLER_BUTTON_LEFT_BUMPER;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
		return CONTROLLER_BUTTON_RIGHT_BUMPER;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_DPAD_UP)) {
		return CONTROLLER_BUTTON_DPAD_UP;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
		return CONTROLLER_BUTTON_DPAD_DOWN;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
		return CONTROLLER_BUTTON_DPAD_LEFT;
	}
	else if (SDL_GameControllerGetButton(sdlController.gGameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
		return CONTROLLER_BUTTON_DPAD_RIGHT;
	}
	else if (SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_TRIGGERLEFT)) {
		return CONTROLLER_BUTTON_LEFT_TRIGGER;
	}
	else if (SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)) {
		return CONTROLLER_BUTTON_RIGHT_TRIGGER;
	}
	else {

		// Y axis motion
		int16_t yVal = SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_LEFTY);
		// Below of dead zone
		if (yVal < -deadZoneLeft) {
			return CONTROLLER_BUTTON_LAXIS_UP;
		}
		// Above of dead zone
		else if (yVal > deadZoneLeft) {
			return CONTROLLER_BUTTON_LAXIS_DOWN;
		}

		// X axis motion
		int16_t xVal = SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_LEFTX);
		// Left of dead zone
		if (xVal < -deadZoneLeft) {
			return CONTROLLER_BUTTON_LAXIS_LEFT;
		}
		// Right of dead zone
		else if (xVal > deadZoneLeft) {
			return CONTROLLER_BUTTON_LAXIS_RIGHT;
		}


		// Y axis motion
		yVal = SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_RIGHTY);
		// Below of dead zone
		if (yVal < -deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_UP;
		}
		// Above of dead zone
		else if (yVal > deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_DOWN;
		}

		// X axis motion
		xVal = SDL_GameControllerGetAxis(sdlController.gGameController, SDL_CONTROLLER_AXIS_RIGHTX);
		// Left of dead zone
		if (xVal < -deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_LEFT;
		}
		// Right of dead zone
		else if (xVal > deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_RIGHT;
		}
	}
#endif
	return CONTROLLER_BUTTON_INVALID;
}

char buttonNames[][16] = {
	"Gamepad A",
	"Gamepad B",
	"Gamepad X",
	"Gamepad Y",
	"Back",
	"Start",
	"Left Stick",
	"Right Stick",
	"Left Bumper",
	"Right Bumper",
	"D-Pad Up",
	"D-Pad Down",
	"D-Pad Left",
	"D-Pad Right",
	"L-Stick Up",
	"L-Stick Down",
	"L-Stick Left",
	"L-Stick Right",
	"R-Stick Up",
	"R-Stick Down",
	"R-Stick Left",
	"R-Stick Right",
	"Left Trigger",
	"Right Trigger"
};

char *SDL_GameControllerGetNameButton(int id) {

	if (id != CONTROLLER_BUTTON_INVALID) {
		return buttonNames[id];
	}

	return "";
}

int SDL_JoystickGetButtonID(void)
{
#ifdef __3DS__
	{
		u32 keys = hidKeysHeld() | hidKeysDown();
		if (keys & KEY_A) return CONTROLLER_BUTTON_A;
		if (keys & KEY_B) return CONTROLLER_BUTTON_B;
		if (keys & KEY_X) return CONTROLLER_BUTTON_X;
		if (keys & KEY_Y) return CONTROLLER_BUTTON_Y;
		if (keys & KEY_SELECT) return CONTROLLER_BUTTON_BACK;
		if (keys & KEY_START) return CONTROLLER_BUTTON_START;
		if (keys & KEY_L) return CONTROLLER_BUTTON_LEFT_BUMPER;
		if (keys & KEY_R) return CONTROLLER_BUTTON_RIGHT_BUMPER;
		if (keys & (KEY_DUP | KEY_CPAD_UP)) return CONTROLLER_BUTTON_DPAD_UP;
		if (keys & (KEY_DDOWN | KEY_CPAD_DOWN)) return CONTROLLER_BUTTON_DPAD_DOWN;
		if (keys & (KEY_DLEFT | KEY_CPAD_LEFT)) return CONTROLLER_BUTTON_DPAD_LEFT;
		if (keys & (KEY_DRIGHT | KEY_CPAD_RIGHT)) return CONTROLLER_BUTTON_DPAD_RIGHT;
		if (keys & KEY_ZL) return CONTROLLER_BUTTON_LEFT_TRIGGER;
		if (keys & KEY_ZR) return CONTROLLER_BUTTON_RIGHT_TRIGGER;
		return CONTROLLER_BUTTON_INVALID;
	}
#endif
	int numAxes, deadZoneLeft, deadZoneRight;

	deadZoneLeft = (sdlController.deadZoneLeft * 32768) / 100;
	deadZoneRight = (sdlController.deadZoneRight * 32768) / 100;

	if (SDL_JoystickGetButton(sdlController.gJoystick, 0)) {
		return CONTROLLER_BUTTON_Y;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 1)) {
		return CONTROLLER_BUTTON_B;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 2)) {
		return CONTROLLER_BUTTON_A;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 3)) {
		return CONTROLLER_BUTTON_X;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 4)) {
		return CONTROLLER_BUTTON_LEFT_TRIGGER;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 5)) {
		return CONTROLLER_BUTTON_RIGHT_TRIGGER;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 6)) {
		return CONTROLLER_BUTTON_LEFT_BUMPER;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 7)) {
		return CONTROLLER_BUTTON_RIGHT_BUMPER;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 8)) {
		return CONTROLLER_BUTTON_BACK;
	}
	else if (SDL_JoystickGetButton(sdlController.gJoystick, 9)) {
		return CONTROLLER_BUTTON_START;
	}
	else {
		numAxes = SDL_JoystickNumAxes(sdlController.gJoystick);

		// Y axis motion
		int16_t yVal = SDL_JoystickGetAxis(sdlController.gJoystick, 1);
		// Below of dead zone
		if (yVal < -deadZoneLeft) {
			return (numAxes <= 2) ? CONTROLLER_BUTTON_DPAD_UP : CONTROLLER_BUTTON_LAXIS_UP;
		}
		// Above of dead zone
		else if (yVal > deadZoneLeft) {
			return (numAxes <= 2) ? CONTROLLER_BUTTON_DPAD_DOWN : CONTROLLER_BUTTON_LAXIS_DOWN;
		}

		// X axis motion
		int16_t xVal = SDL_JoystickGetAxis(sdlController.gJoystick, 0);
		// Left of dead zone
		if (xVal < -deadZoneLeft) {
			return (numAxes <= 2) ? CONTROLLER_BUTTON_DPAD_LEFT : CONTROLLER_BUTTON_LAXIS_LEFT;
		}
		// Right of dead zone
		else if (xVal > deadZoneLeft) {
			return (numAxes <= 2) ? CONTROLLER_BUTTON_DPAD_RIGHT : CONTROLLER_BUTTON_LAXIS_RIGHT;
		}

		// Y axis motion
		yVal = SDL_JoystickGetAxis(sdlController.gJoystick, 2);
		// Below of dead zone
		if (yVal < -deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_UP;
		}
		// Above of dead zone
		else if (yVal > deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_DOWN;
		}

		// X axis motion
		xVal = SDL_JoystickGetAxis(sdlController.gJoystick, 3);
		// Left of dead zone
		if (xVal < -deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_LEFT;
		}
		// Right of dead zone
		else if (xVal > deadZoneRight) {
			return CONTROLLER_BUTTON_RAXIS_RIGHT;
		}
	}

	return CONTROLLER_BUTTON_INVALID;
}

char mouseButtonNames[][20] = {
	"Mouse Left",
	"Mouse Middle",
	"Mouse Right",
	"Mouse X1",
	"Mouse X2",
	"Mouse Wheel Up",
	"Mouse Wheel Down",
	"Mouse Motion Up",
	"Mouse Motion Down",
	"Mouse Motion Left",
	"Mouse Motion Right"
};

char* SDL_MouseGetNameButton(int id)
{
	if (id != MOUSE_BUTTON_INVALID) {
		return mouseButtonNames[id];
	}

	return "";
}

