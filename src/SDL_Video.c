

//Using SDL and standard IO

#ifdef __3DS__
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <3ds.h>
#include <citro3d.h>
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
static u32 g_botW = 320, g_botH = 240;  /* real bottom framebuffer size */
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
static int g_top3D = 0;          /* 1 => stereo enabled this frame (slider > 0) */
static float g_stereoSep = 0.0f; /* angular separation (degrees); 0 = eyes identical (test build) */

/* Flag-only APT hook: do NOT call any gfx function here (GPU state is invalid
   in the hook context and faults). Just record suspend so the present loop
   skips gfxFlushBuffers() while the applet owns the screen -- this stops our
   flush from racing libctru's GSP event thread (dump 98..105: gspEventThreadMain,
   FAR 0xf4). This is the exact setup that made build 711500 graceful on HOME. */
static void stereo_apt_gfx_reacquire(void) {
    /* On resume the OS hands back a DIFFERENT framebuffer address than the one
       cached at init (proven DXX-3DS behavior). Re-fetch so we write live memory
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
    if (hook == APTHOOK_ONSUSPEND) g_gfx_suspended = 1;
    else if (hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP) {
        g_gfx_suspended = 0;
        stereo_apt_gfx_reacquire();
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
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	gfxSet3D(false);                 /* start flat. 3D is toggled on the MAIN THREAD later (slider),
	                                     never in the APT hook (unsafe context -> FAR 0xf4). */
	/* Stereo eye-capture buffers: 400x240 scene per eye (matches the 3D view region). */
	g_topEyeL = (Uint32*)SDL_calloc(1, (size_t)400 * 240 * sizeof(Uint32));
	g_topEyeR = (Uint32*)SDL_calloc(1, (size_t)400 * 240 * sizeof(Uint32));
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
       libctru's GSP/framebuffers for us. We MUST call gfxExit() explicitly,
       or libctru's GSP event thread (gspEventThreadMain) keeps running with
       our framebuffer state after the applet is closed (HOME->FTP) and faults
       at teardown (dump 98..107: Data Abort FAR=0xf4 @ GSP region 0x8043a70).
       ctrWolfen calls gfxExit() here for exactly this reason. */
    g_topFbL = NULL; g_topFbR = NULL; g_botFb = NULL;
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

/* PHASE 1 (Option A): blit the offscreen 400x480 RGBA32 surface into the gfx
   top/bottom framebuffers. Per DXX-3DS (bottom_screen.c), the gfx framebuffer
   is GSP_RGB565_OES: 16-bit, ONE u16 PER PIXEL, stride = width*2 (NOT RGBA8).
   gfx TOP is 240x400, BOTTOM is 240x320 (confirmed: bs_dims.txt bw=240 bh=320).
   The 3DS LCD rotates the buffer 90deg CCW on display, so our landscape
   offscreen (top: 400x240; bottom: 400x240) must rotate into the portrait
   buffer. DXX-3DS mapping (logical x in [0,320) w, y in [0,240) h):
       fx = 239 - y   (logical y -> buffer x)
       fy = x         (logical x -> buffer y)
       idx = fx + fy * g_w
   We replicate that for both screens, converting RGBA32 -> RGB565. */
/* Top/bottom framebuffers are GSP_BGR8_OES (3 bytes/px), allocated by
   SDL_Init(VIDEO)->gfxInitDefault(). GL_BGR byte order: byte0=B,byte1=G,byte2=R.
   Pack RGBA32 (R=0x00FF0000,G=0x0000FF00,B=0x000000FF) into that order. */
static inline Uint32 rgba32_to_bgr8(Uint32 px) {
	int r = (px >> 16) & 0xFF;
	int g = (px >> 8)  & 0xFF;
	int b = (px >> 0)  & 0xFF;
	return (Uint32)(((Uint32)b) | ((Uint32)g << 8) | ((Uint32)r << 16));
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
/* BUILD 712018: STEREO suspend-crash fix (dump 127, FAR 0xf4 = same GSP class as 122-126).
   Root cause: with the slider up, the last present before HOME committed a 3D (800-tall)
   transfer; aptMainLoop() then blocked and the GSP thread faulted servicing it. Fix: in the
   main-thread present, if aptShouldJumpToHome() is true, FORCE FLAT so the present commits a
   400-tall single-eye transfer that the suspend path handles gracefully. APT hook stays gfx-free.
   Slider-gate + identical-eye 3D present pipeline unchanged from 712017. */
static void SDL_PresentGfx(SDL_Surface* surface) {
#ifdef __3DS__
    (void)surface;
    if (g_gfx_suspended) return;  /* HOME menu owns the GPU; skip present+flush */
    if (!g_topFbL || !g_topFbR || !g_botFb || !sdlVideo.screenSurface) return;
    const Uint32* src = (const Uint32*)sdlVideo.screenSurface->pixels;  /* 400x480 RGBA32 */

    /* Slider gate (main thread only): enable stereo when the 3D slider is pushed past
       the detent. This is read HERE (present runs on the main thread), never in the APT
       hook, so gfxSet3D() is never called from the unsafe hook context.
       CRASH FIX (dump 127): if HOME was pressed this frame, FORCE FLAT. aptMainLoop() will
       block right after this present and tear down the GSP transfer; a pending 3D (800-tall)
       transfer faults the GSP thread (FAR 0xf4). Forcing flat here makes the present commit a
       400-tall single-eye transfer, which the suspend path handles gracefully. */
    if (aptShouldJumpToHome())
        g_top3D = 0;
    else
        g_top3D = (osGet3DSliderState() > 0.0f) ? 1 : 0;

    /* Re-fetch the live framebuffers EVERY frame. The 3DS display buffer can
       differ from the one cached at init / after resume, so writing a cached
       pointer paints a buffer the display is NOT showing -> stale bar + flicker.
       Re-fetching guarantees we always write the currently-displayed buffer.
       Single-buffered + flush-only (gfxSetDoubleBuffering(false) at init). */
    {
        u16 tw = 0, th = 0, bw = 0, bh = 0;
        g_topFbL = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
        g_topW = tw ? tw : 240;  g_topH = th ? th : 400;
        if (g_top3D) {
            /* Stereo: top buffer is 240x800 (GSP_SCREEN_HEIGHT_TOP_2X). GFX_LEFT = rows
               0..399, GFX_RIGHT (=second half) = rows 400..799. We derive the right-half
               pointer by offset into the SAME allocated buffer (no gfx call -> crash-safe;
               the 712016 fault was the gfxSet3D() inside the APT hook, not this write). */
            g_topFbR = g_topFbL + (size_t)400 * g_topW * 3;
        } else {
            g_topFbR = g_topFbL;  /* alias LEFT; 3D off => right eye unused */
        }
        g_botFb  = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
        g_botW = bw ? bw : 240;  g_botH = bh ? bh : 320;
    }
    if (!g_topFbL || !g_topFbR || !g_botFb) return;

    /* BOTTOM: 240x320 portrait framebuffer. The working SDL config (devkitPro
       3DS driver, SDL_DUALSCR 400x480) maps the surface LOWER HALF (rows 240..479)
       onto the bottom screen UPRIGHT (NO rotation). The LCD optically rotates the
       portrait buffer 90deg, so an upright landscape must be written rotated 90deg
       CW into the buffer. Proven mapping (dxx / SDL driver):
           buffer_col = g_botW-1 - (src_row - 240)   (src_row 240..479 -> col 239..0)
           buffer_row = src_col * g_botH / 400         (src_col 0..399 -> row 0..319)
       The original bar was killed by clearing SOURCE rows 240..359 (the blank strip
       above the automap tiles, which start at y>=363) to opaque black EVERY frame
       -- the automap bg clear (SDL_FillRect) is a no-op on this surface, so without
       this the stale/garbage strip showed through. We replicate that here. */
    {
        /* 1) Clear the source strip rows 240..359 to opaque black (author fix). */
        Uint32* sp = (Uint32*)sdlVideo.screenSurface->pixels;
        for (int sy = 240; sy < 360; sy++)
            for (int sx = 0; sx < 400; sx++)
                sp[sy * 400 + sx] = 0xFF000000u;  /* A=255, RGB=0 */
    }
    /* NOTE: we render the rotated bottom into the OFF-SCREEN g_botTmp scratch, then
       memcpy it to the live g_botFb in ONE contiguous copy. This keeps the GSP-happy
       "full contiguous buffer write" each frame (removing the per-frame live memset is
       what brought dump 125 back), while never showing a half-black intermediate frame
       -> no CRT flash (the flash was the torn black frame visible mid-rewrite). */
    {
        /* 2) Blit the FULL lower half (rows 240..479) into g_botTmp, rotated 90deg so it
           appears UPRIGHT on screen. CRASH FIX: 709/10/11/12 used a ROW-FLIP (dest written
           row-major) and EVERY one crashed (FAR 0xf4 GSP Data Abort at HOME suspend). The
           proven-graceful rotation is the COL-FLIP write-order (build 712002): dest written
           column-by-column: dx = g_botW-1 - (sy-240), dy = sx * g_botH / 400. This maps
           source rows->dest cols (vertical flip) + source cols->dest rows, i.e. the SAME
           upright orientation as the row-flip but in the graceful write order.
           (709's "180deg" was the g_botH shift-bug, not the rotation direction.) */
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
    if (g_botTmp && g_botFb) SDL_memcpy(g_botFb, g_botTmp, (size_t)g_botW * g_botH * 3);
    /* For reference, the TOP screen (rows 0..239) maps the same way: */
    /* TOP: 400x240 landscape 3D view + HUD (screenSurface rows 0..239) into the
       240x400 portrait top framebuffer, DIRECT upright (LCD rotates 90deg itself).
       Scale source 400-wide -> g_topW (240), 240-tall -> g_topH (400). gfxSet3D(false)
       => only GFX_LEFT shown; write both eyes anyway (harmless). */
    {
        /* TOP: 400x240 landscape 3D view + HUD (surface rows 0..239) into the 240x400
           portrait top framebuffer, rotated 90deg UPRIGHT via the proven-graceful COL-FLIP
           write-order (same as bottom; build 712002 was graceful with this order, row-flip
           crashed). dx = g_topW-1 - sy ; dy = sx * g_topH / 400. */
        for (int sy = 0; sy < 240; sy++) {
            int dx = g_topW - 1 - sy;
            const Uint32* row = src + sy * 400;
            for (int sx = 0; sx < 400; sx++) {
                int dy = (sx * g_topH) / 400;
                if (dy < 0) dy = 0; else if (dy >= g_topH) dy = g_topH - 1;
                int bi = (dy * g_topW + dx) * 3;
                put_bgr8(&g_topFbL[bi], rgba32_to_bgr8(row[sx]));
                put_bgr8(&g_topFbR[bi], rgba32_to_bgr8(row[sx]));
            }
        }
    }

    /* Commit the frame. On the MAIN THREAD, toggle 3D in lockstep with the present so the
       GSP display transfer always matches the current buffer layout:
         - flat:  gfxSet3D(false) + gfxScreenSwapBuffers(GFX_TOP, false)  (single 400-tall eye)
         - 3D:    gfxSet3D(true)  + gfxScreenSwapBuffers(GFX_TOP, true)   (800-tall two-eye transfer)
       (DXX-3DS / PrBoom-Plus-3DS lockstep rule: never leave 3D set without the matching present.)
       On suspend the APT hook only sets g_gfx_suspended=1 (no gfx call) and we skip this, so the
       GSP transfer is never torn down mid-3D -> no FAR 0xf4 fault. */
    if (g_top3D) {
        gfxSet3D(true);
        gfxFlushBuffers();
        gfxScreenSwapBuffers(GFX_TOP, true);
    } else {
        gfxSet3D(false);
        gfxFlushBuffers();
        gfxScreenSwapBuffers(GFX_TOP, false);
    }
#endif
}

void SDL_RenderPresent(SDL_Surface *surface)
{
    /* Clear the bottom-screen top (empty automap region) to opaque black
       every frame. The 3DS automap background clear via SDL_FillRect is
       unreliable on this hardware surface (and only ran on movement), so
       the white framebuffer init showed through as a strip. Reset the
       clip and blit an opaque-black surface instead -- blits present
       reliably. Tiles are drawn lower (y>=363) so this never hides them. */
    /* The offscreen surface is no longer displayed (gfx owns the screens),
       so the old SDL-surface strip-clear is dead code. It also dereferenced
       surface->map (NULL) -> Data Abort 0x5c on present. Present via gfx only. */
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
		hidScanInput();
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

