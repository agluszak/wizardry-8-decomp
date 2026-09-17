#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/sr_api.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_screens/MainGameScreen.h"
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

/* Species 0x224 never counts: both directions answer zero before anything
   else is read. */
enum { W8_NEUTRAL_SPECIES_224 = 0x224 };

// FUNCTION: WIZ8 0x00546e70
void RecountCombatMonsters(void)
{
    gXStatus.field_02d = 0;
    g_dword_6850be = 0;
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fActive && monster->fInCombat) {
            if (monster->ubDisposition == DISP_HOSTILE) {
                ++gXStatus.field_02d;
            }
            if (monster->condition_turns[13] != 0) {
                ++g_dword_6850be;
            }
        }
    }
    if (gXStatus.fCombatMode && gXStatus.field_02d != 0) {
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

/* Whether a spell id can be aimed by monster AI: inside the spell table, not
   one of the two self-only kinds, and carrying a middle target type. */
// FUNCTION: WIZ8 0x005474B0
unsigned char MonsterCanAimSpell005474B0(int spell_id)
{
    if (spell_id > 0x95) {
        srAssertFail("iType < SPELL_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp", 0x1c2, 0);
    }
    if (spell_id != 3 && spell_id != 0x29) {
        int target_type = GetSpellTargetType(spell_id, 0);
        if (target_type > 2 && target_type < 8) {
            return 1;
        }
        return 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00547510
unsigned char CombatAllowsLiveGroups(void)
{
    return gXStatus.fCombatMode != 0 && g_combat_state->flag_a54 == 0 &&
           g_combat_state->value_004 <= 1;
}

static const char COMBAT_HOSTILITY_CPP[] =
    "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp";

// GLOBAL: WIZ8 0x0061ec0c
const unsigned short g_group_hostility_notice_ids[3] = {511, 512, 513};

// GLOBAL: WIZ8 0x0061ec14
const int g_monster_ai_kind_name_ids_61ec14[12] = {0,    1598, 1599, 1600, 1601, 1602,
                                                   1603, 1604, 1605, 1606, 1607, 1608};

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
        WriteGameLog(
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
