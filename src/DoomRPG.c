
//Using SDL and standard IO
#ifdef __3DS__
#include <3ds.h>
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#else
#include <SDL.h>
#endif
#include <stdio.h>


#include "Z_Zone.h"
#include "DoomRPG.h"
#include "DoomCanvas.h"
#include "Render.h"
#include "Menu.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "Sound.h"
#include "EntityDef.h"
#include "Game.h"
#include "Player.h"
#include "ParticleSystem.h"
#include "Combat.h"
#include "SDL_Video.h"
#include "Z_Zip.h"


DoomRPG_t* doomRpg = NULL;

keyMapping_t keyMapping[12];
keyMapping_t keyMappingTemp[12];
keyMapping_t keyMappingDefault[12] = {
	{AVK_UP | AVK_MENU_UP,				{KEY_DUP,-1,-1,-1,-1,-1,-1,-1,-1,-1}},	// Move forward
	{AVK_DOWN | AVK_MENU_DOWN,			{KEY_DDOWN,-1,-1,-1,-1,-1,-1,-1,-1,-1}},	// Move backward
	{AVK_LEFT | AVK_MENU_PAGE_UP,		{KEY_DLEFT,-1,-1,-1,-1,-1,-1,-1,-1,-1}},	// Turn left/page up
	{AVK_RIGHT | AVK_MENU_PAGE_DOWN,	{KEY_DRIGHT,-1,-1,-1,-1,-1,-1,-1,-1,-1}},	// Turn right/page down
	{AVK_MOVELEFT,						{KEY_LEFT,-1,-1,-1,-1,-1,-1,-1,-1,-1}},		// Move left
	{AVK_MOVERIGHT,					{KEY_RIGHT,-1,-1,-1,-1,-1,-1,-1,-1,-1}},		// Move right
	{AVK_NEXTWEAPON,					{KEY_R,-1,-1,-1,-1,-1,-1,-1,-1,-1}},		// Next weapon
	{AVK_PREVWEAPON,					{KEY_L,-1,-1,-1,-1,-1,-1,-1,-1,-1}},		// Prev weapon
	{AVK_SELECT | AVK_MENU_SELECT,		{KEY_A,-1,-1,-1,-1,-1,-1,-1,-1,-1}},// Attack/Talk/Use
	{AVK_PASSTURN,						{KEY_B,-1,-1,-1,-1,-1,-1,-1,-1,-1}},		// Pass Turn
	{AVK_AUTOMAP,						{KEY_SELECT,-1,-1,-1,-1,-1,-1,-1,-1,-1}},	// Automap
	{AVK_MENUOPEN | AVK_MENU_OPEN,		{KEY_START,-1,-1,-1,-1,-1,-1,-1,-1,-1}}	// Open menu/back
};

#include <stdarg.h> //va_list|va_start|va_end

// Необходимо подключить заголовочные файлы для работы с графикой и вводом на 3DS
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> // Для добавления времени в лог

#ifdef __3DS__
#include <3ds.h>
#endif

