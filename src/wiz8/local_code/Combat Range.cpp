#include "wiz8/layouts/character.h"
#include "wiz8/integer_constants.h"
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
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "random.h"

// GLOBAL: WIZ8 0x0068C518
int g_event_sight_blocked = g_first_remapped_event + 30;

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
const float g_float_005ec35c = 12500.0f;

/* The formation. Three party positions per row at 0x00687511, and each
   position's own row number at 0x00687525 with a twelve-byte stride. -1 marks
   an empty place. Both live inside the block Formation & Facing.cpp saves and
   restores whole. */

/* Whether the slot has any attack of `category` that reaches a valid target
   in the scanned groups for `hand`; the condition interrupt uses it to tell
   usable attacks from merely reachable ones. `flag` == 1 skips the monster
   scan and only the out-of-combat and plain-attack categories also scan the
   party for an opposing-side member the chosen action could actually
   strike. */
// FUNCTION: WIZ8 0x00518e30
bool CanPartySlotAttackAnyTarget(int party_slot, W8TargetingContext category, int flag, bool hand)
{
    char side;
    int first = party_slot;

    if (!gXStatus.fCombatMode || !g_combat_state->characters[party_slot].berserk) {
        side = hand;
    } else {
        side = 1;
    }
    side = (side != 0) + 1;
    bool live_groups = CombatAllowsLiveGroups();
    if (flag != 1) {
        unsigned int index;
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info->fActive && monster_info->hp_current != 0 &&
                monster_info->highest_condition < W8_CONDITION_DEAD &&
                (live_groups || MonsterVsCharDisposition(first, monster_info) == side)) {
                for (unsigned int reach_hand = 0; reach_hand < 2; ++reach_hand) {
                    if (CanHandReachTarget(first, reach_hand) &&
                        CanPartyMemberAimAtMonster(first, reach_hand, monster_info, category, 0)) {
                        return true;
                    }
                }
            }
        }
    }
    if (category == W8_TARGETING_CONTEXT_OUT_OF_COMBAT || category == W8_TARGETING_CONTEXT_ATTACK) {
        W8Character* character = &g_status.buffers.Char[first];
        for (int slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
            W8Character* candidate = &g_status.buffers.Char[slot];
            if (slot != first && g_status.buffers.XChar[slot].fOccupied &&
                candidate->hp_current != 0 && candidate->highest_condition < W8_CONDITION_DEAD &&
                CharacterVsCharacterDisposition(first, slot) == side) {
                for (unsigned int reach_hand = 0; reach_hand < 2; ++reach_hand) {
                    if (!CanHandReachTarget(first, reach_hand)) {
                        continue;
                    }
                    if (static_cast<char>(first) == slot) {
                        return true;
                    }
                    W8ActionKind kind;
                    int action;
                    W8ActionDetailBlock* detail;
                    ChooseCombatAction(first, category, &kind, &action, 0, &detail);
                    W8RangeCategory range;
                    switch (kind) {
                    case W8_ACTION_ATTACK:
                    case W8_ACTION_BERSERK:
                        range = GetCharAttackRange(character, reach_hand);
                        break;
                    case W8_ACTION_CAST_SPELL:
                        if (action == 0) {
                            continue;
                        }
                        range = g_spell_records[action].range_category;
                        break;
                    case W8_ACTION_USE_ITEM:
                        range = GetItemSpellRange(detail->item_use.item);
                        break;
                    case W8_ACTION_PROTECT:
                        if (!FrontRankScreens(first, slot)) {
                            return true;
                        }
                        continue;
                    case W8_ACTION_BREATHE:
                        return true;
                    default:
                        continue;
                    }
                    if (range == W8_RANGE_NONE) {
                        continue;
                    }
                    if (range != W8_RANGE_TOUCH) {
                        return true;
                    }
                    if (!FrontRankScreens(first, slot)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/* Whether the slot's chosen action reaches the target it was aimed at:
   `hand` selects the attack side (2 asks for the better hand). A monster
   must be aimable, another party member takes the action's range (with the
   front-rank screen on melee), a point on the ground must sit inside the
   spell's distance and its line of sight, and a group target defers to the
   shared group range test. */
// FUNCTION: WIZ8 0x00519180
bool CharacterActionReachesTarget(int party_slot, int hand, W8TargetingContext context)
{
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, context);
    context = ResolveTargetingContext(party_slot, context);
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0xcb, COMBAT_RANGE_CPP, target->iMonsterID, false);
        if (monster_list_index == 0xffffffff) {
            return false;
        }
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        GetMonsterDataForInfo(monster_info);
        return CanPartyMemberAimAtMonster(party_slot, hand, monster_info, context, 0);
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        int target_slot = target->iChar;
        if (static_cast<char>(party_slot) != target_slot) {
            W8RangeCategory range = GetCharActionRange(party_slot, hand, context);
            if (range == W8_RANGE_NONE) {
                return false;
            }
            if (range == W8_RANGE_TOUCH && FrontRankScreens(party_slot, target_slot)) {
                return false;
            }
        }
    } else if (target->iType == W8_TARGET_KIND_PLACE) {
        srVector3T<float> camera;
        srVector3T<float> camera_top;
        srVector3T<float> point;
        W8TargetSource source;
        float distance;
        bool trace;

        GetCameraPosition(&camera);
        GetCameraPosition(&camera_top);
        point = target->point;
        unsigned int spell_id = GetActionSpellLikeId(party_slot, context);
        if (spell_id == 0) {
            srAssertFail("uiSpell != SPELL_NONE", COMBAT_RANGE_CPP, 0xee, 0);
        }
        W8SpellTargetType target_type = GetSpellTargetType(spell_id, false);
        if (target_type == W8_TARGET_TYPE_CONE) {
            trace = false;
            distance = g_float_005ec35c;
        } else {
            if (target_type != W8_TARGET_TYPE_RADIUS && target_type != W8_TARGET_TYPE_POINT) {
                srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x101, 0);
                return false;
            }

            trace = true;
            camera.y -= g_default_world_height;
            point.y -= g_float_005ebc64;
            distance = CalcRangeDistance(g_spell_records[spell_id].range_category);
        }
        float dx = point.x - camera.x;
        float dz = point.z - camera.z;
        if (distance + g_float_one <
            sqrtf(dx * dx + (point.y - camera.y) * (point.y - camera.y) + dz * dz) -
                g_startup_world->radius) {
            return false;
        }
        if (trace) {
            return g_octree->TraceLineOfSight(&camera_top, &point, true, -3, -3, true, 0) == 0;
        }
    } else if (target->iType == W8_TARGET_KIND_GROUP) {
        unsigned int group_list_index =
            GetMonsterGroupIndexByID(0x197, COMBAT_RANGE_CPP, target->iGroupID, false);
        if (group_list_index == 0xffffffff) {
            return false;
        }
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_list_index);
        if (group == 0) {
            srAssertFail("pMonsterGroup != NULL", COMBAT_RANGE_CPP, 0x1a0, 0);
        }
        W8TargetSource source;
        SetTargetSourceToCharacter(party_slot, &source);
        return IsTargetSourceInRangeOfGroup(&source, group, context);
    }
    return true;
}

