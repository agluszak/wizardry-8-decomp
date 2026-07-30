#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 004511d1-0045368f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00451290
void SetCurrentWorld(W8World* world)
{
    g_value_659ab4 = reinterpret_cast<int>(world);
}
// FUNCTION: WIZ8 0x004512B0
void SetWorld659AB8(W8World* world)
{
    g_world_659ab8 = world;
}
