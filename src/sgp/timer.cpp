/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include <windows.h>
#include "video2.h"
#include "timer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// GLOBAL: WIZ8 0x006eb708
UINT32 guiStartupTime;
// GLOBAL: WIZ8 0x006eb70c
UINT32 guiCurrentTime;

// FUNCTION: WIZ8 0x00406b70
void CALLBACK Clock(HWND hWindow, UINT uMessage, UINT idEvent, DWORD dwTime)
{
    guiCurrentTime = GetTickCount();
    if (guiCurrentTime <
        guiStartupTime) { // Adjust guiCurrentTime because of loopback on the timer value
        guiCurrentTime = guiCurrentTime + (0xffffffff - guiStartupTime);
    } else { // Adjust guiCurrentTime because of loopback on the timer value
        guiCurrentTime = guiCurrentTime - guiStartupTime;
    }
}

// FUNCTION: WIZ8 0x00406ba0
BOOLEAN InitializeClockManager(void)
{

    // Register the start time (use WIN95 API call)
    guiCurrentTime = guiStartupTime = GetTickCount();
    SetTimer(ghWindow, MAIN_TIMER_ID, 10, (TIMERPROC)Clock);

    return TRUE;
}

// FUNCTION: WIZ8 0x00406bd0
void ShutdownClockManager(void)
{

    // Make sure we kill the timer
    KillTimer(ghWindow, MAIN_TIMER_ID);
}

// FUNCTION: WIZ8 0x00406be0
TIMER GetClock(void)
{
    return guiCurrentTime;
}

// FUNCTION: WIZ8 0x00406bf0
TIMER SetCountdownClock(UINT32 uiTimeToElapse)
{
    return (guiCurrentTime + uiTimeToElapse);
}

// FUNCTION: WIZ8 0x00406c00
UINT32 ClockIsTicking(TIMER uiTimer)
{
    if (uiTimer > guiCurrentTime) { // Well timer still hasn't elapsed
        return (uiTimer - guiCurrentTime);
    }
    // Time's up
    return 0;
}
