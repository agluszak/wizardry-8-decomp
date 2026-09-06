#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/xstatus.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"

extern void Function5477D0(W8MonsterInfo* monster_info, int flag);
extern void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
extern unsigned char g_flag_00683F94;
/* Condition-to-notice word table. Only the first word of each four-word
   stride is read, hence the multiplied index. */
extern unsigned short g_condition_notices_0061E570[];
extern wchar_t* GetMonsterName(
    W8MonsterInfo* monster_info, W8MonsterRecord* record, char arg_3);
extern char Function42B740(int saved_level);
extern int g_int_00686A70;
extern char Function521060(
    int id, int* out_id, W8Character** out_character, int a, int b);
extern W8Character* Function52C480(void);
extern void Function536570(int party_slot, int a, int b);
extern void Function52F790(void* character, int condition);
extern unsigned char Function4E79A0(int party_slot, int a, int b, int c);
extern void Function53A930(int party_slot, W8CombatSlot* target);
extern void Function547A50(int party_slot);
extern void Function5237E0(int party_slot);
extern unsigned char Function547940(const W8Character* character, int trait);
extern void Function53AEB0(int party_slot);
extern void Function52F430(void* character);
extern void Function590950(int party_slot, const wchar_t* format, ...);
extern bool FindItemOnCharacter(
    W8Character* character, int item_id, W8ItemInstance** found,
    int include_backpack, const W8ItemInstance* resume_after);
extern void SetTargetToCharacter(int character_slot, int context);
extern void CharacterDies(int party_slot);
extern void Function50E650(int party_slot);
extern unsigned char g_byte_00687500;
extern unsigned char g_enchantment_six_cleared_006840bb;

/* The enchantment slot whose clearing has a consequence beyond the slot
   itself. */
enum { W8_ENCHANTMENT_SLOT_SPECIAL = 6 };

/* The condition that death alone does not lift. */
enum { W8_CONDITION_SURVIVES_DEATH = 10 };

/* Lifting a character's condition clears its duration and any state that
   condition alone maintained, then notifies dependents. */