/* Whether `party_slot` may aim at `monster_info`. The monster must be a live
   threat; with no targeting/combat/spell/item mode outstanding the pick is a
   plain sighting and the ranged aim flag applies, otherwise the slot's chosen
   action supplies the range category (a combat melee band loses the formation
   rows between slot and monster) and selects the aim flag. An allowed aim
   then has to pass the band distance to the monster. */
// FUNCTION: WIZ8 0x005194e0
bool CanPartyMemberAimAtMonster(int party_slot, int hand, W8MonsterInfo* monster_info,
                                W8TargetingContext context, char notify_failure)
{
    W8ActionDetailBlock* detail_block;
    bool flag;
    W8ActionKind action;
    int detail;
    int range;
    char rows;

    if (monster_info->party_threat.sight_state != W8_SIGHT_SEEN) {
        return false;
    }
    if (gXStatus.iTargetingMode == W8_TARGET_NEED_NONE && !gXStatus.fCombatMode &&
        !gXStatus.fSpellCastMode && !gXStatus.fItemSelectMode) {
        range = W8_RANGE_TOUCH;
        flag = true;
    } else {
        W8Character* character = &g_status.buffers.Char[party_slot];

        ChooseCombatAction(party_slot, context, &action, &detail, 0, &detail_block);
        switch (action) {
        case W8_ACTION_ATTACK:
        case W8_ACTION_BERSERK:
            range = GetCharAttackRange(character, hand);
            break;
        case W8_ACTION_BREATHE:
            range = W8_RANGE_LONG;
            break;
        case W8_ACTION_PROTECT:
            range = W8_RANGE_TOUCH;
            break;
        case W8_ACTION_CAST_SPELL:
            if (detail == W8_SPELL_NONE) {
                range = W8_RANGE_NONE;
                break;
            }
            range = g_spell_records[detail].range_category;
            break;
        case W8_ACTION_USE_ITEM:
            range = GetItemSpellRange(detail_block->item_use.item);
            break;
        default:
            range = W8_RANGE_NONE;
            break;
        }
        if (range == W8_RANGE_NONE) {
            return false;
        }
        ChooseCombatAction(party_slot, context, &action, 0, 0, 0);
        flag = action == W8_ACTION_TURN_UNDEAD ||
               (action > W8_ACTION_PROTECT && action < W8_ACTION_EQUIP);
        if (gXStatus.fCombatMode && range >= 0 && range < 2) {
            rows = CountRowsBetween(party_slot, monster_info);
            while (rows != 0) {
                if (range == W8_RANGE_TOUCH) {
                    range = W8_RANGE_NONE;
                    break;
                }
                --range;
                --rows;
            }
        }
        if (range == W8_RANGE_NONE) {
            if (notify_failure != 0) {
                QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_event_target_out_of_range,
                                    0, g_character_event_no_flags, g_character_event_full_volume);
            }
            return false;
        }
    }
    if (!monster_info->party_threat.los_flags[flag]) {
        if (notify_failure != 0) {
            QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_event_sight_blocked, 0,
                                g_character_event_no_flags, g_character_event_full_volume);
        }
        return false;
    }
    if (CalcRangeDistance(static_cast<W8RangeCategory>(range)) <
        monster_info->p3D->GetDistanceToPlayer()) {
        if (notify_failure != 0) {
            QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_event_target_out_of_range, 0,
                                g_character_event_no_flags, g_character_event_full_volume);
        }
        return false;
    }
    return true;
}

