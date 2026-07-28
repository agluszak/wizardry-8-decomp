#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_state_006598a4.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"

#include <string.h>

extern int g_item_manager_initialized_006874C2;
extern int g_item_manager_pending_00683FA9;
extern int g_screen_state_0068ec78;
/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */
extern W8LevelRuntimeBlock* g_item_selection_owner_0068EDCC;
extern W8PList* g_world_item_list_00683fb5;

#define ITEM_MANAGER_CPP "C:\\\\Projects\\\\Wizardry 8\\\\Local Code\\\\ItemManager.cpp"

// FUNCTION: WIZ8 0x004F69F0
bool InitializeItemManagerState()
{
    g_item_manager_initialized_006874C2 = 1;
    g_item_manager_pending_00683FA9 = 0;
    if (g_screen_state_0068ec78 == 7 && g_item_selection_owner_0068EDCC != 0) {
        g_item_selection_owner_0068EDCC->selected_item = -1;
    }
    if (g_world_item_list_00683fb5 != 0) {
        PListClear(g_world_item_list_00683fb5);
        return g_world_item_list_00683fb5 != 0;
    }
    g_world_item_list_00683fb5 = PListCreate();
    return g_world_item_list_00683fb5 != 0;
}

// FUNCTION: WIZ8 0x004F8130
bool ItemHasFlags(W8WorldItem* item, unsigned int mask)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 998, 0);
    }
    return (item->flags & mask) != 0;
}

// FUNCTION: WIZ8 0x004F8170
void SetItemFlags(W8WorldItem* item, unsigned int mask, unsigned char enabled)
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

// FUNCTION: WIZ8 0x004F81C0
void SetItemAndEntityFlags(W8WorldItem* item, unsigned int mask, unsigned char enabled)
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
        item->owner->entity->SetFlags(mask, enabled);
    }
}

// FUNCTION: WIZ8 0x004F8300
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

// FUNCTION: WIZ8 0x004F8340
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

// FUNCTION: WIZ8 0x004EF420
unsigned int GetAveragePartyLevel(void)
{
    unsigned int total_level = 0;
    unsigned int occupied_slots = 0;
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (g_party_slot_rows[slot].flag_00 != 0) {
            total_level += g_party_characters[slot].level;
            ++occupied_slots;
        }
    }
    return total_level / occupied_slots;
}

// FUNCTION: WIZ8 0x004F88A0
int FindItemTableByName(const char* name)
{
    int index;

    for (index = 0; index < (int)g_item_table_count; ++index) {
        if (_stricmp(name, g_item_tables[index]->name) == 0) {
            return index;
        }
    }
    return -1;
}

static __forceinline W8WorldItem* CreateTableItem(
    unsigned int item_id,
    const srVector3T<float>* position)
{
    W8ItemInstance item;
    W8ItemInstance* item_pointer;
    W8WorldItem* result;

    if (item_id == -1) {
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

// FUNCTION: WIZ8 0x004F88F0
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

/* gXStatus.plsItemList, named by the ItemInfo assertion that bounds an index
   against PLLength(gXStatus.plsItemList). */
extern W8PList* g_world_item_list_00683fb5;
/* 0x00689B54: the cursor the iterator below resumes from. */
extern int g_world_item_cursor;

extern char* ConvertWideStringToString(const W8WideChar* wide);

/* Bit 0x20 of the item record's flag word, which is the only bit
   ItemInfoIsWorldPersistent reads. */
enum { W8_ITEM_FLAG_PERSISTENT = 0x20 };
/* Bit 1 of the world item's own flag word. */
enum { W8_WORLD_ITEM_FLAG_02 = 2 };

/* Free a whole group of world items, following the link that chains them. */
// FUNCTION: WIZ8 0x004F6CC0
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
// FUNCTION: WIZ8 0x004F8220
int FindItemRecordByName(const char* name)
{
    int index;
    const char* internal_name;

    for (index = 0; index < g_item_record_count; ++index) {
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
// FUNCTION: WIZ8 0x004F82B0
W8WorldItem* GetNextWorldItem(char restart)
{
    int index;

    if (restart) {
        g_world_item_cursor = 0;
    }
    index = g_world_item_cursor;
    if ((unsigned int)index < PListGetCount(g_world_item_list_00683fb5)) {
        ++g_world_item_cursor;
        return (W8WorldItem*)PListGetAt(g_world_item_list_00683fb5, index);
    }
    return 0;
}

/* Take one item out of the group chained onto another. Unlinking the head
   promotes whatever followed it; unlinking anything else just closes the gap.
   Answers the group's head afterwards. */
// FUNCTION: WIZ8 0x004F83A0
W8WorldItem* ItemInfoRemoveFromGroup(W8WorldItem* head, int unused, W8WorldItem* item)
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
// FUNCTION: WIZ8 0x004F8410
W8WorldItem* ItemInfoGroupGetNext(W8WorldItem* item)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1178,
                     "Bad ITEM_STRUCT in ItemInfoGroupGetNext");
    }
    return item->next;
}

/* Whether the item's record marks it as one the world keeps. */
// FUNCTION: WIZ8 0x004F91E0
bool ItemInfoIsWorldPersistent(const W8WorldItem* item)
{
    if (item == 0) {
        return false;
    }
    return (g_item_records[item->item.item_id].flags_041 & W8_ITEM_FLAG_PERSISTENT) != 0;
}

/* Copy a world item's carried item out onto the heap. */
// FUNCTION: WIZ8 0x004F9210
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
// FUNCTION: WIZ8 0x004F94A0
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
// FUNCTION: WIZ8 0x004F7FE0
W8WorldItem* ItemInfo(unsigned int item_list_index)
{
    W8WorldItem* item;

    if (item_list_index >= PListGetCount(g_world_item_list_00683fb5)) {
        srAssertFail("uiItemListIndex < (UINT32) PLLength(gXStatus.plsItemList)",
                     ITEM_MANAGER_CPP, 961, 0);
    }
    item = (W8WorldItem*)PListGetAt(g_world_item_list_00683fb5, item_list_index);
    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 965,
                     FormatString("ItemInfo: ERROR - PLGet failed, index %d, pList %d",
                                  item_list_index, g_world_item_list_00683fb5));
    }
    return item;
}