void DoomRPG_Error(const char* fmt, ...)
{
	char errMsg[256];
	va_list ap;

	// 1. Формируем сообщение об ошибке
	va_start(ap, fmt);
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
	va_end(ap);

	// 2. Выводим в стандартный поток (полезно для gdb)
	printf("FATAL ERROR: %s\n", errMsg);

#ifdef __3DS__
	// 3. Срём в лог-файл на SD-карте
	FILE* logFile = fopen("/doomrpg_error.log", "a"); // "a" - дописывать в конец файла
	if (logFile)
	{
		time_t now = time(NULL);
		char timeStr[64];
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
		fprintf(logFile, "[%s] FATAL ERROR: %s\n", timeStr, errMsg);
		fclose(logFile);
	}

	// 4. Отображаем ошибку на экране
	// Инициализируем графику только для экрана ошибки.
	// В идеале gfxInitDefault() должен быть в main(), но это "защита от дурака",
	// если вдруг графика не была включена.
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	consoleClear(); // <<<<<<< ВОТ ЭТО ВАЖНО! Очищаем экран от мусора.

	// Рисуем красивую красную рамку (чисто для стиля)
	printf("\x1b[31;1m"); // Включить ярко-красный цвет
	printf("\x1b[1;1H**************************************************");
	for (int i = 2; i < 29; i++) {
		printf("\x1b[%d;1H*", i);
		printf("\x1b[%d;50H*", i);
	}
	printf("\x1b[29;1H**************************************************");
	printf("\x1b[0m"); // Сброс цвета

	// Выводим саму ошибку
	printf("\x1b[3;15H--== DoomRPG Error ==--");
	printf("\x1b[6;3H%s", errMsg); // Вывод отформатированного сообщения
	printf("\x1b[27;10HPress START button to exit");

	// Простой цикл ожидания, чтобы не засирать главный цикл приложения
	while (true)
	{
		hidScanInput();
		if (hidKeysDown() & KEY_START)
		{
			break; // Выход по кнопке START
		}

		// Обновление экрана
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	// Выход из графического режима ошибки
	gfxExit();

#else
	// Код для ПК (SDL) остается без изменений
	const SDL_MessageBoxButtonData buttons[] = {
		{ 0, 0, "Ok" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{ /* .colors (.r, .g, .b) */
			{ 255, 0, 0 }, { 0, 255, 0 }, { 255, 255, 0 },
			{ 0, 0, 255 }, { 255, 0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR, NULL, "DoomRPG Error", errMsg,
		SDL_arraysize(buttons), buttons, &colorScheme
	};
	SDL_ShowMessageBox(&messageboxdata, NULL);
#endif

	// 5. Освобождаем ресурсы и выходим
	// Этот код теперь выполнится после нажатия START на 3DS
	// closeZipFile(&zipFile); // Убедись что zipFile это глобальная переменная или передается
	// DoomRPG_FreeAppData(doomRpg); // То же самое для doomRpg
	// SDL_CloseAudio();
	// SDL_Close();
}

//
// Fixme. __USE_C_FIXED__ or something.
//

fixed_t	DoomRPG_FixedMul(fixed_t a, fixed_t b)
{
	return (fixed_t)((int64_t)a * b >> FRACBITS);
	//return ((long long)a * (long long)b) >> FRACBITS;
}

//
// FixedDiv, C version.
//

fixed_t	DoomRPG_FixedDiv(fixed_t a, fixed_t b)
{
	// CALICO: rewritten to use PC version
	return (ABS(a) >> 14) >= ABS(b) ? ((a ^ b) >> 31) ^ MAXINT :
		(fixed_t)(((int64_t)a << FRACBITS) / b);

	//if ((abs(a) >> 14) >= abs(b))
		//return (a ^ b) < 0 ? MININT : MAXINT;
	//return DoomRPG_FixedDiv2(a, b);
}

/*
fixed_t	DoomRPG_FixedDiv2(fixed_t a, fixed_t b)
{
#if 0
	long long c;
	c = ((long long)a << 16) / ((long long)b);
	return (fixed_t)c;
#else
	double c;

	c = ((double)a) / ((double)b) * FRACUNIT;

	if (c >= 2147483648.0 || c < -2147483648.0) {
		printf("FixedDiv: divide by zero"); while (1) {}
	}

	return (fixed_t)c;
#endif
}
*/

static Uint32 basetime = 0;

//
// Get time in milliseconds
//
unsigned int DoomRPG_GetTimeMS(void)
{
	Uint32 ticks = SDL_GetTicks();

	if (basetime == 0)
		basetime = ticks;

	return ticks;
}

//
// Get time in milliseconds
//
unsigned int DoomRPG_GetUpTimeMS(void)
{
	Uint32 ticks = SDL_GetTicks();

	if (basetime == 0)
		basetime = ticks;

	return ticks - basetime;
}

int DoomRPG_freeMemory(void) { // 0x1EBFC
	return Z_FreeMemory();
}

// New Function
#ifdef __3DS__
int DoomRPG_getEventKey(int mouse_Button, const Uint8* state) {

    int key = AVK_UNDEFINED;
    int i, j;

    int buttonID = hidKeysDown();

    if (buttonID != -1)
    {
        int bindCode = buttonID;

        for (i = 0; i < (sizeof(keyMapping) / sizeof(keyMapping_t)); ++i) {
            for (j = 0; j < KEYBINDS_MAX; j++) {
                if ((keyMapping[i].keyBinds[j]))
                {
                    if (keyMapping[i].keyBinds[j] == bindCode) {
                        key = keyMapping[i].avk_action;
                        goto found_key; // Нашли бинд, переходим к концу
                    }
                }
            }
        }
    }

found_key:
    return key;
}
#else
int DoomRPG_getEventKey(int mouse_Button, const Uint8* state) {

	int i, j, key, num, buttomID;

	key = AVK_UNDEFINED;

	// KeyBoard
	{
#ifdef __3DS__
#else
		for (i = SDL_SCANCODE_1; i <= SDL_SCANCODE_0; ++i) {
			if (state[i]) {
				num = ((i - SDL_SCANCODE_0) + 10) % 10;
				key = (AVK_0 + num);
				break;
			}
		}

		for (i = SDL_SCANCODE_KP_1; i <= SDL_SCANCODE_KP_0; ++i) {
			if (state[i]) {
				num = ((i - SDL_SCANCODE_KP_0) + 10) % 10;
				key = (AVK_0 + num);
				break;
			}
		}

		if (key) {
			return key;
		}

		for (i = 0; i < (sizeof(keyMapping) / sizeof(keyMapping_t)); ++i) {
			for (j = 0; j < KEYBINDS_MAX; j++) {
				if (!(keyMapping[i].keyBinds[j] & (IS_CONTROLLER_BUTTON | IS_MOUSE_BUTTON)))
				{
					if (state[keyMapping[i].keyBinds[j]]) {
						key = keyMapping[i].avk_action;
						break;
					}
				}
			}

			if (key) {
				break;
			}
		}

		// Open Menu/Back
		if (state[SDL_SCANCODE_ESCAPE]) {
			key |= AVK_MENU_OPEN;
		}

		// Menu select
		if (state[SDL_SCANCODE_RETURN]) {
			key |= AVK_MENU_SELECT;
		}

		// Menu up
		if (state[SDL_SCANCODE_UP]) {
			key |= AVK_MENU_UP;
		}

		// Menu down
		if (state[SDL_SCANCODE_DOWN]) {
			key |= AVK_MENU_DOWN;
		}

		// Menu page up
		if (state[SDL_SCANCODE_LEFT]) {
			key |= AVK_MENU_PAGE_UP;
		}

		// Menu page down
		if (state[SDL_SCANCODE_RIGHT]) {
			key |= AVK_MENU_PAGE_DOWN;
		}
#endif

		if (key) {
			return key;
		}
	}


	// Mouse
	if (mouse_Button != MOUSE_BUTTON_INVALID) {
		for (i = 0; i < (sizeof(keyMapping) / sizeof(keyMapping_t)); ++i) {
			for (j = 0; j < KEYBINDS_MAX; j++) {
				if ((keyMapping[i].keyBinds[j] & IS_MOUSE_BUTTON))
				{
					if (mouse_Button == (keyMapping[i].keyBinds[j] & ~(IS_CONTROLLER_BUTTON | IS_MOUSE_BUTTON))) {
						key = keyMapping[i].avk_action;
						break;
					}
				}
			}

			if (key) {
				break;
			}
		}

		// Menu up
		if (mouse_Button == MOUSE_BUTTON_WHELL_UP) {
			key |= AVK_MENU_UP;
		}

		// Menu down
		if (mouse_Button == MOUSE_BUTTON_WHELL_DOWN) {
			key |= AVK_MENU_DOWN;
		}

		// Move Up
		if (mouse_Button == MOUSE_BUTTON_MOTION_UP) {
			key = AVK_UP;
		}

		// Move Down
		if (mouse_Button == MOUSE_BUTTON_MOTION_DOWN) {
			key = AVK_DOWN;
		}

		// Turn Left
		if (mouse_Button == MOUSE_BUTTON_MOTION_LEFT) {
			key = AVK_LEFT;
		}

		// Turn Right
		if (mouse_Button == MOUSE_BUTTON_MOTION_RIGHT) {
			key = AVK_RIGHT;
		}

		if (key) {
			return key;
		}
	}

	// GameController/Joystick
	{
#ifdef __3DS__
#else
		if (sdlController.gGameController) {
			buttomID = SDL_GameControllerGetButtonID();
		}
		else {
			buttomID = SDL_JoystickGetButtonID();
		}
#endif

		if (buttomID != -1) {
			for (i = 0; i < (sizeof(keyMapping) / sizeof(keyMapping_t)); ++i) {
				for (j = 0; j < KEYBINDS_MAX; j++) {
					if ((keyMapping[i].keyBinds[j] & IS_CONTROLLER_BUTTON))
					{
						if (buttomID == (keyMapping[i].keyBinds[j] & ~(IS_CONTROLLER_BUTTON | IS_MOUSE_BUTTON))) {
							key = keyMapping[i].avk_action;
							break;
						}
					}
				}

				if (key) {
					break;
				}
			}

#if 0
			// Open Menu/Back
			if (buttomID == CONTROLLER_BUTTON_START) {
				key |= AVK_MENU_OPEN;
			}

			// Menu select
			if (buttomID == CONTROLLER_BUTTON_A) {
				key |= AVK_MENU_SELECT;
			}
#endif

			// Menu up
			if ((buttomID == CONTROLLER_BUTTON_DPAD_UP) || (buttomID == CONTROLLER_BUTTON_LAXIS_UP)) {
				key |= AVK_MENU_UP;
			}

			// Menu down
			if ((buttomID == CONTROLLER_BUTTON_DPAD_DOWN) || (buttomID == CONTROLLER_BUTTON_LAXIS_DOWN)) {
				key |= AVK_MENU_DOWN;
			}

			// Menu page up
			if ((buttomID == CONTROLLER_BUTTON_DPAD_LEFT) || (buttomID == CONTROLLER_BUTTON_LAXIS_LEFT)) {
				key |= AVK_MENU_PAGE_UP;
			}

			// Menu page down
			if ((buttomID == CONTROLLER_BUTTON_DPAD_RIGHT) || (buttomID == CONTROLLER_BUTTON_LAXIS_RIGHT)) {
				key |= AVK_MENU_PAGE_DOWN;
			}
		}

		if (key) {
			return key;
		}
	}

	return key;
}
#endif
// New Function
void DoomRPG_setDefaultBinds(DoomRPG_t* doomrpg)
{
	SDL_memcpy(keyMapping, keyMappingDefault, sizeof(keyMapping));
	SDL_memcpy(keyMappingTemp, keyMappingDefault, sizeof(keyMapping));
}

// New Function
static void unBind(int* keyBinds, int index)
{
	int temp[KEYBINDS_MAX], next, i;

	next = 0;
	keyBinds[index] = -1;

	// Reorder the list
	for (i = 0; i < KEYBINDS_MAX; i++) {
		temp[i] = -1;
		if (keyBinds[i] == -1) {
			continue;
		}
		temp[next++] = keyBinds[i];
	}

	SDL_memcpy(keyBinds, temp, sizeof(temp));
}

// New Function
static void setBind(int* keyBinds, int keycode)
{
	int i;

	// Examina si existe anteriormente, si es as�, se desvincular� de la lista
	// Examines whether it exists previously, if so, it will be unbind from the list
	for (i = 0; i < KEYBINDS_MAX; i++) {
		if (keyBinds[i] == keycode) {
			unBind(keyBinds, i);
			return;
		}
	}

	// Se guarda el key code en la lista
	// The key code is saved in the list
	for (i = 0; i < KEYBINDS_MAX; i++) {
		if (keyBinds[i] == -1) {
			keyBinds[i] = keycode;
			return;
		}
	}
}

// New Function
void DoomRPG_setBind(DoomRPG_t* doomrpg, int mouse_Button, const Uint8* state) {
	int i, keyMapId, buttomID;

	// KeyBoard
	{
#ifdef __3DS__
		int buttonID = SDL_JoystickGetButtonID();
		if (buttonID != -1)
		{
			// Кнопка найдена! Привязываем ее к действию.
			int keyMapId = doomrpg->menuSystem->items[doomrpg->menuSystem->selectedIndex].action;

			// ВАЖНО: Мы добавляем смещение, чтобы игра могла отличить
			// кнопку геймпада от клавиши клавиатуры. 512 - безопасное число,
			// так как кодов клавиш меньше.
			const int CONTROLLER_BIND_OFFSET = 512;
			setBind(keyMappingTemp[keyMapId].keyBinds, buttonID + CONTROLLER_BIND_OFFSET);

			// Сбрасываем флаги и выходим
			doomrpg->menuSystem->setBind = false;
			doomrpg->menuSystem->paintMenu = true;
			return;
		}
#else
		for (i = 0; i < SDL_NUM_SCANCODES; ++i) {
			if (state[i]) {
				keyMapId = doomrpg->menuSystem->items[doomrpg->menuSystem->selectedIndex].action;
				setBind(keyMappingTemp[keyMapId].keyBinds, i);
				doomrpg->menuSystem->setBind = false;
				doomrpg->menuSystem->paintMenu = true;
				return;
			}
		}
#endif
	}

	// Mouse
	if (mouse_Button != MOUSE_BUTTON_INVALID) {
		keyMapId = doomrpg->menuSystem->items[doomrpg->menuSystem->selectedIndex].action;
		setBind(keyMappingTemp[keyMapId].keyBinds, mouse_Button | IS_MOUSE_BUTTON);
		doomrpg->menuSystem->setBind = false;
		doomrpg->menuSystem->paintMenu = true;
		return;
	}

	// GameController/Joystick
	{
		for (i = 0; i < SDL_NumJoysticks(); ++i) {
#ifdef __3DS__
			buttomID = SDL_JoystickGetButtonID();
			if (buttomID != -1) {
				break; // Если нашли нажатие, выходим из цикла (хотя на 3DS итерация всего одна)
			}
#else
			if (SDL_IsGameController(i))
			{
				buttomID = SDL_GameControllerGetButtonID();
				if (buttomID != -1) {
					break;
				}
			}
			else
			{
				buttomID = SDL_JoystickGetButtonID();
				if (buttomID != -1) {
					break;
				}
			}
#endif
		}

		if (buttomID != -1) {
			keyMapId = doomrpg->menuSystem->items[doomrpg->menuSystem->selectedIndex].action;
			setBind(keyMappingTemp[keyMapId].keyBinds, buttomID | IS_CONTROLLER_BUTTON);
			doomrpg->menuSystem->setBind = false;
			doomrpg->menuSystem->paintMenu = true;
		}
	}
}
int DoomRPG_Init(void) // 0x3141C
{
	int mem;
	printf("DoomRpg_Init\n");

	doomRpg = SDL_malloc(sizeof(DoomRPG_t));
	doomRpg->memoryBeg = DoomRPG_freeMemory();
	doomRpg->imageMemory = 0;
	doomRpg->errorID = 0;
	doomRpg->upTimeMs = 0;
	doomRpg->graphSetCliping = false;
	doomRpg->closeApplet = false;

	// Port: set default Binds
	DoomRPG_setDefaultBinds(doomRpg);

	mem = DoomRPG_freeMemory();
	doomRpg->doomCanvas = DoomCanvas_init(NULL, doomRpg);
	if (doomRpg->doomCanvas)
	{
		doomRpg->doomCanvas->memory = DoomRPG_freeMemory() - mem;
		DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

		mem = DoomRPG_freeMemory();
		doomRpg->render = Render_init(NULL, doomRpg);
		if (doomRpg->render)
		{
			doomRpg->render->memory = DoomRPG_freeMemory() - mem;
			DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

			mem = DoomRPG_freeMemory();
			doomRpg->menu = Menu_init(NULL, doomRpg);
			if (doomRpg->menu)
			{
				doomRpg->menu->memory = DoomRPG_freeMemory() - mem;
				DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

				mem = DoomRPG_freeMemory();
				doomRpg->menuSystem = MenuSystem_init(NULL, doomRpg);
				if (doomRpg->menuSystem)
				{
					doomRpg->menuSystem->memory = DoomRPG_freeMemory() - mem;
					DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

					mem = DoomRPG_freeMemory();
					doomRpg->hud = Hud_init(NULL, doomRpg);
					if (doomRpg->hud)
					{
						doomRpg->hud->memory = DoomRPG_freeMemory() - mem;
						DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

						doomRpg->sound = Sound_init(NULL, doomRpg);
						if (doomRpg->sound)
						{
							mem = DoomRPG_freeMemory();
							doomRpg->entityDef = EntityDef_init(NULL, doomRpg);
							if (doomRpg->entityDef)
							{
								doomRpg->entityDef->memory = DoomRPG_freeMemory() - mem;
								DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

								mem = DoomRPG_freeMemory();
								doomRpg->game = Game_init(NULL, doomRpg);
								if (doomRpg->game)
								{
									doomRpg->game->memory = DoomRPG_freeMemory() - mem;
									DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

									mem = DoomRPG_freeMemory();
									doomRpg->player = Player_init(NULL, doomRpg);
									if (doomRpg->player)
									{
										doomRpg->player->memory = DoomRPG_freeMemory() - mem;
										DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

										mem = DoomRPG_freeMemory();
										doomRpg->particleSystem = ParticleSystem_init(NULL, doomRpg);
										if (doomRpg->particleSystem)
										{
											doomRpg->particleSystem->memory = DoomRPG_freeMemory() - mem;
											DoomCanvas_updateLoadingBar(doomRpg->doomCanvas);

											mem = DoomRPG_freeMemory();
											doomRpg->combat = Combat_init(NULL, doomRpg);
											if (doomRpg->combat)
											{
												doomRpg->combat->memory = DoomRPG_freeMemory() - mem;

												//printf("doomCanvas->memory %d\n", doomRpg->doomCanvas->memory);
												//printf("render->memory %d\n", doomRpg->render->memory);
												//printf("menu->memory %d\n", doomRpg->menu->memory);
												//printf("menuSystem->memory %d\n", doomRpg->menuSystem->memory);
												//printf("hud->memory %d\n", doomRpg->hud->memory);
												//printf("entityDef->memory %d\n", doomRpg->entityDef->memory);
												//printf("game->memory %d\n", doomRpg->game->memory);
												//printf("player->memory %d\n", doomRpg->player->memory);
												//printf("particleSystem->memory %d\n", doomRpg->particleSystem->memory);
												//printf("combat->memory %d\n", doomRpg->combat->memory);

												DoomCanvas_startup(doomRpg->doomCanvas);
												ParticleSystem_startup(doomRpg->particleSystem);
												MenuSystem_startup(doomRpg->menuSystem);

												if (EntityDef_startup(doomRpg->entityDef))
												{
													if (Render_startup(doomRpg->render))
													{
														Game_loadConfig(doomRpg->game);

														if (doomRpg->doomCanvas->skipIntro == false) {
															DoomCanvas_setState(doomRpg->doomCanvas, ST_LEGALS);
														}
														else {
															DoomCanvas_setupmenu(doomRpg->doomCanvas, true);
														}

														return 1;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

void DoomRPG_FreeAppData(DoomRPG_t* doomrpg)
{
	if (doomRpg == NULL)
		return;

	if (doomrpg->combat) {
		Combat_free(doomrpg->combat, true);
	}
	doomrpg->combat = NULL;

	if (doomrpg->particleSystem) {
		ParticleSystem_free(doomrpg->particleSystem, true);
	}
	doomrpg->particleSystem = NULL;

	if (doomrpg->player) {
		SDL_memset(&doomrpg->player->ce, 0, sizeof(doomrpg->player->ce));
		SDL_free(doomrpg->player);
	}
	doomrpg->player = NULL;

	if (doomrpg->game) {
		SDL_memset(&doomrpg->game->entityMonsters, 0, sizeof(doomrpg->game->entityMonsters));
		SDL_memset(&doomrpg->game->entities, 0, sizeof(doomrpg->game->entities));
		SDL_free(doomrpg->game);
	}
	doomrpg->game = NULL;

	if (doomrpg->entityDef) {
		EntityDef_free(doomrpg->entityDef, true);
	}
	doomrpg->entityDef = NULL;

	if (doomrpg->sound) {
		Sound_free(doomrpg->sound, true);
	}
	doomrpg->sound = NULL;

	if (doomrpg->hud) {
		Hud_free(doomrpg->hud, true);
	}
	doomrpg->hud = NULL;

	if (doomrpg->menuSystem) {
		MenuSystem_free(doomrpg->menuSystem, true);
	}
	doomrpg->menuSystem = NULL;

	if (doomrpg->menu) {
		SDL_free(doomrpg->menu);
	}
	doomrpg->menu = NULL;

	if (doomrpg->render) {
		Render_free(doomrpg->render, true);
	}
	doomrpg->render = NULL;

	if (doomrpg->doomCanvas) {
		DoomCanvas_free(doomrpg->doomCanvas, true);
	}
	doomrpg->doomCanvas = NULL;

	SDL_free(doomrpg);
}

void DoomRPG_createImage(DoomRPG_t* doomrpg, const char* resourceName, boolean isTransparentMask, Image_t* img)
{
	SDL_RWops* rw = NULL;
	SDL_Surface* tempSurface = NULL;      // Временная поверхность для загрузки
	//SDL_Surface* optimizedSurface = NULL; // Финальная, оптимизированная поверхность
	char fileName[64];
	int mem;

	mem = DoomRPG_freeMemory();
	snprintf(fileName, sizeof(fileName), "%s", resourceName);

	byte* fdata;
	int fSize;
	fdata = readZipFileEntry(fileName, &zipFile, &fSize);
	if (!fdata) {
		DoomRPG_Error("Failed to read file %s from zip.", fileName);
		return;
	}

	rw = SDL_RWFromMem(fdata, fSize);
	if (!rw) {
		DoomRPG_Error("Error with SDL_RWFromMem: %s\n", SDL_GetError());
		SDL_free(fdata);
		return;
	}

	// 1. Загружаем BMP из памяти во временную поверхность
	tempSurface = SDL_LoadBMP_RW(rw, SDL_TRUE); // SDL_TRUE автоматически закроет rw
	if (!tempSurface) {
		DoomRPG_Error("Unable to load image %s! SDL Error: %s\n", fileName, SDL_GetError());
		SDL_free(fdata);
		return;
	}
	if (isTransparentMask) {
		Uint32 colorKey = SDL_MapRGB(tempSurface->format, 255, 0, 255);

		SDL_SetColorKey(tempSurface, SDL_SRCCOLORKEY, colorKey);
	}

	img->isTransparentMask = isTransparentMask;
	img->width = tempSurface->w;
	img->height = tempSurface->h;
	img->imgBitmap = tempSurface;
	SDL_free(fdata);
}



void DoomRPG_createImageBerserkColor(DoomRPG_t* doomrpg, const char* resourceName, boolean isTransparentMask, Image_t* img)
{
	SDL_RWops* rw = NULL;
#ifdef __3DS__
	SDL_Surface* loadedSurface = NULL;
#else
	SDL_Texture* newTexture = NULL;
	SDL_Surface* loadedSurface = NULL;
#endif
	char fileName[64];
	int mem;
	int r, g, b;

	mem = DoomRPG_freeMemory();

	snprintf(fileName, sizeof(fileName), "%s", resourceName);

	byte* fdata;
	int fSize;
	fdata = readZipFileEntry(fileName, &zipFile, &fSize);

	rw = SDL_RWFromMem(fdata, fSize);
	if (!rw) {
		DoomRPG_Error("Error with SDL_RWFromMem: %s\n", SDL_GetError());
	}

#ifdef __3DS__
	loadedSurface = SDL_LoadBMP_RW(rw, SDL_TRUE);
	if (!loadedSurface) {
		DoomRPG_Error("Unable to load image %s! SDL Error: %s\n", fileName, SDL_GetError());
	}

	if (isTransparentMask) {
		Uint32 colorKey = SDL_MapRGB(loadedSurface->format, 255, 0, 255);
		SDL_SetColorKey(loadedSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	}

	SDL_PixelFormat* fmt = loadedSurface->format;
	if (fmt && fmt->palette) {
		for (int i = 0; i < fmt->palette->ncolors; i++) {
			int color = ((fmt->palette->colors[i].r >> 3) << 11) | ((fmt->palette->colors[i].g >> 2) << 5) | (fmt->palette->colors[i].b >> 3);

			r = ((color >> 11) & 0x1f) + 8;
			if (r > 31) r = 31;

			color = ((color & 0xfffff7df) >> 1) & 0x7ff | (r << 11);

			r = (color >> 11) & 0x1f;
			g = (color >> 5) & 0x3f;
			b = (color & 0x1f);

			fmt->palette->colors[i].r = (r << 3) | (r >> 2);
			fmt->palette->colors[i].g = (g << 2) | (g >> 4);
			fmt->palette->colors[i].b = (b << 3) | (b >> 2);
		}
	}

	img->isTransparentMask = isTransparentMask;
	img->width = loadedSurface->w;
	img->height = loadedSurface->h;
	img->imgBitmap = loadedSurface;

#else
	loadedSurface = SDL_LoadBMP_RW(rw, SDL_TRUE);
	if (!loadedSurface) {
		DoomRPG_Error("Unable to load image %s! SDL Error: %s\n", fileName, SDL_GetError());
	}

	if (isTransparentMask) {
		Uint32 colorKey = SDL_MapRGB(loadedSurface->format, 255, 0, 255);
		SDL_SetColorKey(loadedSurface, SDL_TRUE, colorKey);
	}

	SDL_PixelFormat* fmt = loadedSurface->format;
	if (fmt && fmt->palette) {
		for (int i = 0; i < fmt->palette->ncolors; i++) {
			int color = ((fmt->palette->colors[i].r >> 3) << 11) | ((fmt->palette->colors[i].g >> 2) << 5) | (fmt->palette->colors[i].b >> 3);

			r = ((color >> 11) & 0x1f) + 8;
			if (r > 31) r = 31;

			color = ((color & 0xfffff7df) >> 1) & 0x7ff | (r << 11);

			r = (color >> 11) & 0x1f;
			g = (color >> 5) & 0x3f;
			b = (color & 0x1f);

			fmt->palette->colors[i].r = (r << 3) | (r >> 2);
			fmt->palette->colors[i].g = (g << 2) | (g >> 4);
			fmt->palette->colors[i].b = (b << 3) | (b >> 2);
		}
	}

	newTexture = SDL_CreateTextureFromSurface(sdlVideo.renderer, loadedSurface);
	if (!newTexture) {
		DoomRPG_Error("Unable to create texture from %s! SDL Error: %s\n", fileName, SDL_GetError());
	}

	img->isTransparentMask = isTransparentMask;
	img->width = loadedSurface->w;
	img->height = loadedSurface->h;
	img->imgBitmap = newTexture;

	SDL_FreeSurface(loadedSurface);
#endif

	SDL_free(fdata);
	doomrpg->imageMemory += (DoomRPG_freeMemory() - mem);
}

void DoomRPG_freeImage(DoomRPG_t* doomrpg, Image_t* image)
{
	int mem = DoomRPG_freeMemory();
#ifdef __3DS__
	if (image->imgBitmap) {
		SDL_FreeSurface((SDL_Surface*)image->imgBitmap);
		image->imgBitmap = NULL;
	}
#else
	if (image->imgBitmap) {
		SDL_DestroyTexture((SDL_Texture*)image->imgBitmap);
		image->imgBitmap = NULL;
	}
#endif
	doomrpg->imageMemory -= (mem - DoomRPG_freeMemory());
}


byte *DoomRPG_fileOpenRead(DoomRPG_t* doomrpg, const char* resourceName)
{
	SDL_RWops* rw;
	byte *fdata;
	int fSize;
	char fileName[64];

	/*sprintf_s(fileName, sizeof(fileName), "BarData%s", resourceName);

	rw = SDL_RWFromFile(fileName, "r");
	if (rw == NULL) {
		DoomRPG_Error("DoomRPG_fileOpenRead: cannot open file %s", fileName);
	}

	fSize = (int)SDL_RWsize(rw);

	fdata = (byte*)SDL_malloc(fSize);
	if (fdata == NULL) {
		DoomRPG_Error("DoomRPG_fileOpenRead: Insufficient memory for allocation");
	}

	SDL_RWread(rw, fdata, fSize, 1);

	SDL_RWclose(rw);*/

	snprintf(fileName, sizeof(fileName), "%s", resourceName+1);

	fdata = readZipFileEntry(fileName, &zipFile, &fSize);

	return fdata;
}

void DoomRPG_setErrorID(DoomRPG_t* doomrpg, int ID)
{
	doomrpg->errorID = ID;
}

int DoomRPG_getErrorID(DoomRPG_t* doomrpg)
{
	return doomrpg->errorID;
}

byte DoomRPG_byteAtNext(byte* data, int* posData)
{
	int pos = *posData + 1;
	*posData = pos;
	return (byte)DoomRPG_byteAt(data, pos - 1);
}

byte DoomRPG_byteAt(byte* data, int posData)
{
	return (byte)(data[posData + 0]);
}

short DoomRPG_shortAtNext(byte* data, int* posData)
{
	int pos = *posData + 2;
	*posData = pos;
	return (short)DoomRPG_shortAt(data, pos - 2);
}

short DoomRPG_shortAt(byte* data, int posData)
{
	return (short)((data[posData + 1] << 8) | data[posData + 0]);
}

int DoomRPG_intAtNext(byte* data, int* posData)
{
	int pos = *posData + 4;
	*posData = pos;
	return DoomRPG_intAt(data, pos - 4);
}

int DoomRPG_intAt(byte* data, int posData)
{
	return (int)((data[posData + 3] << 24) | (data[posData + 2] << 16) | (data[posData + 1] << 8) | data[posData + 0]);
}

short DoomRPG_shiftCoordAt(byte* data, int* posData)
{
	int pos = *posData + 1;
	*posData = pos;
	return DoomRPG_byteAt(data, pos - 1) << 3;
}
void DoomRPG_setClipFalse(DoomRPG_t* doomrpg)
{
    SDL_RenderSetClipRect(sdlVideo.screenSurface, NULL);
    doomrpg->graphSetCliping = false;
}
void DoomRPG_setClipFalseSur(DoomRPG_t* doomrpg, SDL_Surface* surface)
{
	SDL_RenderSetClipRect(surface, NULL);
	doomrpg->graphSetCliping = false;
}

void DoomRPG_setClipTrue(DoomRPG_t* doomrpg, int x, int y, int w, int h)
{
    SDL_Rect clip;

    clip.x = doomrpg->doomCanvas->displayRect.x + x;
    clip.y = doomrpg->doomCanvas->displayRect.y + y;
    clip.w = w;
    clip.h = h;

    SDL_RenderSetClipRect(sdlVideo.screenSurface, &clip);
    doomrpg->graphSetCliping = true;
}
void DoomRPG_setClipTrueSur(DoomRPG_t* doomrpg, int x, int y, int w, int h, SDL_Surface* surface)
{
	SDL_Rect clip;

	clip.x = doomrpg->doomCanvas->displayRect.x + x;
	clip.y = doomrpg->doomCanvas->displayRect.y + y;
	clip.w = w;
	clip.h = h;

	SDL_RenderSetClipRect(surface, &clip);
	doomrpg->graphSetCliping = true;
}

void DoomRPG_setColor(DoomRPG_t* doomrpg, int color)
{
    byte a = (color & 0xFF000000) >> 24;
    byte r = (color & 0x00FF0000) >> 16;
    byte g = (color & 0x0000FF00) >> 8;
    byte b = (color & 0x000000FF);

    SDL_SetRenderDrawColor(sdlVideo.screenSurface, r, g, b, a);
}

void DoomRPG_flushGraphics(...)
{
    SDL_RenderPresent(sdlVideo.screenSurface);
}

void DoomRPG_clearGraphics(DoomRPG_t* doomrpg)
{
    SDL_RenderClear(sdlVideo.screenSurface);
}

void DoomRPG_drawRect(DoomRPG_t* doomrpg, int x, int y, int w, int h)
{
    SDL_Rect rect;

    rect.x = doomrpg->doomCanvas->displayRect.x + x;
    rect.y = doomrpg->doomCanvas->displayRect.y + y;
    rect.w = w + 1;
    rect.h = h + 1;
    SDL_RenderDrawRect(sdlVideo.screenSurface, &rect);
}

void DoomRPG_drawRectSur(DoomRPG_t* doomrpg, int x, int y, int w, int h, SDL_Surface* surface)
{
	SDL_Rect rect;

	rect.x = doomrpg->doomCanvas->displayRect.x + x;
	rect.y = doomrpg->doomCanvas->displayRect.y + y;
	rect.w = w + 1;
	rect.h = h + 1;
	SDL_RenderDrawRect(surface, &rect);
}

void DoomRPG_fillRect(DoomRPG_t* doomrpg, int x, int y, int w, int h)
{
    SDL_Rect rect;

    rect.x = doomrpg->doomCanvas->displayRect.x + x;
    rect.y = doomrpg->doomCanvas->displayRect.y + y;
    rect.w = w;
    rect.h = h;
    SDL_RenderFillRect(sdlVideo.screenSurface, &rect);
}
void DoomRPG_fillRectSur(DoomRPG_t* doomrpg, int x, int y, int w, int h, SDL_Surface* surface)
{
	SDL_Rect rect;

	rect.x = doomrpg->doomCanvas->displayRect.x + x;
	rect.y = doomrpg->doomCanvas->displayRect.y + y;
	rect.w = w;
	rect.h = h;
	SDL_RenderFillRect(surface, &rect);
}

void DoomRPG_drawCircle(DoomRPG_t* doomrpg, int x, int y, int r)
{
    int cx = doomrpg->doomCanvas->displayRect.x + x;
    int cy = doomrpg->doomCanvas->displayRect.y + y;
    SDL_RenderDrawCircle(sdlVideo.screenSurface, cx, cy, r);
}

void DoomRPG_fillCircle(DoomRPG_t* doomrpg, int x, int y, int r)
{
    int cx = doomrpg->doomCanvas->displayRect.x + x;
    int cy = doomrpg->doomCanvas->displayRect.y + y;
    SDL_RenderDrawFillCircle(sdlVideo.screenSurface, cx, cy, r);
}

void DoomRPG_drawLine(DoomRPG_t* doomrpg, int x1, int y1, int x2, int y2)
{
    int sx = doomrpg->doomCanvas->displayRect.x + x1;
    int sy = doomrpg->doomCanvas->displayRect.y + y1;
    int ex = doomrpg->doomCanvas->displayRect.x + x2;
    int ey = doomrpg->doomCanvas->displayRect.y + y2;

    SDL_RenderDrawLine(sdlVideo.screenSurface, sx, sy, ex, ey);
}
void DoomRPG_drawLineSur(DoomRPG_t* doomrpg, int x1, int y1, int x2, int y2, SDL_Surface* surface)
{
	int sx = doomrpg->doomCanvas->displayRect.x + x1;
	int sy = doomrpg->doomCanvas->displayRect.y + y1;
	int ex = doomrpg->doomCanvas->displayRect.x + x2;
	int ey = doomrpg->doomCanvas->displayRect.y + y2;

	SDL_RenderDrawLine(surface, sx, sy, ex, ey);
}

void DoomRPG_setFontColor(DoomRPG_t* doomrpg, int color)
{
	doomrpg->doomCanvas->fontColor = color;
}

void DoomRPG_loopGame(DoomRPG_t* doomrpg)
{
	DoomCanvas_run(doomrpg->doomCanvas);

	doomrpg->upTimeMs = DoomRPG_GetUpTimeMS();
	//printf("doomrpg->upTimeMs %d\n", doomrpg->upTimeMs);

	if (doomrpg->errorID) {
		//sys_strtowstr(pTitle, (char*)(DAT_000124c0 + 0x12468), 0x12);
		//(*(code*)global->pIDisplay->pvt->IDisplay_SetColor)(global->pIDisplay, 2, 0xffffff00);
		//(*(code*)global->pIShell->pvt->IShell_MessageBoxText)(global->pIShell, pTitle, global->errorStr);
		DoomRPG_Error("Doom RPG %s", doomrpg->errorStr);
		doomrpg->closeApplet = true;
	}

	return;
}

/******************************************************************************
 *
 * Sets the starting seed for the deterministic pseudo-random number generator.
 *
 ******************************************************************************/

static unsigned int _seed = 0;
void Random_SetSeed(unsigned int seed)
{
	_seed = seed;
}

/******************************************************************************
 *
 * Get a deterministic pseudo-random number.
 *
 ******************************************************************************/

unsigned int Random_Get(void)
{
	_seed = ((1664525 * _seed) + 1013904223);
	return (_seed ^ (_seed >> 10));
}

static int resetRand = 0;
void  Random_getRand(byte* buff, int i)
{
	byte* bf;
	unsigned int result;

	bf = buff;
	if (resetRand == 0)
	{
		resetRand = 1;
		Random_SetSeed(DoomRPG_GetTimeMS());
	}

	result = resetRand;
	while (i-- > 0)
	{
		if (result <= 0xFF) {
			result = Random_Get() | 0x80000000;
		}

		*bf++ = result;
		result >>= 8;
	}
	resetRand = result;
}

void DoomRPG_setRand(Random_t* rand)
{
	Random_getRand(rand->randTable, RANDTABLESIZE);
	rand->nextRand = 0;
}

byte DoomRPG_randNextByte(Random_t* rand)
{
	int next;

	if ((rand->nextRand + sizeof(byte)) >= RANDTABLESIZE) {
		DoomRPG_setRand(rand);
	}

	next = rand->nextRand;
	rand->nextRand = next + sizeof(byte);

	return rand->randTable[next];
}

int DoomRPG_randNextInt(Random_t* rand)
{
	int next, * rnd;

	if ((rand->nextRand + sizeof(int)) >= RANDTABLESIZE) {
		DoomRPG_setRand(rand);
	}

	next = rand->nextRand;
	rand->nextRand = next + sizeof(int);

	rnd = (int*)rand->randTable;
	return rnd[next];
}

void DoomRPG_notifyDestroyed(DoomRPG_t* doomrpg)
{
	doomRpg->closeApplet = true;
}

void File_writeBoolean(SDL_RWops* rw, int i)
{
    uint8_t boolData = (i != 0) ? 1u : 0u;
    if (!rw) return;
    size_t written = SDL_RWwrite(rw, &boolData, sizeof(uint8_t), 1);
#ifdef DEBUG
    if (written != 1) fprintf(stderr, "File_writeBoolean: write failed\n");
#endif
}

/* Writes a signed byte (preserve -1 sentinel). */
void File_writeByte(SDL_RWops* rw, int i)
{
    int8_t bData = (int8_t)i;
    if (!rw) return;
    size_t written = SDL_RWwrite(rw, &bData, sizeof(int8_t), 1);
#ifdef DEBUG
    if (written != 1) fprintf(stderr, "File_writeByte: write failed\n");
#endif
}

/* Writes a 16-bit short in little-endian (explicit 16-bit). */
void File_writeShort(SDL_RWops* rw, int i)
{
    if (!rw) return;
    int16_t sData = (int16_t)i;
    uint16_t u = SDL_SwapLE16((uint16_t)sData);
    size_t written = SDL_RWwrite(rw, &u, sizeof(uint16_t), 1);
#ifdef DEBUG
    if (written != 1) fprintf(stderr, "File_writeShort: write failed\n");
#endif
}

/* Writes a 32-bit int in little-endian (explicit 32-bit). */
void File_writeInt(SDL_RWops* rw, int i)
{
    if (!rw) return;
    int32_t iData = (int32_t)i;
    uint32_t u = SDL_SwapLE32((uint32_t)iData);
    size_t written = SDL_RWwrite(rw, &u, sizeof(uint32_t), 1);
#ifdef DEBUG
    if (written != 1) fprintf(stderr, "File_writeInt: write failed\n");
#endif
}

/* Writes a 32-bit "long" (keeps 32-bit semantics). Prefer File_writeInt in new code. */
void File_writeLong(SDL_RWops* rw, int i)
{
    /* On platforms where long == 4 bytes this matches the old behavior.
       We intentionally write 32 bits to keep file portable. */
    File_writeInt(rw, i);
}

/* ---------- Readers (robust checks, return safe defaults) ---------- */

/* Read boolean (returns false on error). */
boolean File_readBoolean(SDL_RWops* rw)
{
    if (!rw) return 0;
    uint8_t boolData = 0;
    size_t read = SDL_RWread(rw, &boolData, sizeof(uint8_t), 1);
    if (read != 1) {
#ifdef DEBUG
        fprintf(stderr, "File_readBoolean: read failed or EOF\n");
#endif
        return 0;
    }
    return (boolean)(boolData != 0);
}

/* Read signed byte (preserve -1 sentinel). Returns 0 on error but can return negative values if present. */
int File_readByte(SDL_RWops* rw)
{
    if (!rw) return 0;
    int8_t bData = 0;
    size_t read = SDL_RWread(rw, &bData, sizeof(int8_t), 1);
    if (read != 1) {
#ifdef DEBUG
        fprintf(stderr, "File_readByte: read failed or EOF\n");
#endif
        return 0;
    }
    return (int)bData;
}

/* Read 16-bit short (returns 0 on error). */
int File_readShort(SDL_RWops* rw)
{
    if (!rw) return 0;
    uint16_t u = 0;
    size_t read = SDL_RWread(rw, &u, sizeof(uint16_t), 1);
    if (read != 1) {
#ifdef DEBUG
        fprintf(stderr, "File_readShort: read failed or EOF\n");
#endif
        return 0;
    }
    uint16_t v = SDL_SwapLE16(u);
    /* cast to signed 16-bit to preserve negative values if any */
    return (int)(int16_t)v;
}

/* Read 32-bit int (returns 0 on error). */
int File_readInt(SDL_RWops* rw)
{
    if (!rw) return 0;
    uint32_t u = 0;
    size_t read = SDL_RWread(rw, &u, sizeof(uint32_t), 1);
    if (read != 1) {
#ifdef DEBUG
        fprintf(stderr, "File_readInt: read failed or EOF\n");
#endif
        return 0;
    }
    uint32_t v = SDL_SwapLE32(u);
    return (int)(int32_t)v;
}

/* Read 32-bit "long" (keeps 32-bit semantics). Prefer File_readInt in new code. */
int File_readLong(SDL_RWops* rw)
{
    /* Keep same behavior as File_readInt for portability. */
    return File_readInt(rw);
}