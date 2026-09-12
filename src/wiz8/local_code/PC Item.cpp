#include "soundman.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/notices.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"
#include "wiz8/fact_state.h"
#include "wiz8/factions.h"
#include "wiz8/npc_state.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/sound_man.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/targeting.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"

#include <stdio.h>
#include "wiz8/character_skills.h"

// GLOBAL: WIZ8 0x00686901
unsigned int g_shared_item_pool_count;
// GLOBAL
unsigned char g_shared_item_pool[500 * 0xc];

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
// FUNCTION: WIZ8 0x0051b8b0
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
// FUNCTION: WIZ8 0x0051bfd0
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
// FUNCTION: WIZ8 0x0051b840
unsigned int GetItemStackValue(const W8ItemInstance* item)
{
    if (g_item_records[item->item_id].equip_class == 4) {
        return (item->stack_count * g_item_records[item->item_id].value +
                (W8_ITEM_BUNDLE_SIZE - 1)) /
               W8_ITEM_BUNDLE_SIZE;
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
// FUNCTION: WIZ8 0x0051c4e0
int GetItemDefaultEquipSlot(int item_id)
{
    switch (g_item_records[item_id].equip_class) {
    case 0:
    case 1:
        return W8_EQUIP_SLOT_PRIMARY_RIGHT;
    case 3:
        return g_game_started ? W8_EQUIP_SLOT_ALTERNATE_RIGHT : W8_EQUIP_SLOT_PRIMARY_RIGHT;
    case 4:
        return g_game_started ? W8_EQUIP_SLOT_ALTERNATE_LEFT : W8_EQUIP_SLOT_PRIMARY_LEFT;
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
// FUNCTION: WIZ8 0x0051c8b0
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
W8WideChar* g_generic_item_names[W8_GENERIC_ITEM_NAME_COUNT];
// GLOBAL: WIZ8 0x00616e84
const int g_item_spell_presentation[11] = {-1, 20, 20, -1, -1, -1, 12, 9, 23, 7, 0};
// GLOBAL: WIZ8 0x00648c5c
const int g_equip_slot_icons[6] = {0, 0, 0, 0, 0, 0};
// GLOBAL: WIZ8 0x0061E810
extern const unsigned short g_generic_item_name_notice[W8_GENERIC_ITEM_NAME_COUNT] = {
    0x45a, 0x45b, 0x45c, 0x45d, 0x45e, 0x45f, 0x460, 0x461, 0x462, 0x463, 0x464, 0x465, 0x466,
    0x467, 0x468, 0x469, 0x46a, 0x46b, 0x46c, 0x46d, 0x46e, 0x46f, 0x470, 0x471, 0x472, 0x473,
    0x474, 0x475, 0x476, 0x477, 0x478, 0x479, 0x47a, 0x47b, 0x47c, 0x47d, 0x47e, 0x47f, 0x480,
    0x481, 0x482, 0x483, 0x484, 0x485, 0x486, 0x487, 0x488, 0x489, 0x48a, 0x48b, 0x48c, 0x48d,
    0x48e, 0x48f, 0x490, 0x491, 0x492, 0x493, 0x494, 0x495, 0x496, 0x497, 0x498, 0x499, 0x49a,
    0x49b, 0x49c, 0x49d, 0x49e, 0x49f, 0x4a0, 0x4a1, 0x4a2, 0x4a3, 0x4a4, 0x4a5, 0x4a6, 0x4a7,
    0x4a8, 0x4a9, 0x4aa, 0x4ab, 0x4ac, 0x4ad, 0x4ae, 0x4af, 0x4b0, 0x4b1, 0x4b2, 0x4b3, 0x4b4,
    0x4b5, 0x4b6, 0x4b7, 0x4b8, 0x4b9, 0x4ba, 0x4bb, 0x4bc, 0x4bd, 0x4be, 0x4bf, 0x4c0, 0x4c1,
    0x4c2, 0x4c3, 0x4c4, 0x4c5, 0x4c6, 0x4c7, 0x4c8, 0x4c9, 0x4ca, 0x4cb, 0x4cc, 0x4cd, 0x4ce,
    0x4cf, 0x4d0, 0x459, 0x4d1, 0x4d2, 0x4d3, 0x4d4, 0x4d5, 0x4ab, 0x46f, 0x459, 0x46c, 0x4c1,
    0x4d6, 0x4d7, 0x4d8, 0x4d9, 0x4da, 0x4db, 0x4dc, 0x4dd, 0x4de, 0x4df, 0x4e0, 0x4e1, 0x4e2,
    0x4e3, 0x4e4, 0x4e5, 0x4e6,
};
/* Shared scratch returned by the item-name formatter. */
// GLOBAL: WIZ8 0x0068C0B4
W8WideChar g_item_display_name_buffer[42];
// GLOBAL: WIZ8 0x006840C0
int g_held_item_source_006840c0;
// GLOBAL: WIZ8 0x006840C4
unsigned char g_held_item_origin_006840c4;
// GLOBAL: WIZ8 0x006840C5
unsigned short g_held_item_slot_006840c5;

static_assert(sizeof(W8ItemVideoObjectEntry) == 8, "W8ItemVideoObjectEntry_must_be_8");
static_assert(sizeof(W8ItemVideoObjectCache) == 0x0c, "W8ItemVideoObjectCache_must_be_0x0c");

W8ItemVideoObjectCache g_item_video_objects_68ec68;

// FUNCTION: WIZ8 0x0055cdb0
W8ItemVideoObjectEntry::W8ItemVideoObjectEntry() : initialized(0), video_object(0) {}

W8ItemVideoObjectEntry::~W8ItemVideoObjectEntry() {}

// FUNCTION: WIZ8 0x0055cdc0
void W8ItemVideoObjectCache::Initialize(int new_capacity)
{
    capacity = new_capacity;
    data = new W8ItemVideoObjectEntry[new_capacity];
    loaded_count = 0;
}

// FUNCTION: WIZ8 0x0055ce40
void W8ItemVideoObjectCache::Clear()
{
    if (data) {
        delete[] data;
        data = 0;
    }
    loaded_count = 0;
}

static const char g_item_video_object_fallback_names[8][0x30] = {
    "Dagger.sti", "LongSword.sti", "Bipennis.sti", "BattleAxe.sti",
    "Flail.sti",  "Mace.sti",      "Hammer.sti",   "ShortStaff.sti"};

// FUNCTION: WIZ8 0x0055ce80
int W8ItemVideoObjectCache::GetOrCreateVideoObject(int item_id)
{
    W8ItemVideoObjectEntry* entry = data + item_id;
    const W8ItemDatabaseRecord* record = &g_item_records[item_id];
    const char* name = record->video_object_name;
    int object;
    int frame;

    if (entry->initialized) {
        return entry->video_object;
    }
    if (loaded_count == capacity || loaded_count == 1000) {
        return 0;
    }
    object = loaded_count + 0x291;
    frame = loaded_count + 0x1ec;
    if (name[0] == '\0') {
        name = g_item_video_object_fallback_names[record->unidentified_name_index];
    }
    sprintf(g_video_frames_62c430[object].path, "Data\\Items\\%s", name);
    if (strstr(g_video_frames_62c430[object].path, ".sti") == 0) {
        strcat(g_video_frames_62c430[object].path, ".sti");
    }
    g_video_frames_62c430[object].mode = 0;
    g_video_frames_62c430[object].loaded = 0;
    g_video_frames_62c430[object].handle = 0;
    g_video_slots_6448c8[frame].first_frame = object;
    g_video_slots_6448c8[frame].y_offset = 0;
    EnsureCatalogFrameLoaded(frame, 0);
    ++loaded_count;
    entry->video_object = frame;
    entry->initialized = 1;
    return frame;
}

/* 0x0051FE30 */
extern void AddPartyGoldNotice(int channel, const wchar_t* notice, ...);
extern int g_item_message_005ee6fc;
extern int g_item_message_005ee640;
extern int g_item_message_005ee644;
extern int g_item_message_005ee648;
extern int g_item_message_005ee64c;
extern int g_item_message_005ee690;
extern int g_item_message_005ee68c;
extern int g_item_message_005ee664;

/* Whether a weapon and an off-hand item go together, named by its own error
   text at 0x0051C8F0. */

// GLOBAL: WIZ8 0x00652da6
unsigned char g_byte_652da6;

/* Build the stable display form used by notices and inventory controls.  The
   generic unidentified names are allocated once, while this returned buffer
   is shared by the quantity and plain-name forms. */
// FUNCTION: WIZ8 0x0051b5c0
W8WideChar* FormatItemDisplayName(const W8ItemInstance* item, unsigned char include_quantity)
{
    W8WideChar* name;
    unsigned int name_index;

    if (item->identified != 0) {
        name = g_item_records[item->item_id].display_name;
    } else {
        name_index = g_item_records[item->item_id].unidentified_name_index;
        if (g_generic_item_names[name_index] == 0) {
            name = (W8WideChar*)malloc(0x78);
            g_generic_item_names[name_index] = name;
            swprintf((wchar_t*)name, (const wchar_t*)gppStringList[0x79c / 4],
                     gppStringList[g_generic_item_name_notice[name_index]]);
        }
        name = g_generic_item_names[name_index];
    }

    if (include_quantity && item->stack_count > 1) {
        swprintf((wchar_t*)g_item_display_name_buffer, L"%s (%d)", name,
                 (unsigned int)item->stack_count);
    } else {
        swprintf((wchar_t*)g_item_display_name_buffer, L"%s", name);
    }
    return g_item_display_name_buffer;
}

/* The equipment class an item belongs to, or zero when the caller has no
   item - which is indistinguishable from the first real class, so callers
   test the item themselves. */
// FUNCTION: WIZ8 0x0051b8e0
unsigned char GetItemEquipClass(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->item_id].equip_class;
    }
    return 0;
}

/* Which generic name this item wears while unidentified. */
// FUNCTION: WIZ8 0x0051b910
unsigned short GetItemUnidentifiedNameIndex(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->item_id].unidentified_name_index;
    }
    return 0;
}

