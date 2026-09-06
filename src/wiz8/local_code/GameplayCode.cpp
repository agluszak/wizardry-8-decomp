#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/combat_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/fact_state.h"
#include "wiz8/layouts/item_tables.h"

/*
 * Local Code\GameplayCode.cpp.
 *
 * The derived character numbers - experience goals, level bands and the party
 * headcounts the rest of the game asks about.
 */

#define GAMEPLAY_CODE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayCode.cpp"

/* PROF_COUNT and PC_RACE_COUNT, both named by the assertions that bound their
    */
enum { W8_PROF_COUNT = 15, W8_PC_RACE_COUNT = 11 };

/* How much one level in a profession is worth towards physical combat
   experience. The professions split three ways. */
enum {
    W8_PHYS_COMBAT_WEIGHT_FIGHTER = 4,
    W8_PHYS_COMBAT_WEIGHT_MIXED = 3,
    W8_PHYS_COMBAT_WEIGHT_CASTER = 2
};

extern void CalcXPGoal(W8Character* character);                 /* 0x004EF090 */
/* 0x00616604: one entry per faction, race and profession together. */
extern const int g_character_table_00616604[];
extern unsigned char TryCharacterAction(int party_slot, int action, char commit);
extern unsigned char Function547940(const W8Character* character, int ability);
extern unsigned int FatigueArmorPenalty(int fatigue_band);

// GLOBAL: WIZ8 0x00683F94
unsigned char g_in_combat_00683f94;

static const unsigned char g_armor_class_location_weights[5] = {
    15, 40, 30, 10, 5
};

/* Whether any monster is engaged with the party right now: in combat, in the
   engaged state, still alive and not yet on its way out. */
// FUNCTION: WIZ8 0x004eee20
bool AnyMonsterEngaged(void)
{
    unsigned int index;
    const W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->flag_16 == 1 &&
            monster_info->hp_current != 0 && (unsigned int)monster_info->value_107 < 0xe) {
            return true;
        }
    }
    return false;
}

/* How many party members are still on their feet. */
// FUNCTION: WIZ8 0x004eee80
int CountActiveCharacters(void)
{
    int count = 0;
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0 &&
            g_party_characters[party_slot].hp_current != 0 &&
            g_party_characters[party_slot].unknown_0b01 < 0x12) {
            ++count;
        }
    }
    return count;
}

/* Whether anybody is. The same count, narrowed to a yes or no. */
// FUNCTION: WIZ8 0x004eeec0
bool AnyCharacterActive(void)
{
    int count = 0;
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0 &&
            g_party_characters[party_slot].hp_current != 0 &&
            g_party_characters[party_slot].unknown_0b01 < 0x12) {
            ++count;
        }
    }
    return count != 0;
}

/* Walk a character up to a level from scratch, recomputing the experience goal
   at each step so the goals for the level below are the ones they actually
   passed. */
// FUNCTION: WIZ8 0x004ef010
void AdvanceCharacterToLevel(W8Character* character, unsigned int level)
{
    character->level = 1;
    character->experience_previous_goal = 0;
    CalcXPGoal(character);

    while (character->level < level) {
        character->level = character->level + 1;
        character->experience_previous_goal = character->experience_goal;
        CalcXPGoal(character);
    }
    character->experience = character->experience_previous_goal;
}

/* Whether one party slot has earned its next level: occupied, alive, in shape
   to act, and holding at least the experience the next level asks for. */
// FUNCTION: WIZ8 0x004ef3c0
bool IsCharacterReadyToAdvance(int party_slot)
{
    const W8Character* character = &g_party_characters[party_slot];

    if (g_party_slot_rows[party_slot].occupied == 0) {
        return false;
    }
    if (character->hp_current == 0) {
        return false;
    }
    if (character->unknown_0b01 > 0x11) {
        return false;
    }
    return character->experience >= character->experience_goal;
}

