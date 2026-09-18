#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"

/*
 * Local Code\Combat Range.cpp.
 *
 * How far apart two combatants are, and which of the attacks either of them
 * has will reach that far. Range is carried as a category rather than a
 * distance; CalcRangeDistance is the one place the two are related.
 */

#define COMBAT_RANGE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Range.cpp"

/* The range categories are W8RangeCategory, declared with the spell record
   that shares the domain. Above W8_RANGE_SHORT an attack is out of the
   close-quarters band the melee rule restricts itself to. */

/* Three party positions per formation row, and the row stride of the formation
   table. */
enum { W8_FORMATION_ROW_WIDTH = 3 };

// GLOBAL: WIZ8 0x005ec35c
float g_float_005ec35c = 12500.0f;

/* The formation. Three party positions per row at 0x00687511, and each
   position's own row number at 0x00687525 with a twelve-byte stride. -1 marks
   an empty place. Both live inside the block Formation & Facing.cpp saves and
   restores whole. */

/* Whether `party_slot`'s action can reach any member of `group_id` at all.
   The character becomes the source for the shared range test; a miss can
   queue the character's complaint event when `notify` asks for it. */
// FUNCTION: WIZ8 0x00519920
unsigned char IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context,
                                   char notify)
{
    W8TargetSource source;
    W8MonsterGroup* group;
    unsigned int index;

    index = GetMonsterGroupIndexByID(0x197, COMBAT_RANGE_CPP, group_id, 0);
    if (index == 0xffffffff) {
        return 0;
    }
    group = GetMonsterGroupByListIndex(index);
    if (group == 0) {
        srAssertFail("pMonsterGroup != NULL", COMBAT_RANGE_CPP, 0x1a0, 0);
    }
    SetTargetSourceToCharacter(party_slot, &source);
    if (IsTargetSourceInRangeOfGroup(&source, group, context) == 0) {
        if (notify != 0) {
            QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot],
                                g_special_event_0068c530, 0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
        }
        return 0;
    }
    return 1;
}

/* The range category the slot's chosen action works at. Attacks take the
   weapon's reach for the asked hand, a move acts at long range, spells and
   item uses ask their spell record, and anything unknown has no range. */
// FUNCTION: WIZ8 0x005199f0
int GetCharActionRange(int party_slot, int hand, W8TargetingContext context)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    W8ActionDetailBlock* detail_block;
    int action;
    int detail;

    ChooseCombatAction(party_slot, context, &action, &detail, 0, &detail_block);
    switch (action) {
    case 0:
    case 1:
        return GetCharAttackRange(character, hand);
    case 7:
        if (detail != 0) {
            return g_spell_records[detail].range_category;
        }
        break;
    case 8:
        return GetItemSpellRange(detail_block->item_use.item);
    case 5:
        return W8_RANGE_TOUCH;
    case 2:
        return W8_RANGE_LONG;
    }
    return W8_RANGE_NONE;
}

/* The range category the weapon in one of the character's hands attacks at.
   `hand` of HAND_ANY asks both hands and keeps the better answer; only a
   hand actually in play carrying a melee or thrown wield kind has a range to
   report at all. */
// FUNCTION: WIZ8 0x00519ac0
int GetCharAttackRange(W8Character* character, unsigned int hand)
{
    unsigned int index;
    int best;
    int range;

    if (hand >= 2) {
        if (hand != 2) {
            srAssertFail("uiHand == HAND_ANY", COMBAT_RANGE_CPP, 0x1f2, 0);
        }
        best = -1;
        for (index = 0; index < 2; ++index) {
            if (character->hand_attacks[index].in_play != 0) {
                range = GetCharAttackRange(character, index);
                if (best < range) {
                    best = range;
                }
            }
        }
        return best;
    }
    if (character->hand_attacks[hand].in_play == 0) {
        FormatDebugMessage(1,
                           "ERROR: GetCharAttackRange for hand %d which can't attack, uiChar = %d",
                           hand, CharacterPointerToPartySlot(character));
        return -1;
    }
    if (character->hand_attacks[hand].wield_kind != 1 &&
        character->hand_attacks[hand].wield_kind != 3) {
        return 0;
    }
    return g_item_records[character->equipment[6 + (hand != 0)].item_id].wield_group;
}

