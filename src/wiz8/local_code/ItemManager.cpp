#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/game_status.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/combat_state.h"
#include "wiz8/character.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_tables.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/float_constants.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/engine_code/GDProp.h"

#include <string.h>

/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */
#define ITEM_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp"

// FUNCTION: WIZ8 0x004f69f0
bool InitializeItemManagerState()
{
    g_status_685170.next_world_item_id_2352 = 1;
    gXStatus.item_manager_pending = 0;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->selected_item = -1;
    }
    if (gXStatus.plsItemList != 0) {
        PListClear(gXStatus.plsItemList);
        return gXStatus.plsItemList != 0;
    }
    gXStatus.plsItemList = PLCreate();
    return gXStatus.plsItemList != 0;
}

// FUNCTION: WIZ8 0x004f8130
bool ItemHasFlags(W8WorldItem* item, unsigned int mask)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 998, 0);
    }
    return (item->flags & mask) != 0;
}

// FUNCTION: WIZ8 0x004f8170
void SetItemFlags(W8WorldItem* item, unsigned int mask, bool enabled)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1004, 0);
    }
    if (enabled) {
        item->flags |= mask;
    } else {
        item->flags &= ~mask;
    }
}

// FUNCTION: WIZ8 0x004f81c0
void SetItemAndEntityFlags(W8WorldItem* item, unsigned int mask, bool enabled)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1019, 0);
    }
    if (enabled) {
        item->entity_flags |= mask;
    } else {
        item->entity_flags &= ~mask;
    }
    if (item->owner != 0) {
        static_cast<W8ItemRep*>(item->owner->m_pRep)->SetFlags(mask, enabled);
    }
}

// FUNCTION: WIZ8 0x004f8300
int ItemInfoGetNumInGroup(W8WorldItem* item)
{
    if (item == 0) {
        srAssertFail(
            "pItemInfo", ITEM_MANAGER_CPP, 1115, "Bad ITEM_STRUCT in ItemInfoGetNumInGroup");
    }
    item = item->next;
    int count = 1;
    while (item != 0) {
        item = item->next;
        ++count;
    }
    return count;
}

// FUNCTION: WIZ8 0x004f8340
void ItemInfoAddToGroup(W8WorldItem* group, W8WorldItem* item)
{
    W8WorldItem* tail;

    if (group == 0) {
        srAssertFail(
            "pItemInfo", ITEM_MANAGER_CPP, 1130, "Bad ITEM_STRUCT in ItemInfoAddToGroup");
    }
    tail = group;
    while (tail->next != 0) {
        tail = tail->next;
    }
    tail->next = item;
    item->next = 0;
}

struct W8ItemLevelScaleRange {
    unsigned int minimum_party_level;
    unsigned int minimum_item_value;
    unsigned int maximum_item_value;
};

static const W8ItemLevelScaleRange g_item_level_scale_ranges[7] = {
    {1, 0, 500},
    {6, 50, 1000},
    {11, 100, 3000},
    {16, 300, 5000},
    {21, 600, 10000},
    {26, 800, 20000},
    {31, 1000, 1000000},
};


// FUNCTION: WIZ8 0x004f88a0
int FindItemTableByName(const char* name)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
/* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int index;

    for (index = 0; index < (int)gXStatus.uiItemTablesInDatabase; ++index) {
        if (_stricmp(name, g_item_tables[index]->name) == 0) {
            return index;
        }
    }
    return -1;
#pragma clang diagnostic pop
}

static __forceinline W8WorldItem* CreateTableItem(
    unsigned int item_id,
    const srVector3T<float>* position)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
/* Retail tests the item id against the -1 sentinel with VC6's mixed-sign
   compare; the caller-side id domain keeps the unsigned parameter. */
    W8ItemInstance item;
    W8ItemInstance* item_pointer;
    W8WorldItem* result;

    if (item_id == -1) {
#pragma clang diagnostic pop
        item_pointer = 0;
    }
    else {
        ReplaceOrCreateItem(&item, item_id, 0, 0, 0);
        item_pointer = &item;
    }
    result = CreateWorldItem(item_pointer, position, 3, 0);
    if (result == 0) {
        srAssertFail(
            "pItemInfo",
            "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp",
            0x18e,
            0);
    }
    return result;
}

