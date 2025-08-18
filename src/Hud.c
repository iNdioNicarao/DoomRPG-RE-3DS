
#ifdef __3DS__
#include <SDL/SDL.h>
#define skipNullptr 0
#define caching 0
#else
#include <SDL.h>
#endif
#include <stdio.h>
#include <string.h>

#include "DoomRPG.h"
#include "DoomCanvas.h"
#include "Hud.h"
#include "Player.h"
#include "Combat.h"
#include "CombatEntity.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Weapon.h"
#include "SDL_Video.h"

Hud_t* Hud_init(Hud_t* hud, DoomRPG_t* doomRpg)
{
	printf("Hud_init\n");

	if (hud == NULL)
	{
		hud = SDL_malloc(sizeof(Hud_t));
		if (hud == NULL) {
			return NULL;
		}
	}
    SDL_memset(hud, 0, sizeof(Hud_t));

	hud->msgCount = 0;
	hud->doomRpg= doomRpg;

	return hud;
}

void Hud_free(Hud_t* hud, boolean freePtr)
{
    DoomRPG_freeImage(hud->doomRpg, &hud->imgStatusBar);
    DoomRPG_freeImage(hud->doomRpg, &hud->imgStatusBarLarge);
    DoomRPG_freeImage(hud->doomRpg, &hud->imgHudFaces);
    DoomRPG_freeImage(hud->doomRpg, &hud->imgIconSheet);
    DoomRPG_freeImage(hud->doomRpg, &hud->imgAttArrow);
    DoomRPG_freeImage(hud->doomRpg, &hud->imgStatusArrow);

    if (freePtr) {
        SDL_free(hud);
    }
}

void Hud_addMessage(DoomCanvas_t* doomCanvas, char* str)
{
    Hud_addMessageForce(doomCanvas, str, false);
}

void Hud_addMessageForce(DoomCanvas_t* doomCanvas, char* str, boolean force)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    Hud_t* hud = doomCanvas->hud;
    if (str) {
        if (force) {
            hud->msgCount = 0;
        }

        if ((hud->msgCount <= 0) || strcmp(str, hud->messages[hud->msgCount + -1])) {
            if (hud->msgCount == MAX_MESSAGES) {
                Hud_shiftMsgs(doomCanvas);
            }
            strncpy(hud->messages[hud->msgCount], str, MS_PER_CHAR);
            hud->msgCount++;

            if (hud->msgCount == 1) {
                Hud_calcMsgTime(doomCanvas);
                if (force) {
                    hud->msgDuration *= 2;
                }
            }
        }
    }
}

void Hud_calcMsgTime(DoomCanvas_t* doomCanvas)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    Hud_t* hud = doomCanvas->hud;
    int len;
    if (!doomCanvas->time) {
        hud->msgTime = 0;
    }
    else
        hud->msgTime = doomCanvas->time;
    len = strlen(hud->messages[0]);

    if (len <= hud->msgMaxChars) {
        hud->msgDuration = MSG_DISPLAY_TIME;
    }
    else {
        hud->msgDuration = len * 100;
    }
}

void Hud_drawBarTiles(DoomCanvas_t* doomCanvas, int x, int y, int width, boolean isLargerStatusBar)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    Image_t* img;
    int height;

    if (isLargerStatusBar == false) {
        height = doomCanvas->hud->imgStatusBar.height;
        img = &doomCanvas->hud->imgStatusBar;
    }
    else {
        height = doomCanvas->hud->imgStatusBarLarge.height;
        img = &doomCanvas->hud->imgStatusBarLarge;
    }

    DoomCanvas_drawImageSpecial(doomCanvas, img, 0, 0, width, height, 0, x, y, 0);
}
void Hud_drawBarTilesSur(DoomCanvas_t* doomCanvas, int x, int y, int width, boolean isLargerStatusBar, SDL_Surface* surface)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    Image_t* img;
    int height;

    if (isLargerStatusBar == false) {
        height = doomCanvas->hud->imgStatusBar.height;
        img = &doomCanvas->hud->imgStatusBar;
    }
    else {
        height = doomCanvas->hud->imgStatusBarLarge.height;
        img = &doomCanvas->hud->imgStatusBarLarge;
    }

    DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, 0, width, height, 0, x, y, 0, surface);
}
#if caching
void Hud_drawLines(DoomCanvas_t* doomCanvas, SDL_Surface* targetSurface) {
    Hud_t* hud = doomCanvas->hud;
    int top    = doomCanvas->clipRect.h - hud->statusBarHeight;

    DoomRPG_setColor(doomCanvas->doomRpg, 0xFFFFFF);
    DoomRPG_drawLineSur(doomCanvas->doomRpg, 0, top, doomCanvas->clipRect.w, top, targetSurface);
    DoomRPG_drawLineSur(doomCanvas->doomRpg, 0, top + hud->statusBarHeight - 1,
                        doomCanvas->clipRect.w, top + hud->statusBarHeight - 1, targetSurface);
}

