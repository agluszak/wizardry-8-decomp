#include "gameplay_boundaries.h"

// FUNCTION: WIZ8 0x0042B580
int GetLoadedLevelID(void)
{
    return g_loaded_level_id;
}

// FUNCTION: WIZ8 0x00451280
W8World* GetWorld(void)
{
    return g_world;
}

// FUNCTION: WIZ8 0x0051B9E0
int GetItemInHand(void)
{
    if (!g_item_in_hand_valid) {
        return -1;
    }
    return g_item_in_hand.item_id;
}
