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

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* 0x0068C108: one lazily built generic name per unidentified-name index, and
   0x0061E810: the notice each index formats from. The table's extent is the
   pointer bound the release walk stops at. */
enum { W8_GENERIC_ITEM_NAME_COUNT = 147 };
extern W8WideChar* g_generic_item_names[W8_GENERIC_ITEM_NAME_COUNT];
extern const unsigned short g_generic_item_name_notice[];
extern unsigned char g_item_taken_from_pool_006874ca;
extern int g_held_item_source_006840c0;
extern unsigned char g_held_item_origin_006840c4;
extern unsigned short g_held_item_slot_006840c5;

extern void ReleaseItemNameFormatter(void);                  /* 0x0055CE40 */
extern void DropHeldItem(int arg_1);                         /* 0x004F7610 */
extern void ShowNotice(void* notice, int a, int b, int c);   /* 0x0055F260 */
extern void ClearHeldItemDisplay(void);                      /* 0x0055F1E0 */
extern void ReplaceOrCreateItem(
    W8ItemInstance* item, int item_id, int count, unsigned char quality, int arg_5);
extern void MoveItem(W8ItemInstance* to, W8ItemInstance* from, int arg_3, int arg_4);
/* 0x0051FE30 */
extern unsigned char CanCharacterActivateItem(
    W8Character* character, const W8ItemInstance* item);     /* 0x0051D800 */
extern void AddPartyGoldNotice(int channel, const wchar_t* notice, ...);
extern int Function40A910(const char* path);
extern void PlaySound(const char* path, int flags);
extern void WriteGameLog(int channel, const wchar_t* format, ...);
extern wchar_t* FormatWideString(const wchar_t* format, ...);
extern void Function58AC00(int channel, void* message);
/* 0x00686A70 and 0x00686B7D: the running tallies gold is credited into,
   one 0x21-byte row each. */
extern int g_gold_tally_index_00686a70;
extern int g_gold_tally_rows_00686b7d[];

/* Whether a weapon and an off-hand item go together, named by its own error
   text at 0x0051C8F0. */