void Hud_drawCompass(DoomCanvas_t* doomCanvas, int dir, SDL_Surface* targetSurface) {
    Hud_t* hud = doomCanvas->hud;
    int compassX = hud->statusOrientationArrowXpos;
    int compassY = hud->statusBarHeight / 2;

    //Image_t* img = &doomCanvas->imgCompass[dir];
    DoomCanvas_drawImageSur(doomCanvas, &doomCanvas->hud->imgStatusArrow, compassX, compassY, 9, targetSurface);
}

void Hud_drawBarTilesSur(DoomCanvas_t* doomCanvas, int x, int y, int width, boolean isLargerStatusBar, SDL_Surface* surface)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    Image_t* img;
    int height;

    if (isLargerStatusBar == false) {
        height = doomCanvas->hud->imgStatusBar.height;
        img = &doomCanvas->hud->imgStatusBar;
    }
    else {
        height = doomCanvas->hud->imgStatusBarLarge.height;
        img = &doomCanvas->hud->imgStatusBarLarge;
    }

    DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, 0, width, height, 0, x, y, 0, surface);
}
void Hud_drawFace(DoomCanvas_t* doomCanvas, int a, SDL_Surface* surface) {

}
void Hud_drawBottomBar(DoomCanvas_t* doomCanvas) {
    Hud_t* hud = doomCanvas->hud;
    HudCache_t* cache = hud->cache;
    CombatEntity_t* ce = &doomCanvas->player->ce;

    int health = CombatEntity_getHealth(ce);
    int armor  = CombatEntity_getArmor(ce);
    int weapon = doomCanvas->player->weapon;
    int ammo   = doomCanvas->player->ammo[doomCanvas->combat->weaponInfo[weapon].ammoType];;
    int dir    = doomCanvas->destAngle & 255;

    if (cache->oldHealth != health ||
        cache->oldArmor  != armor ||
        cache->oldAmmo[weapon] != ammo ||
        cache->oldDir    != dir ||
        cache->oldWeapon != weapon)
    {
        cache->dirty = 1;
    }

    if (cache->dirty) {
        SDL_FillRect(cache->hudSurface, NULL, 0x00000000); // очистка (прозрачный фон)

        // рисуем всё заново на surface
        Hud_drawBarTilesSur(doomCanvas, 0, 0, 400, hud->largeHud, cache->hudSurface);
        Hud_drawLines(doomCanvas, cache->hudSurface);

        // здоровье
        char num[16];
        snprintf(num, sizeof(num), "%d", health);
        DoomCanvas_drawFontSur(doomCanvas, num, hud->statusHealthXpos,
            hud->statusBarHeight/2, 9, 0, 3, hud->largeHud, cache->hudSurface);

        // броня
        snprintf(num, sizeof(num), "%d", armor);
        DoomCanvas_drawFontSur(doomCanvas, num, hud->statusArmorXpos,
            hud->statusBarHeight/2, 9, 0, 3, hud->largeHud, cache->hudSurface);

        // лицо
        Hud_drawFace(doomCanvas, hud->faceFrame, cache->hudSurface);

        // патроны
        snprintf(num, sizeof(num), "%d", ammo);
        DoomCanvas_drawFontSur(doomCanvas, num, hud->statusAmmoXpos,
            hud->statusBarHeight/2, 9, 0, 3, hud->largeHud, cache->hudSurface);

        // направление
        Hud_drawCompass(doomCanvas, dir, cache->hudSurface);

        // сохранить новые значения
        cache->oldHealth = health;
        cache->oldArmor  = armor;
        cache->oldAmmo[weapon] = ammo;
        cache->oldDir    = dir;
        cache->oldWeapon = weapon;
        cache->oldFaceFrame = hud->faceFrame;

        cache->dirty = 0;
    }

    SDL_Rect dst = {0, doomCanvas->clipRect.h - hud->statusBarHeight,
                    400, hud->statusBarHeight};
    SDL_BlitSurface(cache->hudSurface, NULL, sdlVideo.screenSurface, &dst);
}
#else
void Hud_drawBarLine(DoomCanvas_t* doomCanvas, int x1, int y1, int x2, int y2, SDL_Surface* targetSurface){
    DoomRPG_drawLineSur(doomCanvas->doomRpg, x1, y1, x2, y2, targetSurface);
}
void Hud_drawBottomBar(DoomCanvas_t* doomCanvas)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    //if (doomCanvas->menuSystem->menu)
    //------creating surface & variables
    SDL_Surface* tmpSurface =
        SDL_CreateRGBSurface(SDL_SWSURFACE,
            sdlVideo.screenW,
            doomCanvas->hud->statusBarHeight,
            16,
            0x0,
            0x0,
            0x0,
            0x0);
    Image_t* img;
    CombatEntity_t* ce;
    Combat_t* combat;
    int dispW, dispH, stbH;
    int cx, x, y, dy;
    int lx1, lx2;
    int health, maxHealth;
    int faceState, faceX;
    int weapon;
    char dir[2];
    //set variables
    ce = &doomCanvas->player->ce;
    dispW = 400;
    dispH = doomCanvas->hud->statusBarHeight;
    stbH = doomCanvas->hud->statusBarHeight;
    cx = doomCanvas->SCR_CX;
    y = 10;

    if (doomCanvas->hud->largeHud) {
        cx -= 88;
        y -= 8;
        x = 10;
    }
    else {
        cx -= 64;
        y -= 5;
        x = 7;
    }

    //drawing bottom bar rect
    Hud_drawBarTilesSur(doomCanvas, 0, dispH - stbH, 400, doomCanvas->hud->largeHud, tmpSurface);

    lx1 = doomCanvas->hud->statusLine1Xpos + cx;
    lx2 = doomCanvas->hud->statusLine2Xpos + cx;
    DoomRPG_setColor(doomCanvas->doomRpg, 0x313131);
    Hud_drawBarLine(doomCanvas, lx1, dispH - stbH, lx1, dispH - 1, tmpSurface);
    Hud_drawBarLine(doomCanvas, lx2, dispH - stbH, lx2, dispH - 1, tmpSurface);
    DoomRPG_setColor(doomCanvas->doomRpg, 0x808591);
    Hud_drawBarLine(doomCanvas, lx1 + 1, dispH - stbH, lx1 + 1, dispH - 1, tmpSurface);
    Hud_drawBarLine(doomCanvas, lx2 + 1, dispH - stbH, lx2 + 1, dispH - 1, tmpSurface);
