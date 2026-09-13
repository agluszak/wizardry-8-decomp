#include "wiz8/targeting.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/magic.h"
#include "wiz8/spell_effect.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/local_code/MonsterAI.h"

#include <stdlib.h>
#include <string.h>

/*
 * Local Code\MonsterAI.cpp.
 *
 * What a monster decides to do. Each monster's combat state owns a queue of
 * decided actions; the bodies here build entries for it, run the per-monster
 * decision over every live monster, and answer the questions the decision
 * itself asks about targets.
 */

#define MONSTER_AI_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterAI.cpp"

/* The AI table row value that marks a monster the flee and control rules treat
   differently. */
enum { W8_AI_KIND_ROW_SPECIAL = 6 };

/* The spell the AI casts when it wants a place rather than a target. */
enum { W8_AI_SPELL_PLACE = 0x77 };

/* The monster action kinds the AI validates. */
enum { W8_MONSTER_ACTION_ATTACK = 0, W8_MONSTER_ACTION_SPELL = 2, W8_MONSTER_ACTION_FLEE = 3 };

struct W8SpellEffectEntry;
/* 0x0061EEFC: two dwords per AI kind; only the leading dword is read here. */
// GLOBAL: WIZ8 0x0061EEFC
extern const int g_ai_kind_table[32][2] = {
    {0, 0}, {1, 0}, {1, 0}, {2, 3}, {4, 0}, {1, 1}, {1, 1}, {1, 0}, {5, 4}, {5, 0}, {1, 5},
    {1, 1}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 1}, {1, 5}, {1, 0},
    {1, 1}, {1, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {5, 0}, {6, 0},
};

/* Throw away the queue of actions a monster's AI had decided on. */
// FUNCTION: WIZ8 0x00532330
void DestroyMonsterActionQueue(W8MonsterInfo* monster_info)
{
    W8PList* queue = monster_info->pCombat->pending_actions;

    if (queue != 0 && PLDestroy(queue)) {
        monster_info->pCombat->pending_actions = 0;
    }
}

/* Run the decision over every monster that is in the fight and still alive. */
// FUNCTION: WIZ8 0x005314f0
void UpdateAllMonsterAI(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0) {
            UpdateMonsterAI(monster_info);
        }
    }
}

/* The cycle a monster must have to cast at all. */
enum { W8_MONSTER_CYCLE_SPELL = 0x19 };

/* Reported once, so a monster missing its spell cycle does not flood the log. */
// GLOBAL: WIZ8 0x0068d524
static unsigned char g_spell_cycle_error_reported;

/* Decide what one monster does this round. A monster taken out of the fight
   by its worst condition, or told to give up, stands down and ends its turn.
   Otherwise it rolls to hold back, and if not, a monster that is not yet
   engaged either holds or gives up by its record. An engaged monster with a
   usable ranged attack and the party out of reach closes in (or backs off
   when hurt); otherwise it rolls to flee and to cast, weighs its attacks, and
   only then settles for closing in, backing off or holding. Whatever it
   settles on is checked, and a group that has given up holds instead. */