/* Where in the world item list one runtime id sits. Not finding it is a data
   error rather than a -1. */
// FUNCTION: WIZ8 0x004F8060
unsigned int ItemIndex(int runtime_id)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_world_item_list_00683fb5); ++index) {
        if (ItemInfo(index)->runtime_id == runtime_id) {
            return index;
        }
    }
    srAssertFail("FALSE", ITEM_MANAGER_CPP, 992,
                 FormatString("ItemIndex: ERROR - ItemID %d not found", runtime_id));
    return 0;
}

/* One growable vector of world items, reached field by field because
   ItemInfoMakeGroupList inlines the append rather than calling Add. */
struct W8WorldItemVector {
    void* vptr;                          /* 0x00 */
    int count;                           /* 0x04 */
    int capacity;                        /* 0x08 */
    W8WorldItem** data;                  /* 0x0c */
};

extern void GetWorldItemPosition(float* position);                       /* 0x004B8890 */
extern void GetPartyEyePosition(void* position);                         /* 0x00421070 */
extern void GetWorldItemBounds(float* lower, float* upper);              /* 0x0049FB30 */
extern unsigned char TraceToBounds(void* eye, const float* lower, const float* upper);
/* 0x0046F820 */
extern void Function49FA30(W8World* world);
extern void Function46E5E0(W8World* world);
extern void Function49F720(const float* position);
extern unsigned char SettleItemOnGround(
    float* position, void** out_hit, int arg_3, double limit);           /* 0x00433820 */
extern void RemoveItemFromSector(int sector, W8WorldItem* item);         /* 0x004B7B50 */
extern void AddItemToSector(int sector, W8WorldItem* item);              /* 0x004B7AD0 */
extern void ReplaceOrCreateItem(
    W8ItemInstance* item, int item_id, int count, unsigned char quality, int arg_5);
extern int g_item_manager_pending_00683FA9;
/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */
extern W8LevelRuntimeBlock* g_item_selection_owner_0068EDCC;
extern int g_screen_state_0068ec78;

extern float g_world_scale_005ebc40;

/* Flatten one item's whole group into a vector, the item itself first and then
   everything chained onto it. The append is written out rather than called, so
   a failed growth silently drops that entry and keeps walking. */
// FUNCTION: WIZ8 0x004F8440
int ItemInfoMakeGroupList(W8WorldItem* item, int unused, W8WorldItemVector* out)
{
    W8WorldItem* next;
    W8WorldItem** previous;
    int wanted;
    int index;

    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1185,
                     "Bad ITEM_STRUCT in ItemInfoMakeGroupList");
    }

    wanted = out->count + 1;
    if (out->capacity < wanted) {
        previous = out->data;
        out->data = (W8WorldItem**)operator new(wanted * 4);
        if (out->data == 0) {
            out->data = previous;
            goto walk;
        }
        out->capacity = wanted;
        for (index = 0; index < out->count; ++index) {
            out->data[index] = previous[index];
        }
        operator delete(previous);
    }
    out->data[out->count] = item;
    ++out->count;