/* The slot-vs-slot reach check the target-list builders share: the slot's
   chosen action has to have a range, and melee range additionally has to get
   past the front rank. */
// FUNCTION: WIZ8 0x005197c0
bool CharacterActionReachesSlot(int party_slot, int hand, int target_slot,
                                W8TargetingContext context)
{
    if (static_cast<char>(party_slot) == target_slot) {
        return true;
    }
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8ActionKind kind;
    int action;
    W8ActionDetailBlock* detail;
    ChooseCombatAction(party_slot, context, &kind, &action, 0, &detail);
    W8RangeCategory range;
    switch (kind) {
    case W8_ACTION_ATTACK:
    case W8_ACTION_BERSERK:
        range = GetCharAttackRange(character, hand);
        break;
    case W8_ACTION_BREATHE:
        return true;
    case W8_ACTION_PROTECT:
        range = W8_RANGE_TOUCH;
        break;
    case W8_ACTION_CAST_SPELL:
        if (action == 0) {
            return false;
        }
        range = g_spell_records[action].range_category;
        break;
    case W8_ACTION_USE_ITEM:
        range = GetItemSpellRange(detail->item_use.item);
        break;
    default:
        return false;
    }
    if (range == W8_RANGE_NONE) {
        return false;
    }
    if (range != W8_RANGE_TOUCH) {
        return true;
    }
    if (FrontRankScreens(party_slot, target_slot)) {
        return false;
    }
    return true;
}

/* Whether `party_slot`'s action can reach any member of `group_id` at all.
   The character becomes the source for the shared range test; a miss can
   queue the character's complaint event when `notify` asks for it. */
// FUNCTION: WIZ8 0x00519920
bool IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context, char notify)
{
    W8TargetSource source;
    W8MonsterGroup* group;
    unsigned int index;

    index = GetMonsterGroupIndexByID(0x197, COMBAT_RANGE_CPP, group_id, false);
    if (index == 0xffffffff) {
        return false;
    }
    group = GetMonsterGroupByListIndex(index);
    if (group == 0) {
        srAssertFail("pMonsterGroup != NULL", COMBAT_RANGE_CPP, 0x1a0, 0);
    }
    SetTargetSourceToCharacter(party_slot, &source);
    if (!IsTargetSourceInRangeOfGroup(&source, group, context)) {
        if (notify != 0) {
            QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_event_target_out_of_range, 0,
                                g_character_event_no_flags, g_character_event_full_volume);
        }
        return false;
    }
    return true;
}

/* The range category the slot's chosen action works at. Attacks take the
   weapon's reach for the asked hand, a move acts at long range, spells and
   item uses ask their spell record, and anything unknown has no range. */