//-----drawing health
    img = &doomCanvas->hud->imgIconSheet;
    dy = dispH - (stbH >> 1);

    int healthNow = CombatEntity_getHealth(ce);
    DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, 0, doomCanvas->hud->iconSheetWidth, doomCanvas->hud->iconSheetHeight, 0, doomCanvas->hud->statusHealthXpos + cx, dy, 0x24, tmpSurface);
    SDL_snprintf(doomCanvas->hud->healthNum, 4, "%d", healthNow);
    DoomCanvas_drawFontSur(doomCanvas, doomCanvas->hud->healthNum, doomCanvas->hud->statusHealthXpos + cx + x * 2 + doomCanvas->hud->iconSheetWidth + 1, y, 9, 0, 3, doomCanvas->hud->largeHud, tmpSurface);

    // draw armmor
    int armorNow = CombatEntity_getArmor(ce);
    DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, doomCanvas->hud->iconSheetHeight, doomCanvas->hud->iconSheetWidth, doomCanvas->hud->iconSheetHeight, 0, doomCanvas->hud->statusArmorXpos + cx, dy, 0x24, tmpSurface);
    SDL_snprintf(doomCanvas->hud->armorNum, 4, "%d", armorNow);
    DoomCanvas_drawFontSur(doomCanvas, doomCanvas->hud->armorNum, doomCanvas->hud->statusArmorXpos + cx + x * 2 + doomCanvas->hud->iconSheetWidth, y, 9, 0, 3, doomCanvas->hud->largeHud, tmpSurface);
    // draw face
    health = CombatEntity_getHealth(ce);

    if (health > 0 && doomCanvas->time - doomCanvas->hud->gotFaceTime < 500) {
        faceState = 8;
    }
    else if (health <= 0 || doomCanvas->time >= doomCanvas->hud->damageTime) {
        maxHealth = CombatEntity_getMaxHealth(ce);

        if (health <= maxHealth / 4) {
            faceState = 3;
        }
        else if (health <= maxHealth / 3) {
            faceState = 2;
        }
        else if (health <= maxHealth / 2) {
            faceState = 1;
        }
        else {
            faceState = 0;
        }
    }
    else {
        switch (doomCanvas->hud->damageDir)
        {
        case 2:
            faceState = 7;
            break;
        case 1:
            faceState = 6;
            break;
        case 3:
            faceState = 5;
            break;
        default:
            faceState = 4;
            break;
        }
    }

    faceX = doomCanvas->hud->statusHudFacesXpos + cx;
    DoomRPG_setColor(doomCanvas->doomRpg, 0x323232);
    Hud_drawBarLine(doomCanvas, faceX - 1, dispH - doomCanvas->hud->statusBarHeight, faceX - 1, dispH -1, tmpSurface);
    DoomCanvas_drawImageSpecialSur(doomCanvas,
        &doomCanvas->hud->imgHudFaces,
        0,
        faceState * doomCanvas->hud->hudFaceHeight,
        doomCanvas->hud->hudFaceWidth,
        doomCanvas->hud->hudFaceHeight,
        0,
        faceX,
        dy,
        0x24,
        tmpSurface);
    DoomRPG_setColor(doomCanvas->doomRpg,
        0x828282);
    Hud_drawBarLine(doomCanvas, faceX + doomCanvas->hud->hudFaceWidth, dispH - doomCanvas->hud->statusBarHeight, faceX + doomCanvas->hud->hudFaceWidth, dispH + -1, tmpSurface);

    // draw weapon and ammo
    if (doomCanvas->player->weapons) {
        weapon = doomCanvas->player->weapon;

        if (weapon == 0) {
            DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, doomCanvas->hud->iconSheetHeight << 1, doomCanvas->hud->iconSheetWidth, doomCanvas->hud->iconSheetHeight, 0, doomCanvas->hud->statusAmmoXpos + cx, dy, 0x24, tmpSurface);
            strncpy(doomCanvas->hud->ammoNum, "--", 4);
        }
        else {
            combat = doomCanvas->doomRpg->combat;
            int ammoCount = doomCanvas->player->ammo[combat->weaponInfo[weapon].ammoType];
            DoomCanvas_drawImageSpecialSur(doomCanvas, img, 0, doomCanvas->hud->iconSheetHeight * (combat->weaponInfo[weapon].ammoType + 3), doomCanvas->hud->iconSheetWidth, doomCanvas->hud->iconSheetHeight, 0, doomCanvas->hud->statusAmmoXpos + cx, dy, 0x24, tmpSurface);
            SDL_snprintf(doomCanvas->hud->ammoNum, 3, "%d", ammoCount);
        }

        DoomCanvas_drawFontSur(doomCanvas, doomCanvas->hud->ammoNum, doomCanvas->hud->statusAmmoXpos + cx + doomCanvas->hud->iconSheetWidth + x * 2, y, 9, 0, 2, doomCanvas->hud->largeHud, tmpSurface);
    }

    // draw orientation text
    switch (doomCanvas->destAngle & 255) {
    case 0:
        strncpy(dir, "E", sizeof(dir));
        break;
    case 128:
        strncpy(dir, "W", sizeof(dir));
        break;
    case 192:
        strncpy(dir, "S", sizeof(dir));
        break;
    default:
        strncpy(dir, "N", sizeof(dir));
        break;
    }

    DoomCanvas_drawImageSur(doomCanvas, &doomCanvas->hud->imgStatusArrow, doomCanvas->hud->statusOrientationArrowXpos + cx, y - 3, 9, tmpSurface);
    DoomCanvas_drawFontSur(doomCanvas, dir, doomCanvas->hud->statusOrientationXpos + cx, y + 2, 9, 0, 1, doomCanvas->hud->largeHud, tmpSurface);
    SDL_Rect* srcRect = malloc(sizeof(*srcRect));
    SDL_Rect* dstRect = malloc(sizeof(*srcRect));
    srcRect->h = tmpSurface->h;
    srcRect->w = tmpSurface->w;
    srcRect->x = 0;
    srcRect->y = 0;

    dstRect->h = tmpSurface->h;
    dstRect->w = tmpSurface->w;
    dstRect->x = 0;
    dstRect->y = 240 - doomCanvas->hud->statusBarHeight;

    SDL_BlitSurface(tmpSurface, srcRect, sdlVideo.screenSurface, dstRect);
    SDL_FreeSurface(tmpSurface);
    SDL_free(dstRect);
    SDL_free(srcRect);
}
#endif
void Hud_drawEffects(DoomCanvas_t* doomCanvas)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    DoomRPG_t* doomRpg = doomCanvas->doomRpg;
    char str[8];
    int x, y, srcY;
    Hud_t* hud = doomCanvas->hud;
    if (doomCanvas->time < hud->damageTime) {
        if (hud->damageCount > 0) {
            DoomRPG_setColor(doomRpg, 0xBB0000);
        }
        else {
            DoomRPG_setColor(doomRpg, 0xFFFFFF);
        }

        DoomRPG_fillRect(doomRpg, 0, hud->statusTopBarHeight, 2, 192 + -1);
        DoomRPG_fillRect(doomRpg, 400 + -2, hud->statusTopBarHeight, 2, 192 + -1);
        DoomRPG_fillRect(doomRpg, 0, hud->statusTopBarHeight, 400 - 1, 2);
        DoomRPG_fillRect(doomRpg, 0, hud->statusTopBarHeight + 192 + -2, 400 - 1, 2);

        if (hud->damageDir && (hud->damageCount > 0)) {
            srcY = 0;
            if (hud->damageDir == 1) {
                x = 400 - 20;
                y = 240 >> 1;
                srcY = 36;
            }
            else if (hud->damageDir == 3) {
                x = 20;
                y = 240 >> 1;
                srcY = 18;
            }
            else {
                x = 400 >> 1;
                y = (240 - hud->statusBarHeight) - 20;
            }

            DoomCanvas_drawImageSpecial(doomCanvas, &hud->imgAttArrow, 0, srcY, 18, 18, 0, x, y, 0x30);
        }
    }
    else if (hud->damageTime) {
        DoomCanvas_invalidateRectAndUpdateView(doomCanvas);
        hud->damageTime = 0;
    }

    if (doomRpg->player->berserkerTics) {
        // Bloqueo esta l�nea ya que la puse en otra funci�n.
        // I block this line since I put it in another function.
        //{
        //    Render_setBerserkColor(doomRpg->render);
        //}

        SDL_snprintf(str, sizeof(str), "%d", doomCanvas->player->berserkerTics);
        DoomCanvas_drawString1(doomCanvas, str, 400 - 2, hud->statusTopBarHeight + 2, 9);
    }
}

