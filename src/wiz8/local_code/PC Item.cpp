#include "wiz8/conditions.h"
#include "wiz8/fonts.h"
#include "soundman.h"
#include "wiz8/integer_constants.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/sound_man.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"

#include <stdio.h>
#include "wiz8/character_skills.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/Screens.h"

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
        return g_item_records[item->iItemNo].weight;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0051b9e0
int GetItemInHand(void)
{
    if (!g_status.item_in_cursor) {
        return -1;
    }
    return g_status.item_in_hand.iItemNo;
}

/* What a whole stack weighs. An empty slot weighs nothing, and a slot holding
   an item that does not stack still weighs one of it - the count is zero for
   every quantity kind but the stacking one. */
// FUNCTION: WIZ8 0x0051bfd0
unsigned int GetItemStackWeight(const W8ItemInstance* item)
{
    unsigned int weight;

    weight = 0;
    if (item->iItemNo != -1) {
        weight = g_item_records[item->iItemNo].weight;
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
    if (g_item_records[item->iItemNo].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return (item->stack_count * g_item_records[item->iItemNo].value +
                (W8_ITEM_BUNDLE_SIZE - 1)) /
               W8_ITEM_BUNDLE_SIZE;
    }
    if (g_item_records[item->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK &&
        item->stack_count > 1) {
        return g_item_records[item->iItemNo].value * item->stack_count;
    }
    return g_item_records[item->iItemNo].value;
}

/* Where an item of this kind wants to go. Two of the thirteen equipment
   classes answer differently once play has started: the one-handed weapon and
   the off-hand class move from the primary pair of hand slots to the alternate
   pair, which the character-creation screens do not fill. */
// FUNCTION: WIZ8 0x0051c4e0
W8EquipSlot GetItemDefaultEquipSlot(int item_id)
{
    switch (g_item_records[item_id].equip_class) {
    case W8_ITEM_EQUIP_CLASS_SHORT_WEAPON:
    case W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON:
        return W8_EQUIP_SLOT_PRIMARY_WEAPON;
    case W8_ITEM_EQUIP_CLASS_RANGED_WEAPON:
        return g_status.game_started ? W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON
                                     : W8_EQUIP_SLOT_PRIMARY_WEAPON;
    case W8_ITEM_EQUIP_CLASS_AMMUNITION:
        return g_status.game_started ? W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON
                                     : W8_EQUIP_SLOT_SECONDARY_WEAPON;
    case W8_ITEM_EQUIP_CLASS_THROWN_WEAPON:
        return W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON;
    case W8_ITEM_EQUIP_CLASS_MISC:
        return W8_EQUIP_SLOT_MISC_1;
    case W8_ITEM_EQUIP_CLASS_HEAD:
        return W8_EQUIP_SLOT_HEAD;
    case W8_ITEM_EQUIP_CLASS_TORSO:
        return W8_EQUIP_SLOT_TORSO;
    case W8_ITEM_EQUIP_CLASS_LEGS:
        return W8_EQUIP_SLOT_LEGS;
    case W8_ITEM_EQUIP_CLASS_HANDS:
        return W8_EQUIP_SLOT_HANDS;
    case W8_ITEM_EQUIP_CLASS_FEET:
        return W8_EQUIP_SLOT_FEET;
    case W8_ITEM_EQUIP_CLASS_CLOAK:
        return W8_EQUIP_SLOT_CLOAK;
    case W8_ITEM_EQUIP_CLASS_SHIELD:
        return W8_EQUIP_SLOT_SECONDARY_WEAPON;
    default:
        return W8_EQUIP_SLOT_NONE;
    }
}

/* The hand opposite the one given. Anything that is not a hand has no
   opposite. */
// FUNCTION: WIZ8 0x0051c8b0
W8EquipSlot GetPairedEquipSlot(W8EquipSlot equip_slot)
{
    switch (equip_slot) {
    case W8_EQUIP_SLOT_PRIMARY_WEAPON:
        return W8_EQUIP_SLOT_SECONDARY_WEAPON;
    case W8_EQUIP_SLOT_SECONDARY_WEAPON:
        return W8_EQUIP_SLOT_PRIMARY_WEAPON;
    case W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON:
        return W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON;
    case W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON:
        return W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON;
    default:
        return W8_EQUIP_SLOT_NONE;
    }
}

/* Which slot each bit of an equip-slot mask stands for: bit N is slot N. */
#define W8_EQUIP_SLOT_BIT(slot) ((unsigned short)(1 << (slot)))

/* No weapon skill at all, which GetItemEquipSlotMask treats as a data error. */
enum { W8_WEAPON_SKILL_NONE = -1 };

/* 0x0061E956: the gppStringList message each item use kind shows for whatever
   the item was used on. Retail reads a word at a four-byte stride from this
   run of consecutive ids - every other entry - so the access doubles the use
   kind. The run's extent ends where g_action_kind_message_ids starts;
   its last six ids, from W8_ITEM_PROPERTY_MESSAGE_FIRST, are the item property
   labels the assay dialog reads (0x0061E97C). */
// GLOBAL: WIZ8 0x0061e956
unsigned short g_item_use_messages[25] = {
    0x4f6, 0x4f7, 0x4f8, 0x4f9, 0x4fa, 0x4fb, 0x4fc, 0x4fd, 0x4fe, 0x4ff, 0x500, 0x501, 0x502,
    0x503, 0x504, 0x505, 0x506, 0x507, 0x508, 0x509, 0x50a, 0x50b, 0x50c, 0x50d, 0x50e,
};
/* 0x0068C108: one lazily built generic name per unidentified-name index, and
   0x0061E810: the notice each index formats from. The table's extent is the
   pointer bound the release walk stops at. */
// GLOBAL: WIZ8 0x0068C108
wchar_t* g_generic_item_names[W8_GENERIC_ITEM_NAME_COUNT];
// GLOBAL: WIZ8 0x00616e84
W8Skill g_item_spell_presentation[11] = {
    W8_SKILL_NONE,        W8_SKILL_ARTIFACTS,      W8_SKILL_ARTIFACTS, W8_SKILL_NONE,
    W8_SKILL_NONE,        W8_SKILL_NONE,           W8_SKILL_MUSIC,     W8_SKILL_THROWING_SLING,
    W8_SKILL_ENGINEERING, W8_SKILL_MODERN_WEAPONS, W8_SKILL_SWORD};
/* The twelve slots' paper-doll icons: the two alternate-set hand slots have
   none, which is the value the bound-item predicates refuse a binding behind. */
// GLOBAL: WIZ8 0x00648c5c
int g_equip_slot_icons[12] = {0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0};
/* The leading entries are an offset alias of the tail of
   g_equip_class_name_ids; retail reads both views of one block. */
// GLOBAL: WIZ8 0x0061E810
// offset alias of the tail of g_equip_class_name_ids; shared retail
// storage.
unsigned short g_generic_item_name_notice[W8_GENERIC_ITEM_NAME_COUNT] = {
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
static wchar_t g_item_display_name_buffer[42];

static_assert(sizeof(W8ItemVideoObjectEntry) == 8, "W8ItemVideoObjectEntry_must_be_8");
static_assert(sizeof(W8ItemVideoObjectCache) == 0x0c, "W8ItemVideoObjectCache_must_be_0x0c");

// GLOBAL: WIZ8 0x0068EC68
W8ItemVideoObjectCache g_item_video_objects;

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

// GLOBAL: WIZ8 0x0062a88c
static char g_item_video_object_fallback_names[145][0x30] = {"Dagger.sti",
                                                             "LongSword.sti",
                                                             "Bipennis.sti",
                                                             "BattleAxe.sti",
                                                             "Flail.sti",
                                                             "Mace.sti",
                                                             "Hammer.sti",
                                                             "ShortStaff.sti",
                                                             "Halberd.sti",
                                                             "Spear.sti",
                                                             "BoStaff.sti",
                                                             "LongBow.sti",
                                                             "Crossbow.sti",
                                                             "Sling.sti",
                                                             "GreatSword.sti",
                                                             "Rapier.sti",
                                                             "Katana.sti",
                                                             "LongStaff.sti",
                                                             "Wand.sti",
                                                             "MagicStave.sti",
                                                             "QuestionMark.sti",
                                                             "Shuriken.sti",
                                                             "Arrow.sti",
                                                             "SlingShot.sti",
                                                             "KiteShield.sti",
                                                             "Basnet.sti",
                                                             "PlateTorso.sti",
                                                             "PlateLegs.sti",
                                                             "GauntletSteel.sti",
                                                             "Sollerets.sti",
                                                             "Amulet.sti",
                                                             "Ring.sti",
                                                             "BluePotion.sti",
                                                             "Scroll.sti",
                                                             "PowderPouch.sti",
                                                             "Key.sti",
                                                             "Locket.sti",
                                                             "GreatBow.sti",
                                                             "RoundShield.sti",
                                                             "WizardCone.sti",
                                                             "LeatherHelm.sti",
                                                             "Cuirass.sti",
                                                             "LeatherHauberk.sti",
                                                             "Greaves.sti",
                                                             "LeatherPants.sti",
                                                             "Gloves.sti",
                                                             "Sandals.sti",
                                                             "Buskins.sti",
                                                             "Boots.sti",
                                                             "Book.sti",
                                                             "Ankh.sti",
                                                             "Ninjato.sti",
                                                             "WarHammer.sti",
                                                             "Flamberge.sti",
                                                             "Bullwhip.sti",
                                                             "Sai.sti",
                                                             "Nunchuka.sti",
                                                             "Glaive.sti",
                                                             "BlackSword.sti",
                                                             "FireSword.sti",
                                                             "RobeUpper.sti",
                                                             "RobeLower.sti",
                                                             "HalterTop.sti",
                                                             "Skirt.sti",
                                                             "SkullCap.sti",
                                                             "FeatheredCap.sti",
                                                             "Mitre.sti",
                                                             "QuestionMark.sti",
                                                             "Kabuto.sti",
                                                             "Do-MaruUpper.sti",
                                                             "Tosei-GusokuLower.sti",
                                                             "Burgonet.sti",
                                                             "FurLegs.sti",
                                                             "Cloak.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "Garland.sti",
                                                             "CatONineTails.sti",
                                                             "SilverCross.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "Bracelet.sti",
                                                             "Necklace.sti",
                                                             "Quarrel.sti",
                                                             "QuestionMark.sti",
                                                             "Coif.sti",
                                                             "Hauberk.sti",
                                                             "ChainLower.sti",
                                                             "MailMittens.sti",
                                                             "ChainHosen.sti",
                                                             "Lance.sti",
                                                             "Armet.sti",
                                                             "NinjaCowl.sti",
                                                             "NinjaGarbUpper.sti",
                                                             "NinjaGarbLower.sti",
                                                             "TabiBoots.sti",
                                                             "Heaume.sti",
                                                             "BoxHelm.sti",
                                                             "DiamondRing.sti",
                                                             "Bag.sti",
                                                             "RedPotion.sti",
                                                             "GreenPotion.sti",
                                                             "PurplePotion.sti",
                                                             "YellowPotion.sti",
                                                             "QuestionMark.sti",
                                                             "T'RangStaff.sti",
                                                             "GauntletBronze.sti",
                                                             "QuestionMark.sti",
                                                             "LightSword.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "Dart.sti",
                                                             "PowderShot.sti",
                                                             "Musket.sti",
                                                             "QuestionMark.sti",
                                                             "Stone.sti",
                                                             "Stick.sti",
                                                             "ShortSword.sti",
                                                             "QuestionMark.sti",
                                                             "Bagpipes.sti",
                                                             "Lyre.sti",
                                                             "Lute.sti",
                                                             "Horn.sti",
                                                             "QuestionMark.sti",
                                                             "Bracelet2.sti",
                                                             "Shuriken2.sti",
                                                             "QuestionMark.sti",
                                                             "Wand2.sti",
                                                             "YellowPotion2.sti",
                                                             "MediumShield.sti",
                                                             "Omnigun_1.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "QuestionMark.sti",
                                                             "Saxaphone.sti",
                                                             "Violin.sti",
                                                             "Drum.sti",
                                                             "Bullroar.sti",
                                                             "QuestionMark.sti",
                                                             "emptybottle.sti",
                                                             "Stix.sti",
                                                             "rocket.sti",
                                                             "rocketlauncher.sti"};

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
    sprintf(g_video_frames[object].path, "%s\\%s", "Data\\Items", name);
    if (strstr(g_video_frames[object].path, ".sti") == 0) {
        strcat(g_video_frames[object].path, ".sti");
    }
    g_video_frames[object].storage_kind = W8_VIDEO_STORAGE_OBJECT;
    g_video_frames[object].loaded = false;
    g_video_frames[object].handle = 0;
    g_video_slots[frame].first_frame = object;
    g_video_slots[frame].y_offset = 0;
    EnsureCatalogFrameLoaded(frame, 0);
    ++loaded_count;
    entry->video_object = frame;
    entry->initialized = true;
    return frame;
}

/* 0x0051FE30 */

/* Whether a weapon and an off-hand item go together, named by its own error
   text at 0x0051C8F0. */

// GLOBAL: WIZ8 0x00652da6
unsigned char g_party_has_slope_override_item;

/* Build the stable display form used by notices and inventory controls.  The
   generic unidentified names are allocated once, while this returned buffer
   is shared by the quantity and plain-name forms. */
// FUNCTION: WIZ8 0x0051b5c0
wchar_t* FormatItemDisplayName(const W8ItemInstance* item, bool include_quantity)
{
    wchar_t* name;
    unsigned int name_index;

    if (item->identified) {
        name = g_item_records[item->iItemNo].display_name;
    } else {
        name_index = g_item_records[item->iItemNo].unidentified_name_index;
        if (g_generic_item_names[name_index] == 0) {
            name = static_cast<wchar_t*>(malloc(0x78));
            g_generic_item_names[name_index] = name;
            swprintf(name, gppStringList[0x1e7],
                     gppStringList[g_generic_item_name_notice[name_index]]);
        }
        name = g_generic_item_names[name_index];
    }

    if (include_quantity && item->stack_count > 1) {
        swprintf(g_item_display_name_buffer, L"%s (%d)", name,
                 static_cast<unsigned int>(item->stack_count));
    } else {
        swprintf(g_item_display_name_buffer, L"%s", name);
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
        return g_item_records[item->iItemNo].equip_class;
    }
    return 0;
}

/* Which generic name this item wears while unidentified. */
// FUNCTION: WIZ8 0x0051b910
unsigned short GetItemUnidentifiedNameIndex(const W8ItemInstance* item)
{
    if (item != 0) {
        return g_item_records[item->iItemNo].unidentified_name_index;
    }
    return 0;
}

/* Whether two items are of the same equipment class. */
// FUNCTION: WIZ8 0x0051b940
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second)
{
    if (first != 0 && second != 0) {
        return g_item_records[first->iItemNo].equip_class ==
               g_item_records[second->iItemNo].equip_class;
    }
    return false;
}

/* Whether two items look alike while unidentified. */
// FUNCTION: WIZ8 0x0051b990
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second)
{
    if (first != 0 && second != 0) {
        return g_item_records[first->iItemNo].unidentified_name_index ==
               g_item_records[second->iItemNo].unidentified_name_index;
    }
    return false;
}

/* Whether an item is bound to whoever is wearing it, which is what stops it
   being taken off or swapped away. */
// FUNCTION: WIZ8 0x0051d180
bool IsItemBoundToWearer(const W8ItemInstance* item)
{
    if (item->iItemNo != -1 && g_item_records[item->iItemNo].binds_on_equip != 0 && item->bound) {
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
unsigned short GetItemEquipSlotMask(int item_id, bool primary_off_hand_free,
                                    bool alternate_off_hand_free, bool primary_main_hand_free,
                                    bool alternate_main_hand_free)
{
    unsigned short slots = 0;

    switch (g_item_records[item_id].equip_class) {
    case W8_ITEM_EQUIP_CLASS_SHORT_WEAPON:
    case W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON:
    case W8_ITEM_EQUIP_CLASS_THROWN_WEAPON:
    case W8_ITEM_EQUIP_CLASS_RANGED_WEAPON:
        if (g_item_records[item_id].weapon_skill == W8_WEAPON_SKILL_NONE) {
            FormatDebugMessage(0,
                               "ERROR - Item %ls is a weapon without a skill specified -> Charles",
                               &g_item_records[item_id]);
            return 0;
        }
        if ((g_item_records[item_id].flags & W8_ITEM_FLAG_TWO_HANDED) != 0) {
            if (primary_off_hand_free) {
                slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_WEAPON);
            }
            if (alternate_off_hand_free) {
                return slots | W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON);
            }
            return slots;
        }
        slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_PRIMARY_WEAPON) |
                W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON);
        if ((g_item_records[item_id].flags & W8_ITEM_FLAG_OFF_HAND_ALLOWED) == 0) {
            return slots;
        }
        if (primary_main_hand_free) {
            slots |= W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_SECONDARY_WEAPON);
        }
        break;
    case W8_ITEM_EQUIP_CLASS_AMMUNITION:
    case W8_ITEM_EQUIP_CLASS_SHIELD:
        if (primary_main_hand_free) {
            slots = W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_SECONDARY_WEAPON);
        }
        break;
    case W8_ITEM_EQUIP_CLASS_TORSO:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_TORSO);
    case W8_ITEM_EQUIP_CLASS_LEGS:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_LEGS);
    case W8_ITEM_EQUIP_CLASS_HEAD:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_HEAD);
    case W8_ITEM_EQUIP_CLASS_HANDS:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_HANDS);
    case W8_ITEM_EQUIP_CLASS_FEET:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_FEET);
    case W8_ITEM_EQUIP_CLASS_MISC:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_MISC_1) | W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_MISC_2);
    case W8_ITEM_EQUIP_CLASS_CLOAK:
        return W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_CLOAK);
    default:
        return 0;
    }

    if (alternate_main_hand_free) {
        return slots | W8_EQUIP_SLOT_BIT(W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON);
    }
    return slots;
}