extern unsigned char CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id);

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
        return CompatiblePartnerItems(first_item_id, second_item_id) != 0;
    }
    if (g_item_records[second_item_id].equip_class == 3 ||
        g_item_records[first_item_id].equip_class == 4) {
        return CompatiblePartnerItems(second_item_id, first_item_id) != 0;
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

/* Item categories the usability rules distinguish. Three is the spell source
   the magic code already names; six and eight both cast the record's spell but
   read a different profession level to decide whether the caster is strong
   enough for it. */
enum {
    W8_ITEM_CATEGORY_SPELL_SOURCE = 3,
    W8_ITEM_CATEGORY_CASTER_ITEM_6 = 6,
    W8_ITEM_CATEGORY_CASTER_ITEM_8 = 8
};

/* The faction mask value that admits every faction rather than the two its
   bits would otherwise name. */
enum { W8_ITEM_FACTION_MASK_ANY = 3 };

/* An unused requirement slot. */
enum { W8_ITEM_REQUIREMENT_NONE = 0xff };

/* The two profession levels the casting categories read, by index into
   W8Character::profession_levels. */
enum {
    W8_CASTER_PROFESSION_CATEGORY_8 = 8,
    W8_CASTER_PROFESSION_CATEGORY_6 = 9
};

#define PC_ITEM_CPP "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp"

extern unsigned int GetMinimumCasterLevelForSpell(unsigned int spell_id);
extern bool AddItemToCharacter(
    int character_index,
    W8ItemInstance* item,
    int arg_3,
    int arg_4,
    int arg_5);                       /* 0x0051C300 */
extern bool AddItemToParty(W8ItemInstance* item, int arg_2, int arg_3);
/* 0x00521EF0 */

/* Whether one character is allowed to use one item at all: profession, race
   and faction have to admit them, every attribute and skill floor has to be
   met, and a spell-bearing item additionally has to be one they have not
   already learned or are strong enough to trigger. */
// FUNCTION: WIZ8 0x0051D610
bool CanCharacterUseItem(const W8Character* character, int item_id)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item_id];
    unsigned int index;
    unsigned int spell_id;
    unsigned int minimum_caster_level;

    if ((record->profession_mask & (1 << character->current_profession)) == 0) {
        return false;
    }
    if ((record->race_mask & (1 << character->race)) == 0) {
        return false;
    }
    if (record->faction_mask != W8_ITEM_FACTION_MASK_ANY &&
        (record->faction_mask & (1 << character->faction)) == 0) {
        return false;
    }

    for (index = 0; index < 2; ++index) {
        if (record->attribute_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
            character->attributes[(signed char)record->attribute_requirements[index].stat_id]
                    .effective < record->attribute_requirements[index].minimum) {
            return false;
        }
    }
    for (index = 0; index < 2; ++index) {
        if (record->skill_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
            character->skills[(signed char)record->skill_requirements[index].stat_id].level <
                record->skill_requirements[index].minimum) {
            return false;
        }
    }

    if (record->category == W8_ITEM_CATEGORY_SPELL_SOURCE) {
        spell_id = record->spell_id;
        if (spell_id == 0) {
            srAssertFail("uiSpell != SPELL_NONE", PC_ITEM_CPP, 1832, 0);
        }
        if (character->spell_learned[spell_id] == 1) {
            return false;
        }
    }
    else if (record->category == W8_ITEM_CATEGORY_CASTER_ITEM_6 ||
             record->category == W8_ITEM_CATEGORY_CASTER_ITEM_8) {
        spell_id = record->spell_id;
        if (spell_id == 0) {
            srAssertFail("uiSpell != SPELL_NONE", PC_ITEM_CPP, 1844, 0);
        }
        if (record->category == W8_ITEM_CATEGORY_CASTER_ITEM_6) {
            minimum_caster_level = GetMinimumCasterLevelForSpell(spell_id);
            if ((unsigned int)character->profession_levels[W8_CASTER_PROFESSION_CATEGORY_6] <
                minimum_caster_level) {
                return false;
            }
        }
        else {
            minimum_caster_level = GetMinimumCasterLevelForSpell(spell_id);
            if ((unsigned int)character->profession_levels[W8_CASTER_PROFESSION_CATEGORY_8] <
                minimum_caster_level) {
                return false;
            }
        }
    }
    return true;
}

/* Whether anybody in the party could use this item. Only occupied slots with a
   character who is conscious enough to act are asked. */
// FUNCTION: WIZ8 0x0051D7A0
bool AnyPartyMemberCanUseItem(int item_id)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (g_party_slot_rows[slot].flag_00 != 0 &&
            g_party_characters[slot].hp_current != 0 &&
            g_party_characters[slot].unknown_0b01 < 0x12) {
            if (CanCharacterUseItem(&g_party_characters[slot], item_id)) {
                return true;
            }
        }
    }
    return false;
}

/* Whether both weapon sets are entirely empty. */
// FUNCTION: WIZ8 0x0051F8D0
bool AreAllHandSlotsEmpty(const W8Character* character)
{
    return character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT].item_id == -1 &&
           character->equipment[W8_EQUIP_SLOT_PRIMARY_LEFT].item_id == -1 &&
           character->equipment[W8_EQUIP_SLOT_ALTERNATE_RIGHT].item_id == -1 &&
           character->equipment[W8_EQUIP_SLOT_ALTERNATE_LEFT].item_id == -1;
}

/* Which of four groups an item's home slot belongs to. The four hand slots
   share one group and the rest split two ways; what the groups are used for is
   not established here, only which slots fall together. */
// FUNCTION: WIZ8 0x0051C850
int GetItemEquipSlotGroup(int item_id)
{
    switch (GetItemDefaultEquipSlot(item_id)) {
    case 0:
    case 4:
    case 5:
    case 10:
    case 11:
        return 3;
    case 1:
    case 2:
    case 3:
        return 4;
    case W8_EQUIP_SLOT_PRIMARY_RIGHT:
    case W8_EQUIP_SLOT_PRIMARY_LEFT:
    case W8_EQUIP_SLOT_ALTERNATE_RIGHT:
    case W8_EQUIP_SLOT_ALTERNATE_LEFT:
        return 2;
    default:
        return 5;
    }
}

/* Whether an item's generic name is one of five the callers single out. The
   set is a jump table based at eleven, so it is a property of the shared
   unidentified name rather than of the item itself. */
