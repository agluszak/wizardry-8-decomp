#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/sr_api.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/utility.h"

/*
 * Local Code\Combat Hostility.cpp.
 *
 * Whether two monsters count as hostile to each other: same-species
 * short-circuit, disposition-band equality, and the faction records behind
 * them.
 */

static const char COMBAT_HOSTILITY_CPP[] =
    "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp";

/* Species 0x224 never counts: both directions answer zero before anything
   else is read. */
enum { W8_NEUTRAL_SPECIES_224 = 0x224 };

// FUNCTION: WIZ8 0x00546e70
void RecountCombatMonsters(void)
{
    gXStatus.hostile_monster_count = 0;
    gXStatus.hostile_group_count = 0;
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fActive && monster->fInCombat) {
            if (monster->ubDisposition == DISP_HOSTILE) {
                ++gXStatus.hostile_monster_count;
            }
            if (monster->condition_turns[13] != 0) {
            }
        }
    }
    if (gXStatus.fCombatMode && gXStatus.hostile_monster_count != 0) {
        g_combat_state->flag_a54 = 1;
    }
    RequestRefreshPartyState();
}

/* Compare two monsters for hostility. Equal disposition bands answer two;
   either band clear answers zero; otherwise the faction records decide, and
   only matching non-zero factions fall through to the condition-thirteen
   presence test. */
// FUNCTION: WIZ8 0x00546F80
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second)
{
    W8MonsterRecord* first_record;
    W8MonsterRecord* second_record;
    unsigned int first_faction;
    unsigned int second_faction;

    if (first->monster_species == W8_NEUTRAL_SPECIES_224 ||
        second->monster_species == W8_NEUTRAL_SPECIES_224) {
        return 0;
    }
    if (first->ubDisposition == second->ubDisposition) {
        return 2;
    }
    if (first->ubDisposition == 0 || second->ubDisposition == 0) {
        return 0;
    }
    first_record = GetMonsterDataForInfo(first);
    second_record = GetMonsterDataForInfo(second);
    first_faction = first_record->faction_id_25f;
    if (first_faction == 0) {
        return 1;
    }
    second_faction = second_record->faction_id_25f;
    if (second_faction == 0 || first_faction != second_faction) {
        return 1;
    }
    if ((first->condition_turns[13] != 0) == (second->condition_turns[13] != 0)) {
        return 0;
    }
    return 1;
}

/* Map a monster's disposition band onto the party character: while the
   character carries condition thirteen the hostile and friendly bands swap;
   otherwise the monster's band is returned unchanged. */
// FUNCTION: WIZ8 0x00546f10
char MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info)
{
    unsigned char disposition;

    if (g_status_685170.buffers.characters[character_slot].condition_turns[13] == 0) {
        return monster_info->ubDisposition;
    }
    disposition = monster_info->ubDisposition;
    switch (disposition) {
    case 0:
        return 0;
    case 1:
        return 2;
    case 2:
        return 1;
    default:
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp", 0x69,
                     "MonsterVsCharDisposition: ERROR - Invalid attack mode");
        return 0;
    }
}

// FUNCTION: WIZ8 0x00547010
char CharacterVsCharacterDisposition(int first, int second)
{
    W8Character* characters = g_status_685170.buffers.characters;
    unsigned int first_turns = characters[first].condition_turns[13];
    if (first_turns == 0 && characters[second].condition_turns[13] == 0) {
        return DISP_FRIENDLY;
    }
    if (first_turns == 0 || characters[second].condition_turns[13] == 0) {
        return DISP_HOSTILE;
    }
    return DISP_FRIENDLY;
}