/* Whether two items are of the same equipment class. */
// FUNCTION: WIZ8 0x0051b940
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second)
{
    if (first != 0 && second != 0) {
        return g_item_records[first->item_id].equip_class ==
               g_item_records[second->item_id].equip_class;
    }
    return false;
}

/* Whether two items look alike while unidentified. */
// FUNCTION: WIZ8 0x0051b990
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
// FUNCTION: WIZ8 0x0051d180
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
// FUNCTION: WIZ8 0x0051cf80
unsigned short GetItemEquipSlotMask(int item_id, char primary_off_hand_free,
                                    char alternate_off_hand_free, char primary_main_hand_free,
                                    char alternate_main_hand_free)
{
    unsigned short slots = 0;

    switch (g_item_records[item_id].equip_class) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (g_item_records[item_id].weapon_skill == W8_WEAPON_SKILL_NONE) {
            FormatDebugMessage(0,
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
// FUNCTION: WIZ8 0x0051cea0
bool CanEquipItemInSlot(W8Character* character, int item_id, unsigned char equip_slot,
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

#define PC_ITEM_CPP "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp"

/* The name-index pairs that may be held together; a zero first entry ends
   the run. */
// GLOBAL: WIZ8 0x00616e6c
short g_compatible_partner_pairs_616e6c[6][2] = {
    {114, 113}, {110, 132}, {104, 132}, {144, 143}, {145, 146}, {0, 0},
};

/* Whether an off-hand item may pair with a ranged item when the two are held
   together. A handful of name indices name one partner directly, the
   two-weapon name indices fall back to the pair table, and the specialised
   ranged items carry their own accepted classes. Anything not named is
   refused. */
// FUNCTION: WIZ8 0x0051c8f0
unsigned char CompatiblePartnerItems(int ranged_item_id, int other_item_id)
{
    if ((unsigned int)ranged_item_id >= gXStatus.uiItemsInDatabase) {
        srAssertFail("uiRangedItem < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 0x3fd,
                     FormatString("CompatiblePartnerItems: ERROR - Illegal RANGED item index %d",
                                  ranged_item_id));
    }
    if ((unsigned int)other_item_id >= gXStatus.uiItemsInDatabase) {
        srAssertFail("uiOtherItem < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 0x3fe,
                     FormatString("CompatiblePartnerItems: ERROR - Illegal OTHER item index %d",
                                  other_item_id));
    }

    short ranged_name = (short)g_item_records[ranged_item_id].unidentified_name_index;
    short other_name = (short)g_item_records[other_item_id].unidentified_name_index;

    switch (ranged_name) {
    case 0xb:
    case 0x25:
        if (other_name == 0x16) {
            return 1;
        }
        break;
    case 0xc:
        if (other_name == 0x53) {
            return 1;
        }
        break;
    case 0xd:
        if (other_name == 0x17) {
            return 1;
        }
        break;
    case 0x68:
    case 0x6e:
    case 0x72:
        if (g_item_records[other_item_id].equip_class > 3 ||
            g_item_records[ranged_item_id].wield_group ==
                g_item_records[other_item_id].wield_group) {
            if (ranged_item_id != -1 && g_compatible_partner_pairs_616e6c[0][0] != 0) {
                int index = 0;
                short name = g_compatible_partner_pairs_616e6c[0][0];

                while (name != ranged_name) {
                    ++index;
                    name = g_compatible_partner_pairs_616e6c[index][0];
                    if (name == 0) {
                        return 1;
                    }
                }
                if (other_name != g_compatible_partner_pairs_616e6c[index][1]) {
                    return 0;
                }
            }
            return 1;
        }
        break;
    case 0x83:
        switch (ranged_item_id) {
        case 599:
            if (other_name == 0x17 || other_name == 0x71) {
                return 1;
            }
            break;
        case 600:
        case 0x259:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70) {
                return 1;
            }
            break;
        case 0x25a:
        case 0x25b:
        case 0x25c:
        case 0x25d:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70 ||
                other_name == 0x15 || other_name == 0x7e ||
                (g_item_records[other_item_id].equip_class == 2 && other_name == 0)) {
                return 1;
            }
            break;
        case 0x25e:
        case 0x25f:
        case 0x260:
        case 0x261:
        case 0x262:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70 ||
                other_name == 0x15 || other_name == 0x7e ||
                (g_item_records[other_item_id].equip_class == 2 && other_name == 0) ||
                other_name == 0x16 || other_name == 0x53) {
                return 1;
            }
            return 0;
        }
        break;
    }
    return 0;
}

/* Whether two items may be held at the same time. Nothing pairs with a
   two-handed item. A weapon beside an off-hand item is decided by the weapon
   rule, which takes them in weapon-first order whichever way round they were
   passed. Two things that are not both weapons always agree, and two weapons
   have to belong to the same wield group. */
// FUNCTION: WIZ8 0x0051cc40
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

/* The sex mask value that admits either sex rather than the one bit a
   restrictive record names. */
enum { W8_ITEM_GENDER_MASK_ANY = 3 };

/* An unused requirement slot. */
enum { W8_ITEM_REQUIREMENT_NONE = 0xff };

/* The two profession levels the casting categories read, by index into
   W8Character::profession_levels. */
enum { W8_CASTER_PROFESSION_CATEGORY_8 = 8, W8_CASTER_PROFESSION_CATEGORY_6 = 9 };

/* 0x00521EF0 */

/* Whether one character is allowed to use one item at all: profession, race
   and faction have to admit them, every attribute and skill floor has to be
   met, and a spell-bearing item additionally has to be one they have not
   already learned or are strong enough to trigger. */
// FUNCTION: WIZ8 0x0051d610
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
    if (record->gender_mask != W8_ITEM_GENDER_MASK_ANY &&
        (record->gender_mask & (1 << character->gender)) == 0) {
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
    } else if (record->category == W8_ITEM_CATEGORY_CASTER_ITEM_6 ||
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
        } else {
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
// FUNCTION: WIZ8 0x0051d7a0
bool AnyPartyMemberCanUseItem(int item_id)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (g_party_slot_rows[slot].occupied != 0 && g_party_characters[slot].hp_current != 0 &&
            g_party_characters[slot].unknown_0b01 < 0x12) {
            if (CanCharacterUseItem(&g_party_characters[slot], item_id)) {
                return true;
            }
        }
    }
    return false;
}

