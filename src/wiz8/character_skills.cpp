#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/message_box.h"
#include "wiz8/utility.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/xstatus.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* Original translation-unit ownership is unknown; surrounding anchors do not resolve it. */

// GLOBAL: WIZ8 0x0061ec94
static wchar_t g_format_s_possessive[] = L"%s's";

unsigned int GetCharacterSkillNoticeValue(W8Character* character, W8Skill skill_id)
{
    unsigned int value = character->skills[skill_id].points;
    if (skill_id == g_profession_bonus_skills[character->iProfession]) {
        value = value * 125 / 100;
    }
    return value;
}

void PostSkillIncreaseNotices(const W8SkillNoticePayload* notices)
{
    for (int index = 0; index < notices->count; ++index) {
        int slot = notices->party_slots[index];
        W8Skill skill = static_cast<W8Skill>(notices->skills[index]);
        W8Character* character = &g_status.buffers.Char[slot];
        unsigned int value = GetCharacterSkillNoticeValue(character, skill);
        PostCharacterNotice(slot, gppStringList[0x1d9],
                            gppStringList[g_character_skill_name_ids[skill]], value);
    }
}

// FUNCTION: WIZ8 0x00554170
void AppendSkillIncreaseNoticeText(wchar_t* text, unsigned int* length, int party_slot,
                                   bool continue_line, W8Skill skill_id)
{
    unsigned int skill_level;
    W8Character* character = &g_status.buffers.Char[party_slot];

    if (continue_line != 0) {
        text[*length] = L' ';
        *length += 1;
        text[*length] = L'\n';
        *length += 1;
        text[*length] = L' ';
        *length += 1;
    }
    text[*length] = L' ';
    *length += 1;
    text[*length] = 0xb4; /* font glyph */
    *length += 1;
    text[*length] = GetTable647CCCEntry(
        static_cast<signed char>(g_status.buffers.XChar[party_slot].party_order_index));
    *length += 1;
    text[*length] = L' ';
    *length += 1;
    swprintf(text + *length, g_format_s_possessive, character->name);
    *length = static_cast<unsigned int>(wcslen(text));
    text[*length] = L' ';
    *length += 1;
    text[*length] = 0xb5; /* font glyph */
    *length += 1;
    text[*length] = L' ';
    *length += 1;
    skill_level = GetCharacterSkillNoticeValue(character, skill_id);
    swprintf(text + *length, gppStringList[0x1db],
             gppStringList[g_character_skill_name_ids[skill_id]], skill_level);
    *length = static_cast<unsigned int>(wcslen(text));
}

// FUNCTION: WIZ8 0x005542E0
void FlushDeferredSkillNotices(void)
{
    int count;
    bool have_line;
    wchar_t* text;
    W8SkillNoticePayload* extra;
    int slot;
    unsigned int skill_id;
    unsigned int length;

    count = 0;
    have_line = 0;
    length = 0;
    if (gXStatus.deferred_skill_notices == 0) {
        return;
    }
    text = new wchar_t[0x200];
    memset(text, 0, 0x400);
    extra = new W8SkillNoticePayload;
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        if (g_status.buffers.XChar[slot].fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        for (skill_id = 0; skill_id < W8_SKILL_COUNT; ++skill_id) {
            if (gXStatus.monster_manager_entries[slot].skill_notice_pending[skill_id] == 0) {
                continue;
            }
            extra->party_slots[count] = static_cast<signed char>(slot);
            extra->skills[count] = static_cast<signed char>(skill_id);
            ++count;
            AppendSkillIncreaseNoticeText(text, &length, slot, have_line,
                                          static_cast<W8Skill>(skill_id));
            have_line = 1;
            if (count == 8) {
                extra->count = 8;
                W8MessageBoxPayload skill_notices_payload;
                skill_notices_payload.text = text;
                W8MessageBoxPayload skill_notices_extra;
                skill_notices_extra.skill_notices = extra;
                AddMessageBoxLine(W8_NPC_MSG_SKILL_NOTICES, skill_notices_payload,
                                  skill_notices_extra);
                count = 0;
                length = 0;
                have_line = 0;
                text = new wchar_t[0x200];
                memset(text, 0, 0x400);
                extra = new W8SkillNoticePayload;
            }
        }
    }
    if (have_line == 0) {
        delete[] text;
        delete extra;
    } else {
        extra->count = static_cast<signed char>(count);
        W8MessageBoxPayload skill_notices_payload;
        skill_notices_payload.text = text;
        W8MessageBoxPayload skill_notices_extra;
        skill_notices_extra.skill_notices = extra;
        AddMessageBoxLine(W8_NPC_MSG_SKILL_NOTICES, skill_notices_payload, skill_notices_extra);
    }
    for (slot = 0; slot < 8; ++slot) {
        memset(&gXStatus.monster_manager_entries[slot].skill_notice_pending[0], 0, W8_SKILL_COUNT);
    }
    gXStatus.deferred_skill_notices = 0;
}