// FUNCTION: WIZ8 0x0051CCE0
bool ItemHasSingledOutGenericName(int item_id)
{
    if (item_id != -1) {
        switch (g_item_records[item_id].unidentified_name_index) {
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x25:
        case 0x83:
            return true;
        }
    }
    return false;
}

/* Whether the item counts its quantity the fourth way. Which of the three
   uses-or-charges kinds that is has not been established, so the predicate is
   named for the value it tests. */
// FUNCTION: WIZ8 0x0051CDB0
bool ItemHasQuantityKindFour(int item_id)
{
    if (item_id == -1) {
        return false;
    }
    return g_item_records[item_id].quantity_kind == 4;
}

/* How the interface presents the spell an item carries, drawn from a per
   category table. Two particular spells are excluded and answer with nothing
   at all. */
// FUNCTION: WIZ8 0x0051DCB0
int GetItemSpellPresentation(const W8ItemDatabaseRecord* record)
{
    if (record->spell_id != 'X' && record->spell_id != 't') {
        return g_item_spell_presentation[record->category];
    }
    return -1;
}

/* Whether the item worn in one slot may be taken off. A binding that has not
   yet been announced holds it in place, unless the slot is not a real
   equipment slot or the character is under the influence that overrides it. */
// FUNCTION: WIZ8 0x0051D1C0
bool CanUnequipSlotItem(const W8Character* character, int equip_slot)
{
    const W8ItemInstance* item = &character->equipment[equip_slot];

    if (item->item_id != -1 && g_item_records[item->item_id].binds_on_equip != 0 &&
        item->bind_announced == 0 && g_equip_slot_icons[equip_slot] != -1 &&
        character->condition_turns[W8_CONDITION_EQUIPMENT_UNLOCKED] == 0) {
        return false;
    }
    return true;
}

/* Whether an item may be picked up out of wherever it is sitting. An
   unidentified item always may; an identified one only when it belongs in an
   equipment slot at all and has not bound itself to its wearer. */
// FUNCTION: WIZ8 0x0051F2B0
bool CanItemLeaveItsSlot(const W8ItemInstance* item)
{
    if (item->item_id != -1) {
        if (item->identified == 0) {
            return true;
        }
        if (GetItemDefaultEquipSlot(item->item_id) != W8_EQUIP_SLOT_NONE) {
            return item->bound == 0;
        }
    }
    return false;
}

/* Put an item somewhere it will fit. The flag decides which of the character
   and the party pool is tried first; the other is tried after, and then the
   first again, so a full destination never loses the item. */
