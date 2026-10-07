#ifndef WIZ8_LOCAL_CODE_GAMELOOP_H
#define WIZ8_LOCAL_CODE_GAMELOOP_H

#include "wiz8/layouts/screen_state.h"
#include "Container.h"

void ShutdownGame(void);
void GameLoop(void);
void GameloopExit(unsigned char unload_screens);

/* Local Code\Gameloop.cpp's screen-state machine storage. */
extern W8ScreenStateHandlers g_screen_handlers[W8_SCREEN_COUNT];
extern W8ScreenStateRuntime g_current_screen_state;
extern W8ScreenStateRuntime g_pending_screen_state;
extern bool g_screen_return_requested;
extern HSTACK g_screen_return_stack;
extern W8ScreenId g_previous_screen_id;
extern W8ScreenId g_suspended_screen_id;

#endif