// FUNCTION: WIZ8 0x004f88f0
int GenerateItemsFromTable(
    W8GrowableVector<W8WorldItem*>* output_items,
    unsigned int table_id,
    unsigned int maximum_items)
{
    unsigned int party_level = GetAveragePartyLevel();
    srVector3T<float> position;
    W8GrowableVector<int> candidates;
    W8ItemTableRecord* table;
    W8ItemTableEntry* entry;
    W8ItemDatabaseRecord* item_record;
    const W8ItemLevelScaleRange* range;
    unsigned int item_value;
    unsigned int total_weight;
    unsigned int selected_count;
    unsigned int random_value;
    int entry_index;
    int position_index;

    table_id &= 0xffff;
    for (entry_index = 0; entry_index < 40; ++entry_index) {
        table = g_item_tables[table_id];
        if (table->entries[entry_index].selector_00 != 0) {
            item_record = &g_item_records[table->entries[entry_index].item_id];
            if (table->entries[entry_index].weight == 0) {
                output_items->Add(
                    CreateTableItem(g_item_tables[table_id]->entries[entry_index].item_id,
                                    &position));
            }
            else if (table->level_scaled == 0) {
                candidates.Add(entry_index);
            }
            else {
                item_value = item_record->value;
                for (range = g_item_level_scale_ranges;
                     range < g_item_level_scale_ranges + 7;
                     ++range) {
                    if (range->minimum_party_level <= party_level &&
                        range->minimum_item_value <= item_value &&
                        item_value < range->maximum_item_value) {
                        candidates.Add(entry_index);
                        break;
                    }
                }
            }
        }
    }

    if (candidates.GetCount() < (int)maximum_items &&
        g_item_tables[table_id]->level_scaled != 0) {
        candidates.Clear();
        for (entry_index = 0; entry_index < 40; ++entry_index) {
            if (g_item_tables[table_id]->entries[entry_index].selector_00 != 0) {
                candidates.Add(entry_index);
            }
        }
    }

    total_weight = 0;
    for (entry_index = 0; entry_index < candidates.GetCount(); ++entry_index) {
        total_weight +=
            g_item_tables[table_id]->entries[*candidates.GetAt(entry_index)].weight;
    }

    selected_count = 0;
    while (selected_count < maximum_items) {
        if (candidates.GetCount() == 0) {
            break;
        }
        position_index = Random(candidates.GetCount());
        entry_index = *candidates.GetAt(position_index);
        table = g_item_tables[table_id];
        entry = &table->entries[entry_index];
        random_value = Random(total_weight);
        if (random_value <= entry->weight) {
            output_items->Add(CreateTableItem(entry->item_id, &position));
            candidates.RemoveAt(position_index);
            ++selected_count;
        }
    }

    return output_items->GetCount();
}

#include <stdlib.h>

/* 0x00689B54: the cursor the iterator below resumes from. */

// GLOBAL: WIZ8 0x00689b54
int g_world_item_cursor;

/* Bit 0x20 of the item record's flag word, which is the only bit
   ItemInfoIsWorldPersistent reads. */
enum { W8_ITEM_FLAG_PERSISTENT = 0x20 };
/* Bit 1 of the world item's own flag word. */
enum { W8_WORLD_ITEM_FLAG_02 = 2 };

/* Free a whole group of world items, following the link that chains them. */
// FUNCTION: WIZ8 0x004f6cc0
void FreeWorldItemGroup(W8WorldItem* item)
{
    W8WorldItem* next;

    while (item != 0) {
        next = item->next;
        free(item);
        item = next;
    }
}

/* Look an item record up by its internal name. The name sits at 0x8d in the
   record; a record whose name is empty is matched against the display name
   converted down from wide instead. */