// FUNCTION: WIZ8 0x00547080
char GetOppositeDisposition(W8TargetSource* source)
{
    if (TargetSourceIsCharacter(source, 0)) {
        if (g_status_685170.buffers.characters[source->iChar].condition_turns[13] == 0) {
            return DISP_HOSTILE;
        }
    } else if (TargetSourceIsMonster(source, 0)) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0xd3, COMBAT_HOSTILITY_CPP, source->iMonsterID, '\0');
        if (monster_list_index == 0xffffffff) {
            FormatDebugMessage(1, "GetOppositeDisposition - ERROR - can't find monster %d!",
                               source->iMonsterID);
            return 0;
        }
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info->condition_turns[13] == 0) {
            if (monster_info->ubDisposition == DISP_FRIENDLY) {
                return DISP_HOSTILE;
            }
            return DISP_FRIENDLY;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00547120
void ProvokeListedMonsterGroups(W8TargetSource* source, W8GrowableVector<int>* monsters)
{
    int own_monster_id = -1;
    if (GetOppositeDisposition(source) == '\0') {
        return;
    }
    if (TargetSourceIsMonster(source, 0)) {
        own_monster_id = source->iMonsterID;
    }
    W8CombatSlot target;
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_MONSTER;
    for (unsigned int index = 0; index < static_cast<unsigned int>(monsters->GetCount()); ++index) {
        int monster_id = *monsters->GetAt(index);
        if (monster_id == own_monster_id) {
            continue;
        }
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x122, COMBAT_HOSTILITY_CPP, monster_id, '\x01');
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info->monster_group_id != 0) {
            target.iMonsterID = monster_id;
            MakeTargetGroupHostile(source, &target);
        }
    }
}

// FUNCTION: WIZ8 0x005471d0
void MakeTargetGroupHostile(W8TargetSource* source, W8CombatSlot* target)
{
    char hostility = GetOppositeDisposition(source);
    if (hostility == '\0' || !IsTargetStillPresent(target)) {
        return;
    }
    int group_id;
    W8MonsterGroup* group;
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        if (target->iMonsterID == -1) {
            return;
        }
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x146, COMBAT_HOSTILITY_CPP, target->iMonsterID, '\x01');
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info->condition_turns[13] != 0) {
            return;
        }
        group_id = monster_info->monster_group_id;
    } else if (target->iType == W8_TARGET_KIND_GROUP) {
        if (target->iGroupID == -1) {
            return;
        }
        unsigned int group_list_index =
            GetMonsterGroupIndexByID(0x1ef, COMBAT_HOSTILITY_CPP, target->iGroupID, '\x01');
        group = GetMonsterGroupByListIndex(group_list_index);
        unsigned int index = 0;
        if (ILLength(group->monsters) == 0) {
            return;
        }
        while (true) {
            int monster_id = IListGetAt(group->monsters, index);
            unsigned int monster_list_index =
                MonsterGetIndexByLocationID(500, COMBAT_HOSTILITY_CPP, monster_id, '\x01');
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
            if (monster_info->condition_turns[13] == 0) {
                break;
            }
            if (ILLength(group->monsters) <= ++index) {
                return;
            }
        }
        group_id = target->iGroupID;
    } else {
        return;
    }
    if (group_id != -1) {
        unsigned int group_list_index =
            GetMonsterGroupIndexByID(0x167, COMBAT_HOSTILITY_CPP, group_id, '\x01');
        group = GetMonsterGroupByListIndex(group_list_index);
        SetMonsterGroupHostility(group, hostility, '\x01');
        if (group->fInCombat == '\0') {
            MonsterGroupEnterCombat(group);
        }
    }
}

/* Whether a party action aims at enemies: the four melee kinds do, as do
   spells (and item-spells) whose target type sits in the enemy band. */
