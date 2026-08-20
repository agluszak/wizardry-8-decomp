#include "wiz8/npc_state.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/item_tables.h"
#include "wiz8/item_spawning.h"
#include "wiz8/monster_runtime.h"
#include "random.h"
#include <stdlib.h>
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
        npc->items = PLCreate();
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
                existing_count = PLLength(npc->items);
                for (search = 0; search < existing_count; ++search) {
                    entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, search));
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
                index = PLAdoptAppend(npc->items, entry);
            }
            else {
                entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
            }
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

/* Keep an NPC's stock current. Three passes, each with its own trigger.

   First every entry is normalized: the quantity moves into the count its kind
   uses, and a stackable entry whose stack has fallen below its remaining count
   is topped back up to it.

   Then, once the restock clock is more than 0xa8c0 old or the caller forces it,
   every persistent stock rule is replenished toward its configured quantity.
   Unlike RestockNpcItems this pass rolls no chance: persistent stock always
   comes back, jittered by half up or down.

   Finally, once the maintenance clock is more than 0x15180 old or the caller
   forces it, the stock decays, non-persistent rules restock, emptied entries are
   removed, and both clocks are restamped. Removing an entry steps the cursor back
   so the shifted-down successor is not skipped. */
// FUNCTION: WIZ8 0x0055afa0
unsigned char MaintainNpcStock(W8NpcState* npc, char force)
{
    W8NpcItemEntry* entry;
    W8NpcItemStockRule* rule;
    W8NpcItemStockRule* candidate;
    unsigned int count;
    unsigned int index;
    unsigned int rule_index;
    unsigned int search_count;
    unsigned int search;
    int item_id;
    unsigned char configured;
    unsigned char held;
    unsigned char jitter;
    int roll;

    count = PLLength(npc->items);
    index = 0;
    if (count > 0) {
        do {
            entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
            if (((entry != 0 && entry->quantity != 0) &&
                 (NormalizeItemQuantityKind(&entry->item),
                  g_item_records[entry->item.item_id].quantity_kind == 1)) &&
                entry->quantity > entry->item.stack_count) {
                entry->item.stack_count = entry->quantity;
                entry->quantity = 1;
            }
            ++index;
        } while (index < count);
    }

    if (static_cast<unsigned int>(g_world_clock_00686a48 - npc->restock_clock) > 0xa8c0 ||
        force != 0) {
        npc->restock_clock = g_world_clock_00686a48;
        count = PLLength(npc->record->item_stock_rules);
        rule_index = 0;
        if (count > 0) {
            do {
                rule = static_cast<W8NpcItemStockRule*>(
                    PLGet(npc->record->item_stock_rules, rule_index));
                if (rule->persistent != 0) {
                    item_id = rule->item_id;
                    configured = 0;
                    search_count = PLLength(npc->record->item_stock_rules);
                    for (search = 0; search < search_count; ++search) {
                        candidate = static_cast<W8NpcItemStockRule*>(
                            PLGet(npc->record->item_stock_rules, search));
                        if (candidate->item_id == item_id) {
                            configured = candidate->quantity;
                            break;
                        }
                    }
                    if (configured != 0) {
                        item_id = rule->item_id;
                        held = 0;
                        search_count = PLLength(npc->items);
                        for (search = 0; search < search_count; ++search) {
                            entry = static_cast<W8NpcItemEntry*>(
                                PLGet(npc->items, search));
                            if (entry != 0 && entry->item.item_id == item_id) {
                                held = entry->quantity;
                                break;
                            }
                        }
                        if (held <= configured / 2) {
                            configured = configured - held;
                            roll = Random(3);
                            if (roll == 0) {
                                jitter = static_cast<unsigned char>(configured >> 1);
                                configured = configured + jitter;
                            }
                            else if (roll == 1) {
                                jitter = static_cast<unsigned char>(-(configured >> 1));
                                configured = configured + jitter;
                            }
                            if (configured != 0) {
                                AddNpcItem(npc, rule->item_id, configured);
                            }
                        }
                    }
                }
                ++rule_index;
            } while (rule_index < count);
        }
    }

    if (static_cast<unsigned int>(g_world_clock_00686a48 - npc->maintenance_clock) < 0x15180 &&
        force == 0) {
        return 0;
    }
    DecayNpcInventory(npc);
    RestockNpcItems(npc);
    count = PLLength(npc->items);
    index = 0;
    if (count != 0) {
        do {
            entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
            if (entry != 0 && entry->quantity == 0) {
                delete static_cast<W8NpcItemEntry*>(PLRemoveAt(npc->items, index));
                if (index != 0) {
                    index = index - 1;
                }
                count = PLLength(npc->items);
            }
            ++index;
        } while (index < count);
    }
    npc->maintenance_clock = g_world_clock_00686a48;
    npc->restock_clock = g_world_clock_00686a48;
    return 1;
}

