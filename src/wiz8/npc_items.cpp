#include "wiz8/npc_state.h"
#include "wiz8/layouts/item_tables.h"

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
