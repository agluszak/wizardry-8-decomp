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

/* Which slot each bit of an equip-slot mask stands for: bit N is slot N. */
#define W8_EQUIP_SLOT_BIT(slot) ((unsigned short)(1 << (slot)))

/* Bits of W8ItemDatabaseRecord::flags_041 that recovered bodies read. */
enum {
    W8_ITEM_FLAG_NO_DISCARD = 0x02,
    W8_ITEM_FLAG_TWO_HANDED = 0x04,
    W8_ITEM_FLAG_OFF_HAND_ALLOWED = 0x08
};

/* No weapon skill at all, which GetItemEquipSlotMask treats as a data error. */
enum { W8_WEAPON_SKILL_NONE = 0xff };

/* Everything below equipment class four is a weapon; four and five are the
   off-hand pair, and the rest are worn rather than held. */
enum { W8_EQUIP_CLASS_FIRST_NON_WEAPON = 4 };

/* Whether two items may be held at once. Anything two-handed rules it out
   immediately, and one weapon paired with an off-hand item is decided by the
   weapon's own rule; two weapons have to agree on their wield group. Empty
   hands always agree. */
extern unsigned char CanPairWeaponWithOffHand(int weapon_item_id, int off_hand_item_id);
/* 0x0051C8F0 */

/* The equipment class an item belongs to, or zero when the caller has no
   item - which is indistinguishable from the first real class, so callers
   test the item themselves. */
// FUNCTION: WIZ8 0x0051B8E0
unsigned char GetItemEquipClass(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->item_id].equip_class;
    }
    return 0;
}

/* Which generic name this item wears while unidentified. */
// FUNCTION: WIZ8 0x0051B910
unsigned short GetItemUnidentifiedNameIndex(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->item_id].unidentified_name_index;
    }
    return 0;
}

/* Whether two items are of the same equipment class. */
// FUNCTION: WIZ8 0x0051B940
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second)
{
    if (first != 0 && second != 0) {
        return g_item_records[first->item_id].equip_class ==
               g_item_records[second->item_id].equip_class;
    }
    return false;
}

/* Whether two items look alike while unidentified. */
// FUNCTION: WIZ8 0x0051B990
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second)
{
    if (first != 0 && second != 0) {
        return g_item_records[first->item_id].unidentified_name_index ==
               g_item_records[second->item_id].unidentified_name_index;
    }
    return false;
}

/* Whether an item is bound to whoever is wearing it, which is what stops it
   being taken off or swapped away. */
// FUNCTION: WIZ8 0x0051D180
bool IsItemBoundToWearer(const W8ItemInstance* item)
{
    if (item->item_id != -1 && g_item_records[item->item_id].binds_on_equip != 0 &&
        item->bound != 0) {
        return true;
    }
    return false;
}

/* Every slot this item could be placed in, as a bit per slot.

   Weapons are the interesting case. A two-handed weapon needs the hand
   opposite it free, so each of the two weapon sets contributes its main hand
   only when its off hand is empty. A one-handed weapon can always take either
   main hand, and additionally takes an off hand when the item allows it and
   the main hand of that set is not already holding something two-handed.
   Everything else has exactly one home. */
// FUNCTION: WIZ8 0x0051CF80
unsigned short GetItemEquipSlotMask(
    int item_id,
    char primary_off_hand_free,
    char alternate_off_hand_free,
    char primary_main_hand_free,
    char alternate_main_hand_free)
{
    unsigned short slots = 0;

    switch (g_item_records[item_id].equip_class) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (g_item_records[item_id].weapon_skill == W8_WEAPON_SKILL_NONE) {
            FormatDebugMessage(
                0,
                "ERROR - Item %ls is a weapon without a skill specified -> Charles",
                &g_item_records[item_id]);
            return 0;
        }
        if ((g_item_records[item_id].flags_041 & W8_ITEM_FLAG_TWO_HANDED) != 0) {
            if (primary_off_hand_free) {
                slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_RIGHT);
            }
            if (alternate_off_hand_free) {
                return slots | W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_RIGHT);
            }
            return slots;
        }
        slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_RIGHT) |
                W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_RIGHT);
        if ((g_item_records[item_id].flags_041 & W8_ITEM_FLAG_OFF_HAND_ALLOWED) == 0) {
            return slots;
        }
        if (primary_main_hand_free) {
            slots |= W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_LEFT);
        }
        break;
    case 4:
    case 5:
        if (primary_main_hand_free) {
            slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_LEFT);
        }
        break;
    case 6:
        return W8_EQUIP_SLOT_BIT(4);
    case 7:
        return W8_EQUIP_SLOT_BIT(10);
    case 8:
        return W8_EQUIP_SLOT_BIT(0);
    case 9:
        return W8_EQUIP_SLOT_BIT(5);
    case 10:
        return W8_EQUIP_SLOT_BIT(11);
    case 11:
        return W8_EQUIP_SLOT_BIT(1) | W8_EQUIP_SLOT_BIT(2);
    case 12:
        return W8_EQUIP_SLOT_BIT(3);
    default:
        return 0;
    }

    if (alternate_main_hand_free) {
        return slots | W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_LEFT);
    }
    return slots;
}