/* Whether one character could put this item in one particular slot. The four
   hand slots are read first: a hand counts as available when it is empty, and
   a main hand also counts when whatever it holds is not two-handed. Asking to
   ignore what is worn answers for an empty character instead. */
// FUNCTION: WIZ8 0x0051cea0
bool CanEquipItemInSlot(W8Character* character, int item_id, unsigned char equip_slot,
                        bool ignore_worn_items)
{
    bool primary_off_hand_free;
    bool alternate_off_hand_free;
    bool primary_main_hand_free;
    bool alternate_main_hand_free;

    primary_off_hand_free =
        character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo == -1 || ignore_worn_items;
    alternate_off_hand_free =
        character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo == -1 ||
        ignore_worn_items;
    primary_main_hand_free =
        character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1 ||
        (g_item_records[character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo].flags &
         W8_ITEM_FLAG_TWO_HANDED) == 0 ||
        ignore_worn_items;
    alternate_main_hand_free =
        character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo == -1 ||
        (g_item_records[character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo]
             .flags &
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
static short g_compatible_partner_pairs[6][2] = {
    {114, 113}, {110, 132}, {104, 132}, {144, 143}, {145, 146}, {0, 0},
};

// FUNCTION: WIZ8 0x0051E980
bool GetItemMergeKind(int item_id, short* related_kind)
{
    if (related_kind && item_id != -1) {
        for (int index = 0; g_compatible_partner_pairs[index][0] != 0; ++index) {
            if (g_compatible_partner_pairs[index][0] ==
                g_item_records[item_id].unidentified_name_index) {
                *related_kind = g_compatible_partner_pairs[index][1];
                return true;
            }
        }
    }
    return false;
}

/* Whether an off-hand item may pair with a ranged item when the two are held
   together. A handful of name indices name one partner directly, the
   two-weapon name indices fall back to the pair table, and the specialised
   ranged items carry their own accepted classes. Anything not named is
   refused. */
// FUNCTION: WIZ8 0x0051c8f0
bool CompatiblePartnerItems(int ranged_item_id, int other_item_id)
{
    if (static_cast<unsigned int>(ranged_item_id) >= gXStatus.uiItemsInDatabase) {
        srAssertFail("uiRangedItem < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 0x3fd,
                     FormatString("CompatiblePartnerItems: ERROR - Illegal RANGED item index %d",
                                  ranged_item_id));
    }
    if (static_cast<unsigned int>(other_item_id) >= gXStatus.uiItemsInDatabase) {
        srAssertFail("uiOtherItem < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 0x3fe,
                     FormatString("CompatiblePartnerItems: ERROR - Illegal OTHER item index %d",
                                  other_item_id));
    }

    short ranged_name = static_cast<short>(g_item_records[ranged_item_id].unidentified_name_index);
    short other_name = static_cast<short>(g_item_records[other_item_id].unidentified_name_index);

    switch (ranged_name) {
    case 0xb:
    case 0x25:
        if (other_name == 0x16) {
            return true;
        }
        break;
    case 0xc:
        if (other_name == 0x53) {
            return true;
        }
        break;
    case 0xd:
        if (other_name == 0x17) {
            return true;
        }
        break;
    case 0x68:
    case 0x6e:
    case 0x72:
        if (g_item_records[other_item_id].equip_class > W8_ITEM_EQUIP_CLASS_RANGED_WEAPON ||
            g_item_records[ranged_item_id].range_category ==
                g_item_records[other_item_id].range_category) {
            if (ranged_item_id != -1 && g_compatible_partner_pairs[0][0] != 0) {
                int index = 0;
                short name = g_compatible_partner_pairs[0][0];

                while (name != ranged_name) {
                    ++index;
                    name = g_compatible_partner_pairs[index][0];
                    if (name == 0) {
                        return true;
                    }
                }
                if (other_name != g_compatible_partner_pairs[index][1]) {
                    return false;
                }
            }
            return true;
        }
        break;
    case 0x83:
        switch (ranged_item_id) {
        case 599:
            if (other_name == 0x17 || other_name == 0x71) {
                return true;
            }
            break;
        case 600:
        case 0x259:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70) {
                return true;
            }
            break;
        case 0x25a:
        case 0x25b:
        case 0x25c:
        case 0x25d:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70 ||
                other_name == 0x15 || other_name == 0x7e ||
                (g_item_records[other_item_id].equip_class == W8_ITEM_EQUIP_CLASS_THROWN_WEAPON &&
                 other_name == 0)) {
                return true;
            }
            break;
        case 0x25e:
        case 0x25f:
        case 0x260:
        case 0x261:
        case 0x262:
            if (other_name == 0x17 || other_name == 0x71 || other_name == 0x70 ||
                other_name == 0x15 || other_name == 0x7e ||
                (g_item_records[other_item_id].equip_class == W8_ITEM_EQUIP_CLASS_THROWN_WEAPON &&
                 other_name == 0) ||
                other_name == 0x16 || other_name == 0x53) {
                return true;
            }
            return false;
        }
        break;
    }
    return false;
}

/* Whether two items may be held at the same time. Nothing pairs with a
   two-handed item. A weapon beside an off-hand item is decided by the weapon
   rule, which takes them in weapon-first order whichever way round they were
   passed. Two things that are not both weapons always agree, and two weapons
   have to belong to the same wield group. */
// FUNCTION: WIZ8 0x0051cc40
static bool CanHoldItemsTogether(int first_item_id, int second_item_id)
{
    if (first_item_id == -1 || second_item_id == -1) {
        return true;
    }
    if ((g_item_records[first_item_id].flags & W8_ITEM_FLAG_TWO_HANDED) != 0 ||
        (g_item_records[second_item_id].flags & W8_ITEM_FLAG_TWO_HANDED) != 0) {
        return false;
    }
    if (g_item_records[first_item_id].equip_class == W8_ITEM_EQUIP_CLASS_RANGED_WEAPON ||
        g_item_records[second_item_id].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return CompatiblePartnerItems(first_item_id, second_item_id);
    }
    if (g_item_records[second_item_id].equip_class == W8_ITEM_EQUIP_CLASS_RANGED_WEAPON ||
        g_item_records[first_item_id].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return CompatiblePartnerItems(second_item_id, first_item_id);
    }
    if (g_item_records[first_item_id].equip_class >= W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return true;
    }
    if (g_item_records[second_item_id].equip_class < W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return g_item_records[first_item_id].range_category ==
               g_item_records[second_item_id].range_category;
    }
    return true;
}

// FUNCTION: WIZ8 0x0051CDE0
bool HeldItemFitsPairedSlot(int party_slot, W8EquipSlot equip_slot)
{
    if (party_slot == -1)
        srAssertFail("iChar != -1", PC_ITEM_CPP, 0x4e0, 0);
    if (static_cast<unsigned int>(equip_slot) >= W8_EQUIP_SLOT_COUNT)
        srAssertFail("uiSlot < SLOT_COUNT", PC_ITEM_CPP, 0x4e1, 0);

    W8EquipSlot paired_slot;
    switch (equip_slot) {
    case W8_EQUIP_SLOT_PRIMARY_WEAPON:
        paired_slot = W8_EQUIP_SLOT_SECONDARY_WEAPON;
        break;
    case W8_EQUIP_SLOT_SECONDARY_WEAPON:
        paired_slot = W8_EQUIP_SLOT_PRIMARY_WEAPON;
        break;
    case W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON:
        paired_slot = W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON;
        break;
    case W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON:
        paired_slot = W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON;
        break;
    default:
        return true;
    }
    return CanHoldItemsTogether(
        g_status.item_in_hand.iItemNo,
        g_status.buffers.Char[party_slot].EquippedItem[paired_slot].iItemNo);
}

/* 0x0A: the bound the use-kind assertion names as USE_TYPE_COUNT, and the
   extent of the per-kind message table at 0x0061E956. */
enum { W8_ITEM_USE_TYPE_COUNT = 10 };

/* The sex mask value that admits either sex rather than the one bit a
   restrictive record names. */
enum { W8_ITEM_GENDER_MASK_ANY = 3 };

/* An unused requirement slot. */
enum { W8_ITEM_REQUIREMENT_NONE = 0xff };

/* The two profession levels the casting categories read, named by index into
   W8Character::profession_levels (0x008d + 4 * index, so 0x0ad and 0x0b1). The
   caster-item categories pair with them: CASTER_ITEM_6 reads index 9 through
   the spell-source path and CASTER_ITEM_8 reads index 8. */
enum { W8_CASTER_PROFESSION_INDEX_8 = 8, W8_CASTER_PROFESSION_INDEX_9 = 9 };

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

    if ((record->profession_mask & (W8_PROFESSION_LORD << character->iProfession)) == 0) {
        return false;
    }
    if ((record->race_mask & (W8_RACE_ELF << character->iRace)) == 0) {
        return false;
    }
    if (record->gender_mask != W8_ITEM_GENDER_MASK_ANY &&
        (record->gender_mask & (W8_GENDER_FEMALE << character->gender)) == 0) {
        return false;
    }

    for (index = 0; index < 2; ++index) {
        if (record->attribute_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
            character
                    ->attributes[static_cast<signed char>(
                        record->attribute_requirements[index].stat_id)]
                    .effective < record->attribute_requirements[index].minimum) {
            return false;
        }
    }
    for (index = 0; index < 2; ++index) {
        if (record->skill_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
            character->skills[static_cast<signed char>(record->skill_requirements[index].stat_id)]
                    .level < record->skill_requirements[index].minimum) {
            return false;
        }
    }

    if (record->category == W8_ITEM_CATEGORY_SPELL_SOURCE) {
        spell_id = record->spell_id;
        if (spell_id == W8_SPELL_NONE) {
            srAssertFail("uiSpell != SPELL_NONE", PC_ITEM_CPP, 1832, 0);
        }
        if (character->spell_learned[spell_id] == 1) {
            return false;
        }
    } else if (record->category == W8_ITEM_CATEGORY_CASTER_ITEM_6 ||
               record->category == W8_ITEM_CATEGORY_CASTER_ITEM_8) {
        spell_id = record->spell_id;
        if (spell_id == W8_SPELL_NONE) {
            srAssertFail("uiSpell != SPELL_NONE", PC_ITEM_CPP, 1844, 0);
        }
        if (record->category == W8_ITEM_CATEGORY_CASTER_ITEM_6) {
            minimum_caster_level = GetMinimumCasterLevelForSpell(spell_id);
            if (static_cast<unsigned int>(
                    character->profession_levels[W8_CASTER_PROFESSION_INDEX_9]) <
                minimum_caster_level) {
                return false;
            }
        } else {
            minimum_caster_level = GetMinimumCasterLevelForSpell(spell_id);
            if (static_cast<unsigned int>(
                    character->profession_levels[W8_CASTER_PROFESSION_INDEX_8]) <
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
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (CanPartySlotParticipate(slot)) {
            if (CanCharacterUseItem(&g_status.buffers.Char[slot], item_id)) {
                return true;
            }
        }
    }
    return false;
}

/* Whether this character may activate this item right now. Anything with an
   equip home - flagged on the record or assigned a default slot - must
   actually be worn in a body slot rather than held, then the category gates:
   spell sources are refused here because they go through the cast path,
   category zero is dead weight, spell-less records fail unless they are the
   one exempt item, and an empty charge stack fails. */
// FUNCTION: WIZ8 0x0051d800
bool CanCharacterActivateItem(W8Character* character, const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record;

    if (character == 0) {
        srAssertFail("pPC", PC_ITEM_CPP, 0x769, 0);
    }
    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 0x76a, 0);
    }
    if (item->iItemNo < -1) {
        srAssertFail("pPCItem->iItemNo >= -1", PC_ITEM_CPP, 0x76b, 0);
    }
    if (item->iItemNo >= static_cast<int>(gXStatus.uiItemsInDatabase)) {
        // c-style-cast-ok: verbatim retail assert string
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 0x76c,
                     0);
    }
    if (item->iItemNo == -1) {
        return false;
    }
    if (!CanCharacterUseItem(character, item->iItemNo)) {
        return false;
    }

    record = &g_item_records[item->iItemNo];
    if ((record->flags & W8_ITEM_FLAG_MUST_EQUIP_TO_USE) != 0 ||
        GetItemDefaultEquipSlot(item->iItemNo) != W8_EQUIP_SLOT_NONE) {
        if (!IsItemWornByCharacter(character, item)) {
            return false;
        }
    }

    if (record->category == W8_ITEM_CATEGORY_SPELL_SOURCE || record->category == 0) {
        return false;
    }
    if (item->iItemNo != 0x29f && record->spell_id == W8_SPELL_NONE) {
        return false;
    }
    if (record->quantity_kind == W8_ITEM_QUANTITY_CHARGES && item->uses_or_charges == 0) {
        return false;
    }
    return true;
}

/* Whether both weapon sets are entirely empty. */
// FUNCTION: WIZ8 0x0051f8d0
bool AreAllHandSlotsEmpty(const W8Character* character)
{
    return character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1 &&
           character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo == -1 &&
           character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo == -1 &&
           character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo == -1;
}

/* Which of four groups an item's home slot belongs to. The four hand slots
   share one group and the rest split two ways; what the groups are used for is
   not established here, only which slots fall together. */
// FUNCTION: WIZ8 0x0051c850
W8ItemEquipSlotGroup GetItemEquipSlotGroup(int item_id)
{
    switch (GetItemDefaultEquipSlot(item_id)) {
    case W8_EQUIP_SLOT_PRIMARY_WEAPON:
    case W8_EQUIP_SLOT_SECONDARY_WEAPON:
    case W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON:
    case W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON:
        return W8_ITEM_EQUIP_GROUP_HAND;
    case W8_EQUIP_SLOT_HEAD:
    case W8_EQUIP_SLOT_TORSO:
    case W8_EQUIP_SLOT_HANDS:
    case W8_EQUIP_SLOT_LEGS:
    case W8_EQUIP_SLOT_FEET:
        return W8_ITEM_EQUIP_GROUP_BODY;
    case W8_EQUIP_SLOT_MISC_1:
    case W8_EQUIP_SLOT_MISC_2:
    case W8_EQUIP_SLOT_CLOAK:
        return W8_ITEM_EQUIP_GROUP_ACCESSORY;
    default:
        return W8_ITEM_EQUIP_GROUP_OTHER;
    }
}

/* What to call an item. An identified one is called by its own name, which
   leads its record - so the record address is the name address. An
   unidentified one is called by the generic name its index shares, built once
   on first use and kept. */