// FUNCTION: WIZ8 0x00558610
void InvalidateAndRecalculateCharacterClassData(W8Character* character)
{
    character->portrait_index = -1;
    character->unknown_007d = -1;
    character->personality = -1;
    DeriveCharacterPersonality(character);
    CalcCharacterTableValue(character);
}

/* Profession and race trait-id sets. Values are the retail table contents at
   0x0061507C (fifteen triples) and 0x00615130 (sixteen quintuples); -1 is no
   trait. */
// GLOBAL: WIZ8 0x0061507C
W8ProfessionAbilitySet g_profession_abilities[15] = {
    {{W8_TRAIT_STAMINA_REGENERATION, W8_TRAIT_KNOCKOUT, W8_TRAIT_BERSERK}},
    {{W8_TRAIT_HEALTH_REGENERATION, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_CHEAT_DEATH, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_RANGED_CRITICALS, W8_TRAIT_SEARCH, W8_TRAIT_NONE}},
    {{W8_TRAIT_FEARLESS, W8_TRAIT_LIGHTNING_STRIKE, W8_TRAIT_NONE}},
    {{W8_TRAIT_THROWN_CRITICALS, W8_TRAIT_THROWN_AUTO_PENETRATE, W8_TRAIT_NONE}},
    {{W8_TRAIT_MONK_DAMAGE_RESISTANCE, W8_TRAIT_EFFECTIVE_WHILE_BLIND, W8_TRAIT_NONE}},
    {{W8_TRAIT_BACKSTAB, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_MERGE_GADGETS, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_CAMP_RECOVERY_BONUS, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_PRAY, W8_TRAIT_TURN_UNDEAD, W8_TRAIT_NONE}},
    {{W8_TRAIT_MAKE_POTIONS, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_REMOVE_CURSED_ITEMS, W8_TRAIT_TURN_UNDEAD, W8_TRAIT_NONE}},
    {{W8_TRAIT_FEARLESS, W8_TRAIT_MENTAL_CONDITION_IMMUNITY, W8_TRAIT_NONE}},
    {{W8_TRAIT_MAGIC_RESISTANCE_BONUS, W8_TRAIT_NONE, W8_TRAIT_NONE}},
};

// GLOBAL: WIZ8 0x00615130
W8RaceAbilitySet g_race_abilities[16] = {
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_DWARF_DAMAGE_RESISTANCE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE,
      W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_FAERIE_BASE_ARMOR_CLASS, static_cast<W8Trait>(26), static_cast<W8Trait>(23),
      static_cast<W8Trait>(25), W8_TRAIT_FAERIE_REDUCED_CARRY_CAPACITY}},
    {{W8_TRAIT_LIZARDMAN_SLOW_MAGIC_RECOVERY, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE,
      W8_TRAIT_NONE}},
    {{W8_TRAIT_BREATHE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE, W8_TRAIT_NONE}},
    {{static_cast<W8Trait>(30), W8_TRAIT_CANNOT_LEARN, W8_TRAIT_NONE, W8_TRAIT_NONE,
      W8_TRAIT_NONE}},
};

// GLOBAL: WIZ8 0x00615270
W8RaceResistanceProfile g_race_resistance_profiles[16] = {
    {{{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 20}, {2, 10}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{0, 1003}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 1003}, {3, 10}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 1003}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{2, 15}, {3, 15}, {4, 15}, {5, 15}, {-1, 0}, {-1, 0}}},
    {{{1, 10}, {3, 10}, {0, 15}, {5, -10}, {4, -10}, {-1, 0}}},
    {{{1, 15}, {2, 5}, {5, -5}, {4, -5}, {-1, 0}, {-1, 0}}},
    {{{1, -15}, {3, 10}, {2, 10}, {4, 10}, {-1, 0}, {-1, 0}}},
    {{{1, 10}, {3, 5}, {5, 15}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{5, 10}, {1, 15}, {4, 15}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 20}, {2, 20}, {4, -10}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 15}, {1, 15}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 15}, {2, 15}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{0, 45}, {4, -20}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 75}, {5, -25}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
};

/* Whether the character has the trait through profession, race, or - for
   trait 0x1c - a set second enchantment slot. Profession id -1 and race id
   -1 both mean absent and skip their tables. Read-only, so callers agree
   on a const character. */
