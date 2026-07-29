#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/*
 * Original translation unit: Local Code\Conditions & Enchantments.cpp.
 *
 * Characters and monsters carry the same two arrays: twenty condition
 * durations and eight enchantment slots. The bodies here move conditions
 * between the two, lift them one at a time or all at once, and keep the
 * highest enchantment slot in use up to date as slots empty.
 */

#define CONDITIONS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp"

extern void SetMonsterCondition(
    int location_id, int condition, int duration, int argument, W8TargetSource* target_block, int quiet);
/* 0x00523C00 */
extern void ClearMonsterCondition(int location_id, int condition);       /* 0x00523F40 */
extern void SetCharacterCondition(
    int party_slot, int condition, int duration, int argument, int arg_5, int arg_6);
extern void RemoveCharacterCondition(int party_slot, int condition, int arg_3);
extern void ResetTargetSource(W8TargetSource* target_block);               /* 0x00536150 */
extern int CharacterPointerToPartySlot(const W8Character* character);
extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id, unsigned char assert_on_failure);
extern void NotifySpellPointsChanged(int party_slot);                    /* 0x0055EE30 */
extern void Function4ACD80(W8Monster* monster, int slot, int arg_3);
extern void Function50E8C0(int location_id);
extern void Function50E650(int party_slot);
extern void RefreshMonsterSight(W8MonsterInfo* monster_info);
extern void RequestRedraw(int mask);
extern unsigned char g_enchantment_six_cleared_006840bb;

/* The enchantment slot whose clearing has a consequence beyond the slot
   itself. */
enum { W8_ENCHANTMENT_SLOT_SPECIAL = 6 };

/* The condition that death alone does not lift. */
enum { W8_CONDITION_SURVIVES_DEATH = 10 };

/* Copy every condition a character is under onto something else in the world.
   Condition seven carries an argument alongside its duration, so it is the one
   entry that is not just a duration. */
// FUNCTION: WIZ8 0x005241e0
void CopyCharacterConditionsToTarget(const W8Character* character, const int* target)
{
    W8TargetSource target_block;
    unsigned int condition;
    int duration;
    int argument;

    ResetTargetSource(&target_block);
    for (condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
        duration = character->condition_turns[condition];
        if (duration != 0) {
            if (condition == W8_CONDITION_WITH_ARGUMENT) {
                argument = character->condition_argument;
                duration = character->condition_turns[W8_CONDITION_WITH_ARGUMENT];
            }
            else {
                argument = 0;
            }
            SetMonsterCondition(*target, condition, duration, argument, &target_block, 0);
        }
    }
}

/* The same copy the other way round, from a monster onto one character. Every
   condition carries the monster's argument here, not only the seventh. */
// FUNCTION: WIZ8 0x00524250
void CopyMonsterConditionsToCharacter(int party_slot, const W8MonsterInfo* monster_info)
{
    unsigned int condition;
    int duration;
    int argument;

    for (condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
        duration = monster_info->condition_turns[condition];
        if (duration != 0) {
            argument = monster_info->condition_argument;
            if (condition == W8_CONDITION_WITH_ARGUMENT) {
                duration = monster_info->condition_turns[W8_CONDITION_WITH_ARGUMENT];
            }
            SetCharacterCondition(party_slot, condition, duration, argument, 0, 0);
        }
    }
}

/* Empty one of a character's enchantment slots and find the highest one still
   in use, scanning down from the last. Emptying the sixth also raises the flag
   the interface watches. */
// FUNCTION: WIZ8 0x00523a80
void ClearCharacterEnchantmentSlot(int party_slot, int slot)
{
    W8Character* character = &g_party_characters[party_slot];
    int scan;

    character->enchantments[slot].value_00 = 0;
    character->enchantments[slot].value_04 = 0;
    character->enchantments[slot].value_08 = 0;

    for (scan = 7; scan >= 0; --scan) {
        if (character->enchantments[scan].value_08 != 0) {
            character->enchantment_top = scan;
            break;
        }
    }

    NotifySpellPointsChanged(party_slot);
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(0x200000);
        RequestRedraw(0x8000);
    }
    Function50E650(party_slot);
    if (slot == W8_ENCHANTMENT_SLOT_SPECIAL) {
        g_enchantment_six_cleared_006840bb = 1;
    }
}

/* Empty one of a monster's enchantment slots and tell the live engine object
   that the matching effect is over. */
// FUNCTION: WIZ8 0x00524390
void ClearMonsterEnchantmentSlot(int location_id, int slot)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(948, CONDITIONS_CPP, location_id, 1));

    monster_info->enchantments[slot].value_00 = 0;
    monster_info->enchantments[slot].value_04 = 0;
    monster_info->enchantments[slot].value_08 = 0;
    Function4ACD80(monster_info->monster, slot + 0x10, 0);
    Function50E8C0(location_id);
    if (slot == W8_ENCHANTMENT_SLOT_SPECIAL) {
        RefreshMonsterSight(monster_info);
    }
}

/* Run one of a monster's enchantment slots down by the given number of turns,
   emptying it when nothing is left. The look-up is repeated rather than
   reused, which is what the two separate index calls show. */
// FUNCTION: WIZ8 0x00524400
void TickMonsterEnchantmentSlot(int location_id, int slot, unsigned int turns)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(969, CONDITIONS_CPP, location_id, 1));
    unsigned int remaining = monster_info->enchantments[slot].value_08;

    if (turns < remaining) {
        monster_info->enchantments[slot].value_08 = remaining - turns;
        return;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(948, CONDITIONS_CPP, location_id, 1));
    monster_info->enchantments[slot].value_00 = 0;
    monster_info->enchantments[slot].value_04 = 0;
    monster_info->enchantments[slot].value_08 = 0;
    Function4ACD80(monster_info->monster, slot + 0x10, 0);
    Function50E8C0(location_id);
    if (slot == W8_ENCHANTMENT_SLOT_SPECIAL) {
        RefreshMonsterSight(monster_info);
    }
}

/* Lift one condition from everybody in the party who is under it. */
// FUNCTION: WIZ8 0x005246c0
void RemoveConditionFromParty(int condition)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].condition_turns[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 1);
        }
    }
}

/* Lift one condition from everybody in the world - the party first, then every
   live monster. The monster count is re-read every iteration because lifting a
   condition can remove one. */
// FUNCTION: WIZ8 0x005244a0
void RemoveConditionFromEveryone(int condition)
{
    int party_slot;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].condition_turns[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 1);
        }
    }

    for (monster_index = 0; monster_index < PListGetCount(g_active_monster_list_00683fad);
         ++monster_index) {
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->condition_turns[condition] != 0) {
            ClearMonsterCondition(monster_info->location_id, condition);
        }
    }
}

/* Lift every condition from the whole party. The tenth is left alone, the same
   one death leaves alone, and the last two of the twenty are outside the sweep
   entirely. */
// FUNCTION: WIZ8 0x00524720
void RemoveAllConditionsFromParty(void)
{
    unsigned int condition;
    int party_slot;

    for (condition = 0; condition < W8_CONDITION_CLEARABLE_COUNT; ++condition) {
        if (condition == W8_CONDITION_SURVIVES_DEATH) {
            continue;
        }
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if (g_party_slot_rows[party_slot].flag_00 != 0 &&
                g_party_characters[party_slot].condition_turns[condition] != 0) {
                RemoveCharacterCondition(party_slot, condition, 1);
            }
        }
    }
}
