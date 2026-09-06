#ifndef WIZ8_WINDOWS_H
#define WIZ8_WINDOWS_H

/* Common Windows and DirectDraw declarations for the Wizardry target. */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef DIRECTDRAW_VERSION
#define DIRECTDRAW_VERSION 0x0700
#endif

#include <windows.h>
#include <ddraw.h>

long __stdcall WindowProc4011E0(
    void* window, int message, unsigned int wparam, long lparam);

#endif