// FUNCTION: WIZ8 0x00547940
bool CharacterHasTrait(const W8Character* character, W8Trait trait)
{
    unsigned int index;

    if (character == 0) {
        return false;
    }
    if (character->iProfession != W8_PROFESSION_NONE) {
        const W8Trait* abilities = g_profession_abilities[character->iProfession].ability_ids;
        for (index = 0; index < 3; ++index) {
            if (abilities[index] == trait) {
                return true;
            }
        }
    }
    if (character->iRace != W8_RACE_NONE) {
        const W8Trait* abilities = g_race_abilities[character->iRace].ability_ids;
        for (index = 0; index < 5; ++index) {
            if (abilities[index] == trait) {
                return true;
            }
        }
    }
    if (trait == W8_TRAIT_BREATHE && character->enchantments[W8_ENCHANTMENT_DRACON_BREATH].turns != 0) {
        return true;
    }
    return false;
}

static float ScaleValueByLevel(unsigned int level, float base)
{
    if (level > 0x14) {
        return base;
    }
    float scaled = (level * 2.0f + 60.0f) * base;
    return scaled * g_movement_speed_step;
}

/* Scale a trait's flat value by the character's level in their current
   profession: full value above twenty levels, sixty percent at zero and two
   percent per level in between. The trait id is carried by the call but the
   body never reads it. */
// FUNCTION: WIZ8 0x005479b0
float ScaleValueByProfessionLevel(W8Character* character, W8Trait, float base)
{
    return ScaleValueByLevel(character->profession_levels[character->iProfession], base);
}

// FUNCTION: WIZ8 0x00547a00
float ScaleValueByMonsterLevel(W8MonsterRecord* record, int, float base)
{
    return ScaleValueByLevel(record->effective_level, base);
}

/* The Valkyrie cheat-death trait fires instead of death while the character
   is not already deep into unconsciousness: a notice, an unconscious stint
   shortened by the profession-level scale, and hit points rolled back up -
   from a scaled share of the maximum once per combat, a scaled share of the
   current pool every other time. */
// FUNCTION: WIZ8 0x00547A50
void CheatDeathRevive(int party_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];

    PostCharacterNotice(party_slot, gppStringList[0x173]);
    SetCharacterCondition(party_slot, W8_CONDITION_UNCONSCIOUS,
                          character->uiCondition[W8_CONDITION_UNCONSCIOUS] -
                              static_cast<int>(ScaleValueByProfessionLevel(
                                  character, W8_TRAIT_CHEAT_DEATH, g_float_005ebc28)) +
                              7,
                          0, 0, 1);
    if (g_combat_state != 0 && g_combat_state->characters[party_slot].cheat_death_used == 0) {
        character->hp_current =
            (Random(static_cast<unsigned int>(ScaleValueByProfessionLevel(
                 character, W8_TRAIT_CHEAT_DEATH,
                 static_cast<unsigned int>(character->uiHPMax) * g_float_005ebc7c))) +
             0x32) *
            character->uiHPMax / 100;
        g_combat_state->characters[party_slot].cheat_death_used = true;
    } else {
        character->hp_current = Random(static_cast<unsigned int>(ScaleValueByProfessionLevel(
                                    character, W8_TRAIT_CHEAT_DEATH, character->hp_current))) +
                                1;
    }
}

/* Reveal the target's item bindings with a strength banded by the party
   slot's level in its current profession: level/4 + 1, zero percent. */
// FUNCTION: WIZ8 0x00548E20
int RevealCharacterItemBindingsByProfession(int party_slot, unsigned int target_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    return RevealCharacterItemBindings(
        target_slot, (character->profession_levels[character->iProfession] >> 2) + 1, 0);
}

/* Alchemist auto-brew candidates as {item_id, minimum alchemist profession
   level, maximum level} triples; -1 in the maximum opens the top end and the
   {-1,0,0} row terminates the walk. */
// GLOBAL: WIZ8 0x00616FE0
static int g_alchemist_brew_recipes[][3] = {
    {335, 1, 2},   {346, 2, 4},   {336, 3, 5},   {264, 4, -1},  {345, 5, 9},   {337, 6, 12},
    {317, 6, 12},  {269, 3, 8},   {334, 6, 9},   {347, 6, 12},  {341, 10, 13}, {342, 11, 14},
    {348, 13, 15}, {339, 17, -1}, {429, 18, -1}, {598, 5, 8},   {46, 6, 9},    {47, 7, 10},
    {69, 10, 12},  {338, 11, 13}, {260, 13, 14}, {359, 2, 4},   {358, 3, 6},   {360, 4, 8},
    {364, 12, 13}, {361, 14, -1}, {363, 15, -1}, {349, 4, 7},   {353, 7, 10},  {350, 8, 11},
    {357, 8, 11},  {354, 12, 14}, {362, 12, 13}, {355, 14, 15}, {356, 16, -1}, {343, 10, 13},
    {258, 12, 15}, {-1, 0, 0},
};