// FUNCTION: WIZ8 0x0051C280
bool StoreItemWithCharacterOrParty(
    int character_index,
    W8ItemInstance* item,
    char party_first,
    int arg_4,
    int arg_5)
{
    if (!party_first) {
        if (AddItemToCharacter(character_index, item, arg_5, arg_4, 0)) {
            return true;
        }
    }
    if (AddItemToParty(item, arg_4, 0)) {
        return true;
    }
    if (party_first) {
        if (AddItemToCharacter(character_index, item, arg_5, arg_4, 0)) {
            return true;
        }
    }
    return false;
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


/* Throw away every lazily built generic name. The walk is bounded by the
   address just past the table rather than by a count. */
// FUNCTION: WIZ8 0x0051B580
void ReleaseGenericItemNames(void)
{
    W8WideChar** name;

    ReleaseItemNameFormatter();
    for (name = g_generic_item_names;
         name < g_generic_item_names + W8_GENERIC_ITEM_NAME_COUNT;
         ++name) {
        if (*name != 0) {
            free(*name);
            *name = 0;
        }
    }
}

/* What to call an item. An identified one is called by its own name, which
   leads its record - so the record address is the name address. An
   unidentified one is called by the generic name its index shares, built once
   on first use and kept. */
// FUNCTION: WIZ8 0x0051B7B0
W8WideChar* GetItemDisplayName(const W8ItemInstance* item)
{
    unsigned int name_index;
    W8WideChar* built;

    if (item->identified != 0) {
        return g_item_records[item->item_id].display_name;
    }

    name_index = g_item_records[item->item_id].unidentified_name_index;
    if (g_generic_item_names[name_index] == 0) {
        built = (W8WideChar*)malloc(0x78);
        g_generic_item_names[name_index] = built;
        swprintf((wchar_t*)built, (const wchar_t*)g_notices[0x79c / 4],
                 g_notices[g_generic_item_name_notice[name_index]]);
    }
    return g_generic_item_names[name_index];
}

/* Drop whatever is in hand, unless it is one of the items that may not be
   discarded - in which case say so instead. */
// FUNCTION: WIZ8 0x0051BE50
bool DropItemInHand(int arg_1)
{
    if ((g_item_records[g_item_in_hand.item_id].flags_041 & W8_ITEM_FLAG_NO_DISCARD) != 0) {
        ShowNotice(g_notices[0x13bc / 4], 0, 1, 0);
        return false;
    }
    DropHeldItem(arg_1);
    return true;
}

/* Conjure one item and put it either straight into the party pool or into the
   hand, depending on where the last one was taken from. */
// FUNCTION: WIZ8 0x0051BF60
void CreateItemIntoHandOrPool(int item_id, unsigned char quality)
{
    W8ItemInstance created;

    g_held_item_source_006840c0 = -1;
    g_held_item_origin_006840c4 = 0xff;
    g_held_item_slot_006840c5 = 0xffff;
    ClearHeldItemDisplay();
    ReplaceOrCreateItem(&created, item_id, 1, quality, 0);
    if (g_item_taken_from_pool_006874ca != 0) {
        AddItemToParty(&created, 0, 0);
        return;
    }
    MoveItem(&g_item_in_hand, &created, 0, 1);
}

/* How many of a character's twenty item slots hold something they could use
   right now - the twelve worn and the eight carried, walked as two runs
   rather than one. */
// FUNCTION: WIZ8 0x0051F870
int CountUsableCharacterItems(W8Character* character)
{
    int count = 0;
    int index;

    for (index = 0; index < 12; ++index) {
        if (CanCharacterActivateItem(character, &character->equipment[index])) {
            ++count;
        }
    }
    for (index = 0; index < 8; ++index) {
        if (CanCharacterActivateItem(character, &character->backpack[index])) {
            ++count;
        }
    }
    return count;
}

/* Whether an item's class is one of the three the target rules normalize for.
   The classes here are above the thirteen the equip-slot switch enumerates,
   so the class domain is wider than that switch covers. */
// FUNCTION: WIZ8 0x005207C0
bool ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record)
{
    unsigned char equip_class = record->equip_class;

    if (equip_class != 0x10 && (equip_class < 0x15 || equip_class > 0x16)) {
        return false;
    }
    return true;
}

/* Whether one item sits in a character's equipment, excluding the two
   alternate-set hand slots - a slot in that set answers no even though the
   item is found there. */
// FUNCTION: WIZ8 0x00520F20
bool IsItemWornByCharacter(W8Character* character, const W8ItemInstance* item)
{
    unsigned int slot;

    if (character == 0) {
        return false;
    }
    for (slot = 0; slot < 12; ++slot) {
        if (&character->equipment[slot] == item) {
            return slot != W8_EQUIP_SLOT_ALTERNATE_RIGHT &&
                   slot != W8_EQUIP_SLOT_ALTERNATE_LEFT;
        }
    }
    return false;
}

/* Whether one item sits in a character's own carried slots. */
// FUNCTION: WIZ8 0x00520F60
bool IsItemCarriedByCharacter(W8Character* character, const W8ItemInstance* item)
{
    unsigned int slot;

    if (character == 0) {
        return false;
    }
    for (slot = 0; slot < 8; ++slot) {
        if (&character->backpack[slot] == item) {
            return true;
        }
    }
    return false;
}

/* Give gold to the party. It goes into the purse and into whichever running
   tally is open, and announcing it plays the coin sound - copied to the stack
   first because the sound call takes a writable path. */
