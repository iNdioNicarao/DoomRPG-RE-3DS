#ifndef HUD_H__
#define HUD_H__

#define SCROLL_START_DELAY 750
#define MSG_DISPLAY_TIME 1200
#define MS_PER_CHAR 64
#define MAX_MESSAGES 5
#include "logger.h"
struct DoomRPG_s;
struct Image_s;
typedef struct {
	int oldHealth;
	int oldArmor;
	int oldAmmo[30];
	int oldFaceFrame;
	int oldDir;
	int oldWeapon;

	SDL_Surface* hudSurface; // общий offscreen HUD
	int dirty; // нужно ли обновить кэш
} HudCache_t;
typedef struct Hud_s
{
	int memory;
	short statusBarHeight;
	short statusTopBarHeight;
	int msgMaxChars;
	struct Image_s imgStatusBar;
	struct Image_s imgStatusBarLarge;
	struct Image_s imgIconSheet;
	struct Image_s imgHudFaces;
	struct Image_s imgAttArrow;
	struct Image_s imgStatusArrow;
	int hudFaceWidth;
	int hudFaceHeight;
	int iconSheetWidth;
	int iconSheetHeight;
	int statusHealthXpos;
	int statusArmorXpos;
	int statusHudFacesXpos;
	int unk1;
	int statusAmmoXpos;
	int statusOrientationXpos;
	int statusOrientationArrowXpos;
	int statusLine1Xpos;
	int statusLine2Xpos;
	boolean largeHud;
	char messages[MAX_MESSAGES][MS_PER_CHAR];
	int msgCount;
	int msgTime;
	int msgDuration;
	char logMessage[MS_PER_CHAR];
	char* statBarMessage;
	int gotFaceTime;
	int damageTime;
	int damageCount;
	int damageDir;
	boolean isUpdate;
	char healthNum[4];
	char armorNum[4];
	char ammoNum[3];
	HudCache_t* cache;
	int faceFrame;
	int statusCompassXpos;
	struct DoomRPG_s* doomRpg;
} Hud_t;

Hud_t* Hud_init(Hud_t* hud, DoomRPG_t* doomRpg);
void Hud_free(Hud_t* hud, boolean freePtr);
void Hud_addMessage(DoomCanvas_t* doomCanvas, char* str);
void Hud_addMessageForce(DoomCanvas_t* doomCanvas, char* str, boolean force);
void Hud_calcMsgTime(DoomCanvas_t* doomCanvas);
void Hud_drawBarTiles(DoomCanvas_t* doomCanvas, int x, int y, int width, boolean isLargerStatusBar);
void Hud_drawBottomBar(DoomCanvas_t* doomcanvas);
void Hud_drawEffects(DoomCanvas_t* doomcanvas);
void Hud_drawTopBar(DoomCanvas_t* doomcanvas);
void Hud_finishMessageBufferForce(DoomCanvas_t* doomCanvas, boolean force);
void Hud_finishMessageBuffer(DoomCanvas_t* doomCanvas);
char* Hud_getMessageBufferForce(DoomCanvas_t* doomCanvas, boolean force);
char* Hud_getMessageBuffer(DoomCanvas_t* doomCanvas);
void Hud_shiftMsgs(DoomCanvas_t* doomCanvas);
void Hud_startup(Hud_t* hud, boolean largeStatus);

#endif