/* Brew one potion for a conscious alchemist whose cooldown has elapsed. The
   table bands each potion by alchemist profession level (-1 opens the top
   end); a random in-band entry is created, announced and stowed, then the
   cooldown is re-armed. */
// FUNCTION: WIZ8 0x00548E60
void BrewAlchemistPotion(W8Character* character)
{
    W8ItemInstance item;
    unsigned int slot = CharacterPointerToPartySlot(character);
    unsigned int made = 0;
    int alchemy;
    const int* recipe;
    unsigned int recipes;
    unsigned int pick;

    if (g_status.buffers.XChar[slot].fOccupied != 0 &&
        character->highest_condition < W8_CONDITION_UNCONSCIOUS &&
        CharacterHasTrait(character, W8_TRAIT_MAKE_POTIONS) != 0) {
        alchemy = character->profession_levels[W8_PROFESSION_ALCHEMIST];
        recipes = 0;
        for (recipe = &g_alchemist_brew_recipes[0][2]; recipe[-2] != -1; recipe += 3) {
            if (recipe[-1] <= alchemy && (recipe[0] == -1 || alchemy <= recipe[0])) {
                ++recipes;
            }
        }
        pick = Random(recipes) + 1;
        for (recipe = &g_alchemist_brew_recipes[0][2];
             alchemy < recipe[-1] || (recipe[0] != -1 && recipe[0] < alchemy) || --pick != 0;
             recipe += 3) {
        }
        ReplaceOrCreateItem(&item, recipe[-2], 0, 1, 1);
        if (item.stack_count > 1) {
            item.stack_count = 1;
        }
        ShowNoticef(slot, gppStringList[0x183], character->name, FormatItemDisplayName(&item, 1));
        StoreItemWithCharacterOrParty(character, &item, 0, 0, 0);
        made = 1;
    }
    if (made != 0) {
        character->potion_brew_cooldown = 0x168;
    }
}

/* Skill ids fall into three bands. Below 0x18 and at 0x1c..0x21 they are
   ordinary skills resolved against the profession; 0x18..0x1b are the magic
   realms, gated by the profession's magic-level offset; 0x22..0x28 index the
   attribute records instead, and count as available only once the attribute has
   reached its cap. */
// FUNCTION: WIZ8 0x00553d90
bool IsCharacterSkillAvailable(W8Character* character, W8Skill skill_id,
                               const bool* expert_realm_flags)
{
    W8Profession profession;
    unsigned int index;
    int magic_offset;

    if (g_profession_skill_availability[skill_id][character->iProfession] == 0) {
        return false;
    }
    if (CharacterHasTrait(character, W8_TRAIT_CANNOT_LEARN)) {
        if (static_cast<unsigned int>(skill_id) >= W8_SKILL_SPELLBOOK_WIZARDRY &&
            static_cast<unsigned int>(skill_id) <= W8_SKILL_SPELLBOOK_PSIONICS) {
            return false;
        }
        if (static_cast<unsigned int>(skill_id) >= W8_SKILL_FIRE_MAGIC &&
            static_cast<unsigned int>(skill_id) <= W8_SKILL_DIVINE_MAGIC) {
            return false;
        }
    }
    if (static_cast<unsigned int>(skill_id) >= W8_SKILL_FIRE_MAGIC &&
        static_cast<unsigned int>(skill_id) <= W8_SKILL_DIVINE_MAGIC) {
        for (index = 0x18; index <= 0x1b; ++index) {
            if (character->skills[index].active) {
                break;
            }
        }
        if (index > 0x1b) {
            return false;
        }
        if (character->skill_unlocks[skill_id] > 0) {
            return true;
        }
        if (expert_realm_flags && expert_realm_flags[skill_id - 0x1c]) {
            return true;
        }
        return false;
    }

    profession = character->iProfession;
    if (skill_id != g_profession_bonus_skills[profession]) {
        for (index = 0; index < 4; ++index) {
            if (skill_id == g_profession_skills[profession][index]) {
                return true;
            }
        }
        if (static_cast<unsigned int>(skill_id) >= W8_SKILL_POWER_STRIKE &&
            static_cast<unsigned int>(skill_id) <= W8_SKILL_EAGLE_EYE) {
            return character->attributes[skill_id - 0x22].value >= 100;
        }
        /* The canonical emits a byte index table over 0x00..0x1b, placed after
           the body, with three groups: default for 0x00..0x09 and 0x12, a
           middle band of 0x0a..0x11 and 0x13..0x17, and the magic realms. Its
           first and third groups resolve to the same address, so the middle
           band is a real case group whose body merely returns 1 like the
           default. Neither an empty break nor an explicit return 1 reproduces
           the table: VC6 drops the empty group and range-tests the remainder,
           and returning 1 merges the two bands into one range compare that
           costs 41 bytes more. */
        switch (skill_id) {
        case W8_SKILL_LOCKS_TRAPS:
        case W8_SKILL_STEALTH:
        case W8_SKILL_MUSIC:
        case W8_SKILL_PICKPOCKET:
        case W8_SKILL_MARTIAL_ARTS:
        case W8_SKILL_SCOUTING:
        case W8_SKILL_CLOSE_COMBAT:
        case W8_SKILL_RANGED_COMBAT:
        case W8_SKILL_CRITICAL_STRIKE:
        case W8_SKILL_ARTIFACTS:
        case W8_SKILL_MYTHOLOGY:
        case W8_SKILL_COMMUNICATION:
        case W8_SKILL_ENGINEERING:
            break;
        case W8_SKILL_SPELLBOOK_WIZARDRY:
        case W8_SKILL_SPELLBOOK_DIVINITY:
        case W8_SKILL_SPELLBOOK_ALCHEMY:
        case W8_SKILL_SPELLBOOK_PSIONICS:
            magic_offset = g_profession_magic_level_offsets[profession];
            if (magic_offset < 0 && magic_offset > -0xff) {
                return character->profession_levels[profession] + magic_offset > 0;
            }
            break;
        default:
            break;
        }
    }
    return true;
}

