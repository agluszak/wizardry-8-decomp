#include "item_spawning.h"
#include "sr_api.h"

#include <malloc.h>
#include <string.h>

extern void ReplaceOrCreateItem(
    W8ItemInstance* item,
    int item_id,
    int quantity,
    int charges,
    int identified);
/* Provisional semantic names for item helpers at the stated canonical addresses. */
extern void InitializeItemInstance( /* 0x00520070 */
    W8ItemInstance* item,
    int item_id,
    unsigned char initialize_defaults);
extern void CopyItemInstance( /* 0x0051FE30 */
    W8ItemInstance* destination,
    const W8ItemInstance* source,
    int unknown,
    unsigned char copy_runtime_state);
extern void InitializeWorldItemPlacement(W8WorldItem* item); /* 0x004F93D0 */
extern int AddWorldItemToList(void* list, W8WorldItem* item); /* 0x005E2480 */

// FUNCTION: WIZ8 0x004F6B90
W8WorldItem* CreateWorldItem(
    const W8ItemInstance* item,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world)
{
    W8WorldItem* result = (W8WorldItem*)malloc(sizeof(W8WorldItem));

    if (result == 0) {
        return 0;
    }

    memset(result, 0, sizeof(W8WorldItem));
    InitializeItemInstance(&result->item, 0, 1);
    result->runtime_id = g_next_world_item_id++;
    result->unknown_08 = 0;
    result->unknown_04 = 0;
    result->position = *position;
    result->sector_id = -1;
    InitializeWorldItemPlacement(result);
    result->unknown_25 = unknown;

    if (item != 0) {
        CopyItemInstance(&result->item, item, 0, 1);
    }
    if (add_to_world && AddWorldItemToList(g_world_item_list, result) == -1) {
        return 0;
    }
    return result;
}

// FUNCTION: WIZ8 0x004F6C50
W8WorldItem* SpawnItem(
    int item_id,
    const srVector3T<float>* position,
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