void Hud_drawTopBar(DoomCanvas_t* doomCanvas)
{
#if skipNullptr
    if (!hud->doomRpg->doomCanvas) {
        DoomRPG_ReinitCanvasAndRenderer(hud->doomRpg);
        return;
    }
#endif
    char* text;
    int len;
    int time, w;
    int strBeg, strEnd;
    boolean updateTime;
    Hud_drawBarTiles(doomCanvas, 0, 0, 400, false);

    updateTime = true;
    // New Code Lines
    {
        // No actualiza los memsajes en los siguientes estados
        if (doomCanvas->state == ST_DYING) {
            updateTime = false;
        }
    }

    if (updateTime) {
        if ((doomCanvas->hud->msgCount > 0) && ((doomCanvas->time - doomCanvas->hud->msgTime) > (doomCanvas->hud->msgDuration + 150))) {
            Hud_shiftMsgs(doomCanvas);
        }
    }

    strBeg = 0;
    if (doomCanvas->hud->msgCount > 0)
    {
        text = doomCanvas->hud->messages[0];
        if (updateTime) {
            if (doomCanvas->hud->msgDuration < (doomCanvas->time - doomCanvas->hud->msgTime)) {
                return;
            }
        
            len = SDL_strlen(text) - doomCanvas->hud->msgMaxChars;

            if (len > 0) {
                time = (doomCanvas->time - doomCanvas->hud->msgTime);
                if (time > SCROLL_START_DELAY)
                {
                    strBeg = ((unsigned int)((time - SCROLL_START_DELAY) / 100));
                    if (strBeg > len - 1) {
                        strBeg = len - 1;
                    }
                }
            }
        }
    }
    else if (doomCanvas->hud->statBarMessage != NULL) {
        text = doomCanvas->hud->statBarMessage;
    }
    else if (doomCanvas->hud->logMessage[0] != '\0') {
        text = doomCanvas->hud->logMessage;
    }
    else if ((doomCanvas->state == ST_PLAYING) && (doomCanvas->player->facingEntity) && (doomCanvas->player->facingEntity->def->eType != 9)) {
        text = doomCanvas->player->facingEntity->def->name;
    }
    else {
        return;
    }  

    strEnd = SDL_strlen(text);
    w = 400;
    if (((strEnd * 9) + 10) > w) {
        strEnd = ((unsigned int)((w - 1) / 7)) - 1;
    }

    DoomCanvas_drawFont(doomCanvas, text, 1, (doomCanvas->hud->statusTopBarHeight >> 1) - 5, 0, strBeg, strEnd, false);
}