walk:
    for (next = item->next; next != 0; next = next->next) {
        wanted = out->count + 1;
        if (out->capacity < wanted) {
            previous = out->data;
            out->data = (W8WorldItem**)operator new(wanted * 4);
            if (out->data == 0) {
                out->data = previous;
                continue;
            }
            out->capacity = wanted;
            for (index = 0; index < out->count; ++index) {
                out->data[index] = previous[index];
            }
            operator delete(previous);
        }
        out->data[out->count] = next;
        ++out->count;
    }
    return out->count;
}

/* Take one item out of the world. Its three assertions name the two fields
   they guard - fActive and p3D - and the item keeps its last position and
   entity flags so it can be put back. */
// FUNCTION: WIZ8 0x004F70D0
void DeactivateWorldItem(W8WorldItem* item)
{
    float position[3];

    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 554, 0);
    }
    if (item->unknown_08 == 0) {
        srAssertFail("pItemInfo->fActive", ITEM_MANAGER_CPP, 555, 0);
    }
    if (item->owner == 0) {
        srAssertFail("pItemInfo->p3D != NULL", ITEM_MANAGER_CPP, 556, 0);
    }

    if (g_screen_state_0068ec78 == 7 && g_item_selection_owner_0068EDCC != 0 &&
        g_item_selection_owner_0068EDCC->selected_item == item->runtime_id) {
        g_item_selection_owner_0068EDCC->selected_item = -1;
    }

    GetWorldItemPosition(position);
    item->position.x = position[0];
    item->position.y = position[1];
    item->position.z = position[2];
    item->entity_flags = item->owner->entity->flags;

    Function49FA30(GetWorld());
    Function46E5E0(GetWorld());
    delete item->owner;
    item->owner = 0;
    item->unknown_08 = 0;
    --g_item_manager_pending_00683FA9;
}

/* Whether one world item is close enough to a point to be reached, and in
   sight of the party's eye. The distance is compared before the trace, so a
   far item is never traced to. */
// FUNCTION: WIZ8 0x004F8560
unsigned char IsWorldItemWithinReach(void* item, const float* from, float radius)
{
    float position[3];
    float lower[3];
    float upper[3];
    unsigned char eye[12];
    float dx;
    float dy;
    float dz;

    GetWorldItemPosition(position);
    GetPartyEyePosition(eye);

    dx = position[0] - from[0];
    dy = position[1] - from[1];
    dz = position[2] - from[2];
    if (dx * dx + dy * dy + dz * dz < radius * radius) {
        GetWorldItemBounds(lower, upper);
        lower[0] += position[0];
        lower[1] += position[1];
        lower[2] += position[2];
        upper[0] += position[0];
        upper[1] += position[1];
        upper[2] += position[2];
        if (TraceToBounds(eye, lower, upper)) {
            return 1;
        }
    }
    return 0;
}

/* Drop one item onto the ground below where it is. The search starts one world
   unit up so an item already resting does not settle into the floor; landing
   moves it between sectors and clears its saved-marker flag. */
// FUNCTION: WIZ8 0x004F93D0
int SettleWorldItem(W8WorldItem* item)
{
    float start[3];
    void* hit;
    int sector;

    start[0] = item->position.x;
    start[2] = item->position.z;
    start[1] = item->position.y + g_world_scale_005ebc40;

    item->flags &= ~2u;
    item->unknown_35 = 0;

    if (!SettleItemOnGround(start, &hit, 1, 250.0)) {
        return 0;
    }

    sector = g_engine_state_6598a4->current_sector;
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
        Function49F720(start);
    }
    item->position.x = start[0];
    item->position.y = start[1];
    item->position.z = start[2];
    return 1;
}

/* Rebuild every world item's carried instance from its own item id, which
   re-rolls whatever the record decides rather than keeping what was there. */
// FUNCTION: WIZ8 0x004F94C0
void RebuildAllWorldItemInstances(void)
{
    unsigned int index;
    W8WorldItem* item;

    for (index = 0; index < PListGetCount(g_world_item_list_00683fb5); ++index) {
        item = ItemInfo(index);
        ReplaceOrCreateItem(&item->item, item->item.item_id, 0, 0, 0);
    }
}