/* Whether both weapon sets are entirely empty. */
// FUNCTION: WIZ8 0x0051f8d0
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
// FUNCTION: WIZ8 0x0051c850
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
// FUNCTION: WIZ8 0x0051cce0
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
// FUNCTION: WIZ8 0x0051cdb0
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
// FUNCTION: WIZ8 0x0051dcb0
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
// FUNCTION: WIZ8 0x0051d1c0
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
// FUNCTION: WIZ8 0x0051f2b0
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
// FUNCTION: WIZ8 0x0051c280
bool StoreItemWithCharacterOrParty(W8Character* character, W8ItemInstance* item, char party_first,
                                   int arg_4, int arg_5)
{
    if (!party_first) {
        if (AddItemToCharacter(character, item, arg_5, arg_4, 0)) {
            return true;
        }
    }
    if (AddItemToParty(item, arg_4, 0)) {
        return true;
    }
    if (party_first) {
        if (AddItemToCharacter(character, item, arg_5, arg_4, 0)) {
            return true;
        }
    }
    return false;
}

/* Place an item with one character. Equipment is tried first when requested;
   otherwise compatible stacks are coalesced before the first empty backpack
   slot is used. The source item is consumed only after a destination accepts
   it, so the caller can still fall back to the party pool on failure. */
// FUNCTION: WIZ8 0x0051c300
bool AddItemToCharacter(W8Character* character, W8ItemInstance* item, char equip_if_possible,
                        char announce, char skip_stacking)
{
    W8ItemInstance* stored_item = 0;
    unsigned int stored_index = 0;

    if (equip_if_possible && CanCharacterUseItem(character, item->item_id)) {
        int equip_slot = Function51C5A0(character, item->item_id);
        if (equip_slot != W8_EQUIP_SLOT_NONE) {
            W8ItemInstance* destination = &character->equipment[equip_slot];
            unsigned char stored;
            if (destination->item_id == -1) {
                CopyItemInstance(destination, item, character, 1);
                stored = 1;
            } else {
                stored = MergeItemStacks(destination, item, 0);
            }
            if (character->in_party) {
                Function50E5C0(CharacterPointerToPartySlot(character));
            }
            if (stored) {
                return true;
            }
        }
    }

    W8WideChar* display_name = FormatItemDisplayName(item, 1);
    if (g_item_records[item->item_id].quantity_kind == 1 && !skip_stacking) {
        unsigned int index;
        for (index = 0; index < 12; ++index) {
            if (MergeItemStacks(&character->equipment[index], item, 0)) {
                stored_item = &character->equipment[index];
                stored_index = index;
                break;
            }
        }
        if (stored_item == 0) {
            for (index = 0; index < 8; ++index) {
                if (MergeItemStacks(&character->backpack[index], item, 0)) {
                    stored_item = &character->backpack[index];
                    stored_index = index;
                    break;
                }
            }
        }
    }

    if (stored_item == 0) {
        while (stored_index < 8 && character->backpack[stored_index].item_id != -1) {
            ++stored_index;
        }
        if (stored_index == 8) {
            return false;
        }
        stored_item = &character->backpack[stored_index];
        CopyItemInstance(stored_item, item, 0, 1);
    }

    if (RecalculateCarriedWeight(character)) {
        Function4EDD20();
    }
    if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen_0069c0f4 != 0) {
        g_camp_screen_0069c0f4->item_redraw_flags |= 2 << stored_index;
    }
    if (announce) {
        PostCharacterNotice(CharacterPointerToPartySlot(character),
                            (const wchar_t*)gppStringList[0x7a0 / 4], display_name);
    }
    UpdateFactsAfterAcquiringItem(stored_item);
    Function5227D0(stored_item, 0, character);
    return true;
}

/* Take gold from the party. Asking for more than it has empties the purse
   rather than wrapping it around. */
// FUNCTION: WIZ8 0x0051bf40
void SpendPartyGold(unsigned int amount)
{
    if (amount > g_status_685170.party_gold) {
        g_status_685170.party_gold = 0;
    } else {
        g_status_685170.party_gold -= amount;
    }
}