/* The furthest range category any of this character's hands can reach at. */
// FUNCTION: WIZ8 0x00519ba0
W8RangeCategory GetBestHandRangeCategory(const W8Character* character)
{
    W8RangeCategory best = W8_RANGE_NONE;
    W8RangeCategory category;
    unsigned int hand;

    for (hand = 0; hand < 2; ++hand) {
        if (character->hand_attacks[hand].in_play != 0) {
            category = static_cast<W8RangeCategory>(CalcRangeCategoryToTarget(character, hand));
            if (category > best) {
                best = category;
            }
        }
    }
    return best;
}

/* Whether the first lighting condition applies at distant or extreme range. */
// FUNCTION: WIZ8 0x00519be0
unsigned char RangeCategoryUsesSightCondition(const W8MonsterInfo* monster,
                                              W8RangeCategory range_category)
{
    if (range_category >= W8_RANGE_LONG && range_category <= W8_RANGE_EXTREME) {
        return GetSightCondition37A(monster);
    }
    return false;
}

/* Whether the monster's attack `attack` reaches anyone at all; `hostile_only`
   counts only those it is hostile to. In combat with unknown_015 set the
   hostile filter is forced on. Party members are tested inline; other monsters
   defer to MonsterAttackReachesMonster. */
// FUNCTION: WIZ8 0x00519c00
unsigned char MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack,
                                         char hostile_only)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    char disposition_needed;
    char crossable;
    int party_slot;
    int sight_index;
    int action_kind;
    unsigned int index;
    unsigned int count;
    unsigned int steps;
    W8RangeCategory range;
    W8MonsterInfo* other;
    W8MonsterRecord* attack_record;

    if (monster_info->fInCombat == 0 || monster_info->pCombat->unknown_015 == 0) {
        disposition_needed = hostile_only;
    } else {
        disposition_needed = 1;
    }
    disposition_needed = static_cast<char>((disposition_needed != 0) + 1);

    for (party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8Character* character = &g_status_685170.buffers.characters[party_slot];

        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            continue;
        }
        if (character->hp_current == 0) {
            continue;
        }
        if (character->highest_condition >= 0x12) {
            continue;
        }
        if (MonsterVsCharDisposition(party_slot, monster_info) != disposition_needed) {
            continue;
        }
        if (monster_info->player_visibility.state_04 != 1) {
            continue;
        }

        action_kind = monster_info->action_kind;
        if (action_kind == 0) {
            attack_record = GetMonsterDataForInfo(monster_info);
            if (attack_record->attacks[attack].range_category < W8_RANGE_LONG ||
                attack_record->attacks[attack].range_category > W8_RANGE_EXTREME) {
                sight_index = 0;
            } else {
                sight_index = GetSightCondition37A(monster_info);
            }
        } else if (action_kind == 2) {
            sight_index = GetSightCondition37CIndex(monster_info);
        } else if (action_kind == 3) {
            sight_index = 2;
        } else {
            sight_index = 0;
        }
        if (monster_info->player_visibility.sight_flags_05[sight_index] == 0) {
            continue;
        }

        switch (monster_info->action_kind) {
        case 0:
            if (attack >= W8_MAX_MONSTER_ATTACKS) {
                srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 0x3b5, 0);
            }
            if (record->attacks[attack].fHasAttack == 0) {
                srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 0x3b6, 0);
            }
            range = static_cast<W8RangeCategory>(record->attacks[attack].range_category);
            break;
        case 2:
            range = g_spell_records[monster_info->action_detail].range_category;
            break;
        case 3:
            range = W8_RANGE_LONG;
            break;
        case 8:
            range = W8_RANGE_TOUCH;
            break;
        default:
            range = W8_RANGE_NONE;
            break;
        }
        if (range == W8_RANGE_NONE) {
            continue;
        }
        if (gXStatus.fCombatMode != 0 && range >= W8_RANGE_TOUCH && range < W8_RANGE_LONG) {
            for (crossable = CountRowsBetween(party_slot, monster_info); crossable != 0;
                 --crossable) {
                if (range == W8_RANGE_TOUCH) {
                    goto next_party_slot;
                }
                range = static_cast<W8RangeCategory>(static_cast<int>(range) - 1);
            }
        }
        if (range != W8_RANGE_NONE) {
            steps = 0;
            switch (range) {
            case W8_RANGE_TOUCH:
                steps = 2;
                break;
            case W8_RANGE_SHORT:
                steps = 4;
                break;
            case W8_RANGE_LONG:
                steps = 25;
                break;
            case W8_RANGE_EXTREME:
                steps = 50;
                break;
            default:
                srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x463,
                             "CalcRangeDistance: ERROR - Invalid range category");
            }
            if (monster_info->monster->GetDistanceToPlayer004C7CB0() <=
                steps * g_world_scale_005ebc40) {
                return 1;
            }
        }
    next_party_slot:;
    }

    count = PLLength(gXStatus.plsMonsterList);
    for (index = 0; index < count; ++index) {
        other = MonsterGetScriptPartByLocationIndex(index);
        GetMonsterDataForInfo(other);
        if (other != monster_info && other->fActive != 0 && other->fInCombat != 0 &&
            other->hp_current != 0 && other->highest_condition < 0x12 &&
            MonsterHostility00546F80(monster_info, other) == disposition_needed &&
            MonsterAttackReachesMonster(monster_info, record, attack, other) != 0) {
            return 1;
        }
        count = PLLength(gXStatus.plsMonsterList);
    }
    return 0;
}