// FUNCTION: WIZ8 0x005199f0
W8RangeCategory GetCharActionRange(int party_slot, int hand, W8TargetingContext context)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8ActionDetailBlock* detail_block;
    W8ActionKind action;
    int detail;

    ChooseCombatAction(party_slot, context, &action, &detail, 0, &detail_block);
    switch (action) {
    case W8_ACTION_ATTACK:
    case W8_ACTION_BERSERK:
        return GetCharAttackRange(character, hand);
    case W8_ACTION_CAST_SPELL:
        if (detail != 0) {
            return g_spell_records[detail].range_category;
        }
        break;
    case W8_ACTION_USE_ITEM:
        return GetItemSpellRange(detail_block->item_use.item);
    case W8_ACTION_PROTECT:
        return W8_RANGE_TOUCH;
    case W8_ACTION_BREATHE:
        return W8_RANGE_LONG;
    default:
        break;
    }
    return W8_RANGE_NONE;
}

/* The range category the weapon in one of the character's hands attacks at.
   `hand` of HAND_ANY asks both hands and keeps the better answer; only a
   hand actually in play carrying a melee or thrown wield kind has a range to
   report at all. */
// FUNCTION: WIZ8 0x00519ac0
W8RangeCategory GetCharAttackRange(const W8Character* character, unsigned int hand)
{
    unsigned int index;
    W8RangeCategory best;
    W8RangeCategory range;

    if (hand >= 2) {
        if (hand != 2) {
            srAssertFail("uiHand == HAND_ANY", COMBAT_RANGE_CPP, 0x1f2, 0);
        }
        best = W8_RANGE_NONE;
        for (index = 0; index < 2; ++index) {
            if (character->Hand[index].in_play) {
                range = GetCharAttackRange(character, index);
                if (best < range) {
                    best = range;
                }
            }
        }
        return best;
    }
    if (!character->Hand[hand].in_play) {
        FormatDebugMessage(1,
                           "ERROR: GetCharAttackRange for hand %d which can't attack, uiChar = %d",
                           hand, CharacterPointerToPartySlot(character));
        return W8_RANGE_NONE;
    }
    if (character->Hand[hand].uiHolds != 1 && character->Hand[hand].uiHolds != 3) {
        return W8_RANGE_TOUCH;
    }
    return static_cast<W8RangeCategory>(
        g_item_records[character->EquippedItem[6 + (hand != 0)].iItemNo].range_category);
}

/* The furthest range category any of this character's hands can reach at. */
// FUNCTION: WIZ8 0x00519ba0
W8RangeCategory GetBestHandRangeCategory(const W8Character* character)
{
    W8RangeCategory best = W8_RANGE_NONE;
    W8RangeCategory category;
    unsigned int hand;

    for (hand = 0; hand < 2; ++hand) {
        if (character->Hand[hand].in_play) {
            category = GetCharAttackRange(character, hand);
            if (category > best) {
                best = category;
            }
        }
    }
    return best;
}

/* Whether the first lighting condition applies at distant or extreme range. */
// FUNCTION: WIZ8 0x00519be0
bool RangeCategoryUsesSightCondition(const W8MonsterInfo* monster, W8RangeCategory range_category)
{
    if (range_category >= W8_RANGE_LONG && range_category <= W8_RANGE_EXTREME) {
        return MonsterHasProjectileOrigin(monster);
    }
    return false;
}

/* Whether the monster's attack `attack` reaches anyone at all; `hostile_only`
   counts only those it is hostile to. In combat with berserk set the
   hostile filter is forced on. Party members and other monsters defer to the corresponding
   MonsterAttackReachesCharacter/Monster helpers. */
// FUNCTION: WIZ8 0x00519c00
bool MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack, char hostile_only)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    char disposition_needed;
    int party_slot;
    unsigned int index;
    unsigned int count;
    W8MonsterInfo* other;

    if (!monster_info->fInCombat || !monster_info->pCombat->berserk) {
        disposition_needed = hostile_only;
    } else {
        disposition_needed = 1;
    }
    disposition_needed = static_cast<char>((disposition_needed != 0) + 1);

    for (party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8Character* character = &g_status.buffers.Char[party_slot];

        if (!g_status.buffers.XChar[party_slot].fOccupied) {
            continue;
        }
        if (character->hp_current == 0) {
            continue;
        }
        if (character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        if (MonsterVsCharDisposition(party_slot, monster_info) != disposition_needed) {
            continue;
        }
        if (MonsterAttackReachesCharacter(monster_info, record, attack, party_slot)) {
            return true;
        }
    }

    count = PLLength(gXStatus.plsMonsterList);
    for (index = 0; index < count; ++index) {
        other = MonsterGetScriptPartByLocationIndex(index);
        GetMonsterDataForInfo(other);
        if (other != monster_info && other->fActive && other->fInCombat && other->hp_current != 0 &&
            other->highest_condition < W8_CONDITION_DEAD &&
            MonsterHostility(monster_info, other) == disposition_needed &&
            MonsterAttackReachesMonster(monster_info, record, attack, other)) {
            return true;
        }
        count = PLLength(gXStatus.plsMonsterList);
    }
    return false;
}

