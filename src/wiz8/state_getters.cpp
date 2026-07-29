#include "wiz8/gameplay_boundaries.h"

#include <string.h>

// FUNCTION: WIZ8 0x0042b580
int GetLoadedLevelID(void)
{
    return g_loaded_level_id;
}

// The original bounds-checks only the upper end, so a negative level_id reads
// before the table. Reproduced as-is; LevelGetFolderNameByID checks both ends.
// FUNCTION: WIZ8 0x0042b500
unsigned char LevelGetLocationCodeByID(int level_id, char* location_code)
{
    if (level_id >= 47) {
        return 0;
    }
    if (!location_code) {
        return 0;
    }
    strcpy(location_code, g_level_folders[level_id].location_code);
    return 1;
}

// The original's retained `level_id == -1` and `>= 57` tests are only explicable
// as a separate lookup helper inlined into its one caller: after inlining, VC6
// substitutes the body but does not propagate the returned value's range, so the
// caller's guards survive even though the search can only yield 0..46.
static __inline int LevelFindIDByLocationCode(const char* location_code)
{
    int level_id;

    for (level_id = 0; level_id < 47; level_id++) {
        if (_stricmp(g_level_folders[level_id].location_code, location_code) == 0) {
            return level_id;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x0042b410
int GetLocationIDFromCode(const char* location_code)
{
    W8LevelInfo info;
    int level_id;

    if (!location_code) {
        return -1;
    }
    level_id = LevelFindIDByLocationCode(location_code);
    if (level_id == -1) {
        return level_id;
    }
    if (level_id >= 57) {
        return -1;
    }
    if (level_id < 47) {
        if (strlen(g_level_folders[level_id].folder_name) == 0
            || strlen(g_level_folders[level_id].level_name) == 0) {
            return -1;
        }
    }
    if (!LevelBuildInfoByID(level_id, &info)) {
        return -1;
    }
    return level_id;
}

// FUNCTION: WIZ8 0x0042b550
const char* LevelGetFolderNameByID(int level_id)
{
    if (level_id >= 47 || level_id < 0) {
        return 0;
    }
    return g_level_folders[level_id].folder_name;
}

// FUNCTION: WIZ8 0x00451280
W8World* GetWorld(void)
{
    return g_world;
}

// FUNCTION: WIZ8 0x0051b9e0
int GetItemInHand(void)
{
    if (!g_item_in_hand_valid) {
        return -1;
    }
    return g_item_in_hand.item_id;
}