// FUNCTION: WIZ8 0x0051BEA0
void AddPartyGold(int amount, char announce)
{
    char sound_path[32];
    wchar_t* line;

    strcpy(sound_path, "Data\Sound\Misc\ChaChing.wav");

    g_party_gold += amount;
    if (g_gold_tally_index_00686a70 < 0x2f) {
        g_gold_tally_rows_00686b7d[g_gold_tally_index_00686a70 * 0x21 / 4] += amount;
    }

    if (announce) {
        line = FormatWideString((const wchar_t*)g_notices[0x788 / 4],
                                g_notices[0x57c / 4], amount, g_notices[0x580 / 4],
                                -1, -1, 0);
        Function58AC00(8, line);
        if (!Function40A910(sound_path)) {
            PlaySound(sound_path, 0);
        }
    }
}

extern void AdjustIntegerByPercent(unsigned int* value, unsigned int percent);
extern W8WideChar* FormatItemDisplayName(const W8ItemInstance* item, int form);
extern unsigned char TryIdentifyItemFor(W8Character* character, W8ItemInstance* item);
/* 0x005208F0 */

/* Bind one worn item to its wearer. A binding that has not been announced yet
   is announced as it takes hold; one already announced just takes hold. A slot
   with no interface position binds nothing. */
// FUNCTION: WIZ8 0x0051D0D0
void BindEquippedItem(W8Character* character, int equip_slot)
{
    W8ItemInstance* item = &character->equipment[equip_slot];

    if (item->item_id == -1) {
        return;
    }
    if (g_item_records[item->item_id].binds_on_equip == 0 || item->bind_announced != 0) {
        if (g_equip_slot_icons[equip_slot] != -1 && item->bound == 0) {
            item->bound = 1;
        }
        return;
    }
    if (g_equip_slot_icons[equip_slot] != -1 && item->bound == 0) {
        item->bound = 1;
        WriteGameLog(8, (const wchar_t*)g_notices[0x7a8 / 4],
                     FormatItemDisplayName(item, 1));
    }
}

/* The spell an item carries, with both bounds on the item id asserted - the
   second names the database count as gXStatus.uiItemsInDatabase. */
// FUNCTION: WIZ8 0x00520880
unsigned char GetItemSpell(const W8ItemInstance* item)
{
    if (item == 0) {
        return 0;
    }
    if (item->item_id == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 4003, 0);
    }
    if (item->item_id >= g_item_record_count) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase",
                     PC_ITEM_CPP, 4004, 0);
    }
    return g_item_records[item->item_id].spell_id;
}

/* Let the whole party have a go at identifying one item. Everybody able to
   tries, but only the first attempt's answer is reported - the rest still
   happen for whatever they do to the item. */
// FUNCTION: WIZ8 0x005209F0
char PartyAttemptsToIdentifyItem(W8ItemInstance* item)
{
    char result = 0;
    int party_slot;

    if (item->identified != 0) {
        return 0;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].hp_current != 0 &&
            g_party_characters[party_slot].unknown_0b01 < 0xb) {
            if (result == 0) {
                result = TryIdentifyItemFor(&g_party_characters[party_slot], item);
            }
            else {
                TryIdentifyItemFor(&g_party_characters[party_slot], item);
            }
        }
    }
    return result;
}

/* Score one identify attempt. Three times the attempt's strength, scaled by
   the attempter's percentage, has to reach the item's difficulty; clearing it
   reveals everything about the item at once. */
// FUNCTION: WIZ8 0x00520B40
void ApplyIdentifyAttempt(W8ItemInstance* item, unsigned int strength, unsigned int percent)
{
    unsigned int score = strength * 3;

    AdjustIntegerByPercent(&score, percent);
    if (g_item_records[item->item_id].identify_difficulty <= score) {
        if (item == 0) {
            srAssertFail("pPCItem", PC_ITEM_CPP, 2875, 0);
        }
        item->identified = 1;
        item->unknown_07[0] = 1;
        item->unknown_07[1] = 1;
        item->bound = 1;
    }
}

/* How many attempts of a given strength it takes to clear an item's
   difficulty. Counted downwards from the ceiling division, so the answer is
   the smallest count that still clears it. */
