#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/* Released includes plus the narrow missing Wizardry product surface. */
#include "Types.h"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include "timer.h"
#include "debug.h"
#include "RegInst.h"
#include "VObject.h"
#include "Font.h"
#include "FileMan.h"
#include "input.h"
#include "Random.h"
#include "soundman.h"
#include "MemMan.h"

#define APPLICATION_NAME "Wizardry 8"
#define PIXEL_DEPTH 16
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define TIMER_RESUME 0
#define TIMER_SUSPEND 1
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef MSH_MOUSEWHEEL
#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
#endif

extern void SGPExit(void);
extern void MoveTimer(UINT32 action);
extern BOOLEAN InitializeGame(void);
extern void ShutdownGame(void);
extern void GameLoop(void);
extern void GameloopExit(BOOLEAN unloadScreens);
extern BOOLEAN InitializeVideoManager(HINSTANCE instance, UINT16 show, void *procedure);
extern void ShutdownVideoManager(void);
extern void VideoDumpMemoryLeaks(void);
extern STR VideoGetConfigFile(void);

#endif