// FUNCTION: WIZ8 0x00547310
unsigned char CharacterActionTargetsEnemies(W8Character* character, int action_kind,
                                            int action_detail, W8ActionDetailBlock* detail)
{
    W8ItemInstance* item;
    unsigned char spell_id;
    int target_type;

    switch (action_kind) {
    case W8_ACTION_ATTACK:
    case W8_ACTION_BERSERK:
    case W8_ACTION_BREATHE:
    case W8_ACTION_TURN_UNDEAD:
        return 1;
    case W8_ACTION_CAST_SPELL:
        if (action_detail > 0x95) {
            srAssertFail("iType < SPELL_COUNT",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp", 0x1c2, 0);
        }
        if (action_detail != 3 && action_detail != 0x29) {
            target_type = GetSpellTargetType(action_detail, 0);
            if (target_type > W8_TARGET_TYPE_PARTY && target_type < W8_TARGET_TYPE_POINT) {
                return 1;
            }
        }
        break;
    case W8_ACTION_USE_ITEM:
        item = detail->item_use.item;
        if (CanCharacterActivateItem(character, item) != 0) {
            spell_id = g_item_records[item->item_id].spell_id;
            if (spell_id != 0) {
                if (spell_id > 0x95) {
                    srAssertFail("iType < SPELL_COUNT",
                                 "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp",
                                 0x1c2, 0);
                }
                if (spell_id != 3 && spell_id != 0x29) {
                    target_type = GetSpellTargetType(spell_id, 0);
                    if (target_type > W8_TARGET_TYPE_PARTY && target_type < W8_TARGET_TYPE_POINT) {
                        return 1;
                    }
                }
            }
        }
        break;
    }
    return 0;
}

/* Monster actions 0 and 3 always count as enemy-aimed; action 2 is a spell id. */
// FUNCTION: WIZ8 0x00547440
unsigned char MonsterActionTargetsEnemies(int action_kind, int action_detail,
                                          unsigned int* spell_power_level)
{
    int target_type;

    if (action_kind == 0) {
        return 1;
    }
    if (action_kind == 2) {
        if (action_detail > 0x95) {
            srAssertFail("iType < SPELL_COUNT",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp", 0x1c2, 0);
        }
        if (action_detail != 3 && action_detail != 0x29) {
            target_type = GetSpellTargetType(action_detail, 0);
            if (target_type > W8_TARGET_TYPE_PARTY && target_type < W8_TARGET_TYPE_POINT) {
                return 1;
            }
        }
        return 0;
    }
    if (action_kind == 3) {
        return 1;
    }
    return 0;
}

/* Whether a spell id can be aimed by monster AI: inside the spell table, not
   one of the two self-only kinds, and carrying a middle target type. */
// FUNCTION: WIZ8 0x005474B0
bool MonsterCanAimSpell005474B0(int spell_id)
{
    if (spell_id > 0x95) {
        srAssertFail("iType < SPELL_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp", 0x1c2, 0);
    }
    if (spell_id != 3 && spell_id != 0x29) {
        int target_type = GetSpellTargetType(spell_id, 0);
        if (target_type > W8_TARGET_TYPE_PARTY && target_type < W8_TARGET_TYPE_POINT) {
            return 1;
        }
        return 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00547510
bool CombatAllowsLiveGroups(void)
{
    return gXStatus.fCombatMode != 0 && g_combat_state->flag_a54 == 0 &&
           g_combat_state->value_004 <= 1;
}

// GLOBAL: WIZ8 0x0061ec0c
const unsigned short g_group_hostility_notice_ids[3] = {511, 512, 513};

// GLOBAL: WIZ8 0x0061ec14
const int g_monster_special_attack_name_ids_61ec14[12] = {0,    1598, 1599, 1600, 1601, 1602,
                                                          1603, 1604, 1605, 1606, 1607, 1608};

// FUNCTION: WIZ8 0x00547540
void SetMonsterGroupHostilityByID(int group_id, unsigned int hostility, char recurse)
{
    unsigned int group_list_index =
        GetMonsterGroupIndexByID(0x207, COMBAT_HOSTILITY_CPP, group_id, '\x01');
    W8MonsterGroup* group = GetMonsterGroupByListIndex(group_list_index);
    SetMonsterGroupHostility(group, hostility, recurse);
}

// FUNCTION: WIZ8 0x00547570
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility, char recurse)
{
    if (MonsterGroupAllMembersDying00511850(group)) {
        return;
    }
    W8MonsterInfo* leader = MonsterInfoFromID(0x21e, COMBAT_HOSTILITY_CPP, group->value_9f, 1);
    if (leader != 0 && leader->monster->copied_flag_332) {
        return;
    }
    W8Disposition previous = group->ubDisposition;
    if (previous == static_cast<unsigned char>(hostility)) {
        return;
    }
    group->ubDisposition = static_cast<unsigned char>(hostility);
    group->flag_ca = 0;
    if (MonsterGroupHasVisibleThreat(group)) {
        ShowNoticef(
            9, L"%s %s %s!", GetMonsterGroupName(group),
            gppStringList[0x1d7 + (group->member_count != 1)],
            gppStringList[g_group_hostility_notice_ids[static_cast<unsigned char>(hostility)]]);
    }
    group->value_cb = g_status_685170.world_clock;
    if (previous != 0) {
        SetTargetToGroup(group->group_id, W8_TARGETING_CONTEXT_IN_COMBAT);
    }
    for (unsigned int index = 0; index < ILLength(group->monsters); ++index) {
        int location_id = IListGetAt(group->monsters, index);
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x245, COMBAT_HOSTILITY_CPP, location_id, 1));
        SetMonsterHostility(monster, static_cast<unsigned char>(hostility));
    }
    if (group->leader_group_id != 0) {
        SetMonsterGroupHostility(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                                     0x24c, COMBAT_HOSTILITY_CPP, group->leader_group_id, 1)),
                                 hostility, 0);
    }
    if (group->allied_group_ids[0] != 0) {
        SetMonsterGroupHostility(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                                     0x252, COMBAT_HOSTILITY_CPP, group->allied_group_ids[0], 1)),
                                 hostility, 0);
    }
    if (group->allied_group_ids[1] != 0) {
        SetMonsterGroupHostility(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                                     0x258, COMBAT_HOSTILITY_CPP, group->allied_group_ids[1], 1)),
                                 hostility, 0);
    }
    if (recurse) {
        W8MonsterRecord* record = MonsterGroupGetRecord(group);
        if (record->faction_id_25f != 0) {
            for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
                W8MonsterGroup* other = GetMonsterGroupByListIndex(index);
                W8MonsterRecord* other_record = MonsterGroupGetRecord(other);
                if (other != group && ((other_record->flags_0d0 & 1) == 0 || !other->flag_ca) &&
                    record->faction_id_25f == other_record->faction_id_25f &&
                    MonsterGroupCanSeeGroup(other, group)) {
                    SetMonsterGroupHostility(other, group->ubDisposition, 0);
                }
            }
        }
    }
    RequestRedrawParty();
}