// FUNCTION: WIZ8 0x0051b7b0
wchar_t* GetItemDisplayName(const W8ItemInstance* item)
{
    unsigned int name_index;
    wchar_t* built;

    if (item->identified) {
        return g_item_records[item->iItemNo].display_name;
    }

    name_index = g_item_records[item->iItemNo].unidentified_name_index;
    if (g_generic_item_names[name_index] == 0) {
        built = static_cast<wchar_t*>(malloc(0x78));
        g_generic_item_names[name_index] = built;
        swprintf(built, gppStringList[0x1e7],
                 gppStringList[g_generic_item_name_notice[name_index]]);
    }
    return g_generic_item_names[name_index];
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
bool ItemUsesShots(int item_id)
{
    if (item_id == -1) {
        return false;
    }
    return g_item_records[item_id].quantity_kind == W8_ITEM_QUANTITY_SHOTS;
}

/* How the interface presents the spell an item carries, drawn from a per
   category table. Two particular spells are excluded and answer with nothing
   at all. */
// FUNCTION: WIZ8 0x0051dcb0
W8Skill GetItemSpellPresentation(const W8ItemDatabaseRecord* record)
{
    if (record->spell_id != 'X' && record->spell_id != 't') {
        return g_item_spell_presentation[record->category];
    }
    return W8_SKILL_NONE;
}

/* Use one item as a character's action. The attempts that cannot happen at all
   come first, each with its own notice and character event: a spent charge
   stack, a spent dose, a class-0x0d item whose influence the character is still
   under, and the one item that resolves to an NPC fact instead of a spell.
   Everything else aims the item and casts the spell it carries, which is where
   the spell's presentation skill and the character's own level in it decide how
   hard the attempt is. `out_uses` receives the fatigue cost of the attempt, and
   stays -1 when nothing was attempted. */
/* Several early exits (empty quantity-kind notices, blocked casting aid,
   casting-aid power reduced to zero) never assign `used`; retail returned
   the unset local. Preserve that read. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                                   \
    "-Wsometimes-uninitialized" // uninit-ok: retail returns the unset used byte on rejected/spent-item paths; callers observe that indeterminate result.
#pragma clang diagnostic ignored                                                                   \
    "-Wuninitialized" // uninit-ok: retail returns the unset used byte on rejected/spent-item paths; callers observe that indeterminate result.
// FUNCTION: WIZ8 0x0051dde0
unsigned char UseItem(W8Character* character, W8ItemInstance* item, int* out_uses)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
    unsigned int party_slot = CharacterPointerToPartySlot(character);
    unsigned int power;
    unsigned int chance;
    W8Skill skill;
    int event_type;
    int fatigue_cost = -1;
    W8TargetSource target;
    unsigned char used;

    if (!CanCharacterUseItem(character, item->iItemNo)) {
        PostCharacterNotice(party_slot, gppStringList[0x164], GetItemDisplayName(item));
        *out_uses = -1;
        return 0;
    }
    if (!CanCharacterActivateItem(character, item)) {
        PostCharacterNotice(party_slot, gppStringList[0x165], GetItemDisplayName(item));
        *out_uses = -1;
        return 0;
    }

    if (record->quantity_kind == W8_ITEM_QUANTITY_CHARGES && item->uses_or_charges == 0) {
        PostCharacterNotice(party_slot, gppStringList[0x1f1], GetItemDisplayName(item));
        event_type = g_special_event13;
    } else if (record->quantity_kind == W8_ITEM_QUANTITY_SHOTS && item->uses_or_charges == 0) {
        PostCharacterNotice(party_slot, gppStringList[0x1f2], GetItemDisplayName(item));
        event_type = g_effect26;
    } else {
        if (record->equip_class == W8_ITEM_EQUIP_CLASS_INSTRUMENT &&
            character->uiCondition[W8_CONDITION_SILENCED] != 0) {
            PostCharacterNotice(party_slot, gppStringList[0x166]);
            *out_uses = -1;
            return 0;
        }

        /* The one item that is not used on anybody: it hands the level's NPC
           over to the fact that says the party has met them. */
        if (item->iItemNo == 0x29f) {
            W8NpcState* npc = GetNpcState(
                g_status.buffers.XChar[CharacterPointerToPartySlot(character)].npc_index);
            if (npc == 0) {
                srAssertFail("pNPC", PC_ITEM_CPP, 0x89d, 0);
            }
            if (npc->name_style != W8_NPC_RFS81_A) {
                srAssertFail("pNPC->ubNPCDBaseID == NPC_RFS81A", PC_ITEM_CPP, 0x89e, 0);
            }
            SetFact(W8_FACT_RFS81_HAS_BEEN_FIXED, 1, false);
            SetFact(W8_FACT_QUEST_ANDROID_BROKEN, 0, false);
            RemoveCharacterItem(character, item, true);
            return 1;
        }

        if (record->equip_class != W8_ITEM_EQUIP_CLASS_INSTRUMENT &&
            record->equip_class != W8_ITEM_EQUIP_CLASS_GADGET) {
            /* An ordinary item carries the spell's own power, and the one use
               kind whose presentation skill is nine costs a flat ten. */
            if (g_item_spell_presentation[record->category] == 9) {
                fatigue_cost = 10;
            }
            power = record->spell_power;
        } else {
            /* A casting aid has to beat the difficulty of the character's own
               level in the skill that presents the spell. Each attempt that
               fails is retried one power lower, and the first roll that
               succeeds ends the search.

               Spells 0x58/'X' and 0x74/'t' set skill to -1 via
               GetItemSpellPresentation. Unlike CastItemSpell, which
               skips difficulty and practice for those ids, retail UseItem
               still evaluates skills[skill].level and can
               PracticeCharacterSkill with skill == -1 when power hits zero. */
            skill = GetItemSpellPresentation(record);
            power = 7;
            do {
                if (GetItemUseDifficulty(character, skill, character->skills[skill].level,
                                         record->spell_id, power) > 0xf) {
                    --power;
                    continue;
                }
                chance = power * 7;
                while (power != 0 && chance > Random(100)) {
                    --power;
                    chance -= 7;
                }
                break;
            } while (power != 0);

            if (power == 0) {
                PostCharacterNotice(party_slot, gppStringList[0x1f3], GetItemDisplayName(item));
                PracticeCharacterSkill(character, skill, 1, false);
                *out_uses = -1;
                return 0;
            }
        }

        /* The targeting classes aim through the slot's own out-of-combat
           target; everything else is used on the character acting. */
        SetTargetSourceToCharacter(party_slot, &target);
        if (record->equip_class == W8_ITEM_EQUIP_CLASS_POTION ||
            (record->equip_class > W8_ITEM_EQUIP_CLASS_SCROLL &&
             record->equip_class < W8_ITEM_EQUIP_CLASS_KEY)) {
            if (!gXStatus.fCombatMode) {
                W8CombatSlot* aimed = &g_status.buffers.XChar[party_slot].target_out_of_combat;
                if (aimed->iType == W8_TARGET_KIND_CHARACTER) {
                    SetTargetSourceToCharacter(aimed->iChar, &target);
                } else if (aimed->iType == W8_TARGET_KIND_MONSTER) {
                    SetTargetSourceToMonster(
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0x8fd, PC_ITEM_CPP, aimed->iMonsterID, true)),
                        &target);
                }
            } else {
                AimAtCharacter(party_slot, party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            }
        }
        target.name_known = 1;

        if (record->category >= W8_ITEM_USE_TYPE_COUNT) {
            srAssertFail("pItemDB->ubUseType < USE_TYPE_COUNT", PC_ITEM_CPP, 0x906, 0);
        }
        /* A spell-carrying item the world-cursor handler refuses cannot be used
           at all, and the attempt is over before anything is announced. */
        if (record->spell_id != W8_SPELL_NONE &&
            g_item_records[item->iItemNo].spell_id != W8_SPELL_NONE &&
            g_item_records[item->iItemNo].equip_class != W8_ITEM_EQUIP_CLASS_GADGET &&
            (g_item_records[item->iItemNo].equip_class < W8_ITEM_EQUIP_CLASS_FOOD ||
             g_item_records[item->iItemNo].equip_class > W8_ITEM_EQUIP_CLASS_DRINK) &&
            DispatchWorldCursorNodeCommand(0, 4, 1)) {
            *out_uses = 0;
            return 0;
        }

        if (TargetSourceIsCharacter(&target, 0)) {
            PostCharacterNotice(target.iChar, g_format_s_space_s,
                                gppStringList[g_item_use_messages[record->category * 2]],
                                GetItemDisplayName(item));
        } else if (TargetSourceIsMonster(&target, 0)) {
            ShowNoticef(
                9, L"%s %s %s",
                GetMonsterName(MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                                   0x91d, PC_ITEM_CPP, target.iMonsterID, true)),
                               0, 0),
                gppStringList[g_item_use_messages[record->category * 2]], GetItemDisplayName(item));
        } else {
            srAssertFail("FALSE", PC_ITEM_CPP, 0x924, 0);
        }

        if (g_settings.verbose_combat_messages == 0) {
            SetTextBoxMode(1, -1);
        }
        if (record->spell_id == W8_SPELL_NONE) {
            char* message =
                FormatString("UseItem: ERROR - Usable item %d doesn't do anything!", item->iItemNo);
            srAssertFail("FALSE", PC_ITEM_CPP, 0x934, message);
            used = 0;
            fatigue_cost = 0;
        } else {
            used = CastItemSpell(character, item, power);
            if (!used) {
                fatigue_cost = 0;
            } else {
                RemoveCharacterItem(character, item, true);
            }
        }
        *out_uses = fatigue_cost;
        return used;
    }

    QueueCharacterEvent(&g_status.buffers.Char[party_slot], event_type, 0, g_effect_argument0,
                        g_character_event_full_volume);
    *out_uses = fatigue_cost;
    return used;
}
#pragma clang diagnostic pop

/* Whether the item worn in one slot may be taken off. A binding that has not
   yet been announced holds it in place, unless the slot is not a real
   equipment slot or the character is under the influence that overrides it. */