/* The first free party slot in a range, or -1 when the range is full. */
// FUNCTION: WIZ8 0x004ef460
unsigned int FindFreePartySlot(unsigned int first, unsigned int last)
{
    unsigned int slot;

    for (slot = first; slot < last; ++slot) {
        if (g_party_slot_rows[slot].occupied == 0) {
            return slot;
        }
    }
    return (unsigned int)-1;
}

/* Recompute the eight-band ladder over the character's level in their current
   profession. A character who has changed profession is banded on the whole
   level; one still in their first profession has the starting base taken off
   first. */
// FUNCTION: WIZ8 0x004eed80
void CalcCharacterLevelBand(W8Character* character)
{
    unsigned int level = character->profession_levels[character->current_profession];

    if (character->current_profession == character->original_profession) {
        level -= character->level_band_base;
    }

    if (level == 0) {
        character->level_band = 0;
    }
    else if (level == 1) {
        character->level_band = 1;
    }
    else if (level < 4) {
        character->level_band = 2;
    }
    else if (level < 7) {
        character->level_band = 3;
    }
    else if (level < 0xb) {
        character->level_band = 4;
    }
    else if (level < 0x10) {
        character->level_band = 5;
    }
    else if (level < 0x16) {
        character->level_band = 6;
    }
    else {
        character->level_band = (level > 0x1b) + 7;
    }
}

/* Look up the value that faction, race and profession together select. */
// FUNCTION: WIZ8 0x004ef950
void CalcCharacterTableValue(W8Character* character)
{
    if (character->race > 10) {
        srAssertFail("pPC->iRace < PC_RACE_COUNT", GAMEPLAY_CODE_CPP, 2359, 0);
    }
    character->table_value_0079 =
        g_character_table_00616604[(character->faction * 0x10 + character->race) * W8_PROF_COUNT +
                                   character->current_profession];
}

/* What the character's levels are worth towards physical combat. Every
   profession they have ever held counts, weighted by how much fighting that
   profession does. */
// FUNCTION: WIZ8 0x004ee130
int CalcPhysCombatExperience(W8Character* character)
{
    int total = 0;
    int weight = 0;
    unsigned int profession;
    int levels;

    if (character->current_profession == -1) {
        return 0;
    }
    if (character == 0) {
        srAssertFail("pPC != NULL", GAMEPLAY_CODE_CPP, 398, 0);
    }
    if (character->current_profession < 0 || character->current_profession > 14) {
        srAssertFail("(pPC->iProfession >= 0) && (pPC->iProfession < PROF_COUNT)",
                     GAMEPLAY_CODE_CPP, 399, 0);
    }

    for (profession = 0; profession < W8_PROF_COUNT; ++profession) {
        levels = character->profession_levels[profession];
        if (levels != 0) {
            switch (profession) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                weight = W8_PHYS_COMBAT_WEIGHT_FIGHTER;
                break;
            case 7:
            case 8:
            case 9:
                weight = W8_PHYS_COMBAT_WEIGHT_MIXED;
                break;
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
                weight = W8_PHYS_COMBAT_WEIGHT_CASTER;
                break;
            default:
                srAssertFail("FALSE", GAMEPLAY_CODE_CPP, 444,
                             "CalcPhysCombatExperience: ERROR - Invalid profession");
            }
            total += levels * weight;
        }
    }
    return total;
}

/* Rebuild the character's initiative from level, speed and senses, with the
   initiative skill and equipment/effect modifier applied before encumbrance. */
// FUNCTION: WIZ8 0x004ee000
void CalcInitiative(W8Character* character)
{
    character->initiative =
        ((character->level + 1) >> 1) +
        character->attributes[6].effective / 5 - 10 +
        character->attributes[5].effective / 5;

    if (character->skills[39].flag_00 != 0) {
        character->initiative += character->skills[39].level / 10 + 1;
    }
    character->initiative +=
        static_cast<signed char>(character->unknown_16a2[0xce]);

    switch (character->load_category) {
    case 0:
        break;
    case 1:
        character->initiative -= 1;
        break;
    case 2:
        character->initiative -= 2;
        break;
    case 3:
        character->initiative -= 4;
        break;
    case 4:
        character->initiative -= 8;
        break;
    default:
        srAssertFail("FALSE", GAMEPLAY_CODE_CPP, 380,
                     "CalcInitiative: ERROR - Invalid load category");
        break;
    }
}

