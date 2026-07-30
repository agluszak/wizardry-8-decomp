#include "wiz8/npc_state.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/item_tables.h"
#include "wiz8/item_spawning.h"
#include "wiz8/monster_runtime.h"
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

/* Top up an NPC's stock from its database rules. A rule restocks only while it
   is not the persistent kind, while any fact gating its item is set, and while
   the NPC holds no more than half the configured quantity. The difficulty tier
   then becomes a percentage chance, and the shortfall is jittered by half up or
   down before being added. Item 0x1fc is gated behind fact 0x15f; neither has a
   recovered symbolic name. */
// FUNCTION: WIZ8 0x0055ab80
int RestockNpcItems(W8NpcState* npc)
{
    W8NpcItemStockRule* rule;
    W8NpcItemStockRule* candidate;
    W8NpcItemEntry* entry;
    unsigned int rule_count;
    unsigned int rule_index;
    unsigned int search_count;
    unsigned int search;
    int item_id;
    int configured;
    unsigned char held;
    unsigned char amount;
    unsigned char jitter;
    char tier;
    char chance;
    int roll;

    rule_count = PListGetCount(npc->record->item_stock_rules);
    rule_index = 0;
    if (rule_count > 0) {
        do {
            rule = static_cast<W8NpcItemStockRule*>(
                PListGetAt(npc->record->item_stock_rules, rule_index));
            if (rule->persistent != 0 ||
                (rule->item_id == 0x1fc && GetFact(0x15f) == 0)) {
                goto next_rule;
            }

            item_id = rule->item_id;
            configured = 0;
            search_count = PListGetCount(npc->record->item_stock_rules);
            for (search = 0; search < search_count; ++search) {
                candidate = static_cast<W8NpcItemStockRule*>(
                    PListGetAt(npc->record->item_stock_rules, search));
                if (candidate->item_id == item_id) {
                    configured = candidate->quantity;
                    break;
                }
            }
            if (configured <= 0) {
                goto next_rule;
            }

            item_id = rule->item_id;
            held = 0;
            search_count = PListGetCount(npc->items);
            for (search = 0; search < search_count; ++search) {
                entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, search));
                if (entry != 0 && entry->item.item_id == item_id) {
                    held = entry->quantity;
                    break;
                }
            }
            if (held > configured / 2) {
                goto next_rule;
            }

            if (rule->persistent == 0) {
                tier = RateItemIdentifyDifficulty(npc, rule->item_id);
            }
            else {
                tier = 4;
            }
            switch (tier) {
            case 0:
                chance = 0;
                break;
            case 1:
                chance = 25;
                break;
            case 2:
                chance = 50;
                break;
            case 3:
                chance = 75;
                break;
            case 4:
                chance = 100;
                break;
            default:
                chance = 0;
                break;
            }
            if (static_cast<unsigned int>(chance) <= Random(100)) {
                goto next_rule;
            }

            amount = configured - held;
            roll = Random(3);
            if (roll == 0) {
                jitter = static_cast<unsigned char>(amount >> 1);
                amount = amount + jitter;
            }
            else if (roll == 1) {
                jitter = static_cast<unsigned char>(-(amount >> 1));
                amount = amount + jitter;
            }
            if (amount != 0) {
                AddNpcItem(npc, rule->item_id, amount);
            }

        next_rule:
            ++rule_index;
        } while (rule_index < rule_count);
    }
    return 1;
}

/* Rate an item's identify difficulty against the band the party's average level
   supports. Three means the difficulty sits inside the band, zero that it is
   past the upper bound, and one or two grade how far below the lower bound it
   falls. Every comparison here is signed, so the difficulty byte is read into a
   char, and the tier is returned byte-sized. The NPC argument is unused by the
   original. */
// FUNCTION: WIZ8 0x0055aad0
char RateItemIdentifyDifficulty(W8NpcState* npc, int item_id)
{
    char difficulty;
    char level;
    char base;
    char raw_upper;
    char lower;
    char upper;

    difficulty = static_cast<char>(g_item_records[item_id].identify_difficulty);
    level = static_cast<char>(GetAveragePartyLevel());
    base = static_cast<char>(level * 100 / 30) / 5;
    raw_upper = base + 4;
    if (base <= 16) {
        lower = base < 1 ? 1 : base;
    }
    else {
        lower = 16;
    }
    if (raw_upper <= 20) {
        upper = raw_upper < 4 ? 4 : raw_upper;
    }
    else {
        upper = 20;
    }
    if (difficulty > upper) {
        return 0;
    }
    if (difficulty < lower) {
        return static_cast<char>((difficulty + 4 >= lower) + 1);
    }
    return 3;
}

/* Add stock that only becomes ordinary trade stock once the world clock passes
   the given delay. This is what establishes the leading field as a clock stamp
   rather than a state enum. */
// FUNCTION: WIZ8 0x0055aa80
int AddNpcItemWithDelay(W8NpcState* npc, int item_id, unsigned int quantity, int delay)
{
    int index;
    W8NpcItemEntry* entry;

    if (npc == 0) {
        return -1;
    }
    index = AddNpcItem(npc, item_id, quantity);
    if (index == -1) {
        return -1;
    }
    entry = static_cast<W8NpcItemEntry*>(PListGetAt(npc->items, index));
    entry->available_at = g_world_clock_00686a48 + delay;
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