// FUNCTION: WIZ8 0x004f8220
int FindItemRecordByName(const char* name)
{
    int index;
    const char* internal_name;

    for (index = 0; index < (int)gXStatus.uiItemsInDatabase; ++index) {
        internal_name = g_item_records[index].internal_name;
        if (internal_name[0] == 0) {
            if (_stricmp(ConvertWideStringToString(g_item_records[index].display_name), name) ==
                0) {
                return index;
            }
        }
        else if (_stricmp(internal_name, name) == 0) {
            return index;
        }
    }
    return -1;
}

/* Walk the world item list, resuming where the last call left off. Restarting
   rewinds; running off the end answers nothing without rewinding. */
// FUNCTION: WIZ8 0x004f82b0
W8WorldItem* GetNextWorldItem(char restart)
{
    int index;

    if (restart) {
        g_world_item_cursor = 0;
    }
    index = g_world_item_cursor;
    if ((unsigned int)index < PLLength(gXStatus.plsItemList)) {
        ++g_world_item_cursor;
        return (W8WorldItem*)PLGet(gXStatus.plsItemList, index);
    }
    return 0;
}

/* Take one item out of the group chained onto another. Unlinking the head
   promotes whatever followed it; unlinking anything else just closes the gap.
   Answers the group's head afterwards. */
// FUNCTION: WIZ8 0x004f83a0
W8WorldItem* ItemInfoRemoveFromGroup(W8WorldItem* head, W8WorldItem* item)
{
    W8WorldItem* previous;
    W8WorldItem* scan;

    if (head == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1142,
                     "Bad ITEM_STRUCT in ItemInfoRemoveFromGroup");
    }

    if (head == item) {
        return item;
    }

    scan = head;
    do {
        previous = scan;
        if (previous == 0) {
            break;
        }
        scan = previous->next;
        if (scan == 0) {
            break;
        }
    } while (scan != item);

    if (scan == item) {
        if (item->next != 0) {
            previous->next = item->next;
            item->next = 0;
            return head;
        }
        previous->next = 0;
    }
    item->next = 0;
    return head;
}

/* The next item in a group. */
// FUNCTION: WIZ8 0x004f8410
W8WorldItem* ItemInfoGroupGetNext(W8WorldItem* item)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1178,
                     "Bad ITEM_STRUCT in ItemInfoGroupGetNext");
    }
    return item->next;
}

/* Whether the item's record marks it as one the world keeps. */
// FUNCTION: WIZ8 0x004f91e0
bool ItemInfoIsWorldPersistent(const W8WorldItem* item)
{
    if (item == 0) {
        return false;
    }
    return (g_item_records[item->item.item_id].flags_041 & W8_ITEM_FLAG_PERSISTENT) != 0;
}

/* Copy a world item's carried item out onto the heap. */
// FUNCTION: WIZ8 0x004f9210
W8ItemInstance* CopyWorldItemInstance(const W8WorldItem* item)
{
    W8ItemInstance* copy = (W8ItemInstance*)malloc(0xc);

    if (copy == 0) {
        return 0;
    }
    *(int*)copy = *(const int*)&item->item;
    *((int*)copy + 1) = *((const int*)&item->item + 1);
    *((int*)copy + 2) = *((const int*)&item->item + 2);
    return copy;
}

/* Raise or lower bit one of the world item's own flag word. */
// FUNCTION: WIZ8 0x004f94a0
void SetWorldItemFlag02(W8WorldItem* item, char enabled)
{
    if (enabled) {
        item->flags |= W8_WORLD_ITEM_FLAG_02;
    }
    else {
        item->flags &= ~W8_WORLD_ITEM_FLAG_02;
    }
}

/* The world item at one list position. Both the bound and the fetch are
   asserted, and the second reports the list it failed on. */
// FUNCTION: WIZ8 0x004f7fe0
W8WorldItem* ItemInfo(unsigned int item_list_index)
{
    W8WorldItem* item;

    if (item_list_index >= PLLength(gXStatus.plsItemList)) {
        srAssertFail("uiItemListIndex < (UINT32) PLLength(gXStatus.plsItemList)",
                     ITEM_MANAGER_CPP, 961, 0);
    }
    item = (W8WorldItem*)PLGet(gXStatus.plsItemList, item_list_index);
    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 965,
                     FormatString("ItemInfo: ERROR - PLGet failed, index %d, pList %d",
                                  item_list_index, gXStatus.plsItemList));
    }
    return item;
}

