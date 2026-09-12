#include "wiz8/character.h"
#include "wiz8/utility.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/screen_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/combat_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/fact_state.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/npc_state.h"

/*
 * Local Code\GameplayCode.cpp.
 *
 * The derived character numbers - experience goals, level bands and the party
 * headcounts the rest of the game asks about.
 */

#define GAMEPLAY_CODE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayCode.cpp"

/* PC_RACE_COUNT, named by the assertion that bounds the race field, and the
   eleven playable races out of the sixteen the race tables carry. */
enum { W8_PC_RACE_COUNT = 11 };

/* How much one level in a profession is worth towards physical combat
   experience. The professions split three ways. */
enum {
    W8_PHYS_COMBAT_WEIGHT_FIGHTER = 4,
    W8_PHYS_COMBAT_WEIGHT_MIXED = 3,
    W8_PHYS_COMBAT_WEIGHT_CASTER = 2
};

/* 0x00616604: one entry per sex, race and profession together. */
// GLOBAL: WIZ8 0x00616604
const int g_character_table_00616604[480] = {
    3, 1, 0, 0, 3, 56, 4, 2, 1, 2, 5, 4, 24, 4, 5,
    13, 12, 0, 12, 13, 56, 13, 12, 12, 12, 14, 14, 14, 14, 14,
    19, 19, 0, 19, 19, 56, 20, 20, 20, 20, 18, 18, 18, 18, 18,
    24, 24, 0, 24, 24, 56, 24, 24, 25, 25, 25, 25, 25, 25, 25,
    28, 28, 0, 28, 28, 56, 28, 28, 29, 29, 29, 29, 29, 29, 29,
    32, 32, 0, 32, 32, 56, 32, 32, 33, 33, 33, 33, 33, 33, 33,
    36, 36, 0, 36, 36, 56, 36, 36, 37, 37, 37, 37, 37, 37, 37,
    40, 40, 0, 40, 40, 56, 41, 40, 41, 40, 41, 41, 41, 41, 41,
    45, 45, 0, 45, 45, 56, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    48, 48, 0, 48, 48, 56, 48, 48, 49, 49, 49, 49, 49, 49, 49,
    52, 52, 0, 52, 52, 56, 52, 52, 53, 53, 53, 53, 53, 53, 53,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 11, 9, 10, 9, 57, 10, 11, 7, 7, 7, 8, 8, 8, 8,
    15, 15, 15, 17, 15, 57, 17, 17, 16, 16, 17, 16, 16, 17, 16,
    22, 21, 22, 22, 22, 57, 22, 21, 21, 23, 23, 23, 23, 23, 23,
    26, 26, 26, 26, 26, 57, 26, 26, 27, 27, 27, 27, 27, 27, 27,
    30, 30, 30, 30, 30, 57, 30, 30, 31, 31, 31, 31, 31, 31, 31,
    35, 35, 35, 35, 35, 57, 35, 35, 34, 34, 34, 34, 34, 34, 34,
    38, 38, 38, 38, 38, 57, 38, 38, 39, 39, 39, 39, 39, 39, 39,
    42, 42, 42, 42, 42, 57, 42, 42, 43, 43, 43, 43, 43, 43, 43,
    47, 47, 47, 47, 47, 57, 47, 47, 46, 46, 46, 46, 46, 46, 46,
    50, 50, 50, 50, 50, 57, 50, 50, 51, 51, 51, 51, 51, 51, 51,
    55, 55, 55, 55, 55, 57, 55, 55, 52, 52, 52, 52, 52, 52, 52,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const unsigned char g_armor_class_location_weights[5] = {
    15, 40, 30, 10, 5
};

static const W8Dice g_unarmed_damage_dice[12] = {
    { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 2 }, { 0, 2, 3 },
    { 0, 2, 4 }, { 0, 3, 3 }, { 1, 3, 3 }, { 2, 3, 3 },
    { 0, 3, 5 }, { 0, 4, 4 }, { 2, 4, 4 }, { 4, 4, 4 }
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

/* The next level's goal: the goal the previous level had plus the profession's
   weight, doubled for every level up to ten and grown ten percent per level
   above it. */
// FUNCTION: WIZ8 0x004ef090
void CalcXPGoal(W8Character* character)
{
    int weight;
    unsigned int value;

    if (character == 0) {
        srAssertFail("pPC != NULL", GAMEPLAY_CODE_CPP, 0x725, 0);
    }
    if (character->level == 0) {
        srAssertFail("pPC->uiExpLevel > 0", GAMEPLAY_CODE_CPP, 0x726, 0);
    }
    if (character->current_profession < 0 ||
        character->current_profession >= W8_PROFESSION_COUNT) {
        srAssertFail("(pPC->iProfession >= 0) && (pPC->iProfession < PROF_COUNT)",
                     GAMEPLAY_CODE_CPP, 0x727, 0);
    }
    switch (character->current_profession) {
    case 0:
    case 7:
    case 8:
    case 9:
        weight = 1000;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
        weight = 0x578;
        break;
    case 5:
    case 0xc:
        weight = 0x640;
        break;
    case 10:
    case 0xb:
    case 0xd:
    case 0xe:
        weight = 0x4b0;
        break;
    default:
        srAssertFail("FALSE", GAMEPLAY_CODE_CPP, 0x74c,
                     "CalcXPGoal: ERROR - Invalid profession");
        return;
    }

    if (character->level == 1) {
        character->experience_goal = character->experience_previous_goal + weight;
        return;
    }
    if (character->level <= 10) {
        character->experience_goal = character->experience_previous_goal +
            IntegerPower(2, character->level - 2) * weight;
        return;
    }
    value = IntegerPower(2, 8) * weight;
    unsigned int current = 10;
    do {
        value = value * 12 / 10;
        ++current;
    } while (current < character->level);
    character->experience_goal = character->experience_previous_goal + value;
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

/* Look up the value that sex, race and profession together select. */
// FUNCTION: WIZ8 0x004ef950
void CalcCharacterTableValue(W8Character* character)
{
    if (character->race > 10) {
        srAssertFail("pPC->iRace < PC_RACE_COUNT", GAMEPLAY_CODE_CPP, 2359, 0);
    }
    character->table_value_0079 =
        g_character_table_00616604[(character->gender * 0x10 + character->race) * W8_PROFESSION_COUNT +
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

    for (profession = 0; profession < W8_PROFESSION_COUNT; ++profession) {
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
        character->bonus_1770.value_00;

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

// FUNCTION: WIZ8 0x004ee220
void CalcAttacks(W8Character* character)
{
    W8HandAttack* attacks[2];
    W8ItemDatabaseRecord* records[2];
    W8ItemInstance* equipment[2];
    unsigned int hand;
    int physical_experience;
    int load_penalty = 0;

    for (hand = 0; hand < 2; ++hand) {
        SetHandType(character, hand + 6);
        attacks[hand] = &character->hand_attacks[hand];
        equipment[hand] = &character->equipment[hand + 6];
        if (equipment[hand]->item_id == -1) {
            records[hand] = 0;
            attacks[hand]->weapon_skill = 14;
        }
        else {
            records[hand] = &g_item_records[equipment[hand]->item_id];
            attacks[hand]->weapon_skill = records[hand]->weapon_skill;
        }
    }

    for (hand = 0; hand < 2; ++hand) {
        W8HandAttack* attack = attacks[hand];
        attack->in_play = 1;
        switch (attack->weapon_skill) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 14:
            attack->combat_skill = 16;
            break;
        case 7:
        case 8:
        case 9:
            attack->combat_skill = 17;
            if (ItemHasSingledOutGenericName(equipment[hand]->item_id) &&
                (equipment[hand == 0]->item_id == -1 ||
                 !CompatiblePartnerItems(
                     equipment[hand]->item_id,
                     equipment[hand == 0]->item_id))) {
                attack->in_play = 0;
            }
            if (ItemHasQuantityKindFour(equipment[hand]->item_id) &&
                equipment[hand]->uses_or_charges == 0) {
                attack->in_play = 0;
            }
            break;
        default:
            attack->combat_skill = -1;
            attack->in_play = 0;
        }

        if (hand == 1) {
            if (attacks[0]->combat_skill != attacks[1]->combat_skill) {
                attacks[1]->in_play = 0;
            }
            if (attacks[1]->wield_kind == 2 ||
                attacks[1]->wield_kind == 3) {
                attacks[1]->in_play = 0;
            }
            if (records[0] != 0 && records[0]->unidentified_name_index == 0x83) {
                attacks[1]->in_play = 0;
            }
            if (attacks[1]->wield_kind == 0 && attacks[0]->wield_kind != 0) {
                attacks[1]->in_play = 0;
            }
        }
    }

    character->dual_wielding =
        attacks[0]->wield_kind == 1 && attacks[0]->in_play &&
        attacks[1]->wield_kind == 1 && attacks[1]->in_play;

    physical_experience = CalcPhysCombatExperience(character);
    for (hand = 0; hand < 2; ++hand) {
        W8HandAttack* attack = attacks[hand];
        W8HandAttack* other = attacks[hand == 0];
        int dual_penalty;
        int score;
        unsigned int divisor;

        if (!attack->in_play) {
            continue;
        }

        score = ((character->skills[attack->combat_skill].level +
                  character->skills[attack->weapon_skill].level * 2) * 2) / 3;
        divisor = 20;
        if (other->wield_kind == 1 && other->in_play) {
            score += character->skills[other->weapon_skill].level >> 1;
            divisor = 25;
        }
        if (character->dual_wielding) {
            score += character->skills[18].level;
            divisor += 10;
        }
        attack->combined_skill = score * 10 / divisor;

        switch (character->load_category) {
        case 0:
            load_penalty = 0;
            break;
        case 1:
            load_penalty = -15;
            break;
        case 2:
            load_penalty = -30;
            break;
        case 3:
            load_penalty = -60;
            break;
        case 4:
            load_penalty = -120;
            break;
        default:
            srAssertFail("FALSE", GAMEPLAY_CODE_CPP, 715,
                         "CalcAttacks: ERROR - Invalid load category");
        }
        if (attack->weapon_skill == 7) {
            load_penalty /= 2;
        }

        if (character->dual_wielding) {
            dual_penalty = -10 * (hand + 1) -
                           (100 - character->skills[18].level) / 4;
        }
        else {
            dual_penalty = 0;
        }

        attack->attack_score =
            (dual_penalty + character->attributes[4].effective / 2 +
             attack->combined_skill * 2 + physical_experience) / 3 + 60;
        if (character->in_party && character->race == 15) {
            unsigned int party_slot = CharacterPointerToPartySlot(character);
            W8NpcState* npc = GetNpcState(g_party_slot_rows[party_slot].animation_0fa);
            if (npc != 0 && npc->name_style == ' ' && !GetFact(0x44)) {
                attack->attack_score /= 2;
            }
        }

        score = (((character->attributes[5].effective +
                   character->attributes[4].effective) >> 1) +
                 dual_penalty + physical_experience + load_penalty +
                 attack->combined_skill) / 3;
        attack->attacks = 1;
        if (hand == 0) {
            if (score > 49) {
                attack->attacks = 2;
                if (score > 99) {
                    attack->attacks = 3;
                }
            }
        }
        else if (score > 74) {
            attack->attacks = 2;
        }

        score = (character->attributes[5].effective +
                 attack->swings * 10 + dual_penalty +
                 physical_experience + load_penalty +
                 attack->combined_skill) / 3;
        attack->swings = 1;
        if (score > 66) {
            attack->swings = 2;
            if (score > 99) {
                attack->swings = 3;
            }
        }
        if (records[0] != 0 && records[0]->unidentified_name_index == 0x90) {
            attack->swings = 1;
        }

        attack->damage_bonus = 0;
        attack->hit_bonus = 0;
        attack->value_25 = 0;
        attack->value_29 = 0;
        attack->damage_dice.base = 0;
        attack->damage_dice.count = 0;
        attack->damage_dice.sides = 0;
        if (records[hand] != 0) {
            attack->damage_bonus += records[hand]->attack_damage_bonus;
            attack->hit_bonus += records[hand]->attack_hit_bonus;
            if (other->wield_kind == 3 && records[hand == 0] != 0) {
                attack->hit_bonus += records[hand == 0]->attack_hit_bonus;
            }
            if (ItemHasSingledOutGenericName(equipment[hand]->item_id)) {
                attack->value_29 += records[hand]->damage_dice.base * 10;
            }
        }
        else {
            attack->damage_bonus += attack->combined_skill / 10;
            attack->attack_flags = 0x20;
            if (character->skills[attack->weapon_skill].level > 4) {
                attack->attack_flags = 0x60;
            }
            attack->damage_dice =
                g_unarmed_damage_dice[character->skills[attack->weapon_skill].level / 11];
            if (hand == 0) {
                attack->damage_dice.base += 2;
            }
            attack->value_33 = 0;
            attack->unknown_37[0] = 0;
            attack->unknown_37[1] = 0;
            attack->strength_bonus_39 = 0;
            attack->unknown_3a = 0;
            attack->value_3b = 0;
            attack->value_3f = 0;
            if (character->attributes[0].effective > 49) {
                attack->strength_bonus_39 =
                    (character->attributes[0].effective - 50) / 5;
            }
        }

        divisor = 1;
        if (records[hand] != 0 &&
            (records[hand]->attack_flags_04e & 0xfe6f) == 0) {
            switch (records[hand]->unidentified_name_index) {
            case 0x68:
            case 0x6e:
            case 0x72:
            case 0x83:
            case 0x90:
                break;
            default:
                divisor = 2;
            }
        }

        if (character->attributes[0].effective < 50) {
            attack->hit_bonus -=
                (50 - character->attributes[0].effective) / (divisor * 10);
            attack->value_29 -=
                (50 - character->attributes[0].effective) / divisor;
        }
        else if (character->attributes[0].effective > 50) {
            divisor *= hand + 1;
            attack->hit_bonus +=
                (character->attributes[0].effective - 50) / (divisor * 10);
            attack->value_29 +=
                (character->attributes[0].effective * 2 - 100) / divisor;
        }

        if (character->attributes[4].effective < 50) {
            attack->hit_bonus -= (50 - character->attributes[4].effective) / 10;
        }
        else if (character->attributes[4].effective > 50) {
            attack->hit_bonus += (character->attributes[4].effective - 50) / 10;
        }
        if (character->attributes[6].effective < 30) {
            attack->hit_bonus -= (30 - character->attributes[6].effective) / 10;
        }
        else if (character->attributes[6].effective > 70) {
            attack->hit_bonus += (character->attributes[6].effective - 70) / 10;
        }

        switch (character->load_category) {
        case 0:
            load_penalty = 0;
            break;
        case 1:
            load_penalty = -1;
            break;
        case 2:
            load_penalty = -2;
            break;
        case 3:
            load_penalty = -4;
            break;
        case 4:
            load_penalty = -8;
            break;
        }
        if (hand == 1) {
            load_penalty = load_penalty * 3 / 2;
        }
        if (attack->weapon_skill == 7) {
            load_penalty /= 2;
        }
        attack->hit_bonus += load_penalty;
        if (character->skills[40].flag_00 && attack->combat_skill == 17) {
            attack->hit_bonus += character->skills[40].level / 20 + 1;
        }
        if (character->skills[34].flag_00 && attack->combat_skill == 16) {
            attack->hit_bonus += character->skills[34].level / 20 + 1;
        }
    }
}

/* Rebuild the thirteen shared and location-specific armor-class components,
   then derive the unweighted and body-location-weighted summaries. */
// FUNCTION: WIZ8 0x004ee9d0
void CalcArmorClasses(W8Character* character)
{
    bool defensive_action = false;
    if (gXStatus.fCombatMode) {
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
        if (CharacterHasTrait00547940(character, 0x16)) {
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
            character->bonus_1770.armor_bonus_04;
        character->armor_class_components[8] +=
            static_cast<signed char>(character->bonus_1770.value_4b);
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
            character->bonus_1770.armor_bonus_05;
    }

    character->armor_class_total = 0;
    for (index = 0; index < 12; ++index) {
        if (index != 6) {
            character->armor_class_total +=
                character->armor_class_components[index];
        }
    }
    if (character->bonus_1770.out_of_formation
        && character->armor_class_total > -5) {
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

/* 0x006164F4: personality and voice values by sex and profession class,
   two dwords per row. It ends exactly where the sex/race/profession table
   at 0x00616604 begins. */
// GLOBAL: WIZ8 0x006164F4
const int g_character_value_table_006164f4[34][2] = {
    {0, 2}, {0, 1}, {6, 1}, {0, 1}, {0, 1}, {2, 2}, {0, 1}, {8, 1},
    {7, 2}, {7, 2}, {7, 2}, {7, 2}, {7, 2}, {7, 2}, {4, 1}, {4, 1},
    {1, 1}, {5, 2}, {7, 1}, {6, 2}, {6, 2}, {6, 1}, {3, 2}, {3, 2},
    {5, 1}, {1, 1}, {1, 2}, {5, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 1},
    {2, 2}, {2, 2}
};

/* Derive the character's personality and voice from sex and profession.
   Unaligned characters pick a class through the race shortcut first. */
// FUNCTION: WIZ8 0x004EFA30
void DeriveCharacterPersonality004EFA30(W8Character* character)
{
    int gender = character->gender;
    int value = character->current_profession;
    int index;
    int flag;

    if (gender == 0) {
        switch (value) {
        case 0:
        case 1:
        case 3:
        case 7:
            if (character->race == 2) {
                value = 0x10;
            }
            else if (character->race == 6) {
                value = 0x0f;
            }
            break;
        }
    }
    index = gender + value * 2;
    flag = g_character_value_table_006164f4[index][1];
    character->personality_0081 = g_character_value_table_006164f4[index][0];
    if (flag == 1) {
        character->voice_0085 = 0;
    }
    else {
        character->voice_0085 = 1;
    }
    character->unknown_007d = 0;
}

// FUNCTION: WIZ8 0x004ef420
unsigned int GetAveragePartyLevel(void)
{
    unsigned int total_level = 0;
    unsigned int occupied_slots = 0;
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (g_party_slot_rows[slot].occupied != 0) {
            total_level += g_party_characters[slot].level;
            ++occupied_slots;
        }
    }
    return total_level / occupied_slots;
}

/* 0x00684000: one twelve-byte record per party slot; the join entry stamps
   the record's first byte once the game is already running. */
// GLOBAL: WIZ8 0x00684000
unsigned char g_party_slot_state_684000[8][0xc];

/* Add one character to the party: find a free slot in the requested band,
   copy the record, mark it in party, rebuild its row and formation position
   and enter it in the marching order. The band is 2..7 for a regular member
   and 0..1 for the two auxiliary slots. */
// FUNCTION: WIZ8 0x004ef4a0
int Function4EF4A0(W8Character* character, int slot_kind)
{
    unsigned int slot;

    if (slot_kind == -1) {
        slot = 2;
        while (g_party_slot_rows[slot].occupied != 0) {
            ++slot;
            if (slot > 7) {
                return -1;
            }
        }
    }
    else {
        slot = 0;
        while (g_party_slot_rows[slot].occupied != 0) {
            ++slot;
            if (slot > 1) {
                return -1;
            }
        }
    }
    if ((int)slot < 0) {
        return -1;
    }

    W8Character* destination = &g_party_characters[slot];
    memcpy(destination, character, sizeof(W8Character));
    destination->in_party = 1;
    ResetPartySlotRow(slot);
    Function54B300(slot);

    W8PartySlotRow* row = &g_party_slot_rows[slot];
    row->animation_0fa = slot_kind;
    for (unsigned int index = 0; index < 8; ++index) {
        if (g_status_685170.dwords_18e0[index] == (unsigned int)-1) {
            g_status_685170.dwords_18e0[index] = slot;
            row->party_order_0f1 = index;
            break;
        }
    }
    Function554AE0(&g_status_685170.formation, slot);
    g_status_685170.formation.positions[slot].unknown_01[0] = 0xff;

    if (g_status_685170.game_started != 0) {
        g_party_slot_state_684000[slot][0] = 0xff;
        PostCharacterNotice(slot, gppStringList[0x940 / 4]);
    }
    ++g_status_685170.unknown_000d[2];
    if (slot_kind == -1) {
        ++g_status_685170.unknown_000d[0];
    }
    else {
        ++g_status_685170.unknown_000d[1];
    }

    RebuildCharacterModifierBlock(destination);
    Function4ED9D0(destination);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        Function561EC0();
    }
    return slot;
}