/* Rebuild each attribute's effective value from its base and the modifier
   block's seven adjustment bytes, clamped to one through 125, and then every
   skill's base level from the attribute pair g_skill_attributes names. The
   equipment refresh runs after each attribute so its use checks see the new
   value. */
// FUNCTION: WIZ8 0x005539e0
void ResetCharacterAttributes(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 7; ++index) {
        int value =
            character->bonus.attribute_adjustments[index] + character->attributes[index].value;
        ClampInteger(&value, 1, 0x7d);
        character->attributes[index].effective = value;
        UnequipUnusableItems(character);
    }
    InitializeSkillBaseLevels(character);
}

/* Rebuild every skill level from the value already spent on it, the
   profession's bonus skill and the race and profession adjustment bytes the
   modifier block carries, clamped to zero through 125. The equipment refresh
   runs after each skill for the same reason. */
// FUNCTION: WIZ8 0x00553a60
void ResetCharacterSkills(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 0x29; ++index) {
        int value = character->skills[index].points;
        if (index == static_cast<unsigned int>(g_profession_bonus_skills[character->iProfession])) {
            unsigned int bonus = static_cast<unsigned int>(value * 0x19) / 100;
            if (bonus == 0) {
                bonus = 1;
            }
            value += bonus;
        }
        value += character->bonus.skill_bonus[index];
        ClampInteger(&value, 0, 0x7d);
        character->skills[index].level = value;
        UnequipUnusableItems(character);
    }
}

/* An attribute's base value changed: flip the at-maximum flag on the
   attribute's pseudo-skill entry (id = attribute + 0x22), resetting or
   refunding its row while the character screen is up and announcing the cap
   in the main game, rebuild the effective value from the modifier block's
   adjustment clamped to 1..125, and refresh the equipment and derived state.
   Retail inlines InitializeSkillBaseLevels at the tail. */
// FUNCTION: WIZ8 0x00553AD0
void ApplyAttributeChange(W8Character* character, W8Attribute attribute)
{
    int skill_id = attribute + 0x22;

    if (character->attributes[attribute].value >= 0x64) {
        if (character->skills[skill_id].active == 0) {
            character->skills[skill_id].active = 1;
            if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                ResetCharacterScreenSkill(static_cast<W8Skill>(skill_id));
            }
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                ShowMainGameNoticeLine(
                    FormatWideString(gppStringList[0x1d6],
                                     gppStringList[g_character_description_first_ids[attribute]],
                                     character->name,
                                     gppStringList[g_character_skill_name_ids[skill_id]]),
                    0, 1, 0);
            }
        }
    } else {
        if (character->skills[skill_id].active != 0) {
            character->skills[skill_id].active = 0;
            if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                RefundCharacterScreenSkill(static_cast<W8Skill>(skill_id));
            }
        }
    }
    int effective = character->bonus.attribute_adjustments[attribute] +
                    static_cast<int>(character->attributes[attribute].value);
    ClampInteger(&effective, 1, 0x7d);
    character->attributes[attribute].effective = effective;
    UnequipUnusableItems(character);
    RecalculateCharacterDerivedStats(character);
    InitializeSkillBaseLevels(character);
}