/* Whether one character could put this item in one particular slot. The four
   hand slots are read first: a hand counts as available when it is empty, and
   a main hand also counts when whatever it holds is not two-handed. Asking to
   ignore what is worn answers for an empty character instead. */
// FUNCTION: WIZ8 0x0051CEA0
bool CanEquipItemInSlot(
    W8Character* character,
    int item_id,
    unsigned char equip_slot,
    char ignore_worn_items)
{
    char primary_off_hand_free;
    char alternate_off_hand_free;
    char primary_main_hand_free;
    char alternate_main_hand_free;

    primary_off_hand_free =
        character->equipment[W8_EQUIP_SLOT_PRIMARY_LEFT].item_id == -1 || ignore_worn_items;
    alternate_off_hand_free =
        character->equipment[W8_EQUIP_SLOT_ALTERNATE_LEFT].item_id == -1 || ignore_worn_items;
    primary_main_hand_free =
        character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT].item_id == -1 ||
        (g_item_records[character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT].item_id].flags_041 &
         W8_ITEM_FLAG_TWO_HANDED) == 0 ||
        ignore_worn_items;
    alternate_main_hand_free =
        character->equipment[W8_EQUIP_SLOT_ALTERNATE_RIGHT].item_id == -1 ||
        (g_item_records[character->equipment[W8_EQUIP_SLOT_ALTERNATE_RIGHT].item_id].flags_041 &
         W8_ITEM_FLAG_TWO_HANDED) == 0 ||
        ignore_worn_items;

    return (W8_EQUIP_SLOT_BIT(equip_slot) &
            GetItemEquipSlotMask(item_id, primary_off_hand_free, alternate_off_hand_free,
                                 primary_main_hand_free, alternate_main_hand_free)) != 0;
}

/* Whether two items may be held at the same time. Nothing pairs with a
   two-handed item. A weapon beside an off-hand item is decided by the weapon
   rule, which takes them in weapon-first order whichever way round they were
   passed. Two things that are not both weapons always agree, and two weapons
   have to belong to the same wield group. */
// FUNCTION: WIZ8 0x0051CC40
bool CanHoldItemsTogether(int first_item_id, int second_item_id)
{
    if (first_item_id == -1 || second_item_id == -1) {
        return true;
    }
    if ((g_item_records[first_item_id].flags_041 & W8_ITEM_FLAG_TWO_HANDED) != 0 ||
        (g_item_records[second_item_id].flags_041 & W8_ITEM_FLAG_TWO_HANDED) != 0) {
        return false;
    }
    if (g_item_records[first_item_id].equip_class == 3 ||
        g_item_records[second_item_id].equip_class == 4) {
        return CanPairWeaponWithOffHand(first_item_id, second_item_id) != 0;
    }
    if (g_item_records[second_item_id].equip_class == 3 ||
        g_item_records[first_item_id].equip_class == 4) {
        return CanPairWeaponWithOffHand(second_item_id, first_item_id) != 0;
    }
    if (g_item_records[first_item_id].equip_class >= W8_EQUIP_CLASS_FIRST_NON_WEAPON) {
        return true;
    }
    if (g_item_records[second_item_id].equip_class < W8_EQUIP_CLASS_FIRST_NON_WEAPON) {
        return g_item_records[first_item_id].wield_group ==
               g_item_records[second_item_id].wield_group;
    }
    return true;
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