// FUNCTION: WIZ8 0x00531540
void UpdateMonsterAI(W8MonsterInfo* monster_info)
{
    W8MonsterRecord* record;
    W8RangeCategory range_category;
    unsigned int chance;
    unsigned char rating;
    unsigned int spell;
    int chosen;
    float hp_ratio;
    unsigned char backs_off;
    srVector3T<float> position;

    if ((unsigned int)monster_info->highest_condition >= 0xf) {
        monster_info->action_kind = -1;
        monster_info->pCombat->phase = 0;
        monster_info->pCombat->active = 1;
        return;
    }
    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->monster_species == 0x224) {
        monster_info->action_kind = 1;
        return;
    }
    chance = Function531C00(monster_info, record);
    if (Random(100) < chance) {
        monster_info->action_kind = 4;
        goto validate;
    }
    if (monster_info->flag_16 == 0) {
        if (record->unknown_249 != 0) {
            monster_info->action_kind = -1;
            monster_info->pCombat->phase = 0;
            monster_info->pCombat->active = 1;
        } else {
            monster_info->action_kind = 6;
        }
        goto validate;
    }
    if (record->holds_ground_1b9 == 0 &&
        (range_category = GetBestMonsterAttackRange(record, 1)) != W8_RANGE_NONE &&
        (monster_info->condition_turns[0xc] == 0 || record->kind_0cb == 0xc) &&
        MonsterChooseTarget(monster_info, &chosen, 2) > CalcRangeDistance(range_category)) {
        record = GetMonsterDataForInfo(monster_info);
        hp_ratio = (float)monster_info->hp_current / (float)monster_info->hp_max;
        backs_off = hp_ratio <= 0.7f && record->holds_ground_1b9 == 0;
        monster_info->action_kind = backs_off ? 7 : 5;
    } else {
        rating = RateMonsterBestAttack(monster_info, record, 0);
        chance = record->flee_chance_0e1;
        if (chance != 0) {
            if (rating != 0) {
                chance = 100;
            }
            if (CanMonsterFlee(monster_info, record, 0) && Random(100) < chance) {
                monster_info->action_kind = W8_MONSTER_ACTION_FLEE;
                if (g_ai_kind_table[record->ai_kind][0] == W8_AI_KIND_ROW_SPECIAL) {
                    position = monster_info->monster->GetPosition();
                    ResetCombatSlot(&monster_info->Target);
                    monster_info->Target.iType = W8_TARGET_KIND_PLACE;
                    monster_info->Target.point = position;
                } else if (!AimMonsterAtSpellTarget(monster_info, W8_AI_SPELL_PLACE)) {
                    srAssertFail("fSuccess", MONSTER_AI_CPP, 1052, 0);
                }
                goto validate;
            }
        }
        chance = record->spell_chance_0e0;
        if (chance != 0) {
            if (rating != 0) {
                chance = 100;
            }
            if (record->spell_chance_0e0 != 0) {
                if (!MonsterIsCycleSupported(monster_info->monster, W8_MONSTER_CYCLE_SPELL)) {
                    if (g_spell_cycle_error_reported == 0) {
                        FormatDebugMessage(0, "ERROR: %ls is missing a SPELL animation cycle",
                                           record);
                        g_spell_cycle_error_reported = 1;
                    }
                } else {
                    for (spell = 0; spell < 10; ++spell) {
                        if (IsSpellUsableByMonster(monster_info, record->spells_14d[spell], 1)) {
                            if (Random(100) < chance) {
                                monster_info->action_kind = W8_MONSTER_ACTION_SPELL;
                                monster_info->action_detail =
                                    ChooseMonsterSpell(monster_info, record);
                                if (!AimMonsterAtSpellTarget(monster_info,
                                                             monster_info->action_detail)) {
                                    srAssertFail("fSuccess", MONSTER_AI_CPP, 1076, 0);
                                }
                                goto validate;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (rating != 0) {
            if (rating == W8_MONSTER_ATTACK_OUT_OF_REACH) {
                if (monster_info->condition_turns[0xc] != 0 && record->kind_0cb != 0xc) {
                    monster_info->action_kind = 6;
                } else {
                    record = GetMonsterDataForInfo(monster_info);
                    hp_ratio = (float)monster_info->hp_current / (float)monster_info->hp_max;
                    backs_off = hp_ratio <= 0.7f && record->holds_ground_1b9 == 0;
                    monster_info->action_kind = backs_off ? 7 : 5;
                }
            } else {
                monster_info->action_kind = 6;
            }
        } else if (!ChooseRandomMonsterAction(monster_info, 0, 0, 1)) {
            monster_info->action_kind = 1;
        }
    }
validate:
    if (!IsMonsterActionUsable(monster_info) &&
        GetMonsterGroupFlagC8(monster_info->monster_group_id)) {
        monster_info->action_kind = 6;
    }
}

/* Add one decided action to a monster's queue. The third field only carries a
   value for the plain attack, and which of the two target fields the target
   goes in depends on what kind of target it is. Each entry gets a random tie
   break so two equal decisions do not always resolve the same way. */
// FUNCTION: WIZ8 0x00532360
void QueueMonsterAction(W8MonsterInfo* monster_info, int action_kind, int action_detail,
                        int attack_index, W8TargetKind target_kind, int target_value)
{
    W8MonsterAction* entry = (W8MonsterAction*)malloc(0x30);

    if (entry == 0) {
        return;
    }
    memset(entry, 0, 0x30);
    entry->action_kind = action_kind;
    entry->action_detail = action_detail;
    if (action_kind == W8_MONSTER_ACTION_ATTACK) {
        entry->attack_index = attack_index;
    }
    ResetCombatSlot(&entry->target);
    entry->target.iType = target_kind;
    if (target_kind == W8_TARGET_KIND_CHARACTER) {
        entry->target.iChar = target_value;
    } else if (target_kind == W8_TARGET_KIND_MONSTER) {
        entry->target.iMonsterID = target_value;
    }
    entry->tie_break = (unsigned char)Random(100) + 1;
    PLAdoptAppend(monster_info->pCombat->pending_actions, entry);
}

/* Whether a monster can aim the spell it wants to cast. The two area target
   types aim at the world; anything else either needs no aim at all or has to
   pass the slot check. */
// FUNCTION: WIZ8 0x00534290
unsigned char CanMonsterAimSpell(W8MonsterInfo* monster_info, int spell_id)
{
    int target_type = GetSpellTargetType(spell_id, 0);

    if (target_type > 4 && target_type < 7) {
        return AimMonsterAtSpellTarget(monster_info, spell_id);
    }
    if (g_spell_records[spell_id].needs_aim_13f != 0) {
        return ResolveTargetPoint(&monster_info->Target, 0);
    }
    return 1;
}

/* Aim a monster that wants to get away. A monster of the singled-out AI kind
   aims at where the party is standing instead of at anybody in it. */
// FUNCTION: WIZ8 0x00534cb0
unsigned char AimFleeingMonster(W8MonsterInfo* monster_info, const W8MonsterRecord* record)
{
    srVector3T<float> party;

    if (g_ai_kind_table[record->ai_kind][0] == W8_AI_KIND_ROW_SPECIAL) {
        GetCameraPosition(&party);
        ResetCombatSlot(&monster_info->Target);
        monster_info->Target.iType = W8_TARGET_KIND_PLACE;
        monster_info->Target.point = party;
        return 1;
    }
    return AimMonsterAtSpellTarget(monster_info, W8_AI_SPELL_PLACE) != 0;
}

/* Whether the action a monster has settled on can actually be carried out. An
   attack needs a character to swing at; a spell needs to be castable and needs
   a target of a kind it accepts; fleeing is refused outright to the AI kind
   that has nowhere to flee to. */
// FUNCTION: WIZ8 0x00535150
unsigned char IsMonsterActionUsable(W8MonsterInfo* monster_info)
{
    int spell_id;

    switch (monster_info->action_kind) {
    case W8_MONSTER_ACTION_ATTACK:
        return monster_info->Target.iType == W8_TARGET_KIND_CHARACTER;
    case W8_MONSTER_ACTION_SPELL:
        spell_id = monster_info->action_detail;
        if (!MonsterCanAimSpell005474B0(spell_id)) {
            return 0;
        }
        switch (monster_info->Target.iType) {
        case W8_TARGET_KIND_CHARACTER:
        case W8_TARGET_KIND_PARTY:
            return 1;
        case W8_TARGET_KIND_FIVE:
        case W8_TARGET_KIND_PLACE:
            break;
        default:
            return 0;
        }
        break;
    case W8_MONSTER_ACTION_FLEE:
        if (g_ai_kind_table[GetMonsterDataForInfo(monster_info)->ai_kind][0] ==
            W8_AI_KIND_ROW_SPECIAL) {
            return 0;
        }
        spell_id = W8_AI_SPELL_PLACE;
        break;
    default:
        return 0;
    }
    return Function5353E0(monster_info, spell_id, &monster_info->Target) != 0;
}

/* How near the nearest member of a group has come. */
// FUNCTION: WIZ8 0x005324b0
float GetGroupNearestDistance(W8MonsterGroup* group, float furthest)
{
    unsigned int index;
    int location_id;
    W8MonsterInfo* monster_info;
    float distance;

    if (group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 1840, 0);
    }

    for (index = 0; index < ILLength(group->monsters); ++index) {
        location_id = IListGetAt(group->monsters, index);
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(1845, MONSTER_AI_CPP, location_id, 1));
        distance = monster_info->monster->GetDistanceToPlayer004C7CB0();
        if (distance < furthest) {
            furthest = distance;
        }
    }
    return furthest;
}

/* Whether the point the monster-control effect is anchored to is still within
   reach. effects.data sits at W8SpellEffectEntry + 0x10c; the first visual is
   a W8SpellVisual whose W8Navigator secondary base is the ordinary GrCycle
   conversion at +0x18. With no effect running, or nothing anchored, there is
   nothing to be in range of; failing the test falls back on where the party
   is standing. */
// FUNCTION: WIZ8 0x00534d50
short IsMonsterControlPointInRange(W8MonsterInfo* monster_info)
{
    W8SpellEffectEntry* effect = FindMonsterControlSpellEffect();
    W8SpellVisual* visual;
    W8Navigator* anchor_navigator;
    short in_range;
    srVector3T<float> party;

    if (effect == 0) {
        return 0;
    }
    visual = effect->effects.data[0];
    if (visual == 0) {
        return 0;
    }
    anchor_navigator = visual;
    in_range =
        monster_info->monster->SetMovementTargetToNavigator004526C0(anchor_navigator, 2500.0);
    if (in_range == 0) {
        party = anchor_navigator->GetPosition();
        monster_info->monster->AimAtPosition(&party);
    }
    return in_range;
}