/* A skill's invested value changed: re-scan the availability flags, rebuild
   the skill's level from it - with the profession-primary bonus and the
   modifier block's per-skill adjustment, clamped to 0..125 - and refresh the
   equipment and derived state. The single-skill half of
   ResetCharacterSkills. */
// FUNCTION: WIZ8 0x00553C10
void ApplySkillChange(W8Character* character, W8Skill skill_id)
{
    RefreshCharacterSkillAvailability(character);

    int level = character->skills[skill_id].points;
    if (skill_id == g_profession_bonus_skills[character->iProfession]) {
        unsigned int bonus = static_cast<unsigned int>(level * 0x19) / 100;
        if (bonus == 0) {
            bonus = 1;
        }
        level += bonus;
    }
    level += character->bonus.skill_bonus[skill_id];
    ClampInteger(&level, 0, 0x7d);
    character->skills[skill_id].level = level;
    UnequipUnusableItems(character);
    RecalculateCharacterDerivedStats(character);
}

/* Average the two attribute values g_skill_attributes names for every skill
   into the skill's 0x0a base level. 0x00557D80 and 0x00557B20 seed the
   profession skill levels from these. */
// FUNCTION: WIZ8 0x00553c90
void InitializeSkillBaseLevels(W8Character* character)
{
    for (int index = 0; index < 0x29; ++index) {
        W8Attribute first = g_skill_attributes[index].attribute_1;
        W8Attribute second = g_skill_attributes[index].attribute_2;
        character->skills[index].base_level =
            (character->attributes[first].value + character->attributes[second].value) >> 1;
    }
}

/* Re-scan every skill's availability and mirror each flag change onto the
   open character screen's page 2. The realm flags array marks which expert
   skills gained a spell since the last scan. */
// FUNCTION: WIZ8 0x00553cd0
void RefreshCharacterSkillAvailability(W8Character* character)
{
    bool expert_realm_flags[8];
    int index;

    for (index = 0; index < 8; ++index) {
        expert_realm_flags[index] = 0;
    }
    for (index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index + 1] == -1 || character->spell_learned[index + 1] == 2) {
            expert_realm_flags[g_spell_records[index].realm] = 1;
        }
    }
    for (index = 0; index < 0x29; ++index) {
        bool available =
            IsCharacterSkillAvailable(character, static_cast<W8Skill>(index), expert_realm_flags);
        if (!available) {
            if (character->skills[index].active) {
                character->skills[index].active = 0;
                if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                    RefundCharacterScreenSkill(static_cast<W8Skill>(index));
                }
            }
        } else if (!character->skills[index].active) {
            character->skills[index].active = 1;
            if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                ResetCharacterScreenSkill(static_cast<W8Skill>(index));
            }
        }
    }
}

/* A quarter of the skill's current value, never below one. The bonus skill's
   level gets this added after the profession assignment. */
// FUNCTION: WIZ8 0x00553ee0
unsigned int GetSkillQuarterValue(W8Character* character, W8Skill skill_id)
{
    unsigned int value = (character->skills[skill_id].points * 0x19) / 100;
    if (value == 0) {
        value = 1;
    }
    return value;
}

/* Practice one skill: mark it practiced, unlock it if the character newly
   qualifies, then roll every usage point against the value-scaled chance -
   (100 - value) * base_level / 100, halved again for magic-realm skills -
   and bank a level on every eighth success. A leveled skill refreshes its
   effective level through the same profession-bonus path ApplySkillChange
   uses, then reports immediately on the idle main-game screen or defers the
   notice into the portrait slot's per-skill flag array. */