/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   given what it can see and how far away they stand. Combat mode shortens
   touch/short reach by CountRowsBetween. */
// FUNCTION: WIZ8 0x0051a2f0
unsigned char MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                            unsigned int attack, int party_slot)
{
    int sight_index;
    int action_kind;
    unsigned int steps;
    W8RangeCategory range;
    char crossable;
    W8MonsterRecord* attack_record;

    if (monster_info->player_visibility.state_04 != 1) {
        return 0;
    }

    action_kind = monster_info->action_kind;
    if (action_kind == 0) {
        attack_record = GetMonsterDataForInfo(monster_info);
        if (attack_record->attacks[attack].range_category < W8_RANGE_LONG ||
            attack_record->attacks[attack].range_category > W8_RANGE_EXTREME) {
            sight_index = 0;
        } else {
            sight_index = GetSightCondition37A(monster_info);
        }
    } else if (action_kind == 2) {
        sight_index = GetSightCondition37CIndex(monster_info);
    } else if (action_kind == 3) {
        sight_index = 2;
    } else {
        sight_index = 0;
    }
    if (monster_info->player_visibility.sight_flags_05[sight_index] == 0) {
        return 0;
    }

    switch (monster_info->action_kind) {
    case 0:
        if (attack >= W8_MAX_MONSTER_ATTACKS) {
            srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 0x3b5, 0);
        }
        if (record->attacks[attack].fHasAttack == 0) {
            srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 0x3b6, 0);
        }
        range = static_cast<W8RangeCategory>(record->attacks[attack].range_category);
        break;
    case 2:
        range = g_spell_records[monster_info->action_detail].range_category;
        break;
    case 3:
        range = W8_RANGE_LONG;
        break;
    case 8:
        range = W8_RANGE_TOUCH;
        break;
    default:
        range = W8_RANGE_NONE;
        break;
    }
    if (range == W8_RANGE_NONE) {
        return 0;
    }
    if (gXStatus.fCombatMode != 0 && range >= W8_RANGE_TOUCH && range < W8_RANGE_LONG) {
        for (crossable = CountRowsBetween(party_slot, monster_info); crossable != 0; --crossable) {
            if (range == W8_RANGE_TOUCH) {
                return 0;
            }
            range = static_cast<W8RangeCategory>(static_cast<int>(range) - 1);
        }
    }
    if (range == W8_RANGE_NONE) {
        return 0;
    }
    steps = 0;
    switch (range) {
    case W8_RANGE_TOUCH:
        steps = 2;
        break;
    case W8_RANGE_SHORT:
        steps = 4;
        break;
    case W8_RANGE_LONG:
        steps = 25;
        break;
    case W8_RANGE_EXTREME:
        steps = 50;
        break;
    default:
        srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x463,
                     "CalcRangeDistance: ERROR - Invalid range category");
    }
    if (monster_info->monster->GetDistanceToPlayer004C7CB0() <= steps * g_world_scale_005ebc40) {
        return 1;
    }
    return 0;
}

