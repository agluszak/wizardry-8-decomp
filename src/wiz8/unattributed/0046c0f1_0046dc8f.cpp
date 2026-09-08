#include "wiz8/engine_code/GameData.h"

// GLOBAL: WIZ8 0x00652db0
W8GameData* g_octree_game_data_00652db0;

/* Address quarantine 0046c0f1-0046dc8f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0046D7D0
void __stdcall SetValue652DB0(int value)
{
    g_octree_game_data_00652db0 = reinterpret_cast<W8GameData*>(value);
}