// FUNCTION: WIZ8 0x005477d0
void SetMonsterHostility(W8MonsterInfo* monster, unsigned char hostility)
{
    W8Disposition previous = monster->ubDisposition;
    if (previous == hostility || monster->monster->copied_flag_332) {
        return;
    }
    monster->ubDisposition = hostility;
    if (monster->fInCombat && (hostility == DISP_HOSTILE || previous == DISP_HOSTILE)) {
        RecountCombatMonsters();
    }
    if (previous != DISP_NEUTRAL) {
        SetTargetToMonster(monster->location_id, W8_TARGETING_CONTEXT_IN_COMBAT);
    }
    if (monster->fInCombat && monster->hp_current > 0 && monster->ubDisposition != DISP_NEUTRAL) {
        if (gXStatus.fCombatMode && g_combat_state->eCombatActionStatus != 1 &&
            g_combat_state->pActionMonsterInfo == monster) {
            EndMonsterAttack(monster);
            monster->action_kind = -1;
            monster->pCombat->phase = 0;
        } else if (!monster->pCombat->active) {
            UpdateMonsterAI(monster);
        } else {
            monster->action_kind = -1;
            monster->pCombat->phase = 0;
        }
    }
}

// FUNCTION: WIZ8 0x00547bf0
unsigned char CanPartySlotTurnUndead(int party_slot)
{
    if (!CharacterHasTrait00547940(&g_status_685170.buffers.characters[party_slot], 0x11) ||
        g_combat_state == 0 || g_combat_state->characters[party_slot].turn_undead_used) {
        return 0;
    }
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
            monster->hp_current != 0 && GetMonsterDataForInfo(monster)->kind_0cb == 0x14) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00547cb0