// FUNCTION: WIZ8 0x00553F10
void PracticeCharacterSkill(W8Character* character, W8Skill skill_id, int usage_points,
                            bool suppress_notification)
{
    bool improved;
    unsigned int slot;
    unsigned int threshold;

    improved = false;
    if (usage_points != 0 &&
        g_profession_skill_availability[skill_id][character->iProfession] != 0) {
        W8CharacterSkill* skill = &character->skills[skill_id];
        skill->available = true;
        if (skill->active == 0) {
            if (IsCharacterSkillAvailable(character, skill_id, NULL) == 0) {
                return;
            }
            skill->active = 1;
            if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                ResetCharacterScreenSkill(skill_id);
            }
        }
        if (usage_points != 0) {
            do {
                if (skill->points < 100) {
                    threshold = (100 - (skill->points * 100) / 100) * skill->base_level / 100;
                    if (g_skill_attributes[skill_id].category == 4) {
                        threshold /= 2;
                    }
                    if (threshold == 0) {
                        threshold = 1;
                    }
                    if (Random(100) < threshold) {
                        ++skill->practice_count;
                        if (skill->practice_count >= 8) {
                            ++skill->points;
                            skill->practice_count = 0;
                            skill->improved = 1;
                            improved = true;
                        }
                    }
                }
            } while (--usage_points != 0);
            if (improved) {
                RefreshCharacterSkillAvailability(character);
                int level = skill->points;
                if (skill_id == g_profession_bonus_skills[character->iProfession]) {
                    unsigned int bonus = static_cast<unsigned int>(level * 0x19) / 100;
                    if (bonus == 0) {
                        bonus = 1;
                    }
                    level += bonus;
                }
                level += character->bonus.skill_bonus[skill_id];
                ClampInteger(&level, 0, 0x7d);
                skill->level = level;
                UnequipUnusableItems(character);
                RecalculateCharacterDerivedStats(character);
                slot = CharacterPointerToPartySlot(character);
                if (gXStatus.fCombatMode == 0 && IsModalOpen() == 0 &&
                    gXStatus.item_pick_pending == 0 &&
                    g_current_screen_state.id == W8_SCREEN_MAIN_GAME && IsScreenIdle() != 0 &&
                    suppress_notification == 0) {
                    wchar_t* text = new wchar_t[0x200];
                    memset(text, 0, 0x400);
                    unsigned int length = 0;
                    AppendSkillIncreaseNoticeText(text, &length, slot, 0, skill_id);
                    W8SkillNoticePayload* extra = new W8SkillNoticePayload;
                    extra->count = 1;
                    extra->party_slots[0] = static_cast<signed char>(slot);
                    extra->skills[0] = static_cast<signed char>(skill_id);
                    W8MessageBoxPayload skill_notices_payload;
                    skill_notices_payload.text = text;
                    W8MessageBoxPayload skill_notices_extra;
                    skill_notices_extra.skill_notices = extra;
                    AddMessageBoxLine(W8_NPC_MSG_SKILL_NOTICES, skill_notices_payload,
                                      skill_notices_extra);
                    return;
                }
                gXStatus.monster_manager_entries[slot].skill_notice_pending[skill_id] = 1;
                gXStatus.deferred_skill_notices = 1;
            }
        }
    }
}

/* Rebuilds all six resistance channels from scratch.
 
   The base of each starts at a flat 25 plus a tenth of the matching skill, then
   takes a flat bonus derived from skill 36 when the character has it, and five
   more for profession 14. The race table adds its own adjustments next: a value
   at or below 1000 is a flat amount, and anything above it names a character
   attribute whose fifth is added instead. Two attributes feed two specific
   channels directly, each contributing half of whatever it carries above 80.
 
   The total is then base plus the character's flat all-resistance bonus plus
   the per-channel one, and only the total is clamped - the base is left as
   computed, which is why a subsequent pass over the same character produces the
   same answer rather than compounding. */
/* The profession databases and per-skill attribute records. Contents are the
   retail tables at 0x00615570..0x0061634c; the five profession arrays share
   one contiguous block with the skill-attribute records. -1 in a skill slot
   means no skill, and -255 in a magic offset means the profession casts no
   spells at all. */