static int GetMonsterActionSightIndex(W8MonsterInfo* monster_info, unsigned int attack)
{
    if (monster_info->action_kind == W8_MONSTER_ACTION_ATTACK) {
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
        return RangeCategoryUsesSightCondition(
            monster_info, static_cast<W8RangeCategory>(record->attacks[attack].range_category));
    }
    if (monster_info->action_kind == W8_MONSTER_ACTION_SPELL) {
        return GetMonsterSpellSightIndex(monster_info);
    }
    return monster_info->action_kind == W8_MONSTER_ACTION_SPECIAL_ATTACK ? 2 : 0;
}

static W8RangeCategory GetMonsterPartyTargetRange(W8MonsterInfo* monster_info,
                                                  W8MonsterRecord* record, unsigned int attack,
                                                  int party_slot)
{
    int sight_index;
    W8RangeCategory range;

    if (monster_info->player_visibility.sight_state != W8_SIGHT_SEEN) {
        return W8_RANGE_NONE;
    }

    sight_index = GetMonsterActionSightIndex(monster_info, attack);
    if (!monster_info->player_visibility.los_flags[sight_index]) {
        return W8_RANGE_NONE;
    }

    range = GetMonsterActionRangeCategory(monster_info, record, attack);
    if (range == W8_RANGE_NONE) {
        return W8_RANGE_NONE;
    }
    CloseFormationGap(monster_info, party_slot, &range);
    if (range == W8_RANGE_NONE) {
        return W8_RANGE_NONE;
    }

    return range;
}

/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   given what it can see and how far away they stand. Combat mode shortens
   touch/short reach by CountRowsBetween. */
// FUNCTION: WIZ8 0x0051a2f0
bool MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                   unsigned int attack, int party_slot)
{
    W8RangeCategory range = GetMonsterPartyTargetRange(monster_info, record, attack, party_slot);
    if (range == W8_RANGE_NONE) {
        return false;
    }

    float distance = CalcRangeDistance(range);
    if (monster_info->p3D->GetDistanceToPlayer() <= distance) {
        return true;
    }
    return false;
}

/* Whether the monster's attack `attack` reaches another monster. Untargetable
   attackers never reach; the same monster always does. Sight comes from the
   mon-to-mon visibility row rather than the party record. */
// FUNCTION: WIZ8 0x0051a510
bool MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                 unsigned int attack, W8MonsterInfo* target)
{
    W8VisibilityRecord* visibility;
    int sight_index;
    W8RangeCategory range;

    if (record->untargetable != 0) {
        return false;
    }
    if (target == monster_info) {
        return true;
    }
    visibility = FindMonToMonVisibility(monster_info, target);
    if (visibility == 0) {
        return false;
    }
    if (visibility->sight_state != W8_SIGHT_SEEN) {
        return false;
    }

    sight_index = GetMonsterActionSightIndex(monster_info, attack);
    if (!visibility->los_flags[sight_index]) {
        return false;
    }

    range = GetMonsterActionRangeCategory(monster_info, record, attack);
    if (range == W8_RANGE_NONE) {
        return false;
    }

    float distance = CalcRangeDistance(range);
    if (monster_info->p3D->GetDistanceToMonster(target->p3D) <= distance) {
        return true;
    }
    return false;
}

/* The furthest range category among a monster's three attacks. Asking for the
   close-quarters band only considers the two categories inside it. */