// FUNCTION: WIZ8 0x00520A70
unsigned int CountIdentifyAttemptsNeeded(
    W8ItemInstance* item, unsigned int strength, unsigned int percent)
{
    unsigned int attempts;
    unsigned int candidate;
    unsigned int score;

    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 4113, 0);
    }
    if (item->item_id == -1) {
        return 1;
    }

    attempts = g_item_records[item->item_id].identify_difficulty / 3;
    if (g_item_records[item->item_id].identify_difficulty % 3 != 0) {
        ++attempts;
    }
    candidate = attempts * 3;
    while (attempts > 1) {
        candidate -= 3;
        score = candidate;
        AdjustIntegerByPercent(&score, percent);
        if (score < g_item_records[item->item_id].identify_difficulty) {
            return attempts;
        }
        --attempts;
    }
    return 1;
}

/* Reveal what one character's worn bindings are, as far as the attempt
   reaches. Nothing bound at all answers zero; some still hidden answers one
   and all revealed answers two. */
// FUNCTION: WIZ8 0x00520BC0
char RevealCharacterItemBindings(
    unsigned int party_slot, int strength, unsigned int percent)
{
    W8Character* character = &g_party_characters[party_slot];
    unsigned int score = strength * 3;
    int revealed = 0;
    int still_hidden = 0;
    int slot;

    AdjustIntegerByPercent(&score, percent);
    for (slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->equipment[slot];

        if (item->item_id != -1 && g_item_records[item->item_id].binds_on_equip != 0) {
            if (score < g_item_records[item->item_id].identify_difficulty) {
                ++still_hidden;
            }
            else {
                ++revealed;
                item->bind_announced = 1;
            }
        }
    }
    if (revealed != 0) {
        return (still_hidden == 0) + 1;
    }
    return 0;
}

/* Whether an item still has anything worth identifying: any of the nine bits
   of the mask at 0x04e, or - with the flag at 0x062 clear - any of the six
   bytes at 0x06f, the spell-bearing byte at 0x08c or the three at 0x06c. */
// FUNCTION: WIZ8 0x00520750
bool ItemHasHiddenProperties(int item_id)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item_id];
    unsigned int index;

    for (index = 0; index < 9; ++index) {
        if ((*(const unsigned short*)&record->unknown_048[6] & (1 << index)) != 0) {
            return true;
        }
    }
    if (record->unknown_048[0x1a] != 0) {
        return true;
    }
    for (index = 0; index < 6; ++index) {
        if (record->unknown_06b[4 + index] != 0) {
            return true;
        }
    }
    if (record->binds_on_equip != 0 || record->unknown_06b[1] != 0 ||
        record->unknown_06b[2] != 0 || record->unknown_06b[3] != 0) {
        return true;
    }
    return false;
}

/* The skill an identify attempt practises, and the one whose level supplies
   its strength - a sixth of it. */
enum { W8_SKILL_IDENTIFY = 0x14 };

extern void PracticeCharacterSkill(W8Character* character, int skill, int amount, int arg_4);
extern void RemoveCharacterItem(int party_slot, W8ItemInstance* item, int arg_3);
extern void BindCharacterItems(int party_slot, int arg_2);              /* 0x0051D2C0 */
extern void ShowNotice(int channel, void* notice, int a, int b, int c);
extern unsigned char g_in_combat_00683f94;
extern unsigned char g_flag_00683fce;
extern W8CombatState* g_combat_state;

/* The most of one item a character can hold at once: the record's own quantity
   dice taken at their maximum. */
static int MaximumQuantity(int item_id)
{
    return g_item_records[item_id].initial_quantity.sides *
               g_item_records[item_id].initial_quantity.count +
           g_item_records[item_id].initial_quantity.base;
}

/* Add uses to one item, never past what it can hold. */
// FUNCTION: WIZ8 0x0051E920
void AddItemUses(W8ItemInstance* item, char uses)
{
    unsigned char total = item->uses_or_charges + uses;
    int maximum = MaximumQuantity(item->item_id);

    item->uses_or_charges = total;
    if ((int)(unsigned int)total < maximum) {
        item->uses_or_charges = total;
        return;
    }
    item->uses_or_charges = (unsigned char)maximum;
}

/* Pour one item's uses into another and take that many off the source, one at
   a time - which is what makes the source disappear when it is emptied. Each
   side counts its quantity the way its own record says to. */