/* Whether the monster's attack `attack` reaches another monster. Untargetable
   attackers never reach; the same monster always does. Sight comes from the
   mon-to-mon visibility row rather than the party record. */
// FUNCTION: WIZ8 0x0051a510
unsigned char MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                          unsigned int attack, W8MonsterInfo* target)
{
    W8VisibilityRecord* visibility;
    int sight_index;
    int action_kind;
    unsigned int steps;
    W8RangeCategory range;
    W8MonsterRecord* attack_record;

    if (record->untargetable_24a != 0) {
        return 0;
    }
    if (target == monster_info) {
        return 1;
    }
    visibility = FindMonToMonVisibility(monster_info, target);
    if (visibility == 0) {
        return 0;
    }
    if (visibility->state_04 != 1) {
        return 0;
    }

    action_kind = monster_info->action_kind;
    if (action_kind == 0) {
        attack_record = GetMonsterDataForInfo(monster_info);
        if (attack_record->attacks[attack].range_category < W8_RANGE_LONG ||
            attack_record->attacks[attack].range_category > W8_RANGE_EXTREME) {
            sight_index = 0;
        } else {
            sight_index = GetSightCondition37A(monster_info);
        }
    } else if (action_kind == 2) {
        sight_index = GetSightCondition37CIndex(monster_info);
    } else if (action_kind == 3) {
        sight_index = 2;
    } else {
        sight_index = 0;
    }
    if (visibility->sight_flags_05[sight_index] == 0) {
        return 0;
    }

    switch (monster_info->action_kind) {
    case 0:
        if (attack >= W8_MAX_MONSTER_ATTACKS) {
            srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 0x3b5, 0);
        }
        if (record->attacks[attack].fHasAttack == 0) {
            srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 0x3b6, 0);
        }
        range = static_cast<W8RangeCategory>(record->attacks[attack].range_category);
        break;
    case 2:
        range = g_spell_records[monster_info->action_detail].range_category;
        break;
    case 3:
        range = W8_RANGE_LONG;
        break;
    case 8:
        range = W8_RANGE_TOUCH;
        break;
    default:
        return 0;
    }
    if (range == W8_RANGE_NONE) {
        return 0;
    }
    steps = 0;
    switch (range) {
    case W8_RANGE_TOUCH:
        steps = 2;
        break;
    case W8_RANGE_SHORT:
        steps = 4;
        break;
    case W8_RANGE_LONG:
        steps = 25;
        break;
    case W8_RANGE_EXTREME:
        steps = 50;
        break;
    default:
        srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x463,
                     "CalcRangeDistance: ERROR - Invalid range category");
    }
    if (monster_info->monster->GetDistanceToMonster004C7DD0(target->monster) <=
        steps * g_world_scale_005ebc40) {
        return 1;
    }
    return 0;
}

/* The furthest range category among a monster's three attacks. Asking for the
   close-quarters band only considers the two categories inside it. */
// FUNCTION: WIZ8 0x0051a800
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only)
{
    W8RangeCategory best = W8_RANGE_NONE;
    int attack;
    unsigned char category;

    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0) {
            category = record->attacks[attack].range_category;
            if ((close_quarters_only == 0 || category < W8_RANGE_LONG) && (int)category > best) {
                best = static_cast<W8RangeCategory>(category);
            }
        }
    }
    return best;
}

/* Furthest attack band, optionally raised to long for certain special-attack
   kinds, then the furthest castable spell band when the gates allow. */
