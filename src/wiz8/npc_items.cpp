#include "wiz8/npc_state.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/item_spawning.h"
#include "random.h"
#include <string.h>

/* Add stock to an NPC's item list. Equipment, which is equip_class four, never
   merges: it takes one fresh entry per requested unit and a fixed stock count.
   Anything else merges into the existing entry for that item when there is one,
   and the quantity lands in whichever of the two counts the item's quantity
   kind uses.

   The allocation is 0x14 bytes with a source-level null test around the clear
   and the item build, and the retail body carries no exception frame, so the
   entry is an ordinary cleared allocation rather than a constructed object.

   The original guards the loop and falls through to one shared exit that
   returns the index register, so when no unit is added at all it returns
   whatever that register still held rather than a list index. That is the
   original's own defect, reproduced here by leaving the index unset on the
   path that never enters the loop. */
// FUNCTION: WIZ8 0x0055a7b0
int AddNpcItem(W8NpcState* npc, int item_id, unsigned int quantity)
{
    W8ItemDatabaseRecord* record;
    W8NpcItemEntry* entry;
    unsigned int existing_count;
    unsigned int repeats;
    unsigned int added;
    unsigned int search;
    int index;

    if (npc == 0) {
        return -1;
    }
    if (npc->items == 0) {
        npc->items = PListCreate();
    }
    record = &g_item_records[item_id];
    if (record->equip_class == 4) {
        repeats = quantity;
    }
    else {
        repeats = 1;
    }
    added = 0;
    if (repeats > 0) {
        do {
            index = -1;
            if (record->equip_class != 4) {
                existing_count = PListGetCount(npc->items);
                for (search = 0; search < existing_count; ++search) {
                    entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, search));
                    if (entry != 0 && entry->item.item_id == item_id) {
                        index = search;
                        break;
                    }
                }
            }
            if (index == -1) {
                entry = new W8NpcItemEntry;
                if (entry != 0) {
                    memset(entry, 0, sizeof(*entry));
                    ReplaceOrCreateItem(&entry->item, item_id, 1, 1, 0);
                }
                entry->item.stack_count = 0;
                index = PListAdd(npc->items, entry);
            }
            entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, index));
            if (entry == 0) {
                return -1;
            }
            if (record->equip_class == 4) {
                entry->item.stack_count = 0x19;
                entry->quantity = 1;
            }
            else if (record->quantity_kind == 1) {
                entry->item.stack_count += quantity;
                entry->quantity = 1;
            }
            else {
                entry->item.stack_count = 1;
                entry->quantity += quantity;
            }
            ++added;
        } while (added < repeats);
    }
    return index;
}

// FUNCTION: WIZ8 0x0055ade0
W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index)
{
    return static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, index));
}

// FUNCTION: WIZ8 0x0055b710
unsigned int GetNpcItemCount(W8NpcState* npc)
{
    return PListGetCount(npc->items);
}

// FUNCTION: WIZ8 0x0055ae00
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity)
{
    W8NpcItemEntry* entry;

    if (quantity == 0) {
        return 0;
    }
    entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, index));
    if (entry == 0) {
        return 0;
    }
    if (g_item_records[entry->item.item_id].quantity_kind == 1) {
        if (entry->item.stack_count < quantity) {
            entry->item.stack_count = quantity;
        }
        entry->item.stack_count -= quantity;
        if (entry->item.stack_count == 0) {
            entry->quantity = 0;
            return 1;
        }
    }
    else {
        if (entry->quantity < quantity) {
            quantity = entry->quantity;
        }
        entry->quantity -= quantity;
    }
    return 1;
}

/* Remove ordinary stock that is neither protected by its item flags nor kept
   by the NPC database's persistent stock rule. */
// FUNCTION: WIZ8 0x0055ae70
void DecayNpcInventory(W8NpcState* npc)
{
    int item_id;
    unsigned int item_count;
    W8NpcItemEntry* entry;
    unsigned int rule_count;
    W8NpcItemStockRule* rule;
    char quantity;
    unsigned int rule_index;
    unsigned int item_index;
    unsigned char remaining;

    item_count = PListGetCount(npc->items);
    item_index = 0;
    if (item_count > 0) {
        do {
            entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, item_index));
            if (((entry != 0 && entry->quantity != 0) &&
                 (item_id = entry->item.item_id,
                  (g_item_records[item_id].flags_041 & 0x12) == 0))) {
                if (entry->available_at > 0) {
                    goto next_item;
                }
                rule_count = PListGetCount(npc->record->item_stock_rules);
                for (rule_index = 0; rule_index < rule_count; ++rule_index) {
                    rule = static_cast<W8NpcItemStockRule*>(
                        PListGetAt(npc->record->item_stock_rules, rule_index));
                    if (rule->item_id == item_id) {
                        if (rule->persistent == 1) {
                            goto next_item;
                        }
                        break;
                    }
                }

                remaining = entry->quantity;
                quantity = 0;
                if (remaining > 0) {
                    rule_count = remaining;
                    do {
                        if (Random(100) < 100) {
                            ++quantity;
                        }
                        --rule_count;
                    } while (rule_count != 0);
                    if (quantity != 0) {
                        ConsumeNpcItemQuantity(npc, item_index, quantity);
                    }
                }
            }

        next_item:
            ++item_index;
        } while (item_index < item_count);
    }
}
