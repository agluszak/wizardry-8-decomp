#include "gameplay_boundaries.h"

extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);
extern void ReplaceOrCreateItem(
    W8ItemInstance* item,
    int item_id,
    int quantity,
    int charges,
    int identified);
/* Provisional semantic name for the allocation path at 0x004F6B90. */
extern W8WorldItem* CreateWorldItem(
    const W8ItemInstance* item,
    const W8Vector3* position,
    int unknown,
    unsigned char add_to_world);

// FUNCTION: WIZ8 0x004F6C50
W8WorldItem* SpawnItem(
    int item_id,
    const W8Vector3* position,
    int unknown,
    unsigned char add_to_world)
{
    W8ItemInstance local_item;
    W8ItemInstance* item;
    W8WorldItem* result;

    if (item_id == -1) {
        item = 0;
    } else {
        ReplaceOrCreateItem(&local_item, item_id, 0, 0, 0);
        item = &local_item;
    }

    result = CreateWorldItem(item, position, unknown, add_to_world);
    if (result == 0) {
        srAssertFail(
            "pItemInfo",
            "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp",
            0x18e,
            0);
    }
    return result;
}
