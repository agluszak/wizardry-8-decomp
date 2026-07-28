#include "wiz8/gameplay_boundaries.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"

#include <string.h>

struct W8ItemSelectionOwner0068EDCC {
    unsigned char unknown_000[0x268];
    int selected_item;                   /* 0x268: reset when state seven is active */
};

extern int g_item_manager_initialized_006874C2;
extern int g_item_manager_pending_00683FA9;
extern int g_item_manager_state_0068EC78;
extern W8ItemSelectionOwner0068EDCC* g_item_selection_owner_0068EDCC;
extern W8PList* g_item_manager_entries_00683FB5;

#define ITEM_MANAGER_CPP "C:\\\\Projects\\\\Wizardry 8\\\\Local Code\\\\ItemManager.cpp"

// FUNCTION: WIZ8 0x004F69F0
bool InitializeItemManagerState()
{
    g_item_manager_initialized_006874C2 = 1;
    g_item_manager_pending_00683FA9 = 0;
    if (g_item_manager_state_0068EC78 == 7 && g_item_selection_owner_0068EDCC != 0) {
        g_item_selection_owner_0068EDCC->selected_item = -1;
    }
    if (g_item_manager_entries_00683FB5 != 0) {
        PListClear(g_item_manager_entries_00683FB5);
        return g_item_manager_entries_00683FB5 != 0;
    }
    g_item_manager_entries_00683FB5 = PListCreate();
    return g_item_manager_entries_00683FB5 != 0;
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
