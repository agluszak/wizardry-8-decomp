#ifndef WIZ8_GAMELOOP_H
#define WIZ8_GAMELOOP_H

#include "Types.h"

#ifdef __cplusplus
extern "C" { // C-LINKAGE: src/sgp/sgp.c calls these four game-lifecycle entries
#endif

BOOLEAN InitializeGame(void);
void ShutdownGame(void);
void GameLoop(void);
void GameloopExit(BOOLEAN unload_screens);

#ifdef __cplusplus
}
#endif

#endif