/* Where in the world item list one runtime id sits. Not finding it is a data
   error rather than a -1. */
// FUNCTION: WIZ8 0x004f8060
unsigned int ItemIndex(int runtime_id)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsItemList); ++index) {
        if (ItemInfo(index)->runtime_id == runtime_id) {
            return index;
        }
    }
    srAssertFail("FALSE", ITEM_MANAGER_CPP, 992,
                 FormatString("ItemIndex: ERROR - ItemID %d not found", runtime_id));
    return 0;
}

/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */

/* Flatten one item's whole group into a vector, the item itself first and then
   everything chained onto it. A failed append drops that entry and the walk
   continues. */
// FUNCTION: WIZ8 0x004f8440
int ItemInfoMakeGroupList(
    W8WorldItem* item, int unused, W8GrowableVector<W8WorldItem*>* out)
{
    W8WorldItem* next;

    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1185,
                     "Bad ITEM_STRUCT in ItemInfoMakeGroupList");
    }

    out->Add(item);

    for (next = item->next; next != 0; next = next->next) {
        out->Add(next);
    }
    return out->count;
}

/* Take one item out of the world. Its three assertions name the two fields
   they guard - fActive and p3D - and the item keeps its last position and
   entity flags so it can be put back. */
// FUNCTION: WIZ8 0x004f70d0
void DeactivateWorldItem(W8WorldItem* item)
{
    srVector3T<float> position;

    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 554, 0);
    }
    if (item->unknown_08 == 0) {
        srAssertFail("pItemInfo->fActive", ITEM_MANAGER_CPP, 555, 0);
    }
    if (item->owner == 0) {
        srAssertFail("pItemInfo->p3D != NULL", ITEM_MANAGER_CPP, 556, 0);
    }

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->selected_item == item->runtime_id) {
        g_level_block->selected_item = -1;
    }

    item->owner->m_pRep->GetLocation004B8890(&position);
    item->position = position;
    item->entity_flags = static_cast<W8ItemRep*>(item->owner->m_pRep)->flags;

    item->owner->DetachMesh0049FA30(GetWorld());
    WorldRemoveFromList04(GetWorld(), item->owner);
    delete item->owner;
    item->owner = 0;
    item->unknown_08 = 0;
    --gXStatus.item_manager_pending;
}

/* Push every live world item back to its sector and free the whole list. The
   list is reduced from its head until empty and then destroyed; only the
   destroy failure keeps the global pointing at it. */
// FUNCTION: WIZ8 0x004f6a50
unsigned char ReleaseItemLists(void)
{
    unsigned int count;

    if (gXStatus.plsItemList == 0) {
        srAssertFail("gXStatus.plsItemList != NULL", ITEM_MANAGER_CPP, 0x126, 0);
    }
    count = PLLength(gXStatus.plsItemList);
    while ((int)count >= 1) {
        W8WorldItem* item;

        count = PLLength(gXStatus.plsItemList);
        if (count == 0) {
            srAssertFail(
                "uiItemListIndex < (UINT32) PLLength(gXStatus.plsItemList)",
                ITEM_MANAGER_CPP, 0x3c1, 0);
        }
        item = static_cast<W8WorldItem*>(PLGet(gXStatus.plsItemList, 0));
        if (item == 0) {
            srAssertFail(
                "pItemInfo != NULL", ITEM_MANAGER_CPP, 0x3c5,
                FormatString("ItemInfo: ERROR - PLGet failed, index %d, pList %d",
                             0, gXStatus.plsItemList));
        }
        if (item->sector_id >= 0) {
            RemoveItemFromSector(item->sector_id, item);
        }
        if (item->unknown_08 != 0) {
            DeactivateWorldItem(item);
        }
        while (item != 0) {
            W8WorldItem* next = item->next;

            free(item);
            item = next;
        }
        if (PLRemoveAt(gXStatus.plsItemList, 0) == 0) {
            return 0;
        }
        count = PLLength(gXStatus.plsItemList);
    }
    if (PLDestroy(gXStatus.plsItemList) == 0) {
        return 0;
    }
    gXStatus.plsItemList = 0;
    return 1;
}