// FUNCTION: WIZ8 0x0051a800
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, bool close_quarters_only)
{
    W8RangeCategory best = W8_RANGE_NONE;
    int attack;
    unsigned char category;

    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0) {
            category = record->attacks[attack].range_category;
            if ((!close_quarters_only || category < W8_RANGE_LONG) &&
                static_cast<int>(category) > best) {
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
                                            bool skip_capability_checks, int* out_sight)
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
        *out_sight = MonsterHasProjectileOrigin(monster_info);
    } else {
        *out_sight = 0;
    }

    if (!skip_capability_checks && record->prefer_ranged_actions != 1) {
        return best;
    }
    if (skip_capability_checks || record->flee_chance >= 0x50) {
        if (record->special_attack_kind != 0 &&
            g_special_attack_table[record->special_attack_kind][0] != 6 &&
            (skip_capability_checks || CanMonsterFlee(monster_info, record, true))) {
            if (best < W8_RANGE_LONG) {
                best = W8_RANGE_LONG;
                *out_sight = W8_RANGE_LONG;
            }
        }
    }
    if (!skip_capability_checks &&
        (record->prefer_ranged_actions != 1 || record->spell_chance < 0x50)) {
        return best;
    }

    for (spell = 0; spell < 10; ++spell) {
        unsigned int spell_id = record->spells[spell];

        if (!MonsterCanAimSpell(spell_id)) {
            continue;
        }
        if (!skip_capability_checks &&
            !IsSpellUsableByMonster(monster_info, static_cast<int>(spell_id), true)) {
            continue;
        }
        if (static_cast<int>(best) < static_cast<int>(g_spell_records[spell_id].range_category)) {
            best = g_spell_records[spell_id].range_category;
            *out_sight = GetMonsterSpellSightIndex(monster_info);
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
    case W8_MONSTER_ACTION_ATTACK:
        break;
    case W8_MONSTER_ACTION_SPELL:
        return g_spell_records[monster_info->action_detail].range_category;
    case W8_MONSTER_ACTION_SPECIAL_ATTACK:
        return W8_RANGE_LONG;
    case W8_MONSTER_ACTION_PROTECT:
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
    return steps * g_world_scale;
}

/* Source-relative action range: same band steps as CalcRangeDistance, then add
   the source's navigator radius - the party navigator for a character source,
   the individual monster's for a monster source. */
// FUNCTION: WIZ8 0x0051AA30
float CalcRangeDistance(W8RangeCategory range_category, W8TargetSource* source)
{
    float distance = CalcRangeDistance(range_category);
    if (TargetSourceIsCharacter(source, 0)) {
        return g_startup_world->movement.alternate_radius + distance;
    }
    if (TargetSourceIsMonster(source, 0)) {
        if (source->iMonsterID == -1) {
            srAssertFail("pSource->iMonsterID != -1", COMBAT_RANGE_CPP, 0x483, 0);
        }
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x484, COMBAT_RANGE_CPP, source->iMonsterID, true);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        return monster_info->p3D->movement.alternate_radius + distance;
    }
    return distance;
}

/* Party-relative action range for the world cursor: same band steps as
   CalcRangeDistance, then add the startup navigator's movement collision_radius. */
// FUNCTION: WIZ8 0x0051AB50
float CalcRangeDistanceFromParty(W8RangeCategory range_category)
{
    return CalcRangeDistance(range_category) + g_startup_world->movement.collision_radius;
}

/* Shrink a short-range category by the formation rows CountRowsBetween says
   stand between the monster and `party_slot`. Exhausting the category marks it
   unreachable (-1). */
// FUNCTION: WIZ8 0x0051abe0
void CloseFormationGap(W8MonsterInfo* monster_info, int party_slot, W8RangeCategory* range_category)
{
    char crossable;

    if (!gXStatus.fCombatMode) {
        return;
    }
    if (*range_category < W8_RANGE_TOUCH || *range_category >= W8_RANGE_LONG) {
        return;
    }
    crossable = CountRowsBetween(party_slot, monster_info);
    if (crossable == 0) {
        return;
    }
    while (*range_category != W8_RANGE_TOUCH) {
        --crossable;
        *range_category = static_cast<W8RangeCategory>(*range_category - 1);
        if (crossable == 0) {
            return;
        }
    }
    *range_category = W8_RANGE_NONE;
}

/* Whether anybody standing ahead of this position is still in formation. */
// FUNCTION: WIZ8 0x0051ae60
bool AnyoneStandsAhead(unsigned char position)
{
    unsigned char found = 0;
    unsigned int index;
    signed char slot;

    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status.formation.bOccupantChar[position][index];
        if (slot != -1 && g_status.buffers.Char[slot].bonus.out_of_formation == 0) {
            ++found;
        }
    }
    return found > 0;
}

/* How many formation rows between `party_slot` and the monster block a short
   reach. Same row answers zero; otherwise one when the monster's row is
   occupied, plus one more when the gap is exactly two rows and either that
   row or the front rank screens. */