// FUNCTION: WIZ8 0x0051d1c0
bool CanUnequipSlotItem(const W8Character* character, W8EquipSlot equip_slot)
{
    const W8ItemInstance* item = &character->EquippedItem[equip_slot];

    if (item->iItemNo != -1 && g_item_records[item->iItemNo].binds_on_equip != 0 &&
        !item->bind_announced && g_equip_slot_icons[equip_slot] != -1 &&
        character->uiCondition[W8_CONDITION_DEAD] == 0) {
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
    if (item->iItemNo != -1) {
        if (!item->identified) {
            return true;
        }
        if (GetItemDefaultEquipSlot(item->iItemNo) != W8_EQUIP_SLOT_NONE) {
            return !item->bound;
        }
    }
    return false;
}

/* Combine the held item with another one. Two identified items that some
   database entry names as its components (in either order) are consumed and
   that entry is created in their place, provided the character has the skill
   it asks for; stacks give up as many units as the smaller one holds. Failing
   that, two items of one kind are stacked together. Returns whether a recipe
   merge (or, for stacking, a merge of any units) happened. */
// FUNCTION: WIZ8 0x0051F2F0
bool MergeItems(W8Character* character, W8ItemInstance* destination)
{
    W8ItemInstance* held = &g_status.item_in_hand;
    W8ItemInstance created;
    const W8ItemDatabaseRecord* recipe;
    W8ItemInstance* result_destination;
    unsigned int result_item_id;
    unsigned int index;
    unsigned char quantity;
    bool found = false;
    bool merged = false;
    bool partially_merged = false;

    if (held->iItemNo == -1) {
        srAssertFail("pMergePCItem->iItemNo != -1", PC_ITEM_CPP, 2922, 0);
    }
    if (destination->iItemNo == -1) {
        srAssertFail("pIntoPCItem->iItemNo != -1", PC_ITEM_CPP, 2923, 0);
    }
    if (!destination->identified || !held->identified) {
        ShowCampNoticeLine(gppStringList[0x163], 0, true, false);
        return false;
    }

    for (result_item_id = 0; result_item_id < gXStatus.uiItemsInDatabase; ++result_item_id) {
        recipe = &g_item_records[result_item_id];
        if ((recipe->merge_component_a == held->iItemNo &&
             recipe->merge_component_b == destination->iItemNo) ||
            (recipe->merge_component_b == held->iItemNo &&
             recipe->merge_component_a == destination->iItemNo)) {
            found = true;
            if (recipe->merge_skill == -1) {
                merged = true;
            } else {
                merged = character->skills[recipe->merge_skill].level >= recipe->merge_skill_level;
                if (merged) {
                    QueueCharacterEvent(character, g_learn_sound, 0, g_character_event_no_flags,
                                        g_character_event_full_volume);
                    PracticeCharacterSkill(character, static_cast<W8Skill>(recipe->merge_skill),
                                           recipe->merge_skill_level / 10, false);
                }
            }
            if (merged) {
                if (g_item_records[held->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK &&
                    g_item_records[destination->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
                    quantity = held->stack_count < destination->stack_count
                                   ? held->stack_count
                                   : destination->stack_count;
                    result_destination =
                        destination->stack_count > held->stack_count ? held : destination;
                } else {
                    quantity = 1;
                    result_destination =
                        g_item_records[held->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK
                            ? destination
                            : held;
                }

                if (g_item_records[held->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
                    for (index = 0; index < quantity; ++index) {
                        RemoveCharacterItem(character, held, true);
                    }
                } else {
                    EmptyItemRecord(held, character, true);
                }

                if (g_item_records[destination->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
                    for (index = 0; index < quantity; ++index) {
                        RemoveCharacterItem(character, destination, true);
                    }
                } else {
                    EmptyItemRecord(destination, character, true);
                }

                ReplaceOrCreateItem(&created, result_item_id, false, false, false);
                if (recipe->quantity_kind == W8_ITEM_QUANTITY_STACK) {
                    created.stack_count = quantity;
                }

                if (result_destination >= g_status.party_item_pool &&
                    result_destination <= &g_status.party_item_pool[499]) {
                    for (index = 0; index < g_status.party_item_count; ++index) {
                        if (result_destination == &g_status.party_item_pool[index] &&
                            g_status.party_item_count < 500) {
                            InsertItemIntoPartyPool(&created, index);
                        }
                    }
                } else {
                    CopyItemInstance(result_destination, &created, character, true);
                }
            }
            break;
        }
    }

    if (!merged) {
        if (held->iItemNo == destination->iItemNo) {
            bool stacked = MergeItemStacks(destination, held, &partially_merged);
            if (partially_merged) {
                return true;
            }
            return stacked;
        }
        if (!found) {
            ShowCampNoticeLine(gppStringList[0x161], 0, true, false);
        } else {
            ShowCampNoticeLine(FormatWideString(gppStringList[0x162], character->name), 0, true,
                               false);
        }
    }
    return merged;
}

/* Put an item somewhere it will fit. The flag decides which of the character
   and the party pool is tried first; the other is tried after, and then the
   first again, so a full destination never loses the item. */
// FUNCTION: WIZ8 0x0051c280
bool StoreItemWithCharacterOrParty(W8Character* character, W8ItemInstance* item, bool party_first,
                                   bool announce, bool equip_if_possible)
{
    if (!party_first) {
        if (AddItemToCharacter(character, item, equip_if_possible, announce, false)) {
            return true;
        }
    }
    if (AddItemToParty(item, announce, false)) {
        return true;
    }
    if (party_first) {
        if (AddItemToCharacter(character, item, equip_if_possible, announce, false)) {
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
bool AddItemToCharacter(W8Character* character, W8ItemInstance* item, bool equip_if_possible,
                        bool announce, bool skip_stacking)
{
    W8ItemInstance* stored_item = 0;
    unsigned int stored_index = 0;

    if (equip_if_possible && CanCharacterUseItem(character, item->iItemNo)) {
        W8EquipSlot equip_slot = ChooseCharacterEquipSlot(character, item->iItemNo);
        if (equip_slot != W8_EQUIP_SLOT_NONE) {
            W8ItemInstance* destination = &character->EquippedItem[equip_slot];
            bool stored;
            if (destination->iItemNo == -1) {
                CopyItemInstance(destination, item, character, true);
                stored = true;
            } else {
                stored = MergeItemStacks(destination, item, 0);
            }
            if (character->fInParty) {
                RebuildEquipmentAndDerivedStatsForSlot(CharacterPointerToPartySlot(character));
            }
            if (stored) {
                return true;
            }
        }
    }

    wchar_t* display_name = FormatItemDisplayName(item, true);
    if (g_item_records[item->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK && !skip_stacking) {
        unsigned int index;
        for (index = 0; index < 12; ++index) {
            if (MergeItemStacks(&character->EquippedItem[index], item, 0)) {
                stored_item = &character->EquippedItem[index];
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
        while (stored_index < 8 && character->backpack[stored_index].iItemNo != -1) {
            ++stored_index;
        }
        if (stored_index == 8) {
            return false;
        }
        stored_item = &character->backpack[stored_index];
        CopyItemInstance(stored_item, item, 0, true);
    }

    if (RecalculateCarriedWeight(character)) {
        RedistributePartyEncumbrance();
    }
    if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen != 0) {
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_BACKPACK_CELL_FIRST << stored_index;
    }
    if (announce) {
        PostCharacterNotice(CharacterPointerToPartySlot(character), gppStringList[0x1e8],
                            display_name);
    }
    UpdateFactsAfterAcquiringItem(stored_item);
    DeliverExceptionalItemReaction(stored_item, false, character);
    return true;
}

/* Take gold from the party. Asking for more than it has empties the purse
   rather than wrapping it around. */
// FUNCTION: WIZ8 0x0051bf40
void SpendPartyGold(unsigned int amount)
{
    if (amount > g_status.party_gold) {
        g_status.party_gold = 0;
    } else {
        g_status.party_gold -= amount;
    }
}

/* Resolve a recorded (origin, slot) pair back to the item instance it names:
   the carrier's backpack, their worn equipment or the shared party pool. */
// FUNCTION: WIZ8 0x00522180
W8ItemInstance* FindCharacterItemAt(int party_slot, unsigned char origin, unsigned short slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];

    if (static_cast<signed char>(origin) < 0) {
        srAssertFail("bSlotType >= 0", PC_ITEM_CPP, 0x14fd, 0);
    }
    if (origin >= W8_ITEM_ORIGIN_COUNT) {
        srAssertFail("bSlotType < SLOT_TYPE_COUNT", PC_ITEM_CPP, 0x14fe, 0);
    }
    if (static_cast<short>(slot) < 0) {
        srAssertFail("sSlotIndex >= 0", PC_ITEM_CPP, 0x1500, 0);
    }

    switch (origin) {
    case W8_ITEM_ORIGIN_BACKPACK:
        if (slot >= 8) {
            srAssertFail("sSlotIndex < MAX_CARRY_ITEM_SLOTS", PC_ITEM_CPP, 0x1505, 0);
        }
        return &character->backpack[slot];
    case W8_ITEM_ORIGIN_EQUIPPED:
        if (slot >= 12) {
            srAssertFail("sSlotIndex < SLOT_COUNT", PC_ITEM_CPP, 0x1509, 0);
        }
        return &character->EquippedItem[slot];
    case W8_ITEM_ORIGIN_PARTY_POOL:
        if (slot >= 500) {
            srAssertFail("sSlotIndex < MAX_PARTY_ITEM_SLOTS", PC_ITEM_CPP, 0x150d, 0);
        }
        return &g_status.party_item_pool[slot];
    }
    return 0;
}

// FUNCTION: WIZ8 0x005222d0
void GetOriginOfCharacterItem(int character_index, W8ItemInstance* item, unsigned char* origin,
                              unsigned short* slot)
{
    unsigned int backpack_index;
    unsigned int equipped_index;
    unsigned int pool_index;
    W8Character* character;
    W8ItemInstance* cursor;

    if (item == 0) {
        srAssertFail("pPCItem != NULL", "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp", 0x151b,
                     0);
    }

    character = &g_status.buffers.Char[character_index];
    for (backpack_index = 0; backpack_index < 8; ++backpack_index) {
        if (item == &character->backpack[backpack_index]) {
            *origin = W8_ITEM_ORIGIN_BACKPACK;
            *slot = static_cast<unsigned short>(backpack_index);
            return;
        }
    }

    equipped_index = 0;
    cursor = character->EquippedItem;
    for (; equipped_index < 12; ++equipped_index, ++cursor) {
        if (item == cursor) {
            *origin = W8_ITEM_ORIGIN_EQUIPPED;
            *slot = static_cast<unsigned short>(equipped_index);
            return;
        }
    }

    for (pool_index = 0; pool_index < g_status.party_item_count; ++pool_index) {
        if (item == &g_status.party_item_pool[pool_index]) {
            *origin = W8_ITEM_ORIGIN_PARTY_POOL;
            *slot = static_cast<unsigned short>(pool_index);
            return;
        }
    }

    *origin = W8_ITEM_ORIGIN_NONE;
    *slot = 0xffff;
}

static void DropUnstoredCharacterItem(W8ItemInstance* item)
{
    W8ItemInstance saved_hand;
    bool was_in_cursor;

    was_in_cursor = g_status.item_in_cursor;
    if (was_in_cursor) {
        saved_hand = g_status.item_in_hand;
    }
    gXStatus.held_item_source = -1;
    gXStatus.held_item_origin = W8_ITEM_ORIGIN_NONE;
    gXStatus.held_item_slot = 0xffff;
    ClearHeldItemDisplay();
    CopyItemInstance(&g_status.item_in_hand, item, 0, true);
    if ((g_item_records[g_status.item_in_hand.iItemNo].flags & W8_ITEM_FLAG_NO_DISCARD) == 0) {
        DropHeldItem(0);
    } else {
        ShowNoticeLine(gppStringList[0x4ef], 0, true, false);
    }
    if (was_in_cursor) {
        g_status.item_in_hand = saved_hand;
    }
}

/* Empty a departing character into the party pool. Equipment that is bound
   to its slot stays with the body unless the binding was announced, the slot
   has no cursor icon, or the character is dead. What the pool cannot take is
   parked in the held-item display - the previous contents are restored after
   the drop-or-refuse handling - and announced. The backpack loop keeps
   reading the equipment row's item id, a leftover the retail body shares. */
// FUNCTION: WIZ8 0x005223a0
void StashDepartingCharacterItems(W8Character* character)
{
    int item_id;

    for (int equip_slot = 0; equip_slot < W8_EQUIP_SLOT_COUNT; ++equip_slot) {
        W8ItemInstance* item = &character->EquippedItem[equip_slot];
        item_id = item->iItemNo;
        if (item_id != -1 &&
            (g_item_records[item_id].binds_on_equip == 0 || item->bind_announced ||
             g_equip_slot_icons[equip_slot] == -1 ||
             character->uiCondition[W8_CONDITION_DEAD] != 0) &&
            !AddItemToParty(item, false, false)) {
            DropUnstoredCharacterItem(item);
            ShowNoticef(W8_FONT_PALETTE_RED, gppStringList[0x7d3], &g_item_records[item_id]);
        }
    }

    for (int slot = 0; slot < 8; ++slot) {
        W8ItemInstance* item = &character->backpack[slot];
        item_id = character->EquippedItem[slot].iItemNo;
        if (item_id != -1 && !AddItemToParty(item, false, false)) {
            DropUnstoredCharacterItem(item);
            ShowNoticef(W8_FONT_PALETTE_RED, gppStringList[0x7d3], &g_item_records[item_id]);
        }
    }
}

/* Initialize the fixed item-video-object vector to one entry per item record. */
// FUNCTION: WIZ8 0x0051b560
void InitializeItemVideoObjects(void)
{
    g_item_video_objects.Initialize(gXStatus.uiItemsInDatabase);
}

/* Throw away the item-video-object vector and every lazily built generic name.
   The name walk is bounded by the address just past the table rather than by a
   count. */
// FUNCTION: WIZ8 0x0051b580
void ReleaseGenericItemNames(void)
{
    wchar_t** name;

    g_item_video_objects.Clear();
    for (name = g_generic_item_names; name < g_generic_item_names + W8_GENERIC_ITEM_NAME_COUNT;
         ++name) {
        if (*name != 0) {
            free(*name);
            *name = 0;
        }
    }
}

/* Hand the item in hand to a party member. It is first offered to everybody who
   could identify it, and then stored with the character or the party pool in
   the order the flag asks for. The item-in-hand state is cleared before the
   outcome is tested, and a store that fails is announced: the notice names the
   slot when neither destination was open, and the item otherwise. The flag
   doubles as the result the way the compiled body reads it, so a failed
   party-first attempt still reports success. */
// FUNCTION: WIZ8 0x0051ba00
bool GiveHeldItemToCharacterOrParty(int uiChar, bool party_first)
{
    W8ItemInstance* item = &g_status.item_in_hand;
    bool stored = party_first;
    bool identified = false;
    unsigned int slot;

    if (!g_status.item_in_cursor) {
        srAssertFail("gStatus.fItemInCursor", PC_ITEM_CPP, 376, 0);
    }

    /* Every member who is up and not too badly hurt gets an attempt at the
       item, which is what practises the skill for them; the first one who
       learns what it is bounds the loop's answer. */
    if (!item->identified) {
        for (slot = 0; slot < 8; ++slot) {
            W8Character* character = &g_status.buffers.Char[slot];
            if (!g_status.buffers.XChar[slot].fOccupied) {
                continue;
            }
            /* 0xb is the index the body bounds the sweep at: a member past it is
               too far gone to be offered the item. */
            if (character->hp_current == 0 || character->highest_condition >= W8_CONDITION_INSANE) {
                continue;
            }
            if (!identified) {
                identified = TryIdentifyItemFor(character, item);
            } else {
                TryIdentifyItemFor(character, item);
            }
        }
    }

    if (!g_status.game_started) {
        srAssertFail("gStatus.fGameStarted", PC_ITEM_CPP, 0x194, 0);
    }
    if (!g_status.buffers.XChar[uiChar].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", PC_ITEM_CPP, 0x195, 0);
    }

    if (IsPartySlotEligible(uiChar)) {
        W8Character* character = &g_status.buffers.Char[uiChar];
        if (StoreItemWithCharacterOrParty(character, item, party_first, true, false)) {
            stored = true;
        }
    } else if (stored) {
        stored = AddItemToParty(item, true, false);
    }

    gXStatus.held_item_source = -1;
    gXStatus.held_item_origin = W8_ITEM_ORIGIN_NONE;
    gXStatus.held_item_slot = 0xffff;
    ClearHeldItemDisplay();
    if (stored) {
        return stored;
    }

    if (!IsPartySlotEligible(uiChar) && !party_first) {
        ShowNoticeLine(gppStringList[0x1f5], 0, true, false);
    } else {
        ShowNoticeLine(gppStringList[0x90c], 0, true, false);
    }
    return stored;
}

/* Hand one item to a party member. The character and the party pool are tried
   in the order the flag asks for, with the other tried after - and when the
   slot has nobody in it, only the pool. The source record is consumed once the
   item is stored: the item in hand clears the cursor state instead, and a party
   pool entry that was already empty is shifted out of the packed array. */
// FUNCTION: WIZ8 0x0051bc00
bool GiveItemToCharacterOrParty(int uiChar, W8ItemInstance* item, bool party_first)
{
    bool stored;

    if (!g_status.game_started) {
        srAssertFail("gStatus.fGameStarted", PC_ITEM_CPP, 0x194, 0);
    }
    if (!g_status.buffers.XChar[uiChar].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", PC_ITEM_CPP, 0x195, 0);
    }

    if (!IsPartySlotEligible(uiChar)) {
        if (!party_first) {
            return false;
        }
        stored = AddItemToParty(item, true, false);
        if (!stored) {
            return false;
        }
    } else {
        W8Character* character = &g_status.buffers.Char[uiChar];
        stored = StoreItemWithCharacterOrParty(character, item, party_first, true, false);
        if (!stored) {
            return false;
        }
    }

    EmptyItemRecord(item, 0, true);
    return stored;
}

/* Drop whatever is in hand, unless it is one of the items that may not be
   discarded - in which case say so instead. */
// FUNCTION: WIZ8 0x0051be50
bool DropItemInHand(int unused)
{
    if ((g_item_records[g_status.item_in_hand.iItemNo].flags & W8_ITEM_FLAG_NO_DISCARD) != 0) {
        ShowNoticeLine(gppStringList[0x4ef], 0, true, false);
        return false;
    }
    DropHeldItem(unused);
    return true;
}

/* Conjure one item and put it either straight into the party pool or into the
   hand, depending on where the last one was taken from. */
// FUNCTION: WIZ8 0x0051bf60
void CreateItemIntoHandOrPool(int item_id, bool quality)
{
    W8ItemInstance created;

    gXStatus.held_item_source = -1;
    gXStatus.held_item_origin = W8_ITEM_ORIGIN_NONE;
    gXStatus.held_item_slot = 0xffff;
    ClearHeldItemDisplay();
    ReplaceOrCreateItem(&created, item_id, true, quality, false);
    if (g_status.item_in_cursor) {
        AddItemToParty(&created, false, false);
        return;
    }
    CopyItemInstance(&g_status.item_in_hand, &created, 0, true);
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
        if (CanCharacterActivateItem(character, &character->EquippedItem[index])) {
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

    if (equip_class != W8_ITEM_EQUIP_CLASS_POTION &&
        (equip_class < W8_ITEM_EQUIP_CLASS_FOOD || equip_class > W8_ITEM_EQUIP_CLASS_DRINK)) {
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
        if (&character->EquippedItem[slot] == item) {
            return slot != W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON &&
                   slot != W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON;
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
void AddPartyGold(int amount, bool announce)
{
    char sound_path[32];

    strcpy(sound_path, "Data\\Sound\\Misc\\ChaChing.wav");

    g_status.party_gold += amount;
    if (g_status.current_level < 0x2f) {
        g_status.level_progress[g_status.current_level].gold_collected += amount;
    }

    if (announce) {
        ShowNotice(
            8, FormatWideString(L"%s %d %s.", gppStringList[0x15f], amount, gppStringList[0x160]));
        if (!SoundFileIsPlaying(sound_path)) {
            SoundPlay(sound_path, 0);
        }
    }
}

/* Initialize one live item from its database record.  Reusing a packed party-
   pool slot first removes that slot from the pool; the new instance then gets
   its rolled (or maximum) quantity and the record's identification policy. */
// FUNCTION: WIZ8 0x0051c020
void ReplaceOrCreateItem(W8ItemInstance* item, int item_id, bool maximum_quantity,
                         bool force_identified, bool mark_special)
{
    if (static_cast<unsigned int>(item_id) >= gXStatus.uiItemsInDatabase) {
        srAssertFail(
            "uiItemNo < gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 564,
            reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                String("InitNewItem: error, invalid item # %ld specified", item_id)));
    }

    EmptyItemRecord(item, 0, true);

    item->iItemNo = item_id;
    const W8ItemDatabaseRecord* record = &g_item_records[item_id];
    if (record->quantity_kind != W8_ITEM_QUANTITY_NONE) {
        int maximum = record->initial_quantity.Maximum();
        if (maximum >= 256) {
            srAssertFail("uiMAX_DICE(pMulti) < 256", PC_ITEM_CPP, 578, 0);
        }
        unsigned char quantity =
            maximum_quantity ? static_cast<unsigned char>(maximum)
                             : static_cast<unsigned char>(RollDice(&record->initial_quantity));
        if (record->quantity_kind == W8_ITEM_QUANTITY_STACK) {
            item->stack_count = quantity;
        } else if (record->quantity_kind >= W8_ITEM_QUANTITY_CHARGES &&
                   record->quantity_kind <= W8_ITEM_QUANTITY_SHOTS) {
            item->uses_or_charges = quantity;
        } else {
            srAssertFail("FALSE", PC_ITEM_CPP, 599, "InitNewItem: ERROR - Invalid multi code");
        }
    }
    if (force_identified || (record->flags & W8_ITEM_FLAG_AUTO_IDENTIFY) != 0) {
        item->identified = true;
    }
    if (mark_special) {
        item->bound = true;
    }
}

/* Bind one worn item to its wearer. A binding that has not been announced yet
   is announced as it takes hold; one already announced just takes hold. A slot
   with no interface position binds nothing. */
// FUNCTION: WIZ8 0x0051d0d0
void BindEquippedItem(W8Character* character, W8EquipSlot equip_slot)
{
    W8ItemInstance* item = &character->EquippedItem[equip_slot];

    if (item->iItemNo == -1) {
        return;
    }
    if (g_item_records[item->iItemNo].binds_on_equip == 0 || item->bind_announced) {
        if (g_equip_slot_icons[equip_slot] != -1 && !item->bound) {
            item->bound = true;
        }
        return;
    }
    if (g_equip_slot_icons[equip_slot] != -1 && !item->bound) {
        item->bound = true;
        ShowNoticef(W8_FONT_PALETTE_WHITE, gppStringList[0x1ea], FormatItemDisplayName(item, true));
    }
}

/* The spell an item carries, with both bounds on the item id asserted - the
   second names the database count as gXStatus.uiItemsInDatabase. */
// FUNCTION: WIZ8 0x00520880
int GetItemSpell(const W8ItemInstance* item)
{
    if (item == 0) {
        return 0;
    }
    if (item->iItemNo == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 4003, 0);
    }
    if (item->iItemNo >= static_cast<int>(gXStatus.uiItemsInDatabase)) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 4004, 0);
    }
    return g_item_records[item->iItemNo].spell_id;
}

/* Let the whole party have a go at identifying one item. Every retail caller
   passes a second zero argument; the body does not inspect it. Everybody able
   tries, but only the first attempt's answer is reported - the rest still
   happen for whatever they do to the item. */
// FUNCTION: WIZ8 0x005209f0
bool PartyAttemptsToIdentifyItem(W8ItemInstance* item, int)
{
    bool result = false;
    int party_slot;

    if (item->identified) {
        return false;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied &&
            g_status.buffers.Char[party_slot].hp_current != 0 &&
            g_status.buffers.Char[party_slot].highest_condition < W8_CONDITION_INSANE) {
            if (!result) {
                result = TryIdentifyItemFor(&g_status.buffers.Char[party_slot], item);
            } else {
                TryIdentifyItemFor(&g_status.buffers.Char[party_slot], item);
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
    if (g_item_records[item->iItemNo].identify_difficulty <= score) {
        if (item == 0) {
            srAssertFail("pPCItem", PC_ITEM_CPP, 2875, 0);
        }
        item->identified = true;
        item->spell_hint = true;
        item->unknown_08 = 1;
        item->bound = true;
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
    if (item->iItemNo == -1) {
        return 1;
    }

    attempts = g_item_records[item->iItemNo].identify_difficulty / 3;
    if (g_item_records[item->iItemNo].identify_difficulty % 3 != 0) {
        ++attempts;
    }
    candidate = attempts * 3;
    while (attempts > 1) {
        candidate -= 3;
        score = candidate;
        AdjustIntegerByPercent(&score, percent);
        if (score < g_item_records[item->iItemNo].identify_difficulty) {
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
int RevealCharacterItemBindings(unsigned int party_slot, int strength, unsigned int percent)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    unsigned int score = strength * 3;
    int revealed = 0;
    int still_hidden = 0;
    int slot;

    AdjustIntegerByPercent(&score, percent);
    for (slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->EquippedItem[slot];

        if (item->iItemNo != -1 && g_item_records[item->iItemNo].binds_on_equip != 0) {
            if (score < g_item_records[item->iItemNo].identify_difficulty) {
                ++still_hidden;
            } else {
                ++revealed;
                item->bind_announced = true;
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
        if ((record->attack_flags & (1 << index)) != 0) {
            return true;
        }
    }
    if (record->armor_class_bonus != 0) {
        return true;
    }
    for (index = 0; index < 6; ++index) {
        if (record->resistance_bonus[index] != 0) {
            return true;
        }
    }
    if (record->binds_on_equip != 0 || record->health_regen_bonus != 0 ||
        record->stamina_regen_bonus != 0 || record->spell_regen_bonus != 0) {
        return true;
    }
    return false;
}

/* The skill an identify attempt practises, and the one whose level supplies
   its strength - a sixth of it. */

/* The most of one item a character can hold at once: the record's own quantity
   dice taken at their maximum. */
static int MaximumQuantity(int item_id)
{
    return g_item_records[item_id].initial_quantity.Maximum();
}

/* Add uses to one item, never past what it can hold. */
// FUNCTION: WIZ8 0x0051e920
void AddItemUses(W8ItemInstance* item, char uses)
{
    unsigned char total = item->uses_or_charges + uses;
    int maximum = MaximumQuantity(item->iItemNo);

    item->uses_or_charges = total;
    if (static_cast<int>(static_cast<unsigned int>(total)) < maximum) {
        item->uses_or_charges = total;
        return;
    }
    item->uses_or_charges = static_cast<unsigned char>(maximum);
}

/* Pour one item's uses into another and take that many off the source, one at
   a time - which is what makes the source disappear when it is emptied. Each
   side counts its quantity the way its own record says to. */
// FUNCTION: WIZ8 0x0051e9f0
void MergeItemUses(W8Character* character, W8ItemInstance* into, W8ItemInstance* from)
{
    unsigned char available;
    unsigned char held;
    unsigned int moved;

    available = g_item_records[from->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK
                    ? from->stack_count
                    : from->uses_or_charges;
    held = g_item_records[into->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK
               ? into->stack_count
               : into->uses_or_charges;

    moved = MaximumQuantity(into->iItemNo) - held;
    if (available <= moved) {
        moved = available;
    }
    into->uses_or_charges += static_cast<char>(moved);
    for (; moved != 0; --moved) {
        RemoveCharacterItem(character, from, false);
    }
}

/* Fill one equipment slot with the item that pairs with the given one. Only
   the five generic name kinds the merge path singles out pair at all. The
   alternate left hand is the pair to take when it holds something compatible;
   otherwise the caller may name an item that has to be found on the character.
   Neither being available queues the character's event instead of swapping. */
// FUNCTION: WIZ8 0x0051eb90
void EquipMatchingPartnerItem(W8Character* character, W8ItemInstance* item, int item_id,
                              W8EquipSlot equip_slot)
{
    W8ItemInstance* pair;

    if (!ItemHasSingledOutGenericName(item->iItemNo)) {
        return;
    }
    if (item->iItemNo == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 0xa52, 0);
    }

    if (character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo != -1 &&
        CompatiblePartnerItems(
            item->iItemNo,
            character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo)) {
        pair = &character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON];
    } else if (item_id == -1 || !FindItemOnCharacter(character, item_id, &pair, 1, 0)) {
        QueueCharacterEvent(character, g_effect26, 0, g_character_event_no_flags,
                            g_character_event_full_volume);
        return;
    }

    SwapItemInstances(pair, &character->EquippedItem[equip_slot], character, true);
    PostCharacterNotice(CharacterPointerToPartySlot(character), gppStringList[0x1d4]);
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
            if (&character->EquippedItem[slot] == resume_after) {
                ++slot;
                break;
            }
        }
    }
    for (; slot < 12; ++slot) {
        if (character->EquippedItem[slot].iItemNo == item_id) {
            if (found != 0) {
                *found = &character->EquippedItem[slot];
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
        if (character->backpack[slot].iItemNo == item_id) {
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
    if ((resume_after == 0 || resume_after == &g_status.item_in_hand) && g_status.item_in_cursor &&
        g_status.item_in_hand.iItemNo == item_id) {
        if (found != 0) {
            *found = &g_status.item_in_hand;
        }
        return true;
    }

    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied &&
            FindItemOnCharacter(&g_status.buffers.Char[party_slot], item_id, found,
                                include_backpack, resume_after)) {
            if (found_character != 0) {
                *found_character = &g_status.buffers.Char[party_slot];
            }
            return true;
        }
    }

    if (include_backpack == 2) {
        unsigned int index = 0;
        if (resume_after != 0) {
            while (index < g_status.party_item_count &&
                   &g_status.party_item_pool[index] != resume_after) {
                ++index;
            }
            ++index;
        }
        for (; index < g_status.party_item_count; ++index) {
            W8ItemInstance* item = &g_status.party_item_pool[index];
            if (item->iItemNo == item_id) {
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
        if (character->EquippedItem[slot].iItemNo == item_id) {
            total += character->EquippedItem[slot].stack_count == 0
                         ? 1
                         : character->EquippedItem[slot].stack_count;
            if (first != 0 && *first == 0) {
                *first = &character->EquippedItem[slot];
            }
        }
    }
    if (include_backpack != 0) {
        for (slot = 0; slot < 8; ++slot) {
            if (character->backpack[slot].iItemNo == item_id) {
                /* Retail bug: a matching backpack slot adds the stack count of
                   the equipped slot with the same index (0x5211A0 reads
                   [item - 0xc8]), not its own. */
                total += character->EquippedItem[slot].stack_count == 0
                             ? 1
                             : character->EquippedItem[slot].stack_count;
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
    if (g_status.item_in_cursor && g_status.item_in_hand.iItemNo == item_id) {
        total = g_status.item_in_hand.stack_count;
        if (found != 0 && *found == 0) {
            *found = &g_status.item_in_hand;
        }
    }

    for (unsigned int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied) {
            int count = CountItemOnCharacter(&g_status.buffers.Char[party_slot], item_id, found,
                                             include_backpack);
            if (count != 0) {
                total += count;
                if (first_holder != 0 && *first_holder == 0) {
                    *first_holder = &g_status.buffers.Char[party_slot];
                }
            }
        }
    }

    if (include_backpack == 2) {
        for (unsigned int index = 0; index < g_status.party_item_count; ++index) {
            W8ItemInstance* item = &g_status.party_item_pool[index];
            if (item->iItemNo == item_id) {
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
    unsigned int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied) {
            if (!FindItemOnCharacter(&g_status.buffers.Char[party_slot], item_id, 0,
                                     include_backpack, 0)) {
                return false;
            }
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005213C0
bool FindCharacterItemByDatabaseKind(W8Character* character, short item_kind, W8ItemInstance** out,
                                     int include_backpack)
{
    for (int slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->EquippedItem[slot];
        if (item->iItemNo != -1 &&
            g_item_records[item->iItemNo].unidentified_name_index == item_kind) {
            if (out != 0) {
                *out = item;
            }
            return true;
        }
    }
    if (include_backpack != 0) {
        for (int slot = 0; slot < 8; ++slot) {
            W8ItemInstance* item = &character->backpack[slot];
            if (item->iItemNo != -1 &&
                g_item_records[item->iItemNo].unidentified_name_index == item_kind) {
                if (out != 0) {
                    *out = item;
                }
                return true;
            }
        }
    }
    if (out != 0) {
        *out = 0;
    }
    return false;
}

/* At what range an item's spell works. An item with no spell has no range at
   all, which is a different answer from touch. */
// FUNCTION: WIZ8 0x005207e0
W8RangeCategory GetItemSpellRange(const W8ItemInstance* item)
{
    if (item == 0) {
        return W8_RANGE_NONE;
    }
    if (item->iItemNo == -1) {
        srAssertFail("pPCItem->iItemNo != -1", PC_ITEM_CPP, 4003, 0);
    }
    if (item->iItemNo >= static_cast<int>(gXStatus.uiItemsInDatabase)) {
        srAssertFail("pPCItem->iItemNo < (INT32) gXStatus.uiItemsInDatabase", PC_ITEM_CPP, 4004, 0);
    }
    if (g_item_records[item->iItemNo].spell_id != W8_SPELL_NONE) {
        return g_spell_records[g_item_records[item->iItemNo].spell_id].range_category;
    }
    return W8_RANGE_NONE;
}

/* One character's attempt at identifying an item. Their strength is a sixth of
   the identify skill's level; clearing the difficulty reveals the item, three
   further points also reveal its binding, and an attempt that only just came
   off practises the skill. */
// FUNCTION: WIZ8 0x005208f0
bool TryIdentifyItemFor(W8Character* character, W8ItemInstance* item)
{
    char strength;
    int margin;

    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 4014, 0);
    }
    if (character->hp_current == 0 || character->highest_condition >= W8_CONDITION_INSANE) {
        return false;
    }

    strength = static_cast<char>(character->skills[W8_SKILL_ARTIFACTS].level / 6);
    if (static_cast<char>(g_item_records[item->iItemNo].identify_difficulty) > strength) {
        return false;
    }

    if (item == 0) {
        srAssertFail("pPCItem", PC_ITEM_CPP, 2875, 0);
    }
    item->identified = true;
    item->spell_hint = true;
    item->unknown_08 = 1;
    if (static_cast<char>(g_item_records[item->iItemNo].identify_difficulty + 3) <= strength) {
        item->bound = true;
    }

    margin = strength - g_item_records[item->iItemNo].identify_difficulty;
    if (margin >= 0 && margin < 3) {
        PracticeCharacterSkill(character, W8_SKILL_ARTIFACTS, 2, true);
    }
    return true;
}

/* Bind everything the party is wearing, one character at a time - but not
   during a fight the party has not yet been let out of, which says so
   instead. */
// FUNCTION: WIZ8 0x0051d230
void BindEveryPartyItem(void)
{
    unsigned int party_slot;

    if (gXStatus.fCombatMode && !g_combat_state->round_active && !gXStatus.fPartyMovementMode) {
        ShowNotice(W8_FONT_PALETTE_BEIGE, gppStringList[W8_NOTICE_WEAPON_SWAP_BLOCKED_COMBAT]);
        return;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (!g_status.buffers.XChar[party_slot].item_action_pending) {
            BindCharacterItems(party_slot, false);
        }
    }
    ShowNotice(W8_FONT_PALETTE_WHITE, gppStringList[0x1ed]);
}

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

    if (a->iItemNo == -1) {
        srAssertFail("pPCItem1->iItemNo != BAD_INDEX", PC_ITEM_CPP, 3812, 0);
    }
    if (b->iItemNo == -1) {
        srAssertFail("pPCItem2->iItemNo != BAD_INDEX", PC_ITEM_CPP, 3813, 0);
    }
    ra = &g_item_records[a->iItemNo];
    rb = &g_item_records[b->iItemNo];

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
    if (!a->identified) {
        if (!b->identified) {
            return 0;
        }
        return 1;
    }
    if (!b->identified) {
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
    if (!g_status.game_started) {
        srAssertFail("gStatus.fGameStarted", PC_ITEM_CPP, 3795, 0);
    }
    if (g_status.party_item_count > 1) {
        qsort(g_status.party_item_pool, g_status.party_item_count, sizeof(W8ItemInstance),
              CompareItemsForPool);
    }
}

/* Normalize a stack and split every full overflow stack into the party pool.
   Quantity kinds two through four live in uses_or_charges instead. */
// FUNCTION: WIZ8 0x0051fb40
void NormalizeItemStack(W8ItemInstance* item)
{
    if (item->iItemNo == -1) {
        return;
    }

    const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
    if (record->quantity_kind == W8_ITEM_QUANTITY_STACK) {
        if (item->uses_or_charges != 0) {
            if (item->stack_count == 0) {
                item->stack_count = item->uses_or_charges;
            }
            item->uses_or_charges = 0;
        }
    } else if (record->quantity_kind >= W8_ITEM_QUANTITY_CHARGES &&
               record->quantity_kind <= W8_ITEM_QUANTITY_SHOTS && item->stack_count != 0) {
        if (item->uses_or_charges == 0) {
            item->uses_or_charges = item->stack_count;
        }
        item->stack_count = 0;
    }

    if (record->quantity_kind != W8_ITEM_QUANTITY_STACK) {
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

        if (g_status.party_item_count >= 500) {
            item->stack_count = record->maximum_quantity;
            return;
        }
        W8ItemInstance* destination = &g_status.party_item_pool[g_status.party_item_count];
        memset(destination, 0, sizeof(*destination));
        destination->iItemNo = -1;
        CopyItemInstance(destination, &split, 0, true);
        ++g_status.party_item_count;
        RedistributePartyEncumbrance();
        item->stack_count -= quantity;
        record = &g_item_records[item->iItemNo];
    }
}

/* Merge as much of one stack as fits in another.  A completely consumed
   source is removed from its owner; a partial merge is reported separately so
   the caller can refresh carrying capacity before placing the remainder. */
// FUNCTION: WIZ8 0x0051f900
bool MergeItemStacks(W8ItemInstance* destination, W8ItemInstance* source, bool* partially_merged)
{
    if (destination->iItemNo == -1) {
        return false;
    }

    NormalizeItemStack(destination);
    const W8ItemDatabaseRecord* record = &g_item_records[destination->iItemNo];
    if (record->quantity_kind != W8_ITEM_QUANTITY_STACK) {
        return false;
    }
    if (destination->stack_count > record->maximum_quantity) {
        srAssertFail(
            "FALSE", PC_ITEM_CPP, 3371,
            FormatString("StackItemsIfPossible: ERROR - slot holds %d when max is %d, item %d(%ls)",
                         destination->stack_count, record->maximum_quantity, destination->iItemNo,
                         FormatItemDisplayName(destination, true)));
        destination->stack_count = record->maximum_quantity;
    }
    if (destination->iItemNo != source->iItemNo || destination->identified != source->identified) {
        return false;
    }

    unsigned char room = record->maximum_quantity - destination->stack_count;
    if (room == 0) {
        return false;
    }
    unsigned char moved = source->stack_count < room ? source->stack_count : room;
    destination->stack_count += moved;
    source->stack_count -= moved;
    if (source->stack_count == 0) {
        EmptyItemRecord(source, 0, true);
        return true;
    }
    if (partially_merged != 0) {
        *partially_merged = true;
    }
    return false;
}

/* Move a complete live item into an empty destination and clear its old slot.
   Moving into the held-item slot also remembers the character/slot origin so
   the cursor can return it later. */
// FUNCTION: WIZ8 0x0051fe30
void CopyItemInstance(W8ItemInstance* destination, W8ItemInstance* source, W8Character* character,
                      bool refresh)
{
    unsigned int held_character = static_cast<unsigned int>(-1);
    unsigned char held_origin = W8_ITEM_ORIGIN_NONE;
    unsigned short held_slot = 0xffff;

    if (destination == source) {
        srAssertFail("pToPCItem != pFromPCItem", PC_ITEM_CPP, 3558, 0);
    }
    if (destination->iItemNo != -1) {
        srAssertFail("pToPCItem->iItemNo == -1", PC_ITEM_CPP, 3560, 0);
    }
    if (source->iItemNo == -1) {
        srAssertFail("pFromPCItem->iItemNo != -1", PC_ITEM_CPP, 3561, 0);
    }
    if (source->bind_announced) {
        source->bind_announced = false;
    }
    memmove(destination, source, sizeof(*destination));

    if (destination == &g_status.item_in_hand && character != 0) {
        held_character = CharacterPointerToPartySlot(character);
        GetOriginOfCharacterItem(held_character, source, &held_origin, &held_slot);
    }

    EmptyItemRecord(source, character, refresh);

    RefreshAfterItemRecordChange(destination, character, refresh);
    if (destination == &g_status.item_in_hand) {
        gXStatus.held_item_source = held_character;
        gXStatus.held_item_origin = held_origin;
        gXStatus.held_item_slot = held_slot;
        SetItemCursor(0);
    }
}

/* Move an item into an empty peer slot, or exchange two occupied slots through
   a temporary empty record. The party-row guard prevents observers from
   reacting to the intermediate empty states. */
// FUNCTION: WIZ8 0x0051FD20
void SwapItemInstances(W8ItemInstance* item, W8ItemInstance* destination, W8Character* character,
                       bool refresh)
{
    W8ItemInstance temporary;
    unsigned int party_slot;

    if (item->iItemNo == -1 && destination->iItemNo == -1) {
        srAssertFail("(pPCItem1->iItemNo != -1) || (pPCItem2->iItemNo != -1)", PC_ITEM_CPP, 0xdbc,
                     0);
    }
    party_slot = CharacterPointerToPartySlot(character);
    gXStatus.monster_manager_entries[party_slot].item_swap_in_progress = true;
    if (item->iItemNo == -1) {
        CopyItemInstance(item, destination, character, refresh);
    } else if (destination->iItemNo == -1) {
        CopyItemInstance(destination, item, character, refresh);
    } else {
        memset(&temporary, 0, sizeof(temporary));
        temporary.iItemNo = -1;
        RefreshAfterItemRecordChange(&temporary, 0, refresh);
        CopyItemInstance(&temporary, item, character, refresh);
        CopyItemInstance(item, destination, character, refresh);
        CopyItemInstance(destination, &temporary, character, refresh);
    }
    gXStatus.monster_manager_entries[party_slot].item_swap_in_progress = false;
}

/* Set the wield kind for one primary hand from the item it holds. An empty
   hand wields nothing; otherwise the item's equipment class selects the kind,
   with class two in the off hand deferring to the primary hand's item unless
   that item carries the paired flag. */
// FUNCTION: WIZ8 0x005201B0
void SetHandType(W8Character* character, W8EquipSlot slot)
{
    if (character == 0) {
        srAssertFail("pPC != NULL", PC_ITEM_CPP, 3676, 0);
    }
    if (static_cast<unsigned int>(slot) > W8_EQUIP_SLOT_FEET) {
        srAssertFail("uiSlot < SLOT_COUNT", PC_ITEM_CPP, 3677, 0);
    }
    int hand;
    if (slot == W8_EQUIP_SLOT_PRIMARY_WEAPON) {
        hand = 0;
    } else {
        if (slot != W8_EQUIP_SLOT_SECONDARY_WEAPON) {
            char* message =
                FormatString("SetHandType: ERROR: uiSlot %d is not a hand/weapon slot!", slot);
            srAssertFail("FALSE", PC_ITEM_CPP, 3690, message);
            return;
        }
        hand = 1;
    }
    int item_id = character->EquippedItem[slot].iItemNo;
    int wield_kind;
    if (item_id == -1) {
        wield_kind = 0;
    } else {
        switch (g_item_records[item_id].equip_class) {
        case W8_ITEM_EQUIP_CLASS_SHORT_WEAPON:
        case W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON:
        case W8_ITEM_EQUIP_CLASS_RANGED_WEAPON:
            wield_kind = 1;
            break;
        case W8_ITEM_EQUIP_CLASS_THROWN_WEAPON:
            wield_kind = 1;
            if (slot != W8_EQUIP_SLOT_SECONDARY_WEAPON ||
                character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1 ||
                g_item_records[character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo]
                        .unidentified_name_index != 0x83) {
                break;
            }
            /* fall through */
        case W8_ITEM_EQUIP_CLASS_AMMUNITION:
            wield_kind = 3;
            break;
        case W8_ITEM_EQUIP_CLASS_SHIELD:
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
    character->Hand[hand].uiHolds = wield_kind;
}

/* The binding difficulty of a character's worn items: the worst identify
   difficulty among binds-on-equip pieces whose binding has not been announced
   yet, in thirds rounded up. Cure spells consult it when the target's worn
   items ask for more power than the condition does. */
// FUNCTION: WIZ8 0x00520C70
unsigned int GetEquipmentBindingDifficulty(int character_index)
{
    unsigned char max_difficulty = 0;
    W8ItemInstance* slot = g_status.buffers.Char[character_index].EquippedItem;

    for (int remaining = 12; remaining != 0; --remaining) {
        int item_id = slot->iItemNo;
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 && !slot->bind_announced &&
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
void RefreshAfterItemRecordChange(W8ItemInstance* item, W8Character* character, bool refresh)
{
    if (!character) {
        return;
    }

    bool primary_right = item == &character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON];
    if (primary_right) {
        SetHandType(character, W8_EQUIP_SLOT_PRIMARY_WEAPON);
    }
    bool primary_left = item == &character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON];
    if (primary_left) {
        SetHandType(character, W8_EQUIP_SLOT_SECONDARY_WEAPON);
    }

    if (!character->fInParty || !g_status.game_started) {
        return;
    }

    unsigned int party_slot = CharacterPointerToPartySlot(character);
    if (primary_right || primary_left) {
        CalcAttacks(character);
    }
    if (!refresh) {
        return;
    }

    if (item == &character->EquippedItem[W8_EQUIP_SLOT_FEET]) {
        g_party_has_slope_override_item = FindItemOnParty(0x254, 0, 0, 0, 0);
    }
    RebuildEquipmentAndDerivedStatsForSlot(party_slot);

    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    if (row->action == W8_ACTION_USE_ITEM && row->action_detail1.item_use.item == item) {
        DropCharacterFromRound(party_slot);
    }

    if (primary_right || primary_left) {
        W8ActionKind action_kind = row->action_kind;
        if (action_kind == W8_ACTION_BERSERK) {
            if (!CanCharacterBerserk(party_slot)) {
                row->action_kind = W8_ACTION_ATTACK;
                if (row->action == W8_ACTION_BERSERK) {
                    row->action = W8_ACTION_ATTACK;
                }
            }
        } else if (action_kind == W8_ACTION_ATTACK && row->action_is_berserk &&
                   CanCharacterBerserk(party_slot)) {
            row->action_kind = W8_ACTION_BERSERK;
            if (row->action == W8_ACTION_ATTACK) {
                row->action = W8_ACTION_BERSERK;
            }
        }

        if (gXStatus.fCombatMode) {
            W8ActionKind action = row->action;
            if (action == W8_ACTION_ATTACK || action == W8_ACTION_BERSERK) {
                if (!CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, true,
                                          false)) {
                    AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_IN_COMBAT);
                } else if (!TargetIsInPlay(party_slot, 2, W8_TARGETING_CONTEXT_IN_COMBAT)) {
                    RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0);
                }
            }
            g_combat_state->characters[party_slot].alternate_hand ^= 1;
            if (party_slot == static_cast<unsigned int>(g_status.selected_character)) {
                RequestRefreshPartyState();
            }
        }
        RequestRedraw(1 << (party_slot & 0x1f));
    }

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block) {
        if (g_level_block->combat_end_notification != -1) {
            ReopenSubMenuPanel();
        }
        RefreshFlaggedMainGameState();
    }
}

/* Empty one live item record.  A held item also resets the mouse cursor; a
   carried-pool item is removed from the packed 500-entry array and the tail is
   shifted down exactly once. */
// FUNCTION: WIZ8 0x00520070
void EmptyItemRecord(W8ItemInstance* item, W8Character* character, bool refresh)
{

    if (item == &g_status.item_in_hand) {
        gXStatus.held_item_source = -1;
        gXStatus.held_item_origin = W8_ITEM_ORIGIN_NONE;
        gXStatus.held_item_slot = 0xffff;
        ClearHeldItemDisplay();
    } else {
        memset(item, 0, sizeof(*item));
        item->iItemNo = -1;
        RefreshAfterItemRecordChange(item, character, refresh);
    }

    if (item >= g_status.party_item_pool && item <= &g_status.party_item_pool[499]) {
        for (unsigned int index = 0; index < g_status.party_item_count; ++index) {
            if (item == &g_status.party_item_pool[index]) {
                RemovePartyPoolEntry(index);
                break;
            }
        }
    }
}

/* Empty every equipped and backpack item record a character carries. */
// FUNCTION: WIZ8 0x00520310
void EmptyAllCarriedItems(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 12; ++index) {
        EmptyItemRecord(&character->EquippedItem[index], character, true);
    }
    for (index = 0; index < 8; ++index) {
        EmptyItemRecord(&character->backpack[index], character, true);
    }
}

/* Acquisition of a handful of plot items updates their paired facts. */
// FUNCTION: WIZ8 0x00522640
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item)
{
    switch (item->iItemNo) {
    case 0x243:
        if (!GetFact(W8_FACT_DESTINAE_POSSESS)) {
            SetFact(W8_FACT_DESTINAE_POSSESS, 1, false);
        }
        if (GetFact(W8_FACT_QUEST_GET_DD)) {
            SetFact(W8_FACT_QUEST_GET_DD, 0, false);
        }
        break;
    case 0x242:
        if (!GetFact(W8_FACT_ASTRAL_POSSESS)) {
            SetFact(W8_FACT_ASTRAL_POSSESS, 1, false);
        }
        if (GetFact(W8_FACT_QUEST_GET_AD)) {
            SetFact(W8_FACT_QUEST_GET_AD, 0, false);
        }
        break;
    case 0x244:
        if (!GetFact(W8_FACT_CHAOS_POSSESS)) {
            SetFact(W8_FACT_CHAOS_POSSESS, 1, false);
        }
        if (GetFact(W8_FACT_QUEST_GET_CM)) {
            SetFact(W8_FACT_QUEST_GET_CM, 0, false);
        }
        g_status.fact_b8_pending = true;
        g_status.fact_b8_clock = g_status.world_clock;
        break;
    case 0x264:
        if (!GetFact(W8_FACT_ASTRAL_FAKE_POSSESS)) {
            SetFact(W8_FACT_ASTRAL_FAKE_POSSESS, 1, false);
            SetFact(W8_FACT_ASTRAL_FAKE_POSSESS1, 1, false);
        }
        break;
    case 0x201:
        SetFact(W8_FACT_QUEST_BRAFFIT_CIERDAN, 0, false);
        break;
    case 0x27c:
        SetFact(W8_FACT_QUEST_MYLES_FIND_DIAMOND, 0, false);
        break;
    case 0x1d2:
        if (GetFact(W8_FACT_QUEST_MYLES_WEAPONS_CACHE)) {
            SetFact(W8_FACT_QUEST_MYLES_WEAPONS_CACHE, 0, false);
        }
        break;
    }
}

/* Deliver the one-time character reaction associated with exceptional items.
   Character creation (screen 5) deliberately suppresses every reaction. */
// FUNCTION: WIZ8 0x005227d0
void DeliverExceptionalItemReaction(W8ItemInstance* item, bool choose_character,
                                    W8Character* character)
{
    int message;

    if (g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION) {
        return;
    }
    if (choose_character) {
        int party_slot = GetRandomCharacter(0, 0, -1, -1);
        if (party_slot == -1) {
            return;
        }
        character = &g_status.buffers.Char[party_slot];
    }
    if ((item->effect_used & 1) != 0) {
        return;
    }

    switch (item->iItemNo) {
    case 0x239:
        message = g_item_message10;
        break;
    case 0x242:
        message = g_item_message2;
        break;
    case 0x243:
        message = g_item_message3;
        break;
    case 0x244:
        message =
            GetFactionDispositionScore(W8_FACTION_MOOK) != 0 ? g_item_message4 : g_item_message5;
        break;
    case 0x264:
        message = g_item_message9;
        break;
    case 0x27c: {
        SetFact(W8_FACT_DIAMOND_FOUND, 1, false);
        if (!NpcLeadHasNameStyle(W8_NPC_MYLES) || !GetFact(W8_FACT_QUEST_MYLES_FIND_DIAMOND)) {
            return;
        }
        W8NpcState* npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        signed char npc_slot = npc->group_index;
        QueueCharacterEvent(&g_status.buffers.Char[npc_slot], g_item_message8, 0,
                            g_character_event_no_flags, g_character_event_full_volume);
        return;
    }
    default:
        if (choose_character || !CanCharacterUseItem(character, item->iItemNo) ||
            g_item_records[item->iItemNo].value / character->uiExpLevel < 500 ||
            !item->identified) {
            return;
        }
        message = g_item_message6;
        break;
    }

    QueueCharacterEvent(character, message, 0, g_character_event_no_flags,
                        g_character_event_full_volume);
    item->effect_used |= 1;
}

/* Whether the item's equip class is directly usable (0x17/0x19). */
// FUNCTION: WIZ8 0x00522A00
bool IsUsableItemClass(W8ItemInstance* item)
{
    if (g_item_records[item->iItemNo].equip_class != W8_ITEM_EQUIP_CLASS_KEY &&
        g_item_records[item->iItemNo].equip_class != W8_ITEM_EQUIP_CLASS_OTHER) {
        return false;
    }
    return true;
}

/* Whether the party slot may go through with using this item right now.
   Class-0x17 and -0x19 items and anything the character can cast from answer
   with the out-of-combat gate alone; everything else is refused while the
   character's spellcasting is blocked and the item is a class-0x0d casting
   aid, and otherwise needs a living target for the record's spell plus a
   spell the moment admits. The use-item view's context slot is parked on the
   instance while the target check runs. */
// FUNCTION: WIZ8 0x00522a30
bool CanUseItemForAction(int party_slot, const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
    W8Character* character = &g_status.buffers.Char[party_slot];
    bool usable;

    if (record->equip_class == W8_ITEM_EQUIP_CLASS_KEY ||
        record->equip_class == W8_ITEM_EQUIP_CLASS_OTHER || CanCastFromItem(character, item)) {
        return !gXStatus.fCombatMode && !gXStatus.fCampMode && !gXStatus.fLockInteract &&
               !gXStatus.fTrapInteract;
    }
    if (character->uiCondition[W8_CONDITION_SILENCED] != 0 &&
        record->equip_class == W8_ITEM_EQUIP_CLASS_INSTRUMENT) {
        return false;
    }

    SetUseItemSelectOverrideItem(const_cast<W8ItemInstance*>(item));
    usable =
        SpellHasAnyValidTarget(party_slot, record->spell_id, ItemClassNormalizesTarget(record));
    SetUseItemSelectOverrideItem(0);
    if (!usable) {
        return false;
    }
    return SpellUsableNow(record->spell_id, false);
}

/* Validate an item's embedded spell for use now; nonzero reports use blocked
   with the reason notice queued through the callback. */
// FUNCTION: WIZ8 0x00522B80
bool ValidateItemSpellUse(int character_index, W8ItemInstance* item,
                          W8DialogDestroyCallback callback)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
    W8Character* character = &g_status.buffers.Char[character_index];
    bool has_target;

    if (record->equip_class == W8_ITEM_EQUIP_CLASS_KEY ||
        record->equip_class == W8_ITEM_EQUIP_CLASS_OTHER || CanCastFromItem(character, item)) {
        if (gXStatus.fCombatMode || gXStatus.fCampMode || gXStatus.fLockInteract ||
            gXStatus.fTrapInteract) {
            ShowNoticeLine(gppStringList[0x7a6], callback, true, false);
            return true;
        }
        return false;
    }
    if (character->uiCondition[W8_CONDITION_SILENCED] != 0 &&
        record->equip_class == W8_ITEM_EQUIP_CLASS_INSTRUMENT) {
        ShowNoticeLine(gppStringList[0x7a7], callback, true, false);
        return true;
    }
    if (gXStatus.fItemSelectMode) {
        SetUseItemSelectOverrideItem(item);
        has_target = SpellHasAnyValidTarget(character_index, record->spell_id,
                                            ItemClassNormalizesTarget(record));
        SetUseItemSelectOverrideItem(0);
        if (!has_target) {
            ShowNoticeLine(gppStringList[0x7a8], callback, true, false);
            return true;
        }
    }
    if (!SpellUsableNow(record->spell_id, false)) {
        ShowNoticeLine(gppStringList[0x7a6], callback, true, false);
        return true;
    }
    return false;
}

/* An identified casting item the character can activate whose record holds
   a service spell (0x03/0x29). Every carried location applies this test. */
static bool IsActivatableServiceItem(W8Character* character, W8ItemInstance* item)
{
    if (item->iItemNo != -1 && CanCharacterActivateItem(character, item)) {
        const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
        if (record->equip_class != W8_ITEM_EQUIP_CLASS_KEY &&
            record->equip_class != W8_ITEM_EQUIP_CLASS_OTHER && item->identified &&
            (record->spell_id == W8_SPELL_CHARM || record->spell_id == W8_SPELL_MINDREAD)) {
            return true;
        }
    }
    return false;
}

/* Whether the character carries an activatable service item across the
   backpack, equipment and the party pool. */
// FUNCTION: WIZ8 0x00522D40
bool CharacterHasServiceItem(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 8; ++index) {
        if (IsActivatableServiceItem(character, &character->backpack[index])) {
            return true;
        }
    }
    for (index = 0; index < 0xc; ++index) {
        if (IsActivatableServiceItem(character, &character->EquippedItem[index])) {
            return true;
        }
    }
    for (index = 0; index < g_status.party_item_count; ++index) {
        if (IsActivatableServiceItem(character, &g_status.party_item_pool[index])) {
            return true;
        }
    }
    return false;
}

/* Insert one item into the packed party pool, first coalescing compatible
   stacks unless requested otherwise. */
// FUNCTION: WIZ8 0x00521E20
bool InsertItemIntoPartyPool(W8ItemInstance* item, int index)
{
    W8ItemInstance shifted[500];

    if (g_status.party_item_count >= 500) {
        return false;
    }
    if (g_status.party_item_count != static_cast<unsigned int>(index)) {
        memcpy(&shifted[index], &g_status.party_item_pool[index],
               (g_status.party_item_count - index) * sizeof(W8ItemInstance));
        memcpy(&g_status.party_item_pool[index + 1], &shifted[index],
               (g_status.party_item_count - index) * sizeof(W8ItemInstance));
    }
    memset(&g_status.party_item_pool[index], 0, sizeof(W8ItemInstance));
    g_status.party_item_pool[index].iItemNo = -1;
    CopyItemInstance(&g_status.party_item_pool[index], item, 0, true);
    ++g_status.party_item_count;
    RedistributePartyEncumbrance();
    return true;
}

// FUNCTION: WIZ8 0x00521ef0
bool AddItemToParty(W8ItemInstance* item, bool announce, bool skip_stacking)
{
    wchar_t* display_name = FormatItemDisplayName(item, true);
    bool partially_merged = false;
    unsigned int index = 0;
    bool stored = false;

    if (g_item_records[item->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK && !skip_stacking) {
        while (index < g_status.party_item_count) {
            if (MergeItemStacks(&g_status.party_item_pool[index], item, &partially_merged)) {
                stored = true;
                break;
            }
            ++index;
        }
        if (partially_merged) {
            RedistributePartyEncumbrance();
        }
    }

    if (!stored) {
        if (g_status.party_item_count >= 500) {
            return false;
        }
        index = 0;
        InsertItemIntoPartyPool(item, 0);
        stored = true;
    }

    if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen != 0) {
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
    }
    if (announce) {
        ShowNoticef(W8_FONT_PALETTE_WHITE, gppStringList[0x1e9], display_name);
    }
    W8ItemInstance* stored_item = &g_status.party_item_pool[index];
    UpdateFactsAfterAcquiringItem(stored_item);
    DeliverExceptionalItemReaction(stored_item, true, 0);
    return stored;
}

/* An equipped item the character can no longer use is announced and moved
   into the backpack, or into the party pool when the backpack is full. The
   item record's two requirement pairs choose the attribute or skill name the
   message carries; an item whose requirements are still met gets the generic
   notice instead. The announcement runs before the move, and the party row's
   item is not refreshed by the temporary instance the move builds. */
// FUNCTION: WIZ8 0x0051d960
void UnequipUnusableItems(W8Character* character)
{
    if (!IsPartyCharacterPointer(character)) {
        return;
    }

    for (int slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->EquippedItem[slot];
        if (item->iItemNo == -1) {
            continue;
        }
        if (CanCharacterUseItem(character, item->iItemNo)) {
            continue;
        }

        unsigned int party_slot = CharacterPointerToPartySlot(character);
        const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
        unsigned short message_id = 0;
        bool unmet = false;
        W8ItemInstance destination;
        unsigned int index;

        for (index = 0; index < 2; ++index) {
            if (record->attribute_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
                character
                        ->attributes[static_cast<signed char>(
                            record->attribute_requirements[index].stat_id)]
                        .effective < record->attribute_requirements[index].minimum) {
                message_id = g_character_description_first_ids[static_cast<signed char>(
                    record->attribute_requirements[index].stat_id)];
                unmet = true;
                break;
            }
        }
        for (index = 0; !unmet && index < 2; ++index) {
            if (record->skill_requirements[index].stat_id != W8_ITEM_REQUIREMENT_NONE &&
                character
                        ->skills[static_cast<signed char>(
                            record->skill_requirements[index].stat_id)]
                        .level < record->skill_requirements[index].minimum) {
                message_id = g_character_skill_name_ids[static_cast<signed char>(
                    record->skill_requirements[index].stat_id)];
                unmet = true;
                break;
            }
        }
        if (unmet) {
            PostCharacterNotice(party_slot, gppStringList[0x1ef],
                                gppStringList[g_gender_name_message_rows[character->gender][2]],
                                FormatItemDisplayName(item, true),
                                gppStringList[g_gender_name_message_rows[character->gender][2]],
                                gppStringList[message_id]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x1f0],
                                gppStringList[g_gender_name_message_rows[character->gender][2]],
                                FormatItemDisplayName(item, true), item, 1);
        }

        destination.iItemNo = -1;
        destination.stack_count = 0;
        destination.uses_or_charges = 0;
        destination.identified = false;
        RefreshAfterItemRecordChange(&destination, 0, true);
        SwapItemInstances(item, &destination, character, true);
        if (!AddItemToCharacter(character, &destination, false, false, false)) {
            AddItemToParty(&destination, false, false);
        }
    }
}

/* Pick the equipment slot a newly acquired item goes into. Before the game
   has started the two hand pairs are special-cased: a dual-wield-capable
   item can displace the right hand's item into the left hand, class-three
   gear fills a free pair before anything else, and class-four gear looks for
   a two-handed holder and otherwise fails. */
// FUNCTION: WIZ8 0x0051c5a0
W8EquipSlot ChooseCharacterEquipSlot(W8Character* character, int item_id)
{
    W8EquipSlot slot = GetItemDefaultEquipSlot(item_id);
    if (g_status.game_started ||
        (slot != W8_EQUIP_SLOT_PRIMARY_WEAPON && slot != W8_EQUIP_SLOT_SECONDARY_WEAPON)) {
        return slot;
    }

    int primary_right = character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo;
    int alternate_right = character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo;
    int primary_left = character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo;
    int alternate_left = character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo;
    switch (g_item_records[item_id].equip_class) {
    case W8_ITEM_EQUIP_CLASS_SHORT_WEAPON:
    case W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON:
        if (character->skills[W8_SKILL_DUAL_WEAPONS].points != 0) {
            if (CanEquipItemInSlot(character, item_id, 7, false)) {
                if (primary_right != -1 && primary_left == -1 &&
                    (g_item_records[primary_right].equip_class ==
                         W8_ITEM_EQUIP_CLASS_SHORT_WEAPON ||
                     g_item_records[primary_right].equip_class ==
                         W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON) &&
                    (g_item_records[primary_right].flags & W8_ITEM_FLAG_TWO_HANDED) == 0) {
                    slot = W8_EQUIP_SLOT_SECONDARY_WEAPON;
                }
                if (alternate_right != -1 && alternate_left == -1 &&
                    (g_item_records[alternate_right].equip_class ==
                         W8_ITEM_EQUIP_CLASS_SHORT_WEAPON ||
                     g_item_records[alternate_right].equip_class ==
                         W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON) &&
                    (g_item_records[alternate_right].flags & W8_ITEM_FLAG_TWO_HANDED) == 0) {
                    return W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON;
                }
            } else {
                if (primary_right != -1 && primary_left == -1 &&
                    CanEquipItemInSlot(character, primary_right, 7, false)) {
                    CopyItemInstance(&character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON],
                                     &character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON], 0,
                                     true);
                    return W8_EQUIP_SLOT_PRIMARY_WEAPON;
                }
                if (alternate_right != -1 && alternate_left == -1 &&
                    CanEquipItemInSlot(character, alternate_right, 9, false)) {
                    CopyItemInstance(
                        &character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON],
                        &character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON], 0, true);
                    return W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON;
                }
            }
        }
        break;
    case W8_ITEM_EQUIP_CLASS_RANGED_WEAPON:
        if (primary_right == -1 && primary_left == -1) {
            return W8_EQUIP_SLOT_PRIMARY_WEAPON;
        }
        if (alternate_right == -1 && alternate_left == -1) {
            return W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON;
        }
        break;
    case W8_ITEM_EQUIP_CLASS_AMMUNITION:
        if (primary_right != -1 &&
            g_item_records[primary_right].equip_class == W8_ITEM_EQUIP_CLASS_RANGED_WEAPON) {
            return W8_EQUIP_SLOT_SECONDARY_WEAPON;
        }
        if (alternate_right != -1 &&
            g_item_records[alternate_right].equip_class == W8_ITEM_EQUIP_CLASS_RANGED_WEAPON) {
            return W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON;
        }
        return W8_EQUIP_SLOT_NONE;
    }
    return slot;
}

/* Rate how hard one attempt at an item's spell is. The base is the ordinary
   spell failure chance for the character's level in the skill that presents
   the spell; the two casting-aid skills then add a shortfall term - fifteen
   times the spell's own level against that skill level, in thirds - and a term
   for each power level the caster is short of the spell's minimum. Skill
   twenty is the most forgiving of the four and takes ten percent off, while
   skill nine takes twenty. */
// FUNCTION: WIZ8 0x0051dcd0
unsigned int GetItemUseDifficulty(const W8Character* character, W8Skill skill,
                                  unsigned int skill_level, unsigned int spell_id,
                                  unsigned int power)
{
    unsigned int failure = GetSpellFailureChance(skill_level, spell_id, power);
    unsigned int shortfall;
    int minimum_caster_level;
    int caster_level;
    int adjusted_power;

    switch (skill) {
    case W8_SKILL_THROWING_SLING:
        return failure * 8 / 10;
    case W8_SKILL_MUSIC:
    case W8_SKILL_ENGINEERING:
        shortfall = g_spell_records[spell_id].spell_level * 0xf;
        if (skill_level < shortfall) {
            failure += (shortfall - skill_level) / 3;
        }
        minimum_caster_level = GetMinimumCasterLevelForSpell(spell_id);
        caster_level = skill == W8_SKILL_MUSIC
                           ? character->profession_levels[W8_CASTER_PROFESSION_INDEX_9]
                           : character->profession_levels[W8_CASTER_PROFESSION_INDEX_8];
        adjusted_power = (minimum_caster_level - caster_level) - 1 + static_cast<int>(power);
        if (adjusted_power > 0) {
            return failure + g_spell_records[spell_id].spell_level * adjusted_power;
        }
        break;
    case W8_SKILL_ARTIFACTS:
        failure = failure * 9 / 10;
        break;
    default:
        break;
    }
    return failure;
}

/* Aim the item in an instance at whoever the slot is currently targeting, then
   start the use as that slot's action. The one spell that needs a caster
   picked before it can be used also opens the character screen. */
// FUNCTION: WIZ8 0x0051db60
void AimItemUseAtCurrentTarget(W8Character* character, W8ItemInstance* item)
{
    unsigned int party_slot = CharacterPointerToPartySlot(character);
    W8ActionDetailBlock detail;

    detail.item_use.item = item;
    detail.item_use.kind = -1;
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    StagePartySlotItemUse(party_slot, item, target);
    if (g_item_records[item->iItemNo].spell_id == W8_SPELL_IDENTIFY_ITEM) {
        ChooseAction(party_slot, W8_ACTION_USE_ITEM, -1, &detail, true, 1);
        OpenCharacterScreenForPartySlot(CharacterPointerToPartySlot(character), true);
        return;
    }
    ChooseAction(party_slot, W8_ACTION_USE_ITEM, -1, &detail, false, 1);
}

/* Stage the slot row's item-use detail block, aimed target and provenance for
   an item use that is about to commit. */
// FUNCTION: WIZ8 0x0051DC50
void StagePartySlotItemUse(int party_slot, W8ItemInstance* item, const W8CombatSlot* target)
{
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];

    row->item_detail.item_use.kind = -1;
    row->item_detail.item_use.item = item;
    row->item_target = *target;
    row->item_id = item->iItemNo;
    GetOriginOfCharacterItem(party_slot, item, &row->item_origin, &row->item_slot);
}

/* Take one of an item's uses away, and when that was the last one, empty the
   record. A stack counts down its count and a charged item its charges; an
   already-exhausted charged item can only be cleared when the caller allows
   it, and one whose kind has hidden properties is kept even then. Either way
   the emptied record leaves the packed party pool behind it. */
// FUNCTION: WIZ8 0x0051e760
void RemoveCharacterItem(W8Character* character, W8ItemInstance* item, bool remove_exhausted)
{
    int item_id = item->iItemNo;

    if (item_id == -1) {
        return;
    }
    if (g_item_records[item_id].quantity_kind == W8_ITEM_QUANTITY_NONE &&
        g_item_records[item_id].equip_class != W8_ITEM_EQUIP_CLASS_DRINK) {
        return;
    }

    if (g_item_records[item_id].quantity_kind == W8_ITEM_QUANTITY_STACK) {
        if (item->stack_count != 0) {
            --item->stack_count;
        }
        if (item->stack_count != 0) {
            return;
        }
    } else if (item->uses_or_charges == 0) {
        if (!remove_exhausted) {
            return;
        }
    } else {
        --item->uses_or_charges;
        if (item->uses_or_charges != 0) {
            return;
        }
        if (ItemHasHiddenProperties(item_id)) {
            return;
        }
    }

    EmptyItemRecord(item, character, true);
}

/* The same pairing as EquipMatchingPartnerItem, but for the items that carry a
   number of uses rather than being worn: the compatible-partner table names the
   kind to look for, and the first one found anywhere on the character takes the
   uses. With no partner the character only voices the attempt, and their
   attacks are recomputed for the change the failed attempt implies. */
// FUNCTION: WIZ8 0x0051ea90
void MergeMatchingPartnerItem(W8Character* character, W8ItemInstance* item)
{
    W8ItemInstance* partner;
    int item_id = item->iItemNo;
    int row;

    if (item_id == -1 || !ItemUsesShots(item_id)) {
        return;
    }

    row = 0;
    while (g_compatible_partner_pairs[row][0] != g_item_records[item_id].unidentified_name_index) {
        ++row;
        if (g_compatible_partner_pairs[row][0] == 0) {
            return;
        }
    }

    if (FindCharacterItemByDatabaseKind(character, g_compatible_partner_pairs[row][1], &partner,
                                        1)) {
        MergeItemUses(character, item, partner);
        PostCharacterNotice(CharacterPointerToPartySlot(character), gppStringList[0x1d4]);
        return;
    }

    QueueCharacterEvent(character, g_effect26, 0, g_character_event_no_flags,
                        g_character_event_full_volume);
    CalcAttacks(character);
}

/* Put an item in the party pool, and when the pool will not take it, leave it in
   hand instead. The hand's own item is set aside first, so the newcomer can be
   dropped exactly like any held item - except for the ones that may not be
   discarded, which are announced instead - and the hand's item comes back
   afterwards. */
// FUNCTION: WIZ8 0x00522090
bool AddItemToPartyOrDrop(W8ItemInstance* item, bool announce)
{
    bool stored = AddItemToParty(item, announce, false);
    if (stored) {
        return stored;
    }

    DropUnstoredCharacterItem(item);
    return stored;
}

/* Cast the spell an item carries and spend the uses it cost. The attempt is
   refused outright when the record cannot carry a spell at all; the difficulty
   comes from the skill that presents the spell, paced by the combat clock and
   raised for an item nobody has identified yet; the sound is keyed by the
   item's own resource name; and the effect's outcome decides whether the item
   is marked used, reported as a finding, or practised on afterwards. */
// FUNCTION: WIZ8 0x0051ee70
int CastItemSpell(W8Character* character, W8ItemInstance* item, unsigned int power)
{
    const W8ItemDatabaseRecord* record = &g_item_records[item->iItemNo];
    unsigned int party_slot = CharacterPointerToPartySlot(character);
    unsigned int spell_id = record->spell_id;
    W8TargetSource target;
    bool rejected_spell;
    unsigned int difficulty = 0;
    W8Skill skill;
    int effect;
    int caster_figure;
    int difficulty_kind;

    /* A record with no spell, an off-hand-only class and the two casting-aid
       classes are the ones the spell engine is told about rather than cast. */
    rejected_spell =
        (record->spell_id == W8_SPELL_NONE || record->equip_class == W8_ITEM_EQUIP_CLASS_GADGET ||
         (record->equip_class > W8_ITEM_EQUIP_CLASS_SCROLL &&
          record->equip_class < W8_ITEM_EQUIP_CLASS_KEY))
            ? 1
            : 0;
    if (!ValidateSpellTarget(party_slot, spell_id, power, true, rejected_spell)) {
        return 0;
    }

    if (record->spell_id == 'X' || record->spell_id == 't') {
        skill = W8_SKILL_NONE;
    } else {
        skill = g_item_spell_presentation[record->category];
        if (skill == W8_SKILL_NONE) {
            difficulty = 0;
        } else {
            difficulty = GetItemUseDifficulty(character, skill, character->skills[skill].level,
                                              spell_id, power);
            ScaleByCombatPace(party_slot, &difficulty);
            if (!item->identified) {
                difficulty += 0x1e;
            }
        }
    }

    SetTargetSourceToCharacter(party_slot, &target);
    target.item_cast = 1;
    TrackItemSpellSource(character, spell_id);

    if (skill == W8_SKILL_THROWING_SLING) {
        difficulty_kind = 4;
        caster_figure = character->uiExpLevel;
    } else if (skill == W8_SKILL_MUSIC) {
        difficulty_kind = 2;
        caster_figure = character->profession_levels[W8_CASTER_PROFESSION_INDEX_9];
    } else if (skill == W8_SKILL_ENGINEERING) {
        difficulty_kind = 3;
        caster_figure = character->profession_levels[W8_CASTER_PROFESSION_INDEX_8];
    } else {
        difficulty_kind = 1;
        caster_figure = character->uiExpLevel;
    }
    /* The spell engine reads the difficulty it worked out off the source's own
       tail bytes: 0x1f here, and the flag above at 0x21. */
    target.spell_difficulty =
        static_cast<unsigned char>(GetSpellDifficulty(caster_figure, spell_id, power));

    if (strlen(record->video_object_name + 0x18) != 0) {
        // reinterpret-ok: SGP's String returns UINT8* and SoundPlay takes char*
        SoundPlay(
            reinterpret_cast<char*>(String(s_spell_sound_format, record->video_object_name + 0x18)),
            0);
    }

    {
        int power_out = static_cast<int>(power);
        effect = CastSpellFromSource(
            spell_id, &target, &g_status.buffers.XChar[party_slot].target_out_of_combat, power, 0,
            difficulty, false, &power_out, difficulty_kind, 0, 0);
        power = static_cast<unsigned int>(power_out);
    }

    if (effect == 1) {
        if (g_settings.verbose_combat_messages != 0 || !item->identified) {
            FormatNotice(W8_FONT_PALETTE_WHITE, -1, gppStringList[0x1f4], GetItemDisplayName(item),
                         g_spell_records[spell_id].display_name, power);
            if (g_settings.verbose_combat_messages == 0) {
                SetTextBoxMode(1, -1);
            }
        }
        item->spell_hint = true;
    } else if (effect == 3) {
        PostCharacterNotice(
            party_slot, FormatWideString(gppStringList[0x1a9], FormatItemDisplayName(item, false)));
    }

    if (skill != W8_SKILL_NONE) {
        if (SpellAffectedTarget(character, spell_id,
                                &g_status.buffers.XChar[party_slot].target_out_of_combat, power)) {
            PracticeCharacterSkill(character, skill,
                                   g_spell_records[spell_id].spell_level + (power >> 1), false);
        }
    }
    return effect;
}

/* Split a stack of thrown weapons across the two hands. The source is the hand
   opposite the requested one and the destination is the requested hand, so the
   primary hand takes the larger half of the stack and the off hand the smaller;
   a lone item moves across as itself rather than being split, but only when the
   primary hand is the destination. */
// FUNCTION: WIZ8 0x0051ed30
void SplitThrowableStackBetweenHands(W8Character* character, W8EquipSlot equip_slot)
{
    W8ItemInstance created;
    W8EquipSlot source_slot;
    W8ItemInstance* source;
    int item_id;
    unsigned char moved;

    if (equip_slot != W8_EQUIP_SLOT_PRIMARY_WEAPON &&
        equip_slot != W8_EQUIP_SLOT_SECONDARY_WEAPON) {
        return;
    }
    source_slot = GetPairedEquipSlot(equip_slot);
    source = &character->EquippedItem[source_slot];
    item_id = source->iItemNo;
    /* Equipment class two is the thrown weapon, which is the only kind that
       stacks across the two hands. */
    if (item_id == -1 || g_item_records[item_id].equip_class != W8_ITEM_EQUIP_CLASS_THROWN_WEAPON) {
        return;
    }

    moved = source->stack_count;
    if (equip_slot == W8_EQUIP_SLOT_PRIMARY_WEAPON) {
        if (moved == 1) {
            SwapItemInstances(&character->EquippedItem[equip_slot], source, character, true);
            return;
        }
        moved -= (moved >> 1);
    } else {
        if (moved == 1) {
            return;
        }
        moved >>= 1;
    }

    ReplaceOrCreateItem(&created, item_id, false, source->identified, source->bound);
    source->stack_count -= moved;
    created.stack_count = moved;
    CopyItemInstance(&character->EquippedItem[equip_slot], &created, character, true);
    RefreshAfterItemRecordChange(source, character, true);
}

/* Close the hole one party-pool index leaves: everything after it moves down one
   place and the count drops. Retail allocates a full 500-entry scratch buffer
   (0x1770 bytes) for the shift; the caller has already emptied the record, so an
   entry that still holds an item is left alone. */
// FUNCTION: WIZ8 0x00521c20
void RemovePartyPoolEntry(unsigned int index)
{
    W8ItemInstance shifted[500];

    if (g_status.party_item_pool[index].iItemNo == -1 && index < g_status.party_item_count) {
        memcpy(&shifted[index], &g_status.party_item_pool[index + 1],
               (g_status.party_item_count - index - 1) * sizeof(W8ItemInstance));
        memcpy(&g_status.party_item_pool[index], &shifted[index],
               (g_status.party_item_count - index - 1) * sizeof(W8ItemInstance));
        memset(&g_status.party_item_pool[g_status.party_item_count - 1], 0, sizeof(W8ItemInstance));
        g_status.party_item_pool[g_status.party_item_count - 1].iItemNo = -1;
        --g_status.party_item_count;
        RedistributePartyEncumbrance();
    }
}

/* Empty one of a character's eight carried slots. Retail compares the
   resolved backpack entry to the global item-in-hand address before clearing;
   that path is unreachable for a real character pointer but is emitted. When
   the emptied entry also lives in the party pool, the hole is closed. */
// FUNCTION: WIZ8 0x00521ac0
void EmptyBackpackSlot(W8Character* character, int slot)
{
    if (character == 0 || slot < 0 || slot >= 8) {
        return;
    }

    EmptyItemRecord(&character->backpack[slot], character, true);
}

/* Empty one entry of the packed party pool by index and close the hole.
   Retail takes a signed index and rejects negatives with JL. */
// FUNCTION: WIZ8 0x00521cd0
void EmptyPartyPoolEntry(int index)
{
    if (index < 0 || static_cast<unsigned int>(index) >= g_status.party_item_count) {
        return;
    }

    EmptyItemRecord(&g_status.party_item_pool[index], 0, true);
}

/* The whole-party counterpart of FindCharacterItemByDatabaseKind: the item in
   hand first, then every occupied slot, then the party pool when the caller asks
   for it. The character that held the match comes back through the third
   output. */
// FUNCTION: WIZ8 0x00521480
bool FindItemByDatabaseKindOnParty(unsigned short item_kind, W8ItemInstance** found,
                                   W8Character** found_character, int include_backpack)
{
    if (found_character != 0) {
        *found_character = 0;
    }

    if (g_status.item_in_cursor &&
        g_item_records[g_status.item_in_hand.iItemNo].unidentified_name_index == item_kind) {
        if (found != 0) {
            *found = &g_status.item_in_hand;
        }
        return true;
    }

    for (unsigned int slot = 0; slot < 8; ++slot) {
        if (!g_status.buffers.XChar[slot].fOccupied) {
            continue;
        }
        W8Character* character = &g_status.buffers.Char[slot];
        if (FindCharacterItemByDatabaseKind(character, item_kind, found, include_backpack)) {
            if (found_character != 0) {
                *found_character = character;
            }
            return true;
        }
    }

    if (include_backpack == 2) {
        for (unsigned int index = 0; index < g_status.party_item_count; ++index) {
            W8ItemInstance* entry = &g_status.party_item_pool[index];
            if (entry->iItemNo != -1 &&
                g_item_records[entry->iItemNo].unidentified_name_index == item_kind) {
                if (found != 0) {
                    *found = entry;
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

/* Give the character the class item their current profession level earns. The
   ladder answers with the item id and the notice to show; the first tier hands
   over the welcome item outright, the top tier replaces nothing, and an item
   they already hold only gets its id raised when it is below the tier's. */
// FUNCTION: WIZ8 0x005218c0
void UpgradeProfessionClassItem(W8Character* character)
{
    W8ItemInstance* found = 0;
    W8ItemInstance created;
    int item_id;
    int message_id;

    switch (character->profession_levels[W8_CASTER_PROFESSION_INDEX_8]) {
    case 0:
    case 2:
        return;
    case 1:
        if (FindCharacterItemByDatabaseKind(character, 0x83, &found, 1)) {
            return;
        }
        ReplaceOrCreateItem(&created, 599, false, true, true);
        if (!AddItemToCharacter(character, &created, false, true, false) &&
            !AddItemToParty(&created, true, false)) {
            return;
        }
        PostCharacterNotice(
            CharacterPointerToPartySlot(character),
            FormatWideString(gppStringList[0x167],
                             gppStringList[g_gender_name_message_rows[character->gender][3]]));
        return;
    case 3:
    case 4:
        item_id = 600;
        message_id = 0x168;
        break;
    case 5:
    case 6:
        item_id = 0x259;
        message_id = 0x169;
        break;
    case 7:
    case 8:
        item_id = 0x25a;
        message_id = 0x16a;
        break;
    case 9:
    case 10:
        item_id = 0x25b;
        message_id = 0x16b;
        break;
    case 0xb:
    case 0xc:
        item_id = 0x25c;
        message_id = 0x16c;
        break;
    case 0xd:
    case 0xe:
    case 0xf:
        item_id = 0x25d;
        message_id = 0x16d;
        break;
    case 0x10:
    case 0x11:
        item_id = 0x25e;
        message_id = 0x16e;
        break;
    case 0x12:
    case 0x13:
    case 0x14:
        item_id = 0x25f;
        message_id = 0x16f;
        break;
    case 0x15:
    case 0x16:
        item_id = 0x260;
        message_id = 0x170;
        break;
    case 0x17:
    case 0x18:
    case 0x19:
        item_id = 0x261;
        message_id = 0x171;
        break;
    default:
        item_id = 0x262;
        message_id = 0x172;
        break;
    }

    if ((FindCharacterItemByDatabaseKind(character, 0x83, &found, 1) ||
         FindItemByDatabaseKindOnParty(0x83, &found, 0, 2)) &&
        found != 0 && found->iItemNo < item_id) {
        found->iItemNo = item_id;
        PostCharacterNotice(
            CharacterPointerToPartySlot(character),
            FormatWideString(gppStringList[message_id],
                             gppStringList[g_gender_name_message_rows[character->gender][2]]));
    }
}

/* Take every instance of one item id out of the party: the hand first, then
   every member's slots, then the pool. When remove_all is clear the search
   stops at the first hit. When it is set, retail still only calls
   FindItemOnCharacter once per party member before moving on, so a second copy
   on the same character is not cleared in that pass; the pool walk that follows
   does compact every matching pool entry. */
// FUNCTION: WIZ8 0x005215d0
bool RemovePartyItemByID(int item_id, bool remove_all)
{
    bool removed = false;

    if (g_status.item_in_cursor && g_status.item_in_hand.iItemNo == item_id) {
        EmptyItemRecord(&g_status.item_in_hand, 0, true);
        if (!remove_all) {
            return true;
        }
        removed = true;
    }

    W8ItemInstance* pool = g_status.party_item_pool;
    for (unsigned int slot = 0; slot < 8; ++slot) {
        if (!g_status.buffers.XChar[slot].fOccupied) {
            continue;
        }
        W8Character* character = &g_status.buffers.Char[slot];
        W8ItemInstance* found = 0;
        if (!FindItemOnCharacter(character, item_id, &found, 1, 0)) {
            continue;
        }

        EmptyItemRecord(found, character, true);

        if (!remove_all) {
            return true;
        }
        removed = true;
    }

    for (unsigned int index = 0; index < g_status.party_item_count; ++index) {
        if (pool[index].iItemNo != item_id) {
            continue;
        }
        EmptyPartyPoolEntry(index);
        if (!remove_all) {
            return true;
        }
        removed = true;
        /* The tail moved down into this index, so it is visited again. */
        --index;
    }
    return removed;
}

/* Swap each hand's weapon set for the other pair, and say why when it cannot
   happen. A character who is holding nothing at all has nothing to swap and is
   told so; a weapon bound to its wearer blocks the whole swap until the binding
   has been announced; and an item that cannot be held together with what is
   already there is left where it is, with the pair swapping around it. */
// FUNCTION: WIZ8 0x0051d3b0
bool SwapWeaponSetSlots(int party_slot, bool announce, bool refresh)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    int notice_context = gXStatus.fNpcDialogueMode ? 0 : -1;
    bool blocked_primary = false;
    bool blocked_alternate = false;
    int primary_right_item;
    int alternate_right_item;

    if (character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1 &&
        character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo == -1 &&
        character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo == -1 &&
        character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo == -1) {
        if (announce) {
            PostCharacterNoticeInContext(party_slot, notice_context, gppStringList[0x1eb]);
        }
        return false;
    }

    BindEquippedItem(character, W8_EQUIP_SLOT_PRIMARY_WEAPON);
    BindEquippedItem(character, W8_EQUIP_SLOT_SECONDARY_WEAPON);
    if (!CanUnequipSlotItem(character, W8_EQUIP_SLOT_PRIMARY_WEAPON) ||
        !CanUnequipSlotItem(character, W8_EQUIP_SLOT_SECONDARY_WEAPON)) {
        if (announce) {
            PostCharacterNoticeInContext(party_slot, notice_context, gppStringList[0x1ee]);
        }
        return false;
    }

    primary_right_item = character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo;
    alternate_right_item = character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON].iItemNo;
    if (alternate_right_item == -1 &&
        character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo != -1) {
        blocked_primary = CanHoldItemsTogether(
            primary_right_item,
            character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo);
    }
    if (character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo == -1 &&
        alternate_right_item != -1) {
        blocked_alternate = CanHoldItemsTogether(
            character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo, alternate_right_item);
    }

    if ((character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo != -1 ||
         alternate_right_item != -1) &&
        !blocked_primary) {
        SwapItemInstances(&character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON],
                          &character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON],
                          character, refresh);
    }
    if ((character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo != -1 ||
         character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON].iItemNo != -1) &&
        !blocked_alternate) {
        SwapItemInstances(&character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON],
                          &character->EquippedItem[W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON],
                          character, refresh);
    }

    if (refresh) {
        RequestRedraw(1 << (party_slot & 0x1f));
        RebuildEquipmentAndDerivedStatsForSlot(party_slot);
    }
    if (announce) {
        PostCharacterNoticeInContext(party_slot, notice_context, gppStringList[0x1ec]);
    }
    return true;
}

/* Bind a party member's carried items, which the combat UI only allows once the
   turn is really theirs: a free-turn phase or a party movement mode refuses the
   whole request with a notice, and so does the slot whose action is still being
   chosen. Otherwise the weapon sets swap, the row's one-shot flag clears, and
   the portraits redraw. */
// FUNCTION: WIZ8 0x0051d2c0
void BindCharacterItems(int party_slot, bool announce)
{
    if (gXStatus.fCombatMode) {
        if (!g_combat_state->round_active && !gXStatus.fPartyMovementMode) {
            ShowNotice(W8_FONT_PALETTE_BEIGE, gppStringList[W8_NOTICE_WEAPON_SWAP_BLOCKED_COMBAT]);
            return;
        }
        if (g_combat_state->iActionChar == party_slot && g_combat_state->eCombatActionStatus == 2 &&
            (g_status.buffers.XChar[party_slot].pending_action == W8_ACTION_ATTACK ||
             g_status.buffers.XChar[party_slot].pending_action == W8_ACTION_BERSERK)) {
            ShowNoticef(W8_FONT_PALETTE_WHITE, gppStringList[0x1f7],
                        g_status.buffers.Char[party_slot].name);
            return;
        }
    }

    if (IsPartySlotEligible(party_slot)) {
        if (SwapWeaponSetSlots(party_slot, announce, true)) {
            g_status.buffers.XChar[party_slot].weapon_swap_pending = false;
        }
    }
    RequestRedraw(W8_MAIN_REDRAW_CHARACTER_ACTION);
}