// GLOBAL: WIZ8 0x00615570
float g_profession_hit_point_factors[15] = {
    9.0f, 7.5f, 7.5f, 6.5f, 7.0f, 6.5f, 6.5f, 6.0f, 5.5f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f,
};
// GLOBAL: WIZ8 0x006155b0
W8SkillAttributes g_skill_attributes[0x29] = {
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_SPEED},
    {0, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_DEXTERITY},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_SPEED},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_STRENGTH},
    {0, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_STRENGTH},
    {1, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_INTELLIGENCE},
    {1, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_INTELLIGENCE},
    {1, W8_SKILL_IMPORT_PROFESSION, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_INTELLIGENCE},
    {1, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_SPEED},
    {0, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_SPEED},
    {1, W8_SKILL_IMPORT_PROFESSION, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_INTELLIGENCE},
    {2, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_INTELLIGENCE},
    {2, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_INTELLIGENCE},
    {2, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_SENSES},
    {2, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_SPEED},
    {2, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_SENSES},
    {2, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_INTELLIGENCE},
    {2, W8_SKILL_IMPORT_POLICY_0, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_SENSES},
    {2, W8_SKILL_IMPORT_PROFESSION, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_DEXTERITY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_INTELLIGENCE},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_PIETY, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_INTELLIGENCE},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_INTELLIGENCE},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {3, W8_SKILL_IMPORT_POLICY_1, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_PIETY},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_STRENGTH, W8_ATTRIBUTE_STRENGTH},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_INTELLIGENCE, W8_ATTRIBUTE_INTELLIGENCE},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_PIETY, W8_ATTRIBUTE_PIETY},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_VITALITY, W8_ATTRIBUTE_VITALITY},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_DEXTERITY, W8_ATTRIBUTE_DEXTERITY},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_SPEED, W8_ATTRIBUTE_SPEED},
    {4, W8_SKILL_IMPORT_DISABLED, W8_ATTRIBUTE_SENSES, W8_ATTRIBUTE_SENSES},
};
// GLOBAL: WIZ8 0x00615840
int g_profession_skill_availability[0x29][15] = {
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};
// GLOBAL: WIZ8 0x006161dc
W8Skill g_profession_bonus_skills[15] = {
    W8_SKILL_CLOSE_COMBAT,
    W8_SKILL_DUAL_WEAPONS,
    W8_SKILL_POLEARM,
    W8_SKILL_RANGED_COMBAT,
    W8_SKILL_SWORD,
    W8_SKILL_CRITICAL_STRIKE,
    W8_SKILL_MARTIAL_ARTS,
    W8_SKILL_LOCKS_TRAPS,
    W8_SKILL_MODERN_WEAPONS,
    W8_SKILL_COMMUNICATION,
    W8_SKILL_SPELLBOOK_DIVINITY,
    W8_SKILL_SPELLBOOK_ALCHEMY,
    W8_SKILL_ARTIFACTS,
    W8_SKILL_SPELLBOOK_PSIONICS,
    W8_SKILL_SPELLBOOK_WIZARDRY,
};
// GLOBAL: WIZ8 0x00616218
W8Skill g_profession_skills[15][4] = {
    {W8_SKILL_RANGED_COMBAT, W8_SKILL_SWORD, W8_SKILL_AXE, W8_SKILL_SHIELD},
    {W8_SKILL_CLOSE_COMBAT, W8_SKILL_SWORD, W8_SKILL_DAGGER, W8_SKILL_NONE},
    {W8_SKILL_CLOSE_COMBAT, W8_SKILL_MYTHOLOGY, W8_SKILL_AXE, W8_SKILL_NONE},
    {W8_SKILL_SCOUTING, W8_SKILL_BOW, W8_SKILL_MYTHOLOGY, W8_SKILL_NONE},
    {W8_SKILL_CLOSE_COMBAT, W8_SKILL_DUAL_WEAPONS, W8_SKILL_CRITICAL_STRIKE, W8_SKILL_NONE},
    {W8_SKILL_CLOSE_COMBAT, W8_SKILL_MARTIAL_ARTS, W8_SKILL_THROWING_SLING, W8_SKILL_STEALTH},
    {W8_SKILL_CLOSE_COMBAT, W8_SKILL_CRITICAL_STRIKE, W8_SKILL_STAFF_WAND, W8_SKILL_STEALTH},
    {W8_SKILL_DAGGER, W8_SKILL_DUAL_WEAPONS, W8_SKILL_PICKPOCKET, W8_SKILL_STEALTH},
    {W8_SKILL_RANGED_COMBAT, W8_SKILL_ENGINEERING, W8_SKILL_LOCKS_TRAPS, W8_SKILL_NONE},
    {W8_SKILL_MUSIC, W8_SKILL_MYTHOLOGY, W8_SKILL_ARTIFACTS, W8_SKILL_NONE},
    {W8_SKILL_MACE_FLAIL, W8_SKILL_STAFF_WAND, W8_SKILL_COMMUNICATION, W8_SKILL_NONE},
    {W8_SKILL_MYTHOLOGY, W8_SKILL_THROWING_SLING, W8_SKILL_NONE, W8_SKILL_NONE},
    {W8_SKILL_SPELLBOOK_ALCHEMY, W8_SKILL_SPELLBOOK_WIZARDRY, W8_SKILL_SPELLBOOK_DIVINITY,
     W8_SKILL_SPELLBOOK_PSIONICS},
    {W8_SKILL_COMMUNICATION, W8_SKILL_MYTHOLOGY, W8_SKILL_MENTAL_MAGIC, W8_SKILL_NONE},
    {W8_SKILL_FIRE_MAGIC, W8_SKILL_WATER_MAGIC, W8_SKILL_AIR_MAGIC, W8_SKILL_MENTAL_MAGIC},
};
// GLOBAL: WIZ8 0x00616310
int g_profession_magic_level_offsets[15] = {
    -255, -4, -4, -4, -4, -4, -4, -255, -255, -255, 0, 0, 0, 0, 0,
};
