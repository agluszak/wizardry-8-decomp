#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/* Explicit non-PCH surface from input.c plus Wizardry video declarations. */
#include "Types.h"
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include "debug.h"
#include "input.h"
#include "MemMan.h"
#include "english.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

extern HWND ghWindow;
extern BOOLEAN VideoIsFullScreen(void);
extern void VideoGetClientRect(RECT *pRect);

#endif
