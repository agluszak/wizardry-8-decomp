#include "wiz8/gameplay_boundaries.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"

#include <string.h>

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
                item_value = item_record->unknown_086;
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
