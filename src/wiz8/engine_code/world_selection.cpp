#include "wiz8/engine_code/World.h"

/* World selection. The original translation-unit name is not established;
   these bodies retain their existing compilation unit and link-order slot. */

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