/* Rebuild the thirteen shared and location-specific armor-class components,
   then derive the unweighted and body-location-weighted summaries. */
// FUNCTION: WIZ8 0x004ee9d0
void CalcArmorClasses(W8Character* character)
{
    bool defensive_action = false;
    if (g_in_combat_00683f94) {
        unsigned int slot = CharacterPointerToPartySlot(character);
        defensive_action =
            TryCharacterAction(slot, 4, 0) || TryCharacterAction(slot, 5, 0);
    }

    unsigned int index;
    for (index = 0; index < 13; ++index) {
        character->armor_class_components[index] = 0;
    }

    for (index = 0; index < 12; ++index) {
        int item_id = character->equipment[index].item_id;
        if (index != 0 && index != 4 && index != 5 &&
            index != 8 && index != 9 && index != 10 && index != 11 &&
            item_id != -1) {
            int component = g_item_records[item_id].equip_class == 5 ? 3 : 4;
            character->armor_class_components[component] +=
                g_item_records[item_id].armor_class_bonus;
        }
    }

    if (character->unknown_0b01 <= 0x11) {
        if (Function547940(character, 0x16)) {
            character->armor_class_components[0] += 2;
        }
        unsigned int speed = character->attributes[5].effective;
        if (speed > 79) {
            ++character->armor_class_components[1];
        }
        if (speed > 89) {
            ++character->armor_class_components[1];
        }
        if (speed < 20) {
            --character->armor_class_components[1];
        }
        if (speed < 10) {
            --character->armor_class_components[1];
        }

        character->armor_class_components[2] +=
            character->skills[11].level / 10;
        if (character->skills[38].flag_00) {
            character->armor_class_components[11] +=
                character->skills[38].level / 20 + 1;
        }

        int shield = character->armor_class_components[3];
        if (shield > 0) {
            int skill_bonus = defensive_action
                ? static_cast<int>(character->skills[6].level / 15)
                : static_cast<int>(character->skills[6].level / 25);
            int ceiling = defensive_action ? shield * 3 / 2 : shield;
            if (skill_bonus > ceiling) {
                skill_bonus = ceiling;
            }
            character->armor_class_components[3] += skill_bonus;
        }

        character->armor_class_components[5] +=
            static_cast<signed char>(character->unknown_16a2[0xd2]);
        character->armor_class_components[8] +=
            static_cast<signed char>(character->unknown_17b6[5]);
        if (defensive_action) {
            character->armor_class_components[10] += 2;
        }
        switch (character->load_category) {
        case 2:
            character->armor_class_components[7] -= 1;
            break;
        case 3:
            character->armor_class_components[7] -= 2;
            break;
        case 4:
            character->armor_class_components[7] -= 4;
            break;
        }
        character->armor_class_components[9] -=
            FatigueArmorPenalty(character->fatigue_band) / 10;
        character->armor_class_components[6] +=
            static_cast<signed char>(character->unknown_16a2[0xd3]);
    }

    character->armor_class_total = 0;
    for (index = 0; index < 12; ++index) {
        if (index != 6) {
            character->armor_class_total +=
                character->armor_class_components[index];
        }
    }
    if (character->out_of_formation && character->armor_class_total > -5) {
        character->armor_class_total = -5;
    }

    static const int equipment_slots[5] = { 0, 4, 10, 5, 11 };
    int weighted_total = 0;
    for (index = 0; index < 5; ++index) {
        int armor_class = character->armor_class_total +
                          character->armor_class_components[6];
        int item_id = character->equipment[equipment_slots[index]].item_id;
        if (item_id != -1) {
            armor_class += g_item_records[item_id].armor_class_bonus;
        }
        character->armor_class_by_location[index] = armor_class;
        weighted_total += g_armor_class_location_weights[index] * armor_class;
    }
    character->armor_class_average =
        weighted_total < 0 ? (weighted_total - 50) / 100
                           : (weighted_total + 50) / 100;
}