// FUNCTION: WIZ8 0x005222d0
void GetOriginOfCharacterItem(int character_index, void* item, unsigned char* origin,
                              unsigned short* slot)
{
    unsigned int equipped_index;
    unsigned int carried_index;
    unsigned int pool_index;
    unsigned char* character;
    unsigned char* equipped;

    if (item == 0) {
        srAssertFail("pPCItem != NULL", "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp", 0x151b,
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

/* Initialize the fixed item-video-object vector to one entry per item record. */
// FUNCTION: WIZ8 0x0051b560
void InitializeItemVideoObjects(void)
{
    g_item_video_objects_68ec68.Initialize(gXStatus.uiItemsInDatabase);
}

/* Throw away the item-video-object vector and every lazily built generic name.
   The name walk is bounded by the address just past the table rather than by a
   count. */
// FUNCTION: WIZ8 0x0051b580
void ReleaseGenericItemNames(void)
{
    W8WideChar** name;

    g_item_video_objects_68ec68.Clear();
    for (name = g_generic_item_names; name < g_generic_item_names + W8_GENERIC_ITEM_NAME_COUNT;
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
// FUNCTION: WIZ8 0x0051b7b0
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
        swprintf((wchar_t*)built, (const wchar_t*)gppStringList[0x79c / 4],
                 gppStringList[g_generic_item_name_notice[name_index]]);
    }
    return g_generic_item_names[name_index];
}

/* Drop whatever is in hand, unless it is one of the items that may not be
   discarded - in which case say so instead. */
// FUNCTION: WIZ8 0x0051be50
bool DropItemInHand(int arg_1)
{
    if ((g_item_records[g_status_685170.item_in_hand_235b.item_id].flags_041 &
         W8_ITEM_FLAG_NO_DISCARD) != 0) {
        ShowNoticeLine(gppStringList[0x13bc / 4], 0, 1, 0);
        return false;
    }
    DropHeldItem(arg_1);
    return true;
}

/* Conjure one item and put it either straight into the party pool or into the
   hand, depending on where the last one was taken from. */
// FUNCTION: WIZ8 0x0051bf60
void CreateItemIntoHandOrPool(int item_id, unsigned char quality)
{
    W8ItemInstance created;

    g_held_item_source_006840c0 = -1;
    g_held_item_origin_006840c4 = 0xff;
    g_held_item_slot_006840c5 = 0xffff;
    ClearHeldItemDisplay();
    ReplaceOrCreateItem(&created, item_id, 1, quality, 0);
    if (g_status_685170.item_in_cursor != 0) {
        AddItemToParty(&created, 0, 0);
        return;
    }
    MoveItem(&g_status_685170.item_in_hand_235b, &created, 0, 1);
}

/* How many of a character's twenty item slots hold something they could use
   right now - the twelve worn and the eight carried, walked as two runs
   rather than one. */
// FUNCTION: WIZ8 0x0051f870
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
// FUNCTION: WIZ8 0x005207c0
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
// FUNCTION: WIZ8 0x00520f20
bool IsItemWornByCharacter(W8Character* character, const W8ItemInstance* item)
{
    unsigned int slot;

    if (character == 0) {
        return false;
    }
    for (slot = 0; slot < 12; ++slot) {
        if (&character->equipment[slot] == item) {
            return slot != W8_EQUIP_SLOT_ALTERNATE_RIGHT && slot != W8_EQUIP_SLOT_ALTERNATE_LEFT;
        }
    }
    return false;
}

/* Whether one item sits in a character's own carried slots. */
// FUNCTION: WIZ8 0x00520f60
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
// FUNCTION: WIZ8 0x0051bea0
void AddPartyGold(int amount, char announce)
{
    char sound_path[32];
    wchar_t* line;

    strcpy(sound_path, "Data\\Sound\\Misc\\ChaChing.wav");

    g_status_685170.party_gold += amount;
    if (g_status_685170.current_level < 0x2f) {
        g_status_685170.level_progress[g_status_685170.current_level].gold_collected += amount;
    }

    if (announce) {
        line = FormatWideString((const wchar_t*)gppStringList[0x788 / 4], gppStringList[0x57c / 4],
                                amount, gppStringList[0x580 / 4], -1, -1, 0);
        ShowNotice(8, line);
        if (!SoundFileIsPlaying(sound_path)) {
            SoundPlay(sound_path, 0);
        }
    }
}

/* Initialize one live item from its database record.  Reusing a packed party-
   pool slot first removes that slot from the pool; the new instance then gets
   its rolled (or maximum) quantity and the record's identification policy. */
// FUNCTION: WIZ8 0x0051c020
void ReplaceOrCreateItem(W8ItemInstance* item, int item_id, unsigned char maximum_quantity,
                         unsigned char force_identified, unsigned char mark_special)
{
    W8ItemInstance shifted[500];
    unsigned int index;

    if ((unsigned int)item_id >= gXStatus.uiItemsInDatabase) {
        srAssertFail("uiItemNo < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 564,
                     FormatString("InitNewItem: error: invalid item %d", item_id));
    }

    if (item == &g_status_685170.item_in_hand_235b) {
        g_held_item_source_006840c0 = -1;
        g_held_item_origin_006840c4 = 0xff;
        g_held_item_slot_006840c5 = 0xffff;
        ClearHeldItemDisplay();
    } else {
        memset(item, 0, sizeof(*item));
        item->item_id = -1;
        Function520D10(item, 0, 1);
    }

    if (item >= g_status_685170.party_item_pool_0021 &&
        item <= &g_status_685170.party_item_pool_0021[499]) {
        index = 0;
        while (index < (unsigned int)g_status_685170.party_item_count_1791 &&
               item != &g_status_685170.party_item_pool_0021[index]) {
            ++index;
        }
        if (index < (unsigned int)g_status_685170.party_item_count_1791 &&
            g_status_685170.party_item_pool_0021[index].item_id == -1) {
            unsigned int bytes =
                (g_status_685170.party_item_count_1791 - index - 1) * sizeof(W8ItemInstance);
            memcpy(&shifted[index], &g_status_685170.party_item_pool_0021[index + 1], bytes);
            memcpy(&g_status_685170.party_item_pool_0021[index], &shifted[index], bytes);
            memset(&g_status_685170.party_item_pool_0021[g_status_685170.party_item_count_1791 - 1],
                   0, sizeof(W8ItemInstance));
            g_status_685170.party_item_pool_0021[g_status_685170.party_item_count_1791 - 1]
                .item_id = -1;
            --g_status_685170.party_item_count_1791;
            Function4EDD20();
        }
    }

    item->item_id = item_id;
    const W8ItemDatabaseRecord* record = &g_item_records[item_id];
    if (record->quantity_kind != 0) {
        int maximum = record->initial_quantity.sides * record->initial_quantity.count +
                      record->initial_quantity.base;
        if (maximum >= 256) {
            srAssertFail("uiMAX_DICE(pMulti) < 256", PC_ITEM_CPP, 578, 0);
        }
        unsigned char quantity = maximum_quantity
                                     ? (unsigned char)maximum
                                     : (unsigned char)RollDice(&record->initial_quantity);
        if (record->quantity_kind == 1) {
            item->stack_count = quantity;
        } else if (record->quantity_kind >= 2 && record->quantity_kind <= 4) {
            item->uses_or_charges = quantity;
        } else {
            srAssertFail("FALSE", PC_ITEM_CPP, 599, "InitNewItem: ERROR - Invalid multi code");
        }
    }
    if (force_identified || (record->flags_041 & 1) != 0) {
        item->identified = 1;
    }
    if (mark_special) {
        item->bound = 1;
    }
}

/* Bind one worn item to its wearer. A binding that has not been announced yet
   is announced as it takes hold; one already announced just takes hold. A slot
   with no interface position binds nothing. */
// FUNCTION: WIZ8 0x0051d0d0
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
        WriteGameLog(8, (const wchar_t*)gppStringList[0x7a8 / 4], FormatItemDisplayName(item, 1));
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
    if (item->item_id >= (int)gXStatus.uiItemsInDatabase) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 4004, 0);
    }
    return g_item_records[item->item_id].spell_id;
}

/* Let the whole party have a go at identifying one item. Every retail caller
   passes a second zero argument; the body does not inspect it. Everybody able
   tries, but only the first attempt's answer is reported - the rest still
   happen for whatever they do to the item. */
// FUNCTION: WIZ8 0x005209f0
char PartyAttemptsToIdentifyItem(W8ItemInstance* item, int argument_2)
{
    char result = 0;
    int party_slot;

    if (item->identified != 0) {
        return 0;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0 &&
            g_party_characters[party_slot].hp_current != 0 &&
            g_party_characters[party_slot].unknown_0b01 < 0xb) {
            if (result == 0) {
                result = TryIdentifyItemFor(&g_party_characters[party_slot], item);
            } else {
                TryIdentifyItemFor(&g_party_characters[party_slot], item);
            }
        }
    }
    return result;
}

/* Score one identify attempt. Three times the attempt's strength, scaled by
   the attempter's percentage, has to reach the item's difficulty; clearing it
   reveals everything about the item at once. */
// FUNCTION: WIZ8 0x00520b40
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
// FUNCTION: WIZ8 0x00520a70
unsigned int CountIdentifyAttemptsNeeded(W8ItemInstance* item, unsigned int percent)
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
// FUNCTION: WIZ8 0x00520bc0
char RevealCharacterItemBindings(unsigned int party_slot, int strength, unsigned int percent)
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
            } else {
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
        if ((record->attack_flags_04e & (1 << index)) != 0) {
            return true;
        }
    }
    if (record->armor_class_bonus != 0) {
        return true;
    }
    for (index = 0; index < 6; ++index) {
        if (record->resistance_bonus_06f[index] != 0) {
            return true;
        }
    }
    if (record->binds_on_equip != 0 || record->modifier_06c != 0 || record->modifier_06d != 0 ||
        record->modifier_06e != 0) {
        return true;
    }
    return false;
}

/* The skill an identify attempt practises, and the one whose level supplies
   its strength - a sixth of it. */
enum { W8_SKILL_IDENTIFY = 0x14 };

/* The most of one item a character can hold at once: the record's own quantity
   dice taken at their maximum. */
static int MaximumQuantity(int item_id)
{
    return g_item_records[item_id].initial_quantity.sides *
               g_item_records[item_id].initial_quantity.count +
           g_item_records[item_id].initial_quantity.base;
}

/* Add uses to one item, never past what it can hold. */
// FUNCTION: WIZ8 0x0051e920
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
// FUNCTION: WIZ8 0x0051e9f0
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
// FUNCTION: WIZ8 0x00520f90
bool FindItemOnCharacter(W8Character* character, int item_id, W8ItemInstance** found,
                         int include_backpack, const W8ItemInstance* resume_after)
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

/* The whole-party find for one item: the item in hand first, then every
   occupied slot, and - when the caller asks for the pool - the party item
   pool. The resume item skips everything up to and including itself, which
   is how a caller walks a stack of matches. */
