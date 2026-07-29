#include "wiz8/npc_state.h"
#include "wiz8/layouts/item_tables.h"
#include "random.h"

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
                if (entry->state > 0) {
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