/* Populate an NPC's stock from its database rules on first use. Each rule rolls
   once per configured unit against the chance its difficulty tier selects, and
   the successes are added as one batch. The stock is then sorted and both clock
   stamps are set, which is what makes 0x0055AFA0's two staleness windows start
   from the same instant.

   A rule whose item id is past the end of the item database is skipped, and an
   NPC with no rules or an empty rule list ends up with no item list at all. */
// FUNCTION: WIZ8 0x0055a630
unsigned char PopulateNpcStock(W8NpcState* npc)
{
    W8PList* rules;
    W8NpcItemStockRule* rule;
    unsigned int rule_count;
    unsigned int rule_index;
    unsigned int remaining;
    int item_id;
    char persistent;
    char added;
    char chance;
    char tier;

    rules = npc->record->item_stock_rules;
    if (rules == 0) {
        npc->items = 0;
        return 1;
    }
    rule_count = PLLength(rules);
    if (rule_count == 0) {
        npc->items = 0;
        return 1;
    }
    if (npc->items == 0) {
        npc->items = PLCreate();
    }
    rule_count = PLLength(npc->record->item_stock_rules);
    rule_index = 0;
    if (rule_count > 0) {
        do {
            rule = static_cast<W8NpcItemStockRule*>(
                PLGet(npc->record->item_stock_rules, rule_index));
            item_id = rule->item_id;
            if (item_id < (int)gXStatus.uiItemsInDatabase) {
                persistent = rule->persistent;
                added = 0;
                if (rule->quantity != 0) {
                    remaining = rule->quantity;
                    do {
                        if (persistent == 0) {
                            tier = RateItemIdentifyDifficulty(npc, item_id);
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
                        if (Random(100) < static_cast<unsigned int>(chance)) {
                            ++added;
                        }
                        --remaining;
                    } while (remaining != 0);
                }
                if (added != 0) {
                    AddNpcItem(npc, rule->item_id, added);
                }
            }
            ++rule_index;
        } while (rule_index < rule_count);
    }
    SortNpcItems(npc);
    npc->maintenance_clock = g_world_clock_00686a48;
    npc->restock_clock = g_world_clock_00686a48;
    return 1;
}

/* SortNpcItems stores the clock before the item itself. Its qsort predicate
   therefore delegates to the ordinary item-pool ordering on the embedded item. */
// FUNCTION: WIZ8 0x0055baf0
static int __cdecl CompareNpcItems(const void* left, const void* right)
{
    const W8NpcItemEntry* first = static_cast<const W8NpcItemEntry*>(left);
    const W8NpcItemEntry* second = static_cast<const W8NpcItemEntry*>(right);

    return CompareItemsForPool(&first->item, &second->item);
}

/* Sort an NPC's stock in place by flattening the owned list into an array,
   sorting that, and rebuilding the list from it. The array allocation of one
   0x14-byte element per entry, and the five-dword element copies, are
   independent confirmation that an entry is 0x14 rather than the 0x11 a packed
   layout would give. */
// FUNCTION: WIZ8 0x0055b9c0
void SortNpcItems(W8NpcState* npc)
{
    W8NpcItemEntry* array;
    W8NpcItemEntry* cursor;
    W8NpcItemEntry* entry;
    W8PList* items;
    unsigned int count;
    unsigned int index;

    count = PLLength(npc->items);
    if (count != 0 && (array = new W8NpcItemEntry[count]) != 0) {
        cursor = array;
        for (index = 0; index < count; ++index) {
            *cursor = *static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
            cursor = cursor + 1;
        }
        qsort(array, count, sizeof(W8NpcItemEntry), CompareNpcItems);

        items = npc->items;
        while (PLLength(items) != 0) {
            delete static_cast<W8NpcItemEntry*>(PLRemoveAt(items, 0));
        }
        PListFreeData(items);
        PLDestroy(items);
        npc->items = PLCreate();
        cursor = array;
        for (index = 0; index < count; ++index) {
            entry = new W8NpcItemEntry;
            if (entry == 0) {
                return;
            }
            *entry = *cursor;
            PLAdoptAppend(npc->items, entry);
            cursor = cursor + 1;
        }
        delete[] array;
    }
}

/* Release an NPC's owned item list, but only for the database records that ask
   for it. */
// FUNCTION: WIZ8 0x0055a5d0
void ClearNpcItems(W8NpcState* npc)
{
    W8PList* items;

    if (npc->record->flag_055 != 0 && (items = npc->items) != 0) {
        while (PLLength(items) != 0) {
            delete static_cast<W8NpcItemEntry*>(PLRemoveAt(items, 0));
        }
        PListFreeData(items);
        PLDestroy(items);
        npc->items = 0;
    }
}

/* Add stock described by an item instance. Only the instance's item id is used;
   the entry receives a freshly built item rather than a copy of the argument.
   Equipment, equip_class four, always takes a new entry instead of merging. The
   quantity then lands in whichever of the two counts the item's quantity kind
   uses, and the other count is raised to one while it is still clear. */
// FUNCTION: WIZ8 0x0055a930
int AddNpcItemFromInstance(W8NpcState* npc, const W8ItemInstance* item, char quantity)
{
    W8NpcItemEntry* entry;
    unsigned int existing_count;
    unsigned int search;
    int item_id;
    int index;

    if (npc == 0) {
        return -1;
    }
    if (item->item_id < 0) {
        return -1;
    }
    if (npc->items == 0) {
        npc->items = PLCreate();
    }
    item_id = item->item_id;
    index = -1;
    if (g_item_records[item_id].equip_class != 4) {
        existing_count = PLLength(npc->items);
        for (search = 0; search < existing_count; ++search) {
            entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, search));
            if (entry != 0 && entry->item.item_id == item_id) {
                index = search;
                break;
            }
        }
    }
    if (index == -1) {
        item_id = item->item_id;
        entry = new W8NpcItemEntry;
        if (entry != 0) {
            memset(entry, 0, sizeof(*entry));
            ReplaceOrCreateItem(&entry->item, item_id, 1, 1, 0);
        }
        entry->item.stack_count = 0;
        index = PLAdoptAppend(npc->items, entry);
    }
    else {
        entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
    }
    if (entry == 0) {
        return -1;
    }
    if (g_item_records[entry->item.item_id].quantity_kind == 1) {
        if (entry->quantity == 0) {
            entry->quantity = 1;
        }
        entry->item.stack_count = entry->item.stack_count + quantity;
        return index;
    }
    if (entry->item.stack_count == 0) {
        entry->item.stack_count = 1;
    }
    entry->quantity = entry->quantity + quantity;
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

    rule_count = PLLength(npc->record->item_stock_rules);
    rule_index = 0;
    if (rule_count > 0) {
        do {
            rule = static_cast<W8NpcItemStockRule*>(
                PLGet(npc->record->item_stock_rules, rule_index));
            if (rule->persistent != 0 ||
                (rule->item_id == 0x1fc && GetFact(0x15f) == 0)) {
                goto next_rule;
            }

            item_id = rule->item_id;
            configured = 0;
            search_count = PLLength(npc->record->item_stock_rules);
            for (search = 0; search < search_count; ++search) {
                candidate = static_cast<W8NpcItemStockRule*>(
                    PLGet(npc->record->item_stock_rules, search));
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
            search_count = PLLength(npc->items);
            for (search = 0; search < search_count; ++search) {
                entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, search));
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
    entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
    entry->available_at = g_world_clock_00686a48 + delay;
    return index;
}

// FUNCTION: WIZ8 0x0055ade0
W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index)
{
    return static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
}

// FUNCTION: WIZ8 0x0055b710
unsigned int GetNpcItemCount(W8NpcState* npc)
{
    return PLLength(npc->items);
}

// FUNCTION: WIZ8 0x0055ae00
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity)
{
    W8NpcItemEntry* entry;

    if (quantity == 0) {
        return 0;
    }
    entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
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

    item_count = PLLength(npc->items);
    item_index = 0;
    if (item_count > 0) {
        do {
            entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, item_index));
            if (((entry != 0 && entry->quantity != 0) &&
                 (item_id = entry->item.item_id,
                  (g_item_records[item_id].flags_041 & 0x12) == 0))) {
                if (entry->available_at > 0) {
                    goto next_item;
                }
                rule_count = PLLength(npc->record->item_stock_rules);
                for (rule_index = 0; rule_index < rule_count; ++rule_index) {
                    rule = static_cast<W8NpcItemStockRule*>(
                        PLGet(npc->record->item_stock_rules, rule_index));
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