// FUNCTION: WIZ8 0x00521060
bool FindItemOnParty(int item_id, W8ItemInstance** found, W8Character** found_character,
                     int include_backpack, const W8ItemInstance* resume_after)
{
    if (found_character != 0) {
        *found_character = 0;
    }
    if ((resume_after == 0 || resume_after == &g_status_685170.item_in_hand_235b) &&
        g_status_685170.item_in_cursor != 0 &&
        g_status_685170.item_in_hand_235b.item_id == item_id) {
        if (found != 0) {
            *found = &g_status_685170.item_in_hand_235b;
        }
        return true;
    }

    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0 &&
            FindItemOnCharacter(&g_party_characters[party_slot], item_id, found, include_backpack,
                                resume_after)) {
            if (found_character != 0) {
                *found_character = &g_party_characters[party_slot];
            }
            return true;
        }
    }

    if (include_backpack == 2) {
        unsigned int index = 0;
        if (resume_after != 0) {
            while (index < (unsigned int)g_status_685170.party_item_count_1791 &&
                   &g_status_685170.party_item_pool_0021[index] != resume_after) {
                ++index;
            }
            ++index;
        }
        for (; index < (unsigned int)g_status_685170.party_item_count_1791; ++index) {
            W8ItemInstance* item = &g_status_685170.party_item_pool_0021[index];
            if (item->item_id == item_id) {
                if (found != 0) {
                    *found = item;
                }
                return true;
            }
        }
    }

    if (found != 0) {
        *found = 0;
    }
    return false;
}

/* How many of one item a character holds, counting a stack as its count and
   anything else as one, and optionally reporting the first slot it is in. */
// FUNCTION: WIZ8 0x005211a0
int CountItemOnCharacter(W8Character* character, int item_id, W8ItemInstance** first,
                         int include_backpack)
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

/* The whole-party counterpart of the count: the item in hand, every occupied
   slot, and the party item pool when the caller asks for it. */
// FUNCTION: WIZ8 0x00521240
unsigned int CountItemOnParty(int item_id, W8ItemInstance** found, W8Character** first_holder,
                              int include_backpack)
{
    unsigned int total = 0;

    if (first_holder != 0) {
        *first_holder = 0;
    }
    if (found != 0) {
        *found = 0;
    }
    if (g_status_685170.item_in_cursor != 0 &&
        g_status_685170.item_in_hand_235b.item_id == item_id) {
        total = g_status_685170.item_in_hand_235b.stack_count;
        if (found != 0 && *found == 0) {
            *found = &g_status_685170.item_in_hand_235b;
        }
    }

    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0) {
            int count = CountItemOnCharacter(&g_party_characters[party_slot], item_id, found,
                                             include_backpack);
            if (count != 0) {
                total += count;
                if (first_holder != 0 && *first_holder == 0) {
                    *first_holder = &g_party_characters[party_slot];
                }
            }
        }
    }

    if (include_backpack == 2) {
        for (unsigned int index = 0; index < (unsigned int)g_status_685170.party_item_count_1791;
             ++index) {
            W8ItemInstance* item = &g_status_685170.party_item_pool_0021[index];
            if (item->item_id == item_id) {
                total += item->stack_count == 0 ? 1 : item->stack_count;
                if (found != 0 && *found == 0) {
                    *found = item;
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
        if (g_party_slot_rows[party_slot].occupied != 0) {
            if (!FindItemOnCharacter(&g_party_characters[party_slot], item_id, 0, include_backpack,
                                     0)) {
                return false;
            }
        }
    }
    return true;
}

/* At what range an item's spell works. An item with no spell has no range at
   all, which is a different answer from touch. */
// FUNCTION: WIZ8 0x005207e0
int GetItemSpellRange(const W8ItemInstance* item)
{
    if (item == 0) {
        return -1;
    }
    if (item->item_id == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 4003, 0);
    }
    if (item->item_id >= (int)gXStatus.uiItemsInDatabase) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 4004, 0);
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
// FUNCTION: WIZ8 0x005208f0
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
// FUNCTION: WIZ8 0x0051d230
void BindEveryPartyItem(void)
{
    int party_slot;

    if (gXStatus.fCombatMode != 0 && g_combat_state->flag_001 == 0 &&
        gXStatus.fPartyMovementMode == 0) {
        ShowNotice(0xc, gppStringList[0x7d8 / 4], -1, -1, 0);
        return;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_0f5 == 0) {
            BindCharacterItems(party_slot, 0);
        }
    }
    ShowNotice(8, gppStringList[0x7b4 / 4], -1, -1, 0);
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
// FUNCTION: WIZ8 0x005205b0
void SortPartyItemPool(void)
{
    if (g_game_started == 0) {
        srAssertFail("gStatus.fGameStarted", PC_ITEM_CPP, 3795, 0);
    }
    if (g_status_685170.party_item_count_1791 > 1) {
        qsort(g_status_685170.party_item_pool_0021, g_status_685170.party_item_count_1791,
              sizeof(W8ItemInstance), CompareItemsForPool);
    }
}

/* Normalize a stack and split every full overflow stack into the party pool.
   Quantity kinds two through four live in uses_or_charges instead. */
// FUNCTION: WIZ8 0x0051fb40
void NormalizeItemStack(W8ItemInstance* item)
{
    if (item->item_id == -1) {
        return;
    }

    const W8ItemDatabaseRecord* record = &g_item_records[item->item_id];
    if (record->quantity_kind == 1) {
        if (item->uses_or_charges != 0) {
            if (item->stack_count == 0) {
                item->stack_count = item->uses_or_charges;
            }
            item->uses_or_charges = 0;
        }
    } else if (record->quantity_kind >= 2 && record->quantity_kind <= 4 && item->stack_count != 0) {
        if (item->uses_or_charges == 0) {
            item->uses_or_charges = item->stack_count;
        }
        item->stack_count = 0;
    }

    if (record->quantity_kind != 1) {
        return;
    }
    if (record->maximum_quantity == 0) {
        item->stack_count = 0;
        return;
    }
    while (item->stack_count > record->maximum_quantity) {
        W8ItemInstance split = *item;
        unsigned char quantity = item->stack_count - record->maximum_quantity;
        if (quantity > record->maximum_quantity) {
            quantity = record->maximum_quantity;
        }
        split.stack_count = quantity;

        if (g_status_685170.party_item_count_1791 >= 500) {
            item->stack_count = record->maximum_quantity;
            return;
        }
        W8ItemInstance* destination =
            &g_status_685170.party_item_pool_0021[g_status_685170.party_item_count_1791];
        memset(destination, 0, sizeof(*destination));
        destination->item_id = -1;
        CopyItemInstance(destination, &split, 0, 1);
        ++g_status_685170.party_item_count_1791;
        Function4EDD20();
        item->stack_count -= quantity;
        record = &g_item_records[item->item_id];
    }
}

/* Merge as much of one stack as fits in another.  A completely consumed
   source is removed from its owner; a partial merge is reported separately so
   the caller can refresh carrying capacity before placing the remainder. */
// FUNCTION: WIZ8 0x0051f900
unsigned char MergeItemStacks(W8ItemInstance* destination, W8ItemInstance* source,
                              unsigned char* partially_merged)
{
    if (destination->item_id == -1) {
        return 0;
    }

    NormalizeItemStack(destination);
    const W8ItemDatabaseRecord* record = &g_item_records[destination->item_id];
    if (record->quantity_kind != 1) {
        return 0;
    }
    if (destination->stack_count > record->maximum_quantity) {
        srAssertFail(
            "FALSE", PC_ITEM_CPP, 3371,
            FormatString(
                "StackItemsIfPossible: ERROR - slot quantity %d exceeds maximum %d for item %d %S",
                destination->stack_count, record->maximum_quantity, destination->item_id,
                FormatItemDisplayName(destination, 1)));
        destination->stack_count = record->maximum_quantity;
    }
    if (destination->item_id != source->item_id || destination->identified != source->identified) {
        return 0;
    }

    unsigned char room = record->maximum_quantity - destination->stack_count;
    if (room == 0) {
        return 0;
    }
    unsigned char moved = source->stack_count < room ? source->stack_count : room;
    destination->stack_count += moved;
    source->stack_count -= moved;
    if (source->stack_count == 0) {
        EmptyItemRecord(source, 0, 1);
        return 1;
    }
    if (partially_merged != 0) {
        *partially_merged = 1;
    }
    return 0;
}

/* Move a complete live item into an empty destination and clear its old slot.
   Moving into the held-item slot also remembers the character/slot origin so
   the cursor can return it later. */
// FUNCTION: WIZ8 0x0051fe30
void CopyItemInstance(W8ItemInstance* destination, W8ItemInstance* source, W8Character* character,
                      unsigned char refresh)
{
    W8ItemInstance shifted[500];
    unsigned int held_character = (unsigned int)-1;
    unsigned char held_origin = 0xff;
    unsigned short held_slot = 0xffff;

    if (destination == source) {
        srAssertFail("pToPCItem != pFromPCItem", PC_ITEM_CPP, 3558, 0);
    }
    if (destination->item_id != -1) {
        srAssertFail("pToPCItem->iItemNo == -1", PC_ITEM_CPP, 3560, 0);
    }
    if (source->item_id == -1) {
        srAssertFail("pFromPCItem->iItemNo != -1", PC_ITEM_CPP, 3561, 0);
    }
    if (source->bind_announced != 0) {
        source->bind_announced = 0;
    }
    memmove(destination, source, sizeof(*destination));

    if (destination == &g_status_685170.item_in_hand_235b && character != 0) {
        held_character = CharacterPointerToPartySlot(character);
        GetOriginOfCharacterItem(held_character, source, &held_origin, &held_slot);
    }

    if (source == &g_status_685170.item_in_hand_235b) {
        g_held_item_source_006840c0 = -1;
        g_held_item_origin_006840c4 = 0xff;
        g_held_item_slot_006840c5 = 0xffff;
        ClearHeldItemDisplay();
    } else {
        memset(source, 0, sizeof(*source));
        source->item_id = -1;
        Function520D10(source, character, refresh);
    }

    if (source >= g_status_685170.party_item_pool_0021 &&
        source <= &g_status_685170.party_item_pool_0021[499]) {
        unsigned int index = 0;
        unsigned int count = g_status_685170.party_item_count_1791;
        while (index < count && source != &g_status_685170.party_item_pool_0021[index]) {
            ++index;
        }
        if (index < count && g_status_685170.party_item_pool_0021[index].item_id == -1) {
            unsigned int bytes = (count - index - 1) * sizeof(W8ItemInstance);
            memcpy(&shifted[index], &g_status_685170.party_item_pool_0021[index + 1], bytes);
            memcpy(&g_status_685170.party_item_pool_0021[index], &shifted[index], bytes);
            memset(&g_status_685170.party_item_pool_0021[count - 1], 0, sizeof(W8ItemInstance));
            g_status_685170.party_item_pool_0021[count - 1].item_id = -1;
            --g_status_685170.party_item_count_1791;
            Function4EDD20();
        }
    }

    Function520D10(destination, character, refresh);
    if (destination == &g_status_685170.item_in_hand_235b) {
        g_held_item_source_006840c0 = held_character;
        g_held_item_origin_006840c4 = held_origin;
        g_held_item_slot_006840c5 = held_slot;
        SetItemCursor(0);
    }
}

/* Set the wield kind for one primary hand from the item it holds. An empty
   hand wields nothing; otherwise the item's equipment class selects the kind,
   with class two in the off hand deferring to the primary hand's item unless
   that item carries the paired flag. */
// FUNCTION: WIZ8 0x005201B0
void SetHandType(W8Character* character, unsigned int slot)
{
    if (character == 0) {
        srAssertFail("pPC != NULL", PC_ITEM_CPP, 3676, 0);
    }
    if (slot > 11) {
        srAssertFail("uiSlot < SLOT_COUNT", PC_ITEM_CPP, 3677, 0);
    }
    int hand;
    if (slot == W8_EQUIP_SLOT_PRIMARY_RIGHT) {
        hand = 0;
    } else {
        if (slot != W8_EQUIP_SLOT_PRIMARY_LEFT) {
            char* message =
                FormatString("SetHandType: ERROR: uiSlot %d is not a hand/weapon slot!", slot);
            srAssertFail("FALSE", PC_ITEM_CPP, 3690, message);
            return;
        }
        hand = 1;
    }
    int item_id = character->equipment[slot].item_id;
    int wield_kind;
    if (item_id == -1) {
        wield_kind = 0;
    } else {
        switch (g_item_records[item_id].equip_class) {
        case 0:
        case 1:
        case 3:
            wield_kind = 1;
            break;
        case 2:
            wield_kind = 1;
            if (slot != W8_EQUIP_SLOT_PRIMARY_LEFT ||
                character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT].item_id == -1 ||
                g_item_records[character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT].item_id]
                        .unidentified_name_index != 0x83) {
                break;
            }
            /* fall through */
        case 4:
            wield_kind = 3;
            break;
        case 5:
            wield_kind = 2;
            break;
        default: {
            char* message = FormatString("SetHandType: ERROR: invalid item %d type %d in hand %d!",
                                         item_id, g_item_records[item_id].equip_class, hand);
            srAssertFail("FALSE", PC_ITEM_CPP, 3731, message);
            return;
        }
        }
    }
    character->hand_attacks[hand].wield_kind = wield_kind;
}