// FUNCTION: WIZ8 0x0051E9F0
void MergeItemUses(int party_slot, W8ItemInstance* into, W8ItemInstance* from)
{
    unsigned char available;
    unsigned char held;
    unsigned int moved;

    available = g_item_records[from->item_id].quantity_kind == 1 ? from->stack_count
                                                                : from->uses_or_charges;
    held = g_item_records[into->item_id].quantity_kind == 1 ? into->stack_count
                                                            : into->uses_or_charges;

    moved = MaximumQuantity(into->item_id) - held;
    if (available <= moved) {
        moved = available;
    }
    into->uses_or_charges += (char)moved;
    for (; moved != 0; --moved) {
        RemoveCharacterItem(party_slot, from, 0);
    }
}

/* Where one item id sits on a character. The worn slots are searched first and
   the carried ones only when asked for; a starting slot makes the search
   resume after it rather than from the front. */
// FUNCTION: WIZ8 0x00520F90
bool FindItemOnCharacter(
    W8Character* character,
    int item_id,
    W8ItemInstance** found,
    int include_backpack,
    const W8ItemInstance* resume_after)
{
    unsigned int slot = 0;

    if (resume_after != 0) {
        for (slot = 0; slot < 12; ++slot) {
            if (&character->equipment[slot] == resume_after) {
                ++slot;
                break;
            }
        }
    }
    for (; slot < 12; ++slot) {
        if (character->equipment[slot].item_id == item_id) {
            if (found != 0) {
                *found = &character->equipment[slot];
            }
            return true;
        }
    }

    if (include_backpack == 0) {
        return false;
    }

    slot = 0;
    if (resume_after != 0) {
        for (slot = 0; slot < 8; ++slot) {
            if (&character->backpack[slot] == resume_after) {
                ++slot;
                break;
            }
        }
    }
    for (; slot < 8; ++slot) {
        if (character->backpack[slot].item_id == item_id) {
            if (found != 0) {
                *found = &character->backpack[slot];
            }
            return true;
        }
    }
    if (found != 0) {
        *found = 0;
    }
    return false;
}

/* How many of one item a character holds, counting a stack as its count and
   anything else as one, and optionally reporting the first slot it is in. */
// FUNCTION: WIZ8 0x005211A0
int CountItemOnCharacter(
    W8Character* character, int item_id, W8ItemInstance** first, int include_backpack)
{
    int total = 0;
    int slot;

    for (slot = 0; slot < 12; ++slot) {
        if (character->equipment[slot].item_id == item_id) {
            total += character->equipment[slot].stack_count == 0
                         ? 1
                         : character->equipment[slot].stack_count;
            if (first != 0 && *first == 0) {
                *first = &character->equipment[slot];
            }
        }
    }
    if (include_backpack != 0) {
        for (slot = 0; slot < 8; ++slot) {
            if (character->backpack[slot].item_id == item_id) {
                total += character->backpack[slot].stack_count == 0
                             ? 1
                             : character->backpack[slot].stack_count;
                if (first != 0 && *first == 0) {
                    *first = &character->backpack[slot];
                }
            }
        }
    }
    return total;
}

/* Whether every occupied party slot holds one item. The first character who
   does not settles it. */
// FUNCTION: WIZ8 0x00521360
bool EveryCharacterHasItem(int item_id, int include_backpack)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            if (!FindItemOnCharacter(&g_party_characters[party_slot], item_id, 0,
                                     include_backpack, 0)) {
                return false;
            }
        }
    }
    return true;
}

/* At what range an item's spell works. An item with no spell has no range at
   all, which is a different answer from touch. */
// FUNCTION: WIZ8 0x005207E0
int GetItemSpellRange(const W8ItemInstance* item)
{
    if (item == 0) {
        return -1;
    }
    if (item->item_id == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 4003, 0);
    }
    if (item->item_id >= g_item_record_count) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase",
                     PC_ITEM_CPP, 4004, 0);
    }
    if (g_item_records[item->item_id].spell_id != 0) {
        return g_spell_records[g_item_records[item->item_id].spell_id].range_category;
    }
    return -1;
}

/* One character's attempt at identifying an item. Their strength is a sixth of
   the identify skill's level; clearing the difficulty reveals the item, three
   further points also reveal its binding, and an attempt that only just came
   off practises the skill. */
