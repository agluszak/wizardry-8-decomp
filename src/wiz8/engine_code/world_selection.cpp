#include "wiz8/engine_code/World.h"

/* World selection. Live query: 0x00451280 sits in the gap between
   Engine Code\3dapi.cpp (upper 0x004511D0) and Engine Code\Navigator.cpp
   (lower 0x00452F50). */

// FUNCTION: WIZ8 0x00451280
W8World* GetWorld(void)
{
    return g_world;
}

// FUNCTION: WIZ8 0x00451290
void SetCurrentWorld(W8World* world)
{
    g_world = world;
}
// FUNCTION: WIZ8 0x004512B0
void SetWorld659AB8(W8World* world)
{
    g_world_659ab8 = world;
}