/* The binding difficulty of a character's worn items: the worst identify
   difficulty among binds-on-equip pieces whose binding has not been announced
   yet, in thirds rounded up. Cure spells consult it when the target's worn
   items ask for more power than the condition does. */
// FUNCTION: WIZ8 0x00520C70
unsigned int Function520C70(int character_index)
{
    unsigned char max_difficulty = 0;
    W8ItemInstance* slot = g_status_685170.buffers.characters[character_index].equipment;

    for (int remaining = 12; remaining != 0; --remaining) {
        int item_id = slot->item_id;
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            slot->bind_announced == 0 &&
            max_difficulty < g_item_records[item_id].identify_difficulty) {
            max_difficulty = g_item_records[item_id].identify_difficulty;
        }
        ++slot;
    }
    unsigned int level = max_difficulty / 3;
    if (level * 3 < max_difficulty) {
        ++level;
    }
    return level;
}

/* Reconcile the character and combat state after one equipment record has
   been emptied.  With no character owner this intentionally does nothing;
   that is the path used while the new-game reset clears the carried pool. */
// FUNCTION: WIZ8 0x00520d10
void Function520D10(W8ItemInstance* item, W8Character* character, unsigned char refresh)
{
    if (!character) {
        return;
    }

    unsigned char primary_right = item == &character->equipment[W8_EQUIP_SLOT_PRIMARY_RIGHT];
    if (primary_right) {
        SetHandType(character, W8_EQUIP_SLOT_PRIMARY_RIGHT);
    }
    unsigned char primary_left = item == &character->equipment[W8_EQUIP_SLOT_PRIMARY_LEFT];
    if (primary_left) {
        SetHandType(character, W8_EQUIP_SLOT_PRIMARY_LEFT);
    }

    if (!character->in_party || !g_status_685170.game_started) {
        return;
    }

    unsigned int party_slot = CharacterPointerToPartySlot(character);
    if (primary_right || primary_left) {
        CalcAttacks(character);
    }
    if (!refresh) {
        return;
    }

    if (item == &character->equipment[11]) {
        g_byte_652da6 = FindItemOnParty(0x254, 0, 0, 0, 0);
    }
    Function50E5C0(party_slot);

    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    if (row->action_03d == 8 && row->action_detail_045.item_use.item == item) {
        DropCharacterFromRound(party_slot);
    }

    if (primary_right || primary_left) {
        int hand_state = row->action_kind;
        if (hand_state == 1) {
            if (!Function5458A0(party_slot)) {
                row->action_kind = 0;
                if (row->action_03d == 1) {
                    row->action_03d = 0;
                }
            }
        } else if (hand_state == 0 && row->action_is_kind_one && Function5458A0(party_slot)) {
            row->action_kind = 1;
            if (row->action_03d == 0) {
                row->action_03d = 1;
            }
        }

        if (gXStatus.fCombatMode) {
            int action = row->action_03d;
            if (action == 0 || action == 1) {
                if (!CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 1, 0)) {
                    AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_IN_COMBAT);
                } else if (!Function536F60(party_slot, 2)) {
                    Function536570(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0);
                }
            }
            g_combat_state->characters[party_slot].flag_81 ^= 1;
            if (party_slot == static_cast<unsigned int>(g_status_685170.selected_character)) {
                RequestRefreshPartyState();
            }
        }
        RequestRedraw(1 << (party_slot & 0x1f));
    }

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block) {
        if (g_level_block->combat_end_notification != -1) {
            Function595600();
        }
        Function593330();
    }
}