// FUNCTION: WIZ8 0x005208F0
unsigned char TryIdentifyItemFor(W8Character* character, W8ItemInstance* item)
{
    char strength;
    int margin;

    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 4014, 0);
    }
    if (character->hp_current == 0 || character->unknown_0b01 >= 0xb) {
        return 0;
    }

    strength = (char)(character->skills[W8_SKILL_IDENTIFY].level / 6);
    if ((char)g_item_records[item->item_id].identify_difficulty > strength) {
        return 0;
    }

    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 2875, 0);
    }
    item->identified = 1;
    item->unknown_07[0] = 1;
    item->unknown_07[1] = 1;
    if ((char)(g_item_records[item->item_id].identify_difficulty + 3) <= strength) {
        item->bound = 1;
    }

    margin = strength - g_item_records[item->item_id].identify_difficulty;
    if (margin >= 0 && margin < 3) {
        PracticeCharacterSkill(character, W8_SKILL_IDENTIFY, 2, 1);
    }
    return 1;
}

/* Bind everything the party is wearing, one character at a time - but not
   during a fight the party has not yet been let out of, which says so
   instead. */
// FUNCTION: WIZ8 0x0051D230
void BindEveryPartyItem(void)
{
    int party_slot;

    if (g_in_combat_00683f94 != 0 && g_combat_state->flag_001 == 0 &&
        g_flag_00683fce == 0) {
        ShowNotice(0xc, g_notices[0x7d8 / 4], -1, -1, 0);
        return;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_0f5 == 0) {
            BindCharacterItems(party_slot, 0);
        }
    }
    ShowNotice(8, g_notices[0x7b4 / 4], -1, -1, 0);
}

#include <stdlib.h>

/* Order two pool entries. Both have to hold something - the two assertions say
   so by name - and they are compared by equipment class, then by generic name,
   then unidentified before identified, then by value, then by stack count and
   finally by uses. Every comparison is the reverse of the usual sense, so the
   pool ends up in descending order. */
// FUNCTION: WIZ8 0x00520600
int __cdecl CompareItemsForPool(const void* first, const void* second)
{
    const W8ItemInstance* a = (const W8ItemInstance*)first;
    const W8ItemInstance* b = (const W8ItemInstance*)second;
    const W8ItemDatabaseRecord* ra;
    const W8ItemDatabaseRecord* rb;

    if (a->item_id == -1) {
        srAssertFail("pPCItem1->iItemNo != BAD_INDEX", PC_ITEM_CPP, 3812, 0);
    }
    if (b->item_id == -1) {
        srAssertFail("pPCItem2->iItemNo != BAD_INDEX", PC_ITEM_CPP, 3813, 0);
    }
    ra = &g_item_records[a->item_id];
    rb = &g_item_records[b->item_id];

    if (rb->equip_class < ra->equip_class) {
        return 1;
    }
    if (ra->equip_class < rb->equip_class) {
        return -1;
    }
    if (rb->unidentified_name_index < ra->unidentified_name_index) {
        return 1;
    }
    if (ra->unidentified_name_index < rb->unidentified_name_index) {
        return -1;
    }
    if (a->identified == 0) {
        if (b->identified == 0) {
            return 0;
        }
        return 1;
    }
    if (b->identified == 0) {
        return -1;
    }
    if (ra->value < rb->value) {
        return 1;
    }
    if (rb->value < ra->value) {
        return -1;
    }
    if (a->stack_count < b->stack_count) {
        return 1;
    }
    if (b->stack_count < a->stack_count) {
        return -1;
    }
    if (a->uses_or_charges < b->uses_or_charges) {
        return 1;
    }
    return -(b->uses_or_charges < a->uses_or_charges);
}

/* Put the party pool back in order. Its assertion names gStatus.fGameStarted,
   which is what identified that global in the first place, and a pool of one
   is left alone rather than sorted. */
// FUNCTION: WIZ8 0x005205B0
void SortPartyItemPool(void)
{
    if (g_game_started == 0) {
        srAssertFail("gStatus.fGameStarted", PC_ITEM_CPP, 3795, 0);
    }
    if (g_party_item_count > 1) {
        qsort(g_party_item_pool, g_party_item_count, sizeof(W8ItemInstance),
              CompareItemsForPool);
    }
}
