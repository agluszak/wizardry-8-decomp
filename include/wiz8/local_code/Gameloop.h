#ifndef WIZ8_LOCAL_CODE_GAMELOOP_H
#define WIZ8_LOCAL_CODE_GAMELOOP_H

#include "wiz8/layouts/screen_state.h"
#include "Container.h"

/* Local Code\Gameloop.cpp's screen-state machine storage. */
extern W8ScreenStateHandlers g_screen_handlers[W8_SCREEN_COUNT];
extern W8ScreenStateRuntime g_current_screen_state;
extern W8ScreenStateRuntime g_pending_screen_state;
extern unsigned char g_screen_return_requested;
extern HSTACK g_screen_return_stack;
extern int g_previous_screen_id;
extern int g_suspended_screen_id;
/* Retail 0x006F0628: WinMain's loop flag, set with 0x006F0630 at startup and
   cleared by the exit screen, the state machine's stop paths, and shutdown. */

#endif