/* Empty one live item record.  A held item also resets the mouse cursor; a
   carried-pool item is removed from the packed 500-entry array and the tail is
   shifted down exactly once. */
// FUNCTION: WIZ8 0x00520070
void EmptyItemRecord(W8ItemInstance* item, W8Character* character, unsigned char refresh)
{
    W8ItemInstance shifted[500];

    if (item == &g_status_685170.item_in_hand_235b) {
        g_held_item_source_006840c0 = -1;
        g_held_item_origin_006840c4 = 0xff;
        g_held_item_slot_006840c5 = 0xffff;
        ClearHeldItemDisplay();
    } else {
        memset(item, 0, sizeof(*item));
        item->item_id = -1;
        Function520D10(item, character, refresh);
    }

    W8ItemInstance* pool = g_status_685170.party_item_pool_0021;
    if (item < pool || item > &pool[499]) {
        return;
    }

    unsigned int count = g_status_685170.party_item_count_1791;
    unsigned int index = 0;
    while (index < count && item != &pool[index]) {
        ++index;
    }
    if (index == count) {
        return;
    }
    if (pool[index].item_id != -1 || index >= count) {
        return;
    }

    unsigned int bytes = (count - index - 1) * sizeof(W8ItemInstance);
    memcpy(&shifted[index], &pool[index + 1], bytes);
    memcpy(&pool[index], &shifted[index], bytes);
    memset(&pool[count - 1], 0, sizeof(W8ItemInstance));
    pool[count - 1].item_id = -1;
    --g_status_685170.party_item_count_1791;
    Function4EDD20();
}

/* Empty every item record a character carries. The per-record helper
   0x00520070 was expanded at both loops, so this body repeats its logic
   rather than calling it. */
// FUNCTION: WIZ8 0x00520310
void EmptyAllCarriedItems(W8Character* character)
{
    W8ItemInstance shifted[500];
    unsigned int index;

    for (index = 0; index < 12; ++index) {
        W8ItemInstance* item = &character->equipment[index];
        if (item == &g_status_685170.item_in_hand_235b) {
            g_held_item_source_006840c0 = -1;
            g_held_item_origin_006840c4 = 0xff;
            g_held_item_slot_006840c5 = 0xffff;
            ClearHeldItemDisplay();
        } else {
            memset(item, 0, sizeof(*item));
            item->item_id = -1;
            Function520D10(item, character, 1);
        }

        W8ItemInstance* pool = g_status_685170.party_item_pool_0021;
        if (item >= pool && item <= &pool[499]) {
            unsigned int count = g_status_685170.party_item_count_1791;
            unsigned int position = 0;
            while (position < count && item != &pool[position]) {
                ++position;
            }
            if (position < count && pool[position].item_id == -1) {
                unsigned int bytes = (count - position - 1) * sizeof(W8ItemInstance);
                memcpy(&shifted[position], &pool[position + 1], bytes);
                memcpy(&pool[position], &shifted[position], bytes);
                memset(&pool[count - 1], 0, sizeof(W8ItemInstance));
                pool[count - 1].item_id = -1;
                --g_status_685170.party_item_count_1791;
                Function4EDD20();
            }
        }
    }

    for (index = 0; index < 8; ++index) {
        W8ItemInstance* item = &character->backpack[index];
        if (item == &g_status_685170.item_in_hand_235b) {
            g_held_item_source_006840c0 = -1;
            g_held_item_origin_006840c4 = 0xff;
            g_held_item_slot_006840c5 = 0xffff;
            ClearHeldItemDisplay();
        } else {
            memset(item, 0, sizeof(*item));
            item->item_id = -1;
            Function520D10(item, character, 1);
        }

        W8ItemInstance* pool = g_status_685170.party_item_pool_0021;
        if (item >= pool && item <= &pool[499]) {
            unsigned int count = g_status_685170.party_item_count_1791;
            unsigned int position = 0;
            while (position < count && item != &pool[position]) {
                ++position;
            }
            if (position < count && pool[position].item_id == -1) {
                unsigned int bytes = (count - position - 1) * sizeof(W8ItemInstance);
                memcpy(&shifted[position], &pool[position + 1], bytes);
                memcpy(&pool[position], &shifted[position], bytes);
                memset(&pool[count - 1], 0, sizeof(W8ItemInstance));
                pool[count - 1].item_id = -1;
                --g_status_685170.party_item_count_1791;
                Function4EDD20();
            }
        }
    }
}

/* Acquisition of a handful of plot items updates their paired facts. */
// FUNCTION: WIZ8 0x00522640
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item)
{
    switch (item->item_id) {
    case 0x243:
        if (!GetFact(0x30)) {
            SetFact(0x30, 1, 0);
        }
        if (GetFact(0x166)) {
            SetFact(0x166, 0, 0);
        }
        break;
    case 0x242:
        if (!GetFact(0x22)) {
            SetFact(0x22, 1, 0);
        }
        if (GetFact(0x165)) {
            SetFact(0x165, 0, 0);
        }
        break;
    case 0x244:
        if (!GetFact(0x31)) {
            SetFact(0x31, 1, 0);
        }
        if (GetFact(0x167)) {
            SetFact(0x167, 0, 0);
        }
        g_status_685170.flag_248a = 1;
        g_status_685170.value_2493 = g_status_685170.world_clock;
        break;
    case 0x264:
        if (!GetFact(0x182)) {
            SetFact(0x182, 1, 0);
            SetFact(0x31e, 1, 0);
        }
        break;
    case 0x201:
        SetFact(0x242, 0, 0);
        break;
    case 0x27c:
        SetFact(0x24e, 0, 0);
        break;
    case 0x1d2:
        if (GetFact(0x24f)) {
            SetFact(0x24f, 0, 0);
        }
        break;
    }
}

/* Deliver the one-time character reaction associated with exceptional items.
   Character creation (screen 5) deliberately suppresses every reaction. */
