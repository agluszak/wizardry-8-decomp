#ifndef WIZ8_SGP_COMPAT_GAMELOOP_H
#define WIZ8_SGP_COMPAT_GAMELOOP_H

#include "Types.h"

BOOLEAN InitializeGame(void);
void ShutdownGame(void);
void GameLoop(void);
void GameloopExit(BOOLEAN unload_screens);

#endif