// FUNCTION: WIZ8 0x0051a840
W8RangeCategory GetMonsterBestRangeCategory(W8MonsterInfo* monster_info,
                                            char skip_capability_checks, int* out_sight)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8RangeCategory best = W8_RANGE_NONE;
    int attack;
    int spell;

    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0 &&
            static_cast<int>(record->attacks[attack].range_category) > best) {
            best = static_cast<W8RangeCategory>(record->attacks[attack].range_category);
        }
    }

    if (best >= W8_RANGE_LONG && best <= W8_RANGE_EXTREME) {
        *out_sight = GetSightCondition37A(monster_info);
    } else {
        *out_sight = 0;
    }

    if (skip_capability_checks == 0) {
        if (record->prefer_ranged_actions_1b9 != 1) {
            return best;
        }
        if (record->flee_chance_0e1 < 0x50) {
            if (record->prefer_ranged_actions_1b9 != 1 || record->spell_chance_0e0 < 0x50) {
                return best;
            }
            goto consider_spells;
        }
    }

    if (record->special_attack_kind_0e3 != 0 &&
        g_special_attack_table[record->special_attack_kind_0e3][0] != 6) {
        if (skip_capability_checks == 0 && CanMonsterFlee(monster_info, record, 1) == 0) {
            if (record->prefer_ranged_actions_1b9 != 1 || record->spell_chance_0e0 < 0x50) {
                return best;
            }
            goto consider_spells;
        }
        if (best < W8_RANGE_LONG) {
            best = W8_RANGE_LONG;
            *out_sight = W8_RANGE_LONG;
        }
    }

    if (skip_capability_checks != 0) {
        goto consider_spells;
    }
    if (record->prefer_ranged_actions_1b9 != 1 || record->spell_chance_0e0 < 0x50) {
        return best;
    }

consider_spells:
    for (spell = 0; spell < 10; ++spell) {
        unsigned int spell_id = record->spells_14d[spell];

        if (MonsterCanAimSpell005474B0(spell_id) == 0) {
            continue;
        }
        if (skip_capability_checks == 0 &&
            IsSpellUsableByMonster(monster_info, static_cast<int>(spell_id), 1) == 0) {
            continue;
        }
        if (static_cast<int>(best) < static_cast<int>(g_spell_records[spell_id].range_category)) {
            best = g_spell_records[spell_id].range_category;
            *out_sight = GetSightCondition37CIndex(monster_info);
        }
    }
    return best;
}

/* The range category one monster action works at. A spell takes the range off
   the spell record; two of the actions have a fixed answer and the rest have
   none. A plain attack takes it from the attack itself, which has to exist. */
// FUNCTION: WIZ8 0x0051a730
W8RangeCategory GetMonsterActionRangeCategory(const W8MonsterInfo* monster_info,
                                              const W8MonsterRecord* record, unsigned int attack)
{
    switch (monster_info->action_kind) {
    case 0:
        break;
    case 2:
        return g_spell_records[monster_info->action_detail].range_category;
    case 3:
        return W8_RANGE_LONG;
    case 8:
        return W8_RANGE_TOUCH;
    default:
        return W8_RANGE_NONE;
    }

    if (attack >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 949, 0);
    }
    if (record->attacks[attack].fHasAttack == 0) {
        srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 950, 0);
    }
    return static_cast<W8RangeCategory>(record->attacks[attack].range_category);
}

/* How far a range category actually is. The four categories step 2, 4, 25, 50
   before the world scale multiplies them; no range at all is zero distance. */
// FUNCTION: WIZ8 0x0051a9a0
float CalcRangeDistance(W8RangeCategory range_category)
{
    unsigned int steps = 0;

    switch (range_category) {
    case W8_RANGE_NONE:
        steps = 0;
        break;
    case W8_RANGE_TOUCH:
        steps = 2;
        break;
    case W8_RANGE_SHORT:
        steps = 4;
        break;
    case W8_RANGE_LONG:
        steps = 25;
        break;
    case W8_RANGE_EXTREME:
        steps = 50;
        break;
    default:
        srAssertFail("FALSE", COMBAT_RANGE_CPP, 1123,
                     "CalcRangeDistance: ERROR - Invalid range category");
    }
    return steps * g_world_scale_005ebc40;
}

