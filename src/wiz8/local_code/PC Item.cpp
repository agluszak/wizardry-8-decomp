#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* The twelve places an item can be worn or held. GetItemDefaultEquipSlot maps
   an equipment class onto one of these, and GetPairedEquipSlot swaps a hand
   for the hand opposite it. Only the four hand slots have their pairing
   proven; the rest are numbered by that mapping and nothing more. */
enum {
    W8_EQUIP_SLOT_PRIMARY_RIGHT = 6,
    W8_EQUIP_SLOT_PRIMARY_LEFT = 7,
    W8_EQUIP_SLOT_ALTERNATE_RIGHT = 8,
    W8_EQUIP_SLOT_ALTERNATE_LEFT = 9,
    W8_EQUIP_SLOT_NONE = -1
};

/* Items of equipment class four are priced and carried by the bundle rather
   than singly, so a stack of them divides its bundle value out - rounding up,
   which is where the addend comes from. */
enum { W8_ITEM_BUNDLE_SIZE = 25 };

/* One item's weight, or nothing at all when the caller has no item. Unlike the
   stack form below this ignores how many are held. */
// FUNCTION: WIZ8 0x0051B8B0
unsigned int GetItemUnitWeight(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->item_id].weight;
    }
    return 0;
}

/* What a whole stack weighs. An empty slot weighs nothing, and a slot holding
   an item that does not stack still weighs one of it - the count is zero for
   every quantity kind but the stacking one. */
// FUNCTION: WIZ8 0x0051BFD0
unsigned int GetItemStackWeight(const W8ItemInstance* item)
{
    unsigned int weight;

    weight = 0;
    if (item->item_id != -1) {
        weight = g_item_records[item->item_id].weight;
        weight = (item->stack_count > 0 ? item->stack_count : 1) * weight;
    }
    return weight;
}

/* What a whole stack is worth in gold. Bundled goods divide their bundle price
   across the count and round up; anything else that stacks multiplies, and
   everything else is worth exactly what one of it is worth. */
// FUNCTION: WIZ8 0x0051B840
unsigned int GetItemStackValue(const W8ItemInstance* item)
{
    if (g_item_records[item->item_id].equip_class == 4) {
        return (item->stack_count * g_item_records[item->item_id].value +
                (W8_ITEM_BUNDLE_SIZE - 1)) / W8_ITEM_BUNDLE_SIZE;
    }
    if (g_item_records[item->item_id].quantity_kind == 1 && item->stack_count > 1) {
        return g_item_records[item->item_id].value * item->stack_count;
    }
    return g_item_records[item->item_id].value;
}

/* Where an item of this kind wants to go. Two of the thirteen equipment
   classes answer differently once play has started: the one-handed weapon and
   the off-hand class move from the primary pair of hand slots to the alternate
   pair, which the character-creation screens do not fill. */
// FUNCTION: WIZ8 0x0051C4E0
int GetItemDefaultEquipSlot(int item_id)
{
    switch (g_item_records[item_id].equip_class) {
    case 0:
    case 1:
        return W8_EQUIP_SLOT_PRIMARY_RIGHT;
    case 3:
        return g_game_started ? W8_EQUIP_SLOT_ALTERNATE_RIGHT
                              : W8_EQUIP_SLOT_PRIMARY_RIGHT;
    case 4:
        return g_game_started ? W8_EQUIP_SLOT_ALTERNATE_LEFT
                              : W8_EQUIP_SLOT_PRIMARY_LEFT;
    case 2:
        return W8_EQUIP_SLOT_ALTERNATE_RIGHT;
    case 11:
        return 1;
    case 8:
        return 0;
    case 6:
        return 4;
    case 7:
        return 10;
    case 9:
        return 5;
    case 10:
        return 11;
    case 12:
        return 3;
    case 5:
        return W8_EQUIP_SLOT_PRIMARY_LEFT;
    default:
        return W8_EQUIP_SLOT_NONE;
    }
}

/* The hand opposite the one given. Anything that is not a hand has no
   opposite. */
// FUNCTION: WIZ8 0x0051C8B0
int GetPairedEquipSlot(int equip_slot)
{
    switch (equip_slot) {
    case W8_EQUIP_SLOT_PRIMARY_RIGHT:
        return W8_EQUIP_SLOT_PRIMARY_LEFT;
    case W8_EQUIP_SLOT_PRIMARY_LEFT:
        return W8_EQUIP_SLOT_PRIMARY_RIGHT;
    case W8_EQUIP_SLOT_ALTERNATE_RIGHT:
        return W8_EQUIP_SLOT_ALTERNATE_LEFT;
    case W8_EQUIP_SLOT_ALTERNATE_LEFT:
        return W8_EQUIP_SLOT_ALTERNATE_RIGHT;
    default:
        return W8_EQUIP_SLOT_NONE;
    }
}

/* Take gold from the party. Asking for more than it has empties the purse
   rather than wrapping it around. */
// FUNCTION: WIZ8 0x0051BF40
void SpendPartyGold(unsigned int amount)
{
    if (amount > g_party_gold) {
        g_party_gold = 0;
    } else {
        g_party_gold -= amount;
    }
}

// FUNCTION: WIZ8 0x005222D0
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot)
{
    unsigned int equipped_index;
    unsigned int carried_index;
    unsigned int pool_index;
    unsigned char* character;
    unsigned char* equipped;

    if (item == 0) {
        srAssertFail(
            "pPCItem != NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp",
            0x151b,
            0);
    }

    /* The original holds the character base in one register and advances it in
       place for the carried array, rather than deriving each cursor afresh. */
    equipped_index = 0;
    character = (unsigned char*)(g_party_characters + character_index);
    equipped = character + 0x1029;
    for (; equipped_index < 8; ++equipped_index, equipped += 0xc) {
        if (item == equipped) {
            *origin = 0;
            *slot = (unsigned short)equipped_index;
            return;
        }
    }

    carried_index = 0;
    character += 0xf5d;
    for (; carried_index < 12; ++carried_index, character += 0xc) {
        if (item == character) {
            *origin = 1;
            *slot = (unsigned short)carried_index;
            return;
        }
    }

    for (pool_index = 0; pool_index < g_shared_item_pool_count; ++pool_index) {
        if (item == g_shared_item_pool + pool_index * 0xc) {
            *origin = 2;
            *slot = (unsigned short)pool_index;
            return;
        }
    }

    *origin = 0xff;
    *slot = 0xffff;
}