void Hud_finishMessageBufferForce(DoomCanvas_t* doomCanvas, boolean force)
{
    Hud_t* hud = doomCanvas->hud;
    hud->msgCount++;
    if (hud->msgCount == 1) {
        Hud_calcMsgTime(doomCanvas);
        if (force) {
            hud->msgDuration *= 2;
        }
    }
}

void Hud_finishMessageBuffer(DoomCanvas_t* doomCanvas)
{
    Hud_finishMessageBufferForce(doomCanvas, false);
}

char* Hud_getMessageBufferForce(DoomCanvas_t* doomCanvas, boolean force)
{
    Hud_t* hud = doomCanvas->hud;
    if (force) {
        hud->msgCount = 0;
    }
    if (hud->msgCount == MAX_MESSAGES) {
        Hud_shiftMsgs(doomCanvas);
    }
    hud->messages[hud->msgCount][0] = '\0';
    return hud->messages[hud->msgCount];
}

char* Hud_getMessageBuffer(DoomCanvas_t* doomCanvas)
{
    return Hud_getMessageBufferForce(doomCanvas, false);
}

void Hud_shiftMsgs(DoomCanvas_t* doomCanvas)
{
    Hud_t* hud = doomCanvas->hud;
    int i;

    for (i = 0; i < (hud->msgCount - 1); i++) {
        strncpy(hud->messages[i], hud->messages[i + 1], MS_PER_CHAR);
    }

    hud->msgCount--;
    hud->messages[hud->msgCount][0] = '\0';
    if (hud->msgCount > 0) {
        Hud_calcMsgTime(doomCanvas);
    }
}