int TurnUndead(int party_slot, int* out_cost, char check)
{
    W8GrowableVector<int> monsters;
    W8GrowableVector<int> party;
    if (check) {
        if (!CanPartySlotTurnUndead(party_slot)) {
            PostCharacterNotice(party_slot, gppStringList[0x1b7], gppStringList[0x512]);
            return 0;
        }
        g_combat_state->characters[party_slot].turn_undead_used = 1;
    }

    W8TargetSource source;
    SetTargetSourceToCharacter(party_slot, &source);
    source.unknown_18[2] = 1;
    int power;
    if (!check) {
        power = g_status_685170.buffers.characters[party_slot].profession_levels[0xc] + 10 +
                g_status_685170.buffers.characters[party_slot].profession_levels[10];
        source.unknown_18[0] = 1;
    } else {
        power = g_status_685170.buffers.characters[party_slot].profession_levels[0xc] +
                g_status_685170.buffers.characters[party_slot].profession_levels[10];
        PostCharacterNotice(party_slot, gppStringList[0x182]);
    }

    W8CombatSlot target;
    ResetCombatSlot(&target);
    PopulateSpellTargetMarkers(0x81, 1, &source, &target, &monsters, &party, 0);
    if (monsters.count == 0) {
        return 0;
    }
    SetTextBoxMode(1, -1);
    unsigned int spell_power = power / monsters.count;
    if (spell_power == 0) {
        while (power < monsters.count) {
            monsters.RemoveAt(Random(monsters.count));
        }
        spell_power = 1;
    } else if (spell_power > 6) {
        spell_power = 7;
    }
    CastSpellFromSource(0x81, &source, &target, spell_power, 0, 0, 0, 0, 0, 0, &monsters);
    if (out_cost != 0) {
        *out_cost = CharacterActionFatigueCost(party_slot, 3);
    }
    return monsters.count;
}

// FUNCTION: WIZ8 0x00547f40
unsigned char CanPartySlotPray(int party_slot)
{
    if (!CharacterHasTrait00547940(&g_status_685170.buffers.characters[party_slot], 0xb) ||
        g_combat_state == 0 || g_combat_state->characters[party_slot].pray_used) {
        return 0;
    }
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
            monster->hp_current != 0) {
            return 1;
        }
    }
    return 0;
}

// GLOBAL: WIZ8 0x0061DD38
int g_pray_roll_weights_0061dd38[14];

// GLOBAL: WIZ8 0x0068D814
int g_pray_roll_sums_0068d814[14];

// GLOBAL: WIZ8 0x0068D84C
int g_pray_roll_total_0068d84c;

// GLOBAL: WIZ8 0x00619788
const wchar_t g_pray_dash_00619788[] = L" -- ";

/* Pray: the trait-eleven once-per-combat divine intervention. The flat
   weight table is folded into cumulative sums on first use - the slot one
   past the sums array doubles as the grand total - and the roll is biased by
   how long the fight has run and by the combat difficulty. The selected tier
   retries downward through the table until an action lands. */