// FUNCTION: WIZ8 0x00523330
void RemoveCharacterCondition(int party_slot, int condition, int announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
    int found_id;
    W8Character* found_character;
    unsigned char can_rest;

    if (character->condition_turns[condition] != 0
        || g_byte_00687500 == 0) {
        if (row->occupied == 0) {
            srAssertFail(
                "fCHAR_OCCUPIED(uiChar)",
                "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                0xf4, 0);
        }
        if (character->condition_turns[condition] == 0) {
            srAssertFail(
                "gStatus.Char[uiChar].uiCondition[uiCondition] > 0",
                "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                0xf5, 0);
        }
        if (party_slot >= 0 && party_slot < 8 && row->occupied != 0
            && character->hp_current != 0) {
            can_rest = character->unknown_0b01 < 0xd;
        }
        else {
            can_rest = 0;
        }
        if (condition == 9) {
            if (character->condition_turns[W8_CONDITION_SURVIVES_DEATH] != 0
                && Function42B740(g_int_00686A70) != '\t'
                && Function42B740(g_int_00686A70) != '\n') {
                return;
            }
        }
        else if (condition == 0xb
                 && Function521060(0x243, &found_id, &found_character, 2, 0) != 0
                 && found_id != 0x6874CB) {
            if (found_character == 0) {
                found_character = Function52C480();
            }
            if (CharacterPointerToPartySlot(found_character)
                    == (unsigned int)party_slot
                && !FindItemOnCharacter(found_character, 0x239, 0, 0, 0)) {
                return;
            }
        }
        if (announce != 0) {
            Function590950(
                party_slot, gppStringList[0x90c / 4],
                gppStringList[g_condition_notices_0061E570[condition * 4]]);
        }
        character->condition_turns[condition] = 0;
        Function5237E0(party_slot);
        switch (condition) {
        case 1:
            character->hp_adjustment = 0;
            *(int*)character->unknown_0b21 = 0;
            break;
        case 7:
            character->condition_argument = 0;
            break;
        case 9:
        case 0xC:
            g_enchantment_six_cleared_006840bb = 1;
            break;
        case 0xb:
            if (g_flag_00683F94 != 0
                && ((unsigned char*)g_combat_state)[0x98 + party_slot * 0xD4] != 0) {
                row->target_out_of_combat = row->target_in_combat;
            }
            break;
        case 0xd:
            SetTargetToCharacter(party_slot, 1);
            Function536570(party_slot, 0, 0);
            Function536570(party_slot, 1, 0);
            break;
        }
        Function50E650(party_slot);
        Function52F790(character, condition);
        if (!can_rest && party_slot > -1 && party_slot < 8
            && row->occupied != 0 && character->hp_current != 0
            && character->unknown_0b01 < 0xd && g_flag_00683F94 != 0
            && Function4E79A0(party_slot, 1, 0, 0) != 0) {
            Function53A930(party_slot, &row->target_in_combat);
        }
    }
}
extern void ResetTargetSource(W8TargetSource* target_block);               /* 0x00536150 */
extern void NotifySpellPointsChanged(int party_slot);                    /* 0x0055EE30 */
extern void Function4ACD80(W8Monster* monster, int slot, int arg_3);
extern void Function50E8C0(int location_id);
extern void Function50E650(int party_slot);
extern void RefreshMonsterSight(W8MonsterInfo* monster_info);
extern void ResetCombatSlot(W8CombatSlot* slot);
extern void RequestRedraw(int mask);
extern unsigned char g_enchantment_six_cleared_006840bb;

/*
 * Original translation unit: Local Code\Conditions & Enchantments.cpp.
 *
 * Characters and monsters carry the same two arrays: twenty condition
 * durations and eight enchantment slots. The bodies here move conditions
 * between the two, lift them one at a time or all at once, and keep the
 * highest enchantment slot in use up to date as slots empty.
 */

#define CONDITIONS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp"

/* Keep the live quantity in the byte selected by the database record. Stack
   items use stack_count; charge and use-count items use uses_or_charges. */
// FUNCTION: WIZ8 0x00522e80
void NormalizeItemQuantityKind(W8ItemInstance* item)
{
    unsigned char quantity_kind;

    if (item->item_id == -1) {
        return;
    }

    quantity_kind = g_item_records[item->item_id].quantity_kind;
    if (quantity_kind != 1) {
        if (quantity_kind <= 1 || quantity_kind > 4 ||
            item->stack_count <= 0) {
            return;
        }
        if (item->uses_or_charges > 0) {
            item->stack_count = 0;
            return;
        }
        item->uses_or_charges = item->stack_count;
        item->stack_count = 0;
        return;
    }

    if (item->uses_or_charges > 0) {
        if (item->stack_count > 0) {
            item->uses_or_charges = 0;
            return;
        }
        item->stack_count = item->uses_or_charges;
        item->uses_or_charges = 0;
        return;
    }
    if (item->stack_count == 0) {
        item->stack_count = 1;
    }
}

/* Condition immunity sets by monster kind. Each entry is a kind byte followed
   by twenty condition ids; a match means the condition never lands. The
   two-byte packing is load-bearing: padded to four the stride would be 0x54,
   but the table walks 0x52 per entry. Values are the retail table at
   0x006171AA. */
#pragma pack(push, 2)
typedef struct W8ConditionImmunity {
    unsigned char kind;
    unsigned char pad;
    int conditions[20];
} W8ConditionImmunity;                          /* 0x52 */
#pragma pack(pop)

// GLOBAL: WIZ8 0x006171AA
W8ConditionImmunity g_condition_immunities_006171AA[3] = {
    {20, 0, {2, 7, 17, 3, 6, 11, 15, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {22, 0, {2, 7, 4, 3, 6, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {17, 0, {11, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

/* Setting a monster's condition runs the sameCountdown rescan, group recount
   and motion update as clearing it, plus immunity, argument and combat-hate
   handling on the way in. */
// FUNCTION: WIZ8 0x00523C00
void SetMonsterCondition(
    int location_id, int condition, int duration, int argument,
    W8TargetSource* target, int quiet)
{
    unsigned int list_index;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8MonsterGroup* monster_group;
    W8ConditionImmunity* immunity;
    unsigned char kind;
    int index;
    int old_duration;
    int slot;
    unsigned char handled;

    if (argument != 0 && condition != W8_CONDITION_WITH_ARGUMENT) {
        srAssertFail(
            "(uiPoisonStrength == 0) || (uiCondition == COND_POISONED)",
            "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            0x220, 0);
    }
    if (condition == W8_CONDITION_WITH_ARGUMENT && argument == 0) {
        return;
    }
    switch (condition) {
    case W8_CONDITION_FATIGUE_DOUBLED:
    case W8_CONDITION_SURVIVES_DEATH:
    case 0x12:
    case 0x13:
        duration = W8_CONDITION_INDEFINITE;
        break;
    default:
        if (duration == 0) {
            return;
        }
        break;
    }
    list_index = MonsterGetIndexByLocationID(
        0x23c,
        "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
        location_id, 1);
    monster_info = MonsterGetScriptPartByLocationIndex(list_index);
    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->hp_current == 0) {
        return;
    }
    kind = record->kind_0cb;
    for (immunity = g_condition_immunities_006171AA;
         immunity < g_condition_immunities_006171AA + 3;
         ++immunity) {
        if (immunity->kind == kind) {
            for (index = 0; index < 0x14; ++index) {
                if (condition == immunity->conditions[index]) {
                    return;
                }
            }
        }
    }
    old_duration = monster_info->condition_turns[condition];
    if (old_duration < duration) {
        monster_info->condition_turns[condition] = duration;
        if (old_duration == 0) {
            if (condition == 9 || condition == 0xC) {
                RefreshMonsterSight(monster_info);
            }
            else if (condition == W8_CONDITION_HOSTILE) {
                if (monster_info->flag_16 == 0) {
                    monster_info->condition_turns[W8_CONDITION_HOSTILE] = 0;
                    return;
                }
                Function5477D0(
                    monster_info, (monster_info->flag_16 == 1) + 1);
            }
        }
        slot = 0x13;
        while (monster_info->condition_turns[slot] == 0) {
            if (slot == 0) {
                break;
            }
            --slot;
        }
        monster_info->value_107 = slot;
        list_index = GetMonsterGroupIndexByID(
            0x34e,
            "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            monster_info->monster_group_id, 1);
        monster_group = GetMonsterGroupByListIndex(list_index);
        RecountActiveMonsterGroupMembers(monster_group);
        if (monster_info->flag_14 != 0) {
            MonsterInfoSetMotionless(
                monster_info,
                (unsigned int)monster_info->value_107 < 0xE ? 0 : 1);
        }
        if (old_duration == 0 && condition != 0 && condition <= 0x12) {
            Function4ACD80(monster_info->monster, condition - 1, 1);
        }
        if (monster_info->fInCombat != 0
            && TargetSourceIsCharacter(target, 0) != 0
            && target->iChar != -1) {
            int* hate = (int*)&monster_info->pCombat->unknown_01a[target->iChar * 4];
            record = GetMonsterDataForInfo(monster_info);
            *hate += (record->missile_value_24f * (unsigned int)condition) / 3;
        }
        handled = 1;
    }
    else {
        handled = 0;
    }
    if (TargetSourceIsCharacter(target, 0) != 0
        || TargetSourceIsMonster(target, 0) != 0) {
        if (target->fBackfire == 0 && target->fReflection == 0
            && target->unknown_1d[1] == 0) {
            for (index = 0; index < 13; ++index) {
                ((int*)&monster_info->unknown_301[3])[index] =
                    ((const int*)target)[index];
            }
        }
    }
    if ((unsigned int)argument > (unsigned int)monster_info->condition_argument) {
        monster_info->condition_argument = argument;
        handled = 1;
    }
    Function50E8C0(location_id);
    if (handled == 0) {
        return;
    }
    if ((unsigned int)condition >= 0xD) {
        ResetCombatSlot(&monster_info->combat_slot_2ba);
    }
    if ((unsigned int)condition >= 0x12) {
        MonsterStartsDying(monster_info, quiet);
        return;
    }
    if (quiet != 0
        && (g_flag_00683F94 != 0 || monster_info->flag_2ab != 0)) {
        wchar_t* name = GetMonsterName(monster_info, 0, 0);
        WriteGameLog(9, L"%s %s!", name, g_condition_notices_0061E570[condition * 4]);
    }
    if (monster_info->monster->IsCycleInterruptable(
            monster_info->monster->m_pRep->pending_cycle) != 0) {
        StartMonsterCycle(monster_info, 0x14, 1);
    }
}
/* Clearing a monster's condition also re-derives its highest set condition
   index, recounts its group, and possibly stops it moving. */
// FUNCTION: WIZ8 0x00523F40
void ClearMonsterCondition(int location_id, int condition)
{
    unsigned int list_index;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* monster_group;
    int slot;

    list_index = MonsterGetIndexByLocationID(
        0x2d0,
        "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
        location_id, 1);
    monster_info = MonsterGetScriptPartByLocationIndex(list_index);
    if (monster_info->hp_current != 0) {
        if (monster_info->condition_turns[condition] == 0) {
            srAssertFail(
                "pMonsterInfo->uiCondition[uiCondition] > 0",
                "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                0x2d8, 0);
        }
        if (condition == 0xd && monster_info->condition_turns[0xd] != 0) {
            list_index = GetMonsterGroupIndexByID(
                0x2e0,
                "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                monster_info->monster_group_id, 1);
            monster_group = GetMonsterGroupByListIndex(list_index);
            Function5477D0(monster_info, monster_group->flag_2a);
        }
        if (g_flag_00683F94 != 0 || monster_info->flag_2ab != 0) {
            WriteGameLog(
                9, gppStringList[0x910 / 4],
                GetMonsterName(monster_info, 0, 0),
                g_condition_notices_0061E570[condition * 4]);
        }
        monster_info->condition_turns[condition] = 0;
        slot = 0x13;
        while (monster_info->condition_turns[slot] == 0) {
            if (slot == 0) {
                break;
            }
            --slot;
        }
        monster_info->value_107 = slot;
        list_index = GetMonsterGroupIndexByID(
            0x34e,
            "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            monster_info->monster_group_id, 1);
        monster_group = GetMonsterGroupByListIndex(list_index);
        RecountActiveMonsterGroupMembers(monster_group);
        if (monster_info->flag_14 != 0) {
            MonsterInfoSetMotionless(
                monster_info, (unsigned int)monster_info->value_107 < 0xE ? 0 : 1);
        }
        if (condition != 0 && condition < 0x13) {
            Function4ACD80(monster_info->monster, condition - 1, 0);
        }
        switch (condition) {
        case 6:
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->unknown_13d[0xE] = 0;
            }
            break;
        case 7:
            monster_info->condition_argument = 0;
            Function50E8C0(location_id);
            return;
        case 9:
        case 0xC:
            RefreshMonsterSight(monster_info);
            Function50E8C0(location_id);
            return;
        }
        Function50E8C0(location_id);
    }
}

/* Setting a character's condition runs poison/immunity gates, the duration
   switch, trait gates, and the same rescan/notify/tail handling as the other
   transitions. */
// FUNCTION: WIZ8 0x00522FE0
unsigned char SetCharacterCondition(
    int party_slot, int condition, int duration, int argument,
    char value_5, char value_6)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
    int old_b01;
    unsigned int old_duration;
    unsigned char handled;

    if (row->occupied == 0) {
        srAssertFail(
            "fCHAR_OCCUPIED(uiChar)",
            "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            0x2a, 0);
    }
    if (argument != 0 && condition != W8_CONDITION_WITH_ARGUMENT) {
        srAssertFail(
            "(uiPoisonStrength == 0) || (uiCondition == COND_POISONED)",
            "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            0x2d, 0);
    }
    if (condition == W8_CONDITION_WITH_ARGUMENT && argument == 0) {
        return 0;
    }
    if (condition == 0x12
        && Function547940(character, 2) != 0
        && character->condition_turns[17] < 7) {
        Function547A50(party_slot);
        return 0;
    }
    switch (condition) {
    case W8_CONDITION_FATIGUE_DOUBLED:
    case W8_CONDITION_SURVIVES_DEATH:
    case 0x12:
    case 0x13:
        duration = W8_CONDITION_INDEFINITE;
        break;
    default:
        if (duration == 0) {
            return 0;
        }
        break;
    }
    if (g_byte_00687500 != 0) {
        Function590950(
            party_slot, gppStringList[0x908 / 4],
            gppStringList[g_condition_notices_0061E570[condition * 4]]);
        return 0;
    }
    switch (condition) {
    case 6:
        if (Function547940(character, 3) != 0) {
            Function590950(party_slot, gppStringList[0x600 / 4]);
            return 0;
        }
        /* fall through */
    case 2:
    case 3:
    case 4:
    case 7:
    case 0xf:
        if (Function547940(character, 0x1e) != 0) {
            return 0;
        }
        break;
    case 0xb:
        if (duration == 9999) {
            break;
        }
        /* fall through */
    case 0xd:
        if (Function547940(character, 0xe) != 0) {
            Function590950(party_slot, gppStringList[0x604 / 4]);
            return 0;
        }
        break;
    }
    old_b01 = character->unknown_0b01;
    old_duration = character->condition_turns[condition];
    if (old_duration < (unsigned int)duration) {
        character->condition_turns[condition] = duration;
        if (old_duration == 0) {
            if (condition == 9 || condition == 0xC) {
                g_enchantment_six_cleared_006840bb = 1;
            }
            else if (condition == 0xd) {
                SetTargetToCharacter(party_slot, 1);
            }
        }
        handled = 1;
    }
    else {
        handled = 0;
    }
    if ((unsigned int)character->condition_argument < (unsigned int)argument) {
        character->condition_argument = argument;
        handled = 1;
    }
    Function50E650(party_slot);
    if (handled == 0) {
        return 0;
    }
    if (condition == 0x12) {
        CharacterDies(party_slot);
    }
    else if (!(condition < 0x12)) {
        SetTargetToCharacter(party_slot, 0);
    }
    if (old_b01 != character->unknown_0b01) {
        Function52F430(character);
    }
    if (value_6 != 0) {
        if (condition == 0x13 && value_5 != 0) {
            Function590950(party_slot, gppStringList[0x754 / 4]);
        }
        else {
            Function590950(
                party_slot, L"%s!",
                gppStringList[g_condition_notices_0061E570[condition * 4]]);
        }
    }
    if ((party_slot < 0 || party_slot > 7 || row->occupied == 0
         || character->hp_current == 0 || character->unknown_0b01 > 0xC)
        && g_flag_00683F94 != 0) {
        Function53AEB0(party_slot);
    }
    return 1;
}

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
        if (g_party_slot_rows[party_slot].occupied != 0 &&
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
        if (g_party_slot_rows[party_slot].occupied != 0 &&
            g_party_characters[party_slot].condition_turns[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 1);
        }
    }

    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
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
            if (g_party_slot_rows[party_slot].occupied != 0 &&
                g_party_characters[party_slot].condition_turns[condition] != 0) {
                RemoveCharacterCondition(party_slot, condition, 1);
            }
        }
    }
}