/* Party-relative action range for the world cursor: same band steps as
   CalcRangeDistance, then add the startup navigator's movement value_0b0. */
// FUNCTION: WIZ8 0x0051AB50
float CalcRangeDistanceFromParty0051AB50(W8RangeCategory range_category)
{
    unsigned int steps = 0;

    switch (range_category) {
    case W8_RANGE_NONE:
        steps = 0;
        break;
    case W8_RANGE_TOUCH:
        steps = 2;
        break;
    case W8_RANGE_SHORT:
        steps = 4;
        break;
    case W8_RANGE_LONG:
        steps = 25;
        break;
    case W8_RANGE_EXTREME:
        steps = 50;
        break;
    default:
        srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x463,
                     "CalcRangeDistance: ERROR - Invalid range category");
    }
    return steps * g_world_scale_005ebc40 + g_startup_world_659c0c->movement_0c0.value_0b0;
}

/* Shrink a short-range category by the formation rows CountRowsBetween says
   stand between the monster and `party_slot`. Exhausting the category marks it
   unreachable (-1). */
// FUNCTION: WIZ8 0x0051abe0
void CloseFormationGap(W8MonsterInfo* monster_info, int party_slot, int* rows_apart)
{
    char crossable;

    if (gXStatus.fCombatMode == 0) {
        return;
    }
    if (*rows_apart < 0 || *rows_apart >= 2) {
        return;
    }
    crossable = CountRowsBetween(party_slot, monster_info);
    if (crossable == 0) {
        return;
    }
    while (*rows_apart != 0) {
        --crossable;
        --*rows_apart;
        if (crossable == 0) {
            return;
        }
    }
    *rows_apart = -1;
}

/* Whether anybody standing ahead of this position is still in formation. */
// FUNCTION: WIZ8 0x0051ae60
bool AnyoneStandsAhead(unsigned char position)
{
    int found = 0;
    unsigned int index;
    signed char slot;

    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.bOccupantChar[position][index];
        if (slot != -1 &&
            g_status_685170.buffers.characters[slot].bonus_1770.out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* How many formation rows between `party_slot` and the monster block a short
   reach. Same row answers zero; otherwise one when the monster's row is
   occupied, plus one more when the gap is exactly two rows and either that
   row or the front rank screens. */
// FUNCTION: WIZ8 0x0051aec0
char CountRowsBetween(int party_slot, W8MonsterInfo* monster_info)
{
    unsigned char monster_quadrant = static_cast<unsigned char>(GetMonsterQuadrant(monster_info));
    signed char party_quadrant = g_status_685170.formation.positions[party_slot].bQuadrant;
    char rows = 0;
    unsigned int index;
    signed char slot;
    int gap;
    char found_front;

    if (monster_quadrant == static_cast<unsigned char>(party_quadrant)) {
        return 0;
    }

    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.bOccupantChar[monster_quadrant][index];
        if (slot != -1 &&
            g_status_685170.buffers.characters[slot].bonus_1770.out_of_formation == 0) {
            ++rows;
        }
    }
    if (rows != 0) {
        rows = 1;
    }

    if (monster_quadrant == 4) {
        srAssertFail("ubMonsterQuadrant != QUADRANT_CENTER", COMBAT_RANGE_CPP, 0x598, 0);
    }
    if (party_quadrant == 4) {
        return rows;
    }

    gap = static_cast<int>(monster_quadrant) - party_quadrant;
    if (gap < 0) {
        gap = -gap;
    }
    if (gap != 2) {
        return rows;
    }
    if (rows != 0) {
        return static_cast<char>(rows + 1);
    }

    found_front = 0;
    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.bOccupantChar[4][index];
        if (slot != -1 &&
            g_status_685170.buffers.characters[slot].bonus_1770.out_of_formation == 0) {
            ++found_front;
        }
    }
    if (found_front != 0) {
        return static_cast<char>(rows + 1);
    }
    return rows;
}

/* Whether the front rank stands between two positions. Only positions exactly
   two rows apart can be screened, and the fifth row is never in the way. */