void Hud_startup(Hud_t* hud, boolean largeStatus)
{
	DoomRPG_t* doomRpg;
    int height = 0;

	doomRpg = hud->doomRpg;

	hud->msgMaxChars = (400 - 4) / 7;
	hud->statusTopBarHeight = 20;
    DoomRPG_createImage(doomRpg, "bar_lg.bmp", false, &hud->imgStatusBarLarge);
    DoomRPG_createImage(doomRpg, "k.bmp", false, &hud->imgStatusBar);
    DoomRPG_createImage(doomRpg, "n.bmp", true, &hud->imgAttArrow);
    DoomRPG_createImage(doomRpg, "o.bmp", true, &hud->imgStatusArrow);

    if (largeStatus == 0) {
        hud->largeHud = false;
        DoomRPG_createImage(doomRpg, "l.bmp", false, &hud->imgHudFaces);
        DoomRPG_createImage(doomRpg, "m.bmp", true, &hud->imgIconSheet);
        hud->statusArmorXpos = 35;
        hud->statusHudFacesXpos = 66;
        hud->statusAmmoXpos = 85;
        hud->statusOrientationXpos = 126;
        hud->statusOrientationArrowXpos = 128;
        hud->statusLine1Xpos = 33;
        hud->statusLine2Xpos = 155;
        hud->statusHealthXpos = 2;
        height = hud->imgStatusBar.height;
    }
    else {
        hud->largeHud = true;
        DoomRPG_createImage(doomRpg, "larger HUD faces.bmp", false, &hud->imgHudFaces);
        DoomRPG_createImage(doomRpg, "larger_HUD_icon_sheet.bmp", true, &hud->imgIconSheet);
        hud->statusArmorXpos = 46;
        hud->statusHudFacesXpos = 88;
        hud->statusAmmoXpos = 115;
        hud->statusOrientationXpos = 171;
        hud->statusOrientationArrowXpos = 172;
        hud->statusLine1Xpos = 44;
        hud->statusLine2Xpos = 158;
        hud->statusHealthXpos = 2;
        height = hud->imgStatusBarLarge.height;
    }
    hud->statusBarHeight = height;

    hud->hudFaceWidth = hud->imgHudFaces.width;
    hud->hudFaceHeight = (hud->imgHudFaces.height / 9);
    hud->iconSheetWidth = hud->imgIconSheet.width;
    hud->iconSheetHeight = (hud->imgIconSheet.height / 9);
}