// FUNCTION: WIZ8 0x0051aec0
char CountRowsBetween(int party_slot, W8MonsterInfo* monster_info)
{
    unsigned char monster_quadrant = static_cast<unsigned char>(GetMonsterQuadrant(monster_info));
    signed char party_quadrant = g_status.formation.positions[party_slot].bQuadrant;
    char rows = 0;
    unsigned int index;
    signed char slot;
    int gap;
    char found_front;

    if (monster_quadrant == static_cast<unsigned char>(party_quadrant)) {
        return 0;
    }

    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status.formation.bOccupantChar[monster_quadrant][index];
        if (slot != -1 && g_status.buffers.Char[slot].bonus.out_of_formation == 0) {
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
        slot = g_status.formation.bOccupantChar[4][index];
        if (slot != -1 && g_status.buffers.Char[slot].bonus.out_of_formation == 0) {
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
    unsigned char from_row = g_status.formation.positions[from_position].bQuadrant;
    unsigned char to_row = g_status.formation.positions[to_position].bQuadrant;
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
    rows_apart = from_row - static_cast<signed char>(to_row);
    if (rows_apart < 0) {
        rows_apart = -rows_apart;
    }
    if (rows_apart != 2) {
        return false;
    }

    found = 0;
    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status.formation.bOccupantChar[4][index];
        if (slot != -1 && g_status.buffers.Char[slot].bonus.out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* Every other seated, living party member whose side matches `relationship`
   and that the slot's chosen action could actually strike is collected; one of
   them is picked at random, or -1 when nobody qualifies. The berserk and
   turncoat paths use it to pick a victim. */
// FUNCTION: WIZ8 0x0051b0a0
int PickReachableSlotByDisposition(int party_slot, char relationship)
{
    int candidates[8];
    int* next = candidates;
    int count = 0;
    for (int slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
        if (slot == party_slot || !g_status.buffers.XChar[slot].fOccupied ||
            g_status.buffers.Char[slot].hp_current == 0 ||
            g_status.buffers.Char[slot].highest_condition >= W8_CONDITION_DEAD ||
            CharacterVsCharacterDisposition(party_slot, slot) != relationship) {
            continue;
        }
        for (unsigned int hand = 0; hand < 2; ++hand) {
            if (!CanHandReachTarget(party_slot, hand)) {
                continue;
            }
            if (!CharacterActionReachesSlot(party_slot, hand, slot,
                                            W8_TARGETING_CONTEXT_OUT_OF_COMBAT)) {
                continue;
            }
            ++count;
            *next = slot;
            ++next;
            break;
        }
    }
    if (count == 0) {
        return -1;
    }
    return candidates[Random(count)];
}

/* Two seven-byte constant readers the range rules share. */
// FUNCTION: WIZ8 0x0051b300
float GetMonsterEngagementRange(void)
{
    return g_float_005ec360;
}

// FUNCTION: WIZ8 0x0051b310
float GetGroundTargetRange(void)
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

    W8RangeCategory best_range = W8_RANGE_NONE;
    for (unsigned int attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0 &&
            best_range < record->attacks[attack].range_category) {
            best_range = static_cast<W8RangeCategory>(record->attacks[attack].range_category);
        }
    }

    srVector3T<float> position;
    monster_info->has_projectile_origin =
        best_range > W8_RANGE_SHORT && monster_info->p3D->GetProjectilePosition(&position);

    monster_info->has_spell_origin = record->spell_chance != 0 &&
                                     MonsterIsCycleSupported(monster_info->p3D, 0x19) &&
                                     monster_info->p3D->GetSpellPosition(&position);
}

// FUNCTION: WIZ8 0x0051b3f0
unsigned char TraceModeRejectsNoHit(int mode)
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
   the target kind and monster id when applicable. */
// FUNCTION: WIZ8 0x0051ac30
float MonsterChooseTarget(W8MonsterInfo* monster_info, W8CombatSlot* out, int kind)
{
    float best = 999999.0f;

    out->iType = W8_TARGET_KIND_NONE;
    if (monster_info->ubDisposition == W8_DISPOSITION_HOSTILE &&
        IsVisibleUnderConditions(monster_info, &monster_info->player_visibility, kind) &&
        (best = monster_info->p3D->GetDistanceToPlayer(), best < 999999.0f)) {
        out->iType = W8_TARGET_KIND_CHARACTER;
    }
    if (out->iType == W8_TARGET_KIND_NONE || !monster_info->pCombat->reconsider_action) {
        unsigned int count = PLLength(gXStatus.plsMonsterList);

        for (unsigned int index = 0; index < count; ++index) {
            W8MonsterInfo* other = MonsterGetScriptPartByLocationIndex(index);

            if (other != monster_info && other->fActive && other->hp_current != 0 &&
                other->fInCombat &&
                MonsterHostility(monster_info, other) == W8_DISPOSITION_HOSTILE) {
                W8VisibilityRecord* row = FindMonToMonVisibility(monster_info, other);
                if (IsVisibleUnderConditions(monster_info, row, kind)) {
                    float distance = monster_info->p3D->GetDistanceToMonster(other->p3D);
                    if (distance < best) {
                        out->iType = W8_TARGET_KIND_MONSTER;
                        out->iMonsterID = other->location_id;
                        best = distance;
                    }
                }
            }
            count = PLLength(gXStatus.plsMonsterList);
        }
    }
    return best;
}
/* Whether the monster's current action still reaches the slot `target`
   carries: its own line of sight has to hold for a character or the party, the
   target monster has to be reachable, and a group target is checked against
   every member. */
// FUNCTION: WIZ8 0x00519f80
bool MonsterActionReachesTarget(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                unsigned int attack, W8CombatSlot* target)
{
    W8RangeCategory range;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        int party_slot = target->iChar;
        range = GetMonsterPartyTargetRange(monster_info, record, attack, party_slot);
        if (range == W8_RANGE_NONE) {
            return false;
        }
        if (CalcRangeDistance(range) < monster_info->p3D->GetDistanceToPlayer()) {
            return false;
        }
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        unsigned int index =
            MonsterGetIndexByLocationID(0x310, COMBAT_RANGE_CPP, target->iMonsterID, true);
        W8MonsterInfo* target_monster = MonsterGetScriptPartByLocationIndex(index);
        return MonsterAttackReachesMonster(monster_info, record, attack, target_monster);
    } else if (target->iType == W8_TARGET_KIND_PARTY) {
        int party_slot = GetRandomCharacter(1, 1, -1, -1);
        range = GetMonsterPartyTargetRange(monster_info, record, attack, party_slot);
        if (range == W8_RANGE_NONE) {
            return false;
        }
        if (CalcRangeDistance(range) < monster_info->p3D->GetDistanceToPlayer()) {
            return false;
        }
    } else if (target->iType == W8_TARGET_KIND_GROUP) {
        W8TargetSource source;
        SetTargetSourceToMonster(monster_info, &source);
        unsigned int index =
            GetMonsterGroupIndexByID(0x39e, COMBAT_RANGE_CPP, target->iGroupID, true);
        W8MonsterGroup* group = GetMonsterGroupByListIndex(index);
        return IsTargetSourceInRangeOfGroup(&source, group, W8_TARGETING_CONTEXT_CURRENT);
    }
    return true;
}

// FUNCTION: WIZ8 0x0051ad60
int FindNearestVisibleGroupMonster(W8MonsterInfo* monster_info, int group_id, int kind)
{
    int best_id = -1;
    float best = 999999.0f;
    unsigned int index = GetMonsterGroupIndexByID(0x520, COMBAT_RANGE_CPP, group_id, true);
    W8MonsterGroup* group = GetMonsterGroupByListIndex(index);
    for (index = 0; index < ILLength(group->monsters); ++index) {
        int location_id = IListGetAt(group->monsters, index);
        unsigned int list_index =
            MonsterGetIndexByLocationID(0x525, COMBAT_RANGE_CPP, location_id, true);
        W8MonsterInfo* target = MonsterGetScriptPartByLocationIndex(list_index);
        if (target->fActive && target->hp_current != 0 && target->fInCombat) {
            W8VisibilityRecord* row = FindMonToMonVisibility(monster_info, target);
            if (IsVisibleUnderConditions(monster_info, row, kind)) {
                float distance = monster_info->p3D->GetDistanceToMonster(target->p3D);
                if (distance < best) {
                    best_id = target->location_id;
                    best = distance;
                }
            }
        }
    }
    return best_id;
}

/* The attack-origin offset for `kind` 1 (projectile) or 3 (spell): the
   monster's height offset, shifted by the delta between its navigator
   position and the muzzle/hand point when that point resolves. */
// FUNCTION: WIZ8 0x0051b320
void GetMonsterAttackSourceOffset(W8Monster* monster, int kind, srVector3T<float>* out)
{
    out->Set(0.0f, monster->movement.height_offset, 0.0f);
    if (kind == 1) {
        if (monster->GetProjectilePosition(out)) {
            srVector3T<float> position = monster->GetPosition();
            *out -= position;
            return;
        }
    } else {
        if (kind != 3) {
            return;
        }
        if (monster->GetSpellPosition(out)) {
            srVector3T<float> position = monster->GetPosition();
            *out -= position;
            return;
        }
    }
    srAssertFail("FALSE", COMBAT_RANGE_CPP, 0x646, 0);
}

// FUNCTION: WIZ8 0x0051b4e0
bool SourceActionReachesTarget(W8TargetSource* source, W8CombatSlot* target)
{
    if (TargetSourceIsCharacter(source, 0)) {
        return CharacterActionReachesTarget(source->iChar, 0, W8_TARGETING_CONTEXT_CURRENT);
    }
    if (TargetSourceIsMonster(source, 0)) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x69e, COMBAT_RANGE_CPP, source->iMonsterID, true);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
        return MonsterActionReachesTarget(monster_info, record, 0, target);
    }
    return true;
}