// FUNCTION: WIZ8 0x0051b000
bool FrontRankScreens(unsigned int from_position, unsigned int to_position)
{
    unsigned char from_row = g_status_685170.formation.positions[from_position].bQuadrant;
    unsigned char to_row = g_status_685170.formation.positions[to_position].bQuadrant;
    int rows_apart;
    int found;
    unsigned int index;
    signed char slot;

    if (from_row == to_row) {
        return false;
    }
    if (from_row == 4 || to_row == 4) {
        return false;
    }
    rows_apart = from_row - (signed char)to_row;
    if (rows_apart < 0) {
        rows_apart = -rows_apart;
    }
    if (rows_apart != 2) {
        return false;
    }

    found = 0;
    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.bOccupantChar[4][index];
        if (slot != -1 &&
            g_status_685170.buffers.characters[slot].bonus_1770.out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* Two seven-byte constant readers the range rules share. */
// FUNCTION: WIZ8 0x0051b300
float GetRangeConstant5EC360(void)
{
    return g_float_005ec360;
}

// FUNCTION: WIZ8 0x0051b310
float GetRangeConstant5EC35C(void)
{
    return g_float_005ec35c;
}

/* Cache which origin points this monster can actually provide. Projectile
   attacks need a usable launch point only when their best range is long or
   extreme; spell casting also requires the spell cycle. */
// FUNCTION: WIZ8 0x0051B420
void InitializeMonsterRangeCapabilities(W8MonsterInfo* monster_info, const W8MonsterRecord* record)
{
    monster_info->unknown_379 = 1;
    monster_info->unknown_37b = 1;

    int best_range = W8_RANGE_NONE;
    for (unsigned int attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0 &&
            best_range < record->attacks[attack].range_category) {
            best_range = record->attacks[attack].range_category;
        }
    }

    srVector3T<float> position;
    monster_info->has_missile_37a =
        best_range > W8_RANGE_SHORT &&
        monster_info->monster->GetProjectilePosition004C77F0(&position) == 1;

    monster_info->has_spell_37c = record->spell_chance_0e0 != 0 &&
                                  MonsterIsCycleSupported(monster_info->monster, 0x19) &&
                                  monster_info->monster->GetSpellPosition004C78E0(&position) == 1;
}

// FUNCTION: WIZ8 0x0051b3f0
unsigned char TraceModeRejectsNoHit0051B3F0(int mode)
{
    switch (mode) {
    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
        return 1;
    default:
        return static_cast<unsigned char>(mode);
    }
}

/* Choose what one monster aims at: the player when it can see them, otherwise
   the nearest hostile visible monster. Answers the chosen distance and fills
   the two-word target output. */
// FUNCTION: WIZ8 0x0051ac30
float MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int kind)
{
    float best = 1000000.0f;

    *out = 0;
    if (monster_info->ubDisposition == 1 &&
        IsVisibleUnderConditions(monster_info, &monster_info->player_visibility, kind) &&
        (best = monster_info->monster->GetDistanceToPlayer004C7CB0(), best < 1000000.0f)) {
        *out = 2;
    }
    if (*out == 0 || monster_info->pCombat->unknown_151[1] == 0) {
        unsigned int count = PLLength(gXStatus.plsMonsterList);

        for (unsigned int index = 0; index < count; ++index) {
            W8MonsterInfo* other = MonsterGetScriptPartByLocationIndex(index);

            if (other != monster_info && other->fActive != 0 && other->hp_current != 0 &&
                other->fInCombat != 0 && MonsterHostility00546F80(monster_info, other) == 1) {
                W8VisibilityRecord* row = FindMonToMonVisibility(monster_info, other);
                if (IsVisibleUnderConditions(monster_info, row, kind)) {
                    float distance =
                        monster_info->monster->GetDistanceToMonster004C7DD0(other->monster);
                    if (distance < best) {
                        *out = 3;
                        out[2] = other->location_id;
                        best = distance;
                    }
                }
            }
            count = PLLength(gXStatus.plsMonsterList);
        }
    }
    return best;
}