// FUNCTION: WIZ8 0x005227d0
void Function5227D0(W8ItemInstance* item, unsigned char choose_character, W8Character* character)
{
    int message;

    if (g_current_screen_state.id == 5) {
        return;
    }
    if (choose_character) {
        int party_slot = GetRandomCharacter(0, 0, -1, -1);
        if (party_slot == -1) {
            return;
        }
        character = &g_party_characters[party_slot];
    }
    if ((item->unknown_07[2] & 1) != 0) {
        return;
    }

    switch (item->item_id) {
    case 0x239:
        message = g_item_message_005ee6fc;
        break;
    case 0x242:
        message = g_item_message_005ee640;
        break;
    case 0x243:
        message = g_item_message_005ee644;
        break;
    case 0x244:
        message = GetFactionDispositionScore(W8_FACTION_MOOK) != 0 ? g_item_message_005ee648
                                                                   : g_item_message_005ee64c;
        break;
    case 0x264:
        message = g_item_message_005ee690;
        break;
    case 0x27c: {
        SetFact(0xe5, 1, 0);
        if (!NpcLeadHasNameStyle(7) || !GetFact(0x24e)) {
            return;
        }
        W8NpcState* npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        signed char npc_slot = npc->group_index;
        Function52E690(&g_party_characters[npc_slot], g_item_message_005ee68c, 0,
                       g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        return;
    }
    default:
        if (choose_character || !CanCharacterUseItem(character, item->item_id) ||
            g_item_records[item->item_id].value / character->level < 500 || item->identified == 0) {
            return;
        }
        message = g_item_message_005ee664;
        break;
    }

    Function52E690(character, message, 0, g_effect_argument_005ed8c8, g_effect_argument_005ed914);
    item->unknown_07[2] |= 1;
}

/* Insert one item into the packed party pool, first coalescing compatible
   stacks unless requested otherwise. */
// FUNCTION: WIZ8 0x00521ef0
bool AddItemToParty(W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking)
{
    W8ItemInstance shifted[500];
    W8WideChar* display_name = FormatItemDisplayName(item, 1);
    unsigned char partially_merged = 0;
    unsigned int index = 0;
    bool stored = false;

    if (g_item_records[item->item_id].quantity_kind == 1 && !skip_stacking) {
        while (index < (unsigned int)g_status_685170.party_item_count_1791) {
            if (MergeItemStacks(&g_status_685170.party_item_pool_0021[index], item,
                                &partially_merged)) {
                stored = true;
                break;
            }
            ++index;
        }
        if (partially_merged) {
            Function4EDD20();
        }
    }

    if (!stored) {
        if (g_status_685170.party_item_count_1791 >= 500) {
            return false;
        }
        index = 0;
        if (g_status_685170.party_item_count_1791 != 0) {
            unsigned int bytes = g_status_685170.party_item_count_1791 * sizeof(W8ItemInstance);
            memcpy(shifted, g_status_685170.party_item_pool_0021, bytes);
            memcpy(&g_status_685170.party_item_pool_0021[1], shifted, bytes);
        }
        memset(&g_status_685170.party_item_pool_0021[0], 0, sizeof(W8ItemInstance));
        g_status_685170.party_item_pool_0021[0].item_id = -1;
        CopyItemInstance(&g_status_685170.party_item_pool_0021[0], item, 0, 1);
        ++g_status_685170.party_item_count_1791;
        Function4EDD20();
        stored = true;
    }

    if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen_0069c0f4 != 0) {
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
    }
    if (announce) {
        WriteGameLog(8, (const wchar_t*)gppStringList[0x7a4 / 4], display_name);
    }
    W8ItemInstance* stored_item = &g_status_685170.party_item_pool_0021[index];
    UpdateFactsAfterAcquiringItem(stored_item);
    Function5227D0(stored_item, 1, 0);
    return stored;
}

/* An equipped item the character can no longer use is announced and moved
   into the backpack, or into the party pool when the backpack is full. The
   item record's two requirement pairs choose the attribute or skill name the
   message carries; an item whose requirements are still met gets the generic
   notice instead. The announcement runs before the move, and the party row's
   item is not refreshed by the temporary instance the move builds. */
// FUNCTION: WIZ8 0x0051d960
void Function51D960(W8Character* character)
{
    if (!IsPartyCharacterPointer(character)) {
        return;
    }

    for (int slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->equipment[slot];
        if (item->item_id == -1) {
            continue;
        }
        if (CanCharacterUseItem(character, item->item_id)) {
            continue;
        }

        unsigned int party_slot = CharacterPointerToPartySlot(character);
        const W8ItemDatabaseRecord* record = &g_item_records[item->item_id];
        unsigned short message_id = 0;
        W8ItemInstance destination;
        unsigned int index;

        for (index = 0; index < 2; ++index) {
            if (record->attribute_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
                character->attributes[(signed char)record->attribute_requirements[index].stat_id]
                        .effective < record->attribute_requirements[index].minimum) {
                message_id = g_character_description_first_ids_61e3a4
                    [(signed char)record->attribute_requirements[index].stat_id];
                goto announce_requirement;
            }
        }
        for (index = 0; index < 2; ++index) {
            if (record->skill_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
                character->skills[(signed char)record->skill_requirements[index].stat_id].level <
                    record->skill_requirements[index].minimum) {
                message_id =
                    g_character_skill_name_ids_61e454[(signed char)record->skill_requirements[index]
                                                          .stat_id];
                goto announce_requirement;
            }
        }
        PostCharacterNotice(party_slot, gppStringList[0x7c0 / 4],
                            gppStringList[g_gender_name_message_rows_61e430[character->gender][2]],
                            FormatItemDisplayName(item, 1), item, 1);
        goto move_item;

    announce_requirement:
        PostCharacterNotice(party_slot, gppStringList[0x7bc / 4],
                            gppStringList[g_gender_name_message_rows_61e430[character->gender][2]],
                            FormatItemDisplayName(item, 1),
                            gppStringList[g_gender_name_message_rows_61e430[character->gender][2]],
                            gppStringList[message_id]);

    move_item:
        destination.item_id = -1;
        destination.stack_count = 0;
        destination.uses_or_charges = 0;
        destination.identified = 0;
        Function520D10(&destination, 0, 1);
        Function51FD20(item, &destination, character, 1);
        if (!AddItemToCharacter(character, &destination, 0, 0, 0)) {
            AddItemToParty(&destination, 0, 0);
        }
    }
}

/* Pick the equipment slot a newly acquired item goes into. Before the game
   has started the two hand pairs are special-cased: a dual-wield-capable
   item can displace the right hand's item into the left hand, class-three
   gear fills a free pair before anything else, and class-four gear looks for
   a two-handed holder and otherwise fails. */
// FUNCTION: WIZ8 0x0051c5a0
int Function51C5A0(W8Character* character, int item_id)
{
    int slot = GetItemDefaultEquipSlot(item_id);
    if (g_status_685170.game_started != 0 || (slot != 6 && slot != 7)) {
        return slot;
    }

    int primary_right = character->equipment[6].item_id;
    int alternate_right = character->equipment[8].item_id;
    int primary_left = character->equipment[7].item_id;
    int alternate_left = character->equipment[9].item_id;
    switch (g_item_records[item_id].equip_class) {
    case 0:
    case 1:
        if (character->skills[0x12].value_02 != 0) {
            if (CanEquipItemInSlot(character, item_id, 7, 0)) {
                if (primary_right != -1 && primary_left == -1 &&
                    (g_item_records[primary_right].equip_class == 0 ||
                     g_item_records[primary_right].equip_class == 1) &&
                    (g_item_records[primary_right].flags_041 & 4) == 0) {
                    slot = 7;
                }
                if (alternate_right != -1 && alternate_left == -1 &&
                    (g_item_records[alternate_right].equip_class == 0 ||
                     g_item_records[alternate_right].equip_class == 1) &&
                    (g_item_records[alternate_right].flags_041 & 4) == 0) {
                    return 9;
                }
            } else {
                if (primary_right != -1 && primary_left == -1 &&
                    CanEquipItemInSlot(character, primary_right, 7, 0)) {
                    CopyItemInstance(&character->equipment[7], &character->equipment[6], 0, 1);
                    return 6;
                }
                if (alternate_right != -1 && alternate_left == -1 &&
                    CanEquipItemInSlot(character, alternate_right, 9, 0)) {
                    CopyItemInstance(&character->equipment[9], &character->equipment[8], 0, 1);
                    return 8;
                }
            }
        }
        break;
    case 3:
        if (primary_right == -1 && primary_left == -1) {
            return 6;
        }
        if (alternate_right == -1 && alternate_left == -1) {
            return 8;
        }
        break;
    case 4:
        if (primary_right != -1 && g_item_records[primary_right].equip_class == 3) {
            return 7;
        }
        if (alternate_right != -1 && g_item_records[alternate_right].equip_class == 3) {
            return 9;
        }
        return W8_EQUIP_SLOT_NONE;
    }
    return slot;
}