/* Whether one world item is close enough to a point to be reached, and in
   sight of the party's eye. The distance is compared before the trace, so a
   far item is never traced to. */
// FUNCTION: WIZ8 0x004f8560
unsigned char IsWorldItemWithinReach(W8Item* owner, const float* from, float radius)
{
    srVector3T<float> position;
    float lower[3];
    float upper[3];
    srVector3T<float> eye;
    float dx;
    float dy;
    float dz;

    owner->m_pRep->GetLocation004B8890(&position);
    GetCameraPosition(&eye);

    dx = position.x - from[0];
    dy = position.y - from[1];
    dz = position.z - from[2];
    if (dx * dx + dy * dy + dz * dz < radius * radius) {
        GetWorldItemBounds(lower, upper);
        lower[0] += position.x;
        lower[1] += position.y;
        lower[2] += position.z;
        upper[0] += position.x;
        upper[1] += position.y;
        upper[2] += position.z;
        if (TraceToBounds(&eye, lower, upper)) {
            return 1;
        }
    }
    return 0;
}

/* Drop one item onto the ground below where it is. The search starts one world
   unit up so an item already resting does not settle into the floor; landing
   moves it between sectors and clears its saved-marker flag. */
// FUNCTION: WIZ8 0x004f93d0
unsigned char SettleWorldItem(W8WorldItem* item)
{
    srVector3T<float> start;
    unsigned char hit;
    int sector;

    start.x = item->position.x;
    start.z = item->position.z;
    start.y = item->position.y + g_world_scale_005ebc40;

    item->flags &= ~2u;
    item->unknown_35 = 0;

    g_octree_6598a4->SettleToGround00433820(&start, &hit, 1, 250.0f);
    if (hit == 0) {
        return 0;
    }

    sector = g_octree_6598a4->current_sector;
    if (sector != item->sector_id) {
        if (item->sector_id >= 0) {
            RemoveItemFromSector(item->sector_id, item);
        }
        if (sector >= 0) {
            AddItemToSector(sector, item);
        }
        item->sector_id = sector;
    }
    if (item->owner != 0) {
        item->owner->SetLocation0049F720(&start);
    }
    item->position = start;
    return 1;
}

/* Rebuild every world item's carried instance from its own item id, which
   re-rolls whatever the record decides rather than keeping what was there. */
// FUNCTION: WIZ8 0x004f94c0
void RebuildAllWorldItemInstances(void)
{
    unsigned int index;
    W8WorldItem* item;

    for (index = 0; index < PLLength(gXStatus.plsItemList); ++index) {
        item = ItemInfo(index);
        ReplaceOrCreateItem(&item->item, item->item.item_id, 0, 0, 0);
    }
}

// FUNCTION: WIZ8 0x004f6b90
W8WorldItem* CreateWorldItem(
    W8ItemInstance* item,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world)
{
    W8WorldItem* result = (W8WorldItem*)malloc(sizeof(W8WorldItem));

    if (result == 0) {
        return 0;
    }

    memset(result, 0, sizeof(W8WorldItem));
    Function520070(&result->item, 0, 1);
    result->runtime_id = g_status_685170.next_world_item_id_2352++;
    result->unknown_08 = 0;
    result->owner = 0;
    result->position = *position;
    result->sector_id = -1;
    SettleWorldItem(result);
    result->entity_flags = unknown;

    if (item != 0) {
        CopyItemInstance(&result->item, item, 0, 1);
    }
    if (add_to_world && PLAdoptAppend(gXStatus.plsItemList, result) == -1) {
        return 0;
    }
    return result;
}

// FUNCTION: WIZ8 0x004f6c50
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