// FUNCTION: WIZ8 0x00547FE0
int CharacterPrayAction00547FE0(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    W8GrowableVector<int> monster_targets;
    bool found = false;
    bool stop = false;
    int outcome;
    W8TargetSource source;
    W8CombatSlot target;
    unsigned int cost;
    unsigned int power_level;
    unsigned int roll;
    unsigned int in_range;
    unsigned int index;
    int action;
    int pick;
    int best;

    if (!CanPartySlotPray(party_slot)) {
        return 0;
    }
    if (g_pray_roll_total_0068d84c == 0) {
        int* const end = &g_pray_roll_total_0068d84c + 1;
        for (index = 0; index < 14; ++index) {
            const int weight = g_pray_roll_weights_0061dd38[index];
            for (int* sum = &g_pray_roll_sums_0068d814[index]; sum < end; ++sum) {
                *sum += weight;
            }
        }
    }
    SetTargetSourceToCharacter(party_slot, &source);
    source.unknown_18[0] = 1;
    source.unknown_18[2] = 1;
    ResetCombatSlot(&target);
    PostCharacterNotice(party_slot, gppStringList[0x174]);
    cost = CharacterActionFatigueCost(party_slot, 6);
    if (character->stamina < static_cast<int>(cost) &&
        character->stamina < static_cast<int>(Random(cost))) {
        PostCharacterNotice(party_slot, gppStringList[0x175]);
        return cost;
    }
    if (g_settings_6850c8.verbose_combat_messages == 0) {
        SetTextBoxMode(1, -1);
        AppendToLastTextLine(g_pray_dash_00619788, -1);
        SetTextBoxMode(1, -1);
    }
    roll = Random(g_pray_roll_total_0068d84c);
    if (g_combat_state->value_004 < 4) {
        roll += (g_combat_state->value_004 * 3 - 12) * 5;
    } else if (g_combat_state->value_004 > 8) {
        roll += Random(10);
    }
    if (g_settings_6850c8.difficulty == 0) {
        roll -= 10;
    } else if (g_settings_6850c8.difficulty == 2) {
        roll += 10;
    }
    action = 0;
    for (index = 0; index < 14; ++index) {
        action = index;
        if (static_cast<int>(roll) <= g_pray_roll_sums_0068d814[index]) {
            break;
        }
        action = 0;
    }
    power_level =
        static_cast<unsigned int>(character->profession_levels[character->current_profession]) / 3 +
        2;
    for (;;) {
        --action;
        switch (action) {
        case 0:
            if (Random(2) == 0) {
                AppendToLastTextLine(gppStringList[0x178], -1);
                g_combat_state->value_014 += 10;
            } else {
                AppendToLastTextLine(gppStringList[0x177], -1);
                AddPartyGold(100, 1);
            }
            goto done;
        case 1:
            for (index = 0; index < 8; ++index) {
                W8Character* member = &g_status_685170.buffers.characters[index];
                if (g_status_685170.buffers.party_rows[index].occupied && member->hp_current != 0 &&
                    member->stamina < member->stamina_max) {
                    AppendToLastTextLine(gppStringList[0x179], -1);
                    CastSpellFromSource(0x2c, &source, &target, 7, 0, 0, 1, &outcome, 0, 0, 0);
                    goto done;
                }
            }
            break;
        case 2:
            found = false;
            in_range = 0;
            for (index = 0; index < 8; ++index) {
                W8Character* member = &g_status_685170.buffers.characters[index];
                if (g_status_685170.buffers.party_rows[index].occupied && member->hp_current != 0 &&
                    member->highest_condition < 0x12 && member->enchantments[2].value_00 == 0) {
                    ++in_range;
                    found = true;
                }
            }
            if (found) {
                pick = Random(in_range) + 1;
                for (index = 0; index < 8; ++index) {
                    W8Character* member = &g_status_685170.buffers.characters[index];
                    if (g_status_685170.buffers.party_rows[index].occupied &&
                        member->hp_current != 0 && member->highest_condition < 0x12 &&
                        member->enchantments[2].value_00 == 0 && --pick == 0) {
                        AppendToLastTextLine(
                            FormatWideString(gppStringList[0x17a], member->name, -1), -1);
                        target.iType = W8_TARGET_KIND_CHARACTER;
                        if (power_level > 6) {
                            power_level = 7;
                        }
                        target.iChar = index;
                        CastSpellFromSource(0x15, &source, &target, power_level, 0, 0, 0, &outcome,
                                            0, 0, 0);
                        goto done;
                    }
                }
            }
            break;
        case 3:
            found = false;
            in_range = 0;
            for (index = 0; index < 8; ++index) {
                W8Character* member = &g_status_685170.buffers.characters[index];
                if (g_status_685170.buffers.party_rows[index].occupied && member->hp_current != 0 &&
                    member->highest_condition < 0x12 &&
                    (member->condition_turns[0xb] != 0 || member->condition_turns[0xd] != 0)) {
                    ++in_range;
                    found = true;
                }
            }
            if (found) {
                pick = Random(in_range) + 1;
                for (index = 0; index < 8; ++index) {
                    W8Character* member = &g_status_685170.buffers.characters[index];
                    if (g_status_685170.buffers.party_rows[index].occupied &&
                        member->hp_current != 0 && member->highest_condition < 0x12 &&
                        (member->condition_turns[0xb] != 0 || member->condition_turns[0xd] != 0) &&
                        --pick == 0) {
                        AppendToLastTextLine(gppStringList[0x179], -1);
                        target.iType = W8_TARGET_KIND_CHARACTER;
                        stop = power_level > 7;
                        if (stop) {
                            power_level = 7;
                        }
                        target.iChar = index;
                        CastSpellFromSource(0x4a, &source, &target, power_level, 0, 0, stop,
                                            &outcome, 0, 0, 0);
                        goto done;
                    }
                }
            }
            break;
        case 4:
            if (power_level > 7) {
                if ((power_level & ~1u) < 14) {
                    power_level >>= 1;
                } else {
                    power_level = 7;
                }
                ResetCombatSlot(&target);
                target.iType = W8_TARGET_KIND_PARTY;
                AppendToLastTextLine(gppStringList[0x179], -1);
                CastSpellFromSource(0x44, &source, &target, power_level, 0, 0, 1, &outcome, 0, 0,
                                    0);
                goto done;
            }
            best = -1;
            for (index = 0; index < 8; ++index) {
                W8Character* member = &g_status_685170.buffers.characters[index];
                if (g_status_685170.buffers.party_rows[index].occupied && member->hp_current != 0 &&
                    member->highest_condition < 0x12 &&
                    member->hp_current < static_cast<unsigned int>(member->hp_max) &&
                    (best == -1 ||
                     member->hp_current < g_status_685170.buffers.characters[best].hp_current)) {
                    best = index;
                }
            }
            if (best != -1) {
                ResetCombatSlot(&target);
                target.iType = W8_TARGET_KIND_CHARACTER;
                target.iChar = best;
                AppendToLastTextLine(gppStringList[0x179], -1);
                CastSpellFromSource(6, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0, 0);
                goto done;
            }
            break;
        case 5:
            found = false;
            for (index = 0; index < 6; ++index) {
                if (g_combat_state->effect_slots_85a[index].active &&
                    g_combat_state->effect_slots_85a[index].effect_id == 2) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                AppendToLastTextLine(gppStringList[0x17b], -1);
                CastSpellFromSource(2, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0, 0);
                goto done;
            }
            break;
        case 6: {
            const float range = CalcRangeDistance(g_spell_records[0x19].range_category);
            in_range = 0;
            for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
                if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
                    monster->hp_current != 0 &&
                    monster->monster->GetDistanceToPlayer004C7CB0() <= range) {
                    ++in_range;
                }
            }
            if (in_range != 0) {
                pick = Random(in_range) + 1;
                for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                    W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
                    if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
                        monster->hp_current != 0 &&
                        monster->monster->GetDistanceToPlayer004C7CB0() <= range && --pick == 0) {
                        AppendToLastTextLine(gppStringList[0x179], -1);
                        target.iType = W8_TARGET_KIND_MONSTER;
                        target.iMonsterID = monster->location_id;
                        if (power_level > 6) {
                            power_level = 7;
                        }
                        monster_targets.Add(monster->location_id);
                        break;
                    }
                }
                CastSpellFromSource(0x19, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    &monster_targets);
                goto done;
            }
            break;
        }
        case 7:
            stop = TurnUndead(party_slot, 0, 0) > 0;
            break;
        case 8:
            AppendToLastTextLine(gppStringList[0x179], -1);
            for (index = 0; index < 0x12; ++index) {
                RemoveConditionFromParty(index);
            }
            if (g_combat_state != 0) {
                for (index = 0; index < 9; ++index) {
                    if (g_combat_state->effect_slots[index].active) {
                        ResetPartyEffectBlock(&g_combat_state->effect_slots[index]);
                    }
                }
            }
            goto done;
        case 9:
            if (CombatHasCondition(0x3b) == 0 || CombatHasCondition(0x35) == 0) {
                target.iType = W8_TARGET_KIND_PARTY;
                target.iChar = -1;
                AppendToLastTextLine(gppStringList[0x17b], -1);
                CastSpellFromSource(0x3b, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                CastSpellFromSource(0x35, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                goto done;
            }
            break;
        case 10:
            if (PartyHasCondition(0x28) == 0 || PartyHasCondition(0x14) == 0 ||
                PartyHasCondition(0x20) == 0 || PartyHasCondition(0x1a) == 0) {
                target.iType = W8_TARGET_KIND_PARTY;
                target.iChar = -1;
                AppendToLastTextLine(gppStringList[0x17b], -1);
                CastSpellFromSource(0x28, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                CastSpellFromSource(0x14, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                CastSpellFromSource(0x20, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                CastSpellFromSource(0x1a, &source, &target, power_level, 0, 0, 0, &outcome, 0, 0,
                                    0);
                goto done;
            }
            break;
        case 0xb:
            for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
                if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
                    monster->hp_current != 0 && monster->condition_turns[6] == 0) {
                    AppendToLastTextLine(
                        FormatWideString(
                            gppStringList[0x17c],
                            gppStringList[g_gender_name_message_rows_61e430[character->gender][2]],
                            -1),
                        -1);
                    target.iType = W8_TARGET_KIND_FIVE;
                    CastSpellFromSource(0x75, &source, &target, power_level, 0, 0, 0, &outcome, 0,
                                        0, 0);
                    goto done;
                }
            }
            break;
        case 0xc:
            power_level >>= 1;
            {
                const float range = CalcRangeDistance(g_spell_records[0x60].range_category);
                for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                    W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
                    if (monster->fActive && monster->fInCombat && monster->ubDisposition == 1 &&
                        monster->hp_current != 0 &&
                        monster->monster->GetDistanceToPlayer004C7CB0() <= range) {
                        AppendToLastTextLine(gppStringList[0x179], -1);
                        ResetCombatSlot(&target);
                        CastSpellFromSource(0x60, &source, &target, power_level, 0, 0, 0, &outcome,
                                            0, 0, 0);
                        break;
                    }
                }
            }
            continue;
        default:
            AppendToLastTextLine(gppStringList[0x176], -1);
            goto done;
        }
        if (stop) {
        done:
            g_combat_state->characters[party_slot].pray_used = 1;
            return cost;
        }
    }
}

// FUNCTION: WIZ8 0x005478A0
void AlertSameFactionGroups(W8MonsterGroup* monster_group)
{
    W8MonsterRecord* record = MonsterGroupGetRecord(monster_group);
    if (record->faction_id_25f != 0) {
        for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
            W8MonsterGroup* other = GetMonsterGroupByListIndex(index);
            W8MonsterRecord* other_record = MonsterGroupGetRecord(other);
            if (other != monster_group &&
                ((other_record->flags_0d0 & 1) == 0 || other->flag_ca == 0) &&
                record->faction_id_25f == other_record->faction_id_25f &&
                MonsterGroupCanSeeGroup(other, monster_group) != 0) {
                SetMonsterGroupHostility(other, monster_group->ubDisposition, 0);
            }
        }
    }
}
