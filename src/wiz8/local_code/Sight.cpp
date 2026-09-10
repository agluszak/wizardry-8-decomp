#include "wiz8/local_code/Sight.h"
#include "wiz8/character.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/magic.h"
#include "wiz8/monster_generators.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/npc_state.h"
#include "wiz8/startup_runtime_state.h"
#include "random.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/startup_world.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/combat_state.h"

#include <stdlib.h>
#include <string.h>

/*
 * Local Code\Sight.cpp.
 *
 * Who can see what. A monster carries one visibility record per other monster
 * it has an opinion about, and the two release paths below own that list. The
 * sweeps re-run the sight update over every live monster, once for each
 * direction the update takes.
 */

#define SIGHT_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Sight.cpp"

/* The two lighting conditions the per-condition visibility table is indexed
   by. Their meaning is not established; only that each shifts the lookup one
   entry along. */
struct W8SightConditions {
    unsigned char unknown_000[0x37a];
    unsigned char condition_37a;         /* 0x37a */
    unsigned char unknown_37b;
    unsigned char condition_37c;         /* 0x37c */
};

/* One row of per-visibility-kind flags. The two conditions above select
   between adjacent entries in the two pairs. */
struct W8VisibilityRow {
    unsigned char unknown_00[4];
    unsigned char visible_04;            /* 0x04 */
    unsigned char visible_05;            /* 0x05 */
    unsigned char unknown_06;
    unsigned char visible_07;            /* 0x07 */
    unsigned char unknown_08[3];
    unsigned char visible_0b;            /* 0x0b */
    unsigned char unknown_0c[0x1c];
    unsigned char visible_28;            /* 0x28 */
};

// GLOBAL: WIZ8 0x005ec254
float g_sight_default_005ec254 = 12.0f;

/* Put the sight subsystem back to its starting state. */
// FUNCTION: WIZ8 0x005048e0
void ResetSight(void)
{
    SetViewDistance(12.0f);
    SetNavigatorLinkMode00452F50(0);
    g_object_6598bc->ResetDurationScale();
    ResetMonsterGeneratorTimers0048CBE0();
}

/* Whether the view distance has been moved off its default. */
// FUNCTION: WIZ8 0x00504910
bool IsSightRangeOverridden(void)
{
    return GetViewDistance() != g_sight_default_005ec254;
}

/* Re-run both directions of the sight update for one monster. */
// FUNCTION: WIZ8 0x00505760
void RefreshMonsterSight(W8MonsterInfo* monster_info)
{
    UpdateMonsterSight(monster_info, 1, 0);
    UpdateMonsterSight(monster_info, 0, 0);
}

/* Re-run the outward direction over every live monster, and tell the combat
   display about it if a fight is on. The count is re-read every step because
   the update can remove a monster. */
// FUNCTION: WIZ8 0x00505780
void RefreshOutwardSightForAllMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 1, 0);
    }
    if (gXStatus.fCombatMode != 0) {
        Function593330();
        RefreshAllPartyTargets0053BF80();
    }
}

/* The inward direction on its own, with nothing to tell the display. */
// FUNCTION: WIZ8 0x005057d0
void RefreshInwardSightForAllMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 0, 0);
    }
}

/* Both directions over every monster, in two separate passes rather than one
   pass doing both - the outward pass and its display notification have to
   finish before the inward pass starts. */
// FUNCTION: WIZ8 0x00505810
void RefreshAllSight(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 1, 0);
    }
    if (gXStatus.fCombatMode != 0) {
        Function593330();
        RefreshAllPartyTargets0053BF80();
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 0, 0);
    }
}

/* Drop every visibility record anyone held about one departing monster. */
// FUNCTION: WIZ8 0x00505ba0
void ReleaseMonToMonVisibilityInfoAbout(int location_id)
{
    unsigned int monster_index;
    int index;
    W8MonsterInfo* monster_info;
    W8MonToMonVisibility* visibility;

    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
         ++monster_index) {
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->flag_14 == 0) {
            continue;
        }
        for (index = 0;
             index < (int)PLLength(monster_info->mon_to_mon_visibility);
             ++index) {
            visibility =
                (W8MonToMonVisibility*)PLGet(monster_info->mon_to_mon_visibility, index);
            if (visibility == 0) {
                return;
            }
            if (visibility->about_location_id == location_id) {
                visibility = (W8MonToMonVisibility*)PLRemoveAt(
                    monster_info->mon_to_mon_visibility, index);
                if (visibility == 0) {
                    srAssertFail(
                        "pVisibility != NULL", SIGHT_CPP, 968,
                        FormatString(
                            "ReleaseMonToMonVisibilityInfoAbout: ERROR - PLRemoveAt failed, index %d",
                            index));
                }
                free(visibility);
                break;
            }
        }
    }
}

/* Empty and destroy one monster's whole visibility list, then drop what
   everybody else held about it. */
// FUNCTION: WIZ8 0x00505c80
void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info)
{
    W8MonToMonVisibility* visibility;

    while ((int)PLLength(monster_info->mon_to_mon_visibility) > 0) {
        visibility =
            (W8MonToMonVisibility*)PLRemoveAt(monster_info->mon_to_mon_visibility, 0);
        if (visibility == 0) {
            srAssertFail(
                "pVisibility != NULL", SIGHT_CPP, 990,
                FormatString("ReleaseMonToMonVisibilityList: ERROR - PLRemoveAt failed"));
        }
        free(visibility);
    }
    if (PLDestroy(monster_info->mon_to_mon_visibility)) {
        monster_info->mon_to_mon_visibility = 0;
        ReleaseMonToMonVisibilityInfoAbout(monster_info->location_id);
    }
}

/* Whether a monster group still has anybody in it worth seeing: alive, not
   dying, not too far gone, and marked as a threat. */
// FUNCTION: WIZ8 0x00505ea0
bool MonsterGroupHasVisibleThreat(W8MonsterGroup* group)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < ILLength(group->monsters); ++index) {
        monster_info = MonsterInfoFromID(1120, SIGHT_CPP, IListGetAt(group->monsters, index), 1);
        if (monster_info->flag_14 != 0 && !monster_info->monster->IsDying() &&
            monster_info->hp_current != 0 && (unsigned int)monster_info->value_107 < 0xc &&
            monster_info->party_threat.state_04 == 1) {
            return true;
        }
    }
    return false;
}

/* Which of the two adjacent visibility entries the first lighting condition
   selects. */
// FUNCTION: WIZ8 0x00505e60
bool GetSightCondition37A(const W8SightConditions* conditions)
{
    return conditions->condition_37a != 0;
}

/* The second condition, answered as the entry index it picks rather than as a
   flag - two or three. */
// FUNCTION: WIZ8 0x00505e80
char GetSightCondition37CIndex(const W8SightConditions* conditions)
{
    return (conditions->condition_37c != 0) + 2;
}

/* Read one flag out of a visibility row. Two of the seven kinds are pairs that
   the lighting conditions choose between; the rest are fixed, and anything
   past the named kinds falls back to the last flag. */
// FUNCTION: WIZ8 0x00505dd0
bool IsVisibleUnderConditions(
    const W8SightConditions* conditions, const W8VisibilityRow* row, int kind)
{
    if (row == 0) {
        return false;
    }
    switch (kind) {
    case 0:
        return row->visible_04 != 0;
    case 1:
        return row->visible_0b != 0;
    case 2:
        return row->visible_05 != 0;
    case 3:
        return row->visible_07 != 0;
    case 4:
        return (&row->visible_05)[conditions->condition_37a != 0] != 0;
    case 5:
        return (&row->visible_07)[conditions->condition_37c != 0] != 0;
    default:
        return row->visible_28 != 0;
    }
}

extern void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3);
/* 0x00503990 */

/* Bring every monster's sight up to date with the clock, in whole two-minute
   steps - anything short of one step is left for next time. */
// FUNCTION: WIZ8 0x00504930
unsigned int AgeAllMonsterSight(void)
{
    unsigned int elapsed;
    unsigned int steps;
    unsigned int index;
    W8MonsterInfo* monster_info;

    elapsed = g_status_685170.world_clock * 1000 -
              g_status_685170.level_progress[g_status_685170.current_level].sight_clock * 1000;
    steps = elapsed / 120000;
    if (steps == 0) {
        return 0;
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if ((unsigned int)monster_info->value_107 < 0x12) {
            AgeMonsterSight(monster_info, steps, 1);
        }
    }
    return PLLength(gXStatus.plsMonsterList);
}

/* What one monster has recorded about another, if anything. Both monsters have
   to be in the world - the two assertions name that field fActive - and a null
   entry in the list is itself an error rather than an end marker. */
// FUNCTION: WIZ8 0x00505d20
W8MonToMonVisibility* FindMonToMonVisibility(
    W8MonsterInfo* source, int unused, W8MonsterInfo* target)
{
    int index;
    W8MonToMonVisibility* visibility;

    if (source->flag_14 == 0) {
        srAssertFail("pSourceMonsterInfo->fActive", SIGHT_CPP, 1020, 0);
    }
    if (target->flag_14 == 0) {
        srAssertFail("pTargetMonsterInfo->fActive", SIGHT_CPP, 1021, 0);
    }

    for (index = 0; index < (int)PLLength(source->mon_to_mon_visibility); ++index) {
        visibility =
            (W8MonToMonVisibility*)PLGet(source->mon_to_mon_visibility, index);
        if (visibility == 0) {
            srAssertFail("FALSE", SIGHT_CPP, 1030, 0);
        }
        else if (visibility->about_location_id == target->location_id) {
            return visibility;
        }
    }
    return 0;
}

/* Bring every monster's sight up to date from a cleared starting state: the
   two per-monster sight blocks are emptied first, then the outward pass, the
   combat display notification it feeds, and finally the inward pass. */
// FUNCTION: WIZ8 0x005060c0
void ResetAndRefreshAllSight005060C0(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info =
            MonsterGetScriptPartByLocationIndex(index);

        memset(&monster_info->party_threat, 0,
               sizeof(monster_info->party_threat));
        memset(&monster_info->player_visibility, 0,
               sizeof(monster_info->player_visibility));
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 1, 0);
    }
    if (g_in_combat_00683f94 != 0) {
        Function593330();
        RefreshAllPartyTargets0053BF80();
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 0, 0);
    }
}

// GLOBAL: WIZ8 0x00689b6c
int g_sight_marker_tick_00689b6c;
// GLOBAL: WIZ8 0x00689b70
int g_sight_fade_in_tick_00689b70;
// GLOBAL: WIZ8 0x00689b74
int g_sight_fade_out_tick_00689b74;
// GLOBAL: WIZ8 0x00683fc5
unsigned char g_sight_messages_enabled_00683fc5;

/* The effect ids the two "someone noticed you" notices post. Their slots are
   the four consecutive dwords the producer reads. */
// GLOBAL: WIZ8 0x005ee694
int g_sight_effect_005ee694 = 0x43;
// GLOBAL: WIZ8 0x005ee698
int g_sight_effect_005ee698 = 0x44;
// GLOBAL: WIZ8 0x005ee620
int g_sight_effect_005ee620 = 0x26;
// GLOBAL: WIZ8 0x005ee66c
int g_sight_effect_005ee66c = 0x39;
// GLOBAL: WIZ8 0x005ed7f8
const float g_sight_threat_scale_005ed7f8 = 0.6666667f;

/* Recompute one monster's sight state. Direction zero is the monster-to-
   monster pass, which revisits every other monster's per-monster record;
   nonzero is the player pass, which refreshes the party-facing record, stamps
   the notice and fade state, and ends in the player-to-monster flag pass. */
// FUNCTION: WIZ8 0x005049c0
void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int use_bounds)
{
    W8MonsterRecord* record;
    W8Monster* monster;
    float own_x;
    float own_y;
    float own_z;
    float viewing_distance;
    srVector3T<float> camera_position;
    srVector3T<float> other_position;
    srVector3T<float> trace_position;

    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME
        || monster_info->flag_14 == 0 || monster_info->hp_current == 0) {
        return;
    }
    viewing_distance = static_cast<float>(WorldGetFarClip(GetWorld()));
    if (viewing_distance <= g_float_005ebb34) {
        srAssertFail("flViewingDistance > 0", SIGHT_CPP, 0x5e, 0);
    }
    record = GetMonsterDataForInfo(monster_info);
    monster = monster_info->monster;
    own_x = monster->movement_0c0.position_040.x;
    own_y = monster->movement_0c0.position_040.y
            + monster->movement_0c0.height_offset_0b8;
    own_z = monster->movement_0c0.position_040.z;

    if (direction == 0) {
        unsigned int count;
        unsigned int index;

        if (g_in_combat_00683f94 == 0) {
            return;
        }
        index = 0;
        count = PLLength(gXStatus.plsMonsterList);
        if (count == 0) {
            return;
        }
        do {
            W8MonsterInfo* other = MonsterGetScriptPartByLocationIndex(index);

            if (other != monster_info && other->fInCombat != 0
                && other->flag_14 != 0 && other->hp_current != 0) {
                W8MonToMonVisibility* entry = 0;
                bool found = false;
                unsigned int record_index;

                for (record_index = 0;
                     record_index < PLLength(monster_info->mon_to_mon_visibility);
                     ++record_index) {
                    entry = static_cast<W8MonToMonVisibility*>(
                        PLGet(monster_info->mon_to_mon_visibility, record_index));
                    if (entry == 0) {
                        return;
                    }
                    if (entry->about_location_id == other->location_id) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    entry = static_cast<W8MonToMonVisibility*>(
                        malloc(sizeof(*entry)));
                    if (entry == 0) {
                        return;
                    }
                    memset(entry, 0, sizeof(*entry));
                    entry->about_location_id = other->location_id;
                    if (PLAdoptAppend(
                            monster_info->mon_to_mon_visibility, entry) == -1) {
                        return;
                    }
                }
                {
                    W8Monster* other_monster = other->monster;
                    float distance;

                    other_position.x =
                        other_monster->movement_0c0.position_040.x;
                    other_position.y =
                        other_monster->movement_0c0.position_040.y
                        + other_monster->movement_0c0.height_offset_0b8;
                    other_position.z =
                        other_monster->movement_0c0.position_040.z;
                    entry->flag_0b = 0;
                    entry->line_of_sight_28 = 0;
                    distance = monster->GetDistanceToMonster004C7DD0(
                        other->monster);
                    if (viewing_distance >= distance) {
                        unsigned char line_of_sight =
                            monster->HasLineOfSightToMonster004C4AF0(
                                other_monster);

                        entry->line_of_sight_28 = line_of_sight;
                        if (line_of_sight != 0) {
                            unsigned char can_see =
                                Function5058A0(monster_info, other, entry);

                            entry->flag_0b = can_see;
                            if (can_see != 0) {
                                entry->state_04 = 1;
                                entry->last_seen_clock_0c =
                                    g_status_685170.world_clock;
                                entry->subject_x_10 = static_cast<int>(own_x);
                                entry->subject_y_14 = static_cast<int>(own_y);
                                entry->subject_z_18 = static_cast<int>(own_z);
                                entry->target_x_1c =
                                    static_cast<int>(other_position.x);
                                entry->target_y_20 =
                                    static_cast<int>(other_position.y);
                                entry->target_z_24 =
                                    static_cast<int>(other_position.z);
                                goto sight_flags;
                            }
                        }
                    }
                    if (entry->last_seen_clock_0c == 0) {
                        entry->state_04 = 0;
                    }
                    else {
                        entry->state_04 = static_cast<unsigned char>(
                            (0xf0U < static_cast<unsigned int>(
                                g_status_685170.world_clock
                                - entry->last_seen_clock_0c)) ? 0 : 2);
                    }
                sight_flags:
                    if (entry->line_of_sight_28 == 0) {
                        memset(entry->sight_flags_05, 0, 4);
                    }
                    else {
                        monster->GetMonsterSightFlags004C4B70(
                            other->monster, entry->sight_flags_05,
                            entry->sight_flags_05 + 2);
                        if (monster_info->has_missile_37a != 0) {
                            unsigned char found =
                                monster->GetProjectilePosition004C77F0(
                                    &trace_position);
                            unsigned char clear;

                            if (found == 0) {
                                srAssertFail(
                                    "fFoundMissileVertex", SIGHT_CPP, 0x1f5, 0);
                                clear = 0;
                            }
                            else {
                                short line = g_octree_6598a4->TraceLineOfSight(
                                    &trace_position, &other_position, 1,
                                    monster_info->location_id,
                                    other->location_id, 1, 1);

                                clear = (line != 0) ? 0 : 1;
                            }
                            entry->sight_flags_05[1] = clear;
                        }
                        if (monster_info->has_spell_37c != 0) {
                            unsigned char found =
                                monster->GetSpellPosition004C78E0(
                                    &trace_position);

                            if (found == 0) {
                                srAssertFail(
                                    "fFoundSpellVertex", SIGHT_CPP, 0x1fd, 0);
                                entry->sight_flags_05[3] = 0;
                            }
                            else {
                                short line = g_octree_6598a4->TraceLineOfSight(
                                    &trace_position, &other_position, 1, -3,
                                    -3, 1, 0);

                                entry->sight_flags_05[3] = (line == 1) ? 0 : 1;
                            }
                        }
                    }
                }
            }
            ++index;
            count = PLLength(gXStatus.plsMonsterList);
            if (count <= index) {
                return;
            }
        } while (true);
    }

    GetCameraPosition(&camera_position);
    {
        float distance = monster->GetDistanceToPlayer004C7CB0();
        unsigned char visible_to_player;

        monster_info->flag_24d = distance <= viewing_distance;
        monster_info->player_visibility.flag_0b = 0;
        monster_info->player_visibility.line_of_sight_28 = 0;
        if (viewing_distance < distance) {
            if (monster_info->player_visibility.last_seen_clock_0c == 0) {
                monster_info->player_visibility.state_04 = 0;
            }
            else {
                monster_info->player_visibility.state_04 = static_cast<unsigned char>(
                    (0xf0U < static_cast<unsigned int>(
                        g_status_685170.world_clock - monster_info->player_visibility.last_seen_clock_0c))
                        ? 0 : 2);
            }
            goto after_sight;
        }
        monster_info->player_visibility.line_of_sight_28 =
            monster->CheckLineOfSightToPlayer004C4810();
        if (monster_info->player_visibility.line_of_sight_28 == 0) {
            goto after_sight;
        }
        if (monster_info->hp_current == 0 || monster_info->value_107 > 0xe) {
            visible_to_player = 0;
        }
        else {
            float yaw = GetCameraYawRadians();
            unsigned int light;
            unsigned int minimum_level = 9999;
            unsigned int character_offset = 0;
            int row_offset = 0;
            unsigned char fade_flag;
            float player_distance;

            if (g_in_combat_00683f94 == 0) {
                light = g_status_685170.value_232d;
            }
            else {
                light = 0;
            }
            do {
                W8PartySlotRow* row = reinterpret_cast<W8PartySlotRow*>(
                    reinterpret_cast<char*>(g_status_685170.buffers.party_rows)
                    + row_offset); /* reinterpret-ok: party rows are stored at
                                      their serialized 0x106 stride */
                W8Character* character = reinterpret_cast<W8Character*>(
                    reinterpret_cast<char*>(g_status_685170.buffers.characters)
                    + character_offset); /* reinterpret-ok: the party is stored
                                            at its serialized 0x1862 stride */

                if (row->occupied != 0 && character->hp_current != 0
                    && character->unknown_0b01 < 0xf
                    && character->level < minimum_level) {
                    minimum_level = character->level;
                }
                row_offset += 0x106;
                character_offset += 0x1862;
            } while (character_offset < 0xc310);

            player_distance = monster->GetDistanceToPlayer004C7CB0();
            if (record->kind_0cb == 4) {
                int bonus = record->missile_value_24f * 5;

                fade_flag = static_cast<unsigned char>(bonus > 0x7d ? 0x7d : bonus);
            }
            else {
                fade_flag = 0;
            }
            {
                float threshold = Function505A40(
                    own_x, own_y, own_z, camera_position.x, camera_position.y,
                    camera_position.z, yaw,
                    monster_info->converted_attributes_247[4], fade_flag,
                    monster_info->condition_turns[0xc] != 0,
                    record->kind_0cb == 0xc, static_cast<int>(minimum_level),
                    static_cast<int>(light), monster_info->player_visibility.state_04, 0,
                    player_distance);

                if (threshold < player_distance) {
                    visible_to_player = 0;
                }
                else {
                    visible_to_player = 1;
                }
            }
        }
        monster_info->player_visibility.flag_0b = visible_to_player;
        if (visible_to_player == 0) {
            goto after_sight;
        }
        monster_info->player_visibility.state_04 = 1;
        monster_info->player_visibility.last_seen_clock_0c = g_status_685170.world_clock;
        monster_info->player_visibility.own_position_1c.x = own_x;
        monster_info->player_visibility.own_position_1c.y = own_y;
        monster_info->player_visibility.own_position_1c.z = own_z;
        monster_info->player_visibility.camera_position_10 = camera_position;
    }

after_sight:
    if (monster_info->player_visibility.line_of_sight_28 == 0) {
        monster_info->player_visibility.sight_flags_05[0] = 0;
        monster_info->player_visibility.sight_flags_05[1] = 0;
        monster_info->player_visibility.sight_flags_05[2] = 0;
        monster_info->player_visibility.sight_flags_05[3] = 0;
    }
    else {
        monster->GetPlayerSightFlags004C4870(
            monster_info->player_visibility.sight_flags_05, monster_info->player_visibility.sight_flags_05 + 2);
        if (monster_info->has_missile_37a != 0) {
            unsigned char found =
                monster->GetProjectilePosition004C77F0(&trace_position);
            unsigned char clear;

            if (found == 0) {
                srAssertFail("fFoundMissileVertex", SIGHT_CPP, 0x9b, 0);
                clear = 0;
            }
            else {
                short line = g_octree_6598a4->TraceLineOfSight(
                    &trace_position, &camera_position, 1,
                    monster_info->location_id, -1, 1, 1);

                clear = (line != 0) ? 0 : 1;
            }
            monster_info->player_visibility.sight_flags_05[1] = clear;
        }
        if (monster_info->has_spell_37c != 0) {
            unsigned char found =
                monster->GetSpellPosition004C78E0(&trace_position);

            if (found == 0) {
                srAssertFail("fFoundSpellVertex", SIGHT_CPP, 0xa3, 0);
                monster_info->player_visibility.sight_flags_05[3] = 0;
            }
            else {
                short line = g_octree_6598a4->TraceLineOfSight(
                    &trace_position, &camera_position, 1, -3, -3, 1, 0);

                monster_info->player_visibility.sight_flags_05[3] = (line == 1) ? 0 : 1;
            }
        }
    }

    {
        unsigned char in_range = monster_info->flag_24d;

        monster_info->party_threat.flag_07 = 0;
        monster_info->party_threat.flag_25 = 0;
        monster_info->party_threat.threat_state_24 = 0;
        if (in_range != 0) {
            bool seen_by_party;

            if (monster_info->party_threat.state_04 != 0) {
                use_bounds = 1;
            }
            monster_info->party_threat.threat_state_24 =
                static_cast<unsigned char>(use_bounds);
            seen_by_party =
                monster->IsVisibleToPlayer004C4920(use_bounds) != 0;
            monster_info->party_threat.flag_25 = seen_by_party ? 1 : 0;
            if (seen_by_party) {
                float yaw;
                float distance;
                unsigned char npc_fade_flag;
                unsigned int character_offset;
                int row_offset;

                record = GetMonsterDataForInfo(monster_info);
                GetCameraPosition(&camera_position);
                yaw = GetCameraYawRadians();
                distance = monster->GetDistanceToPlayer004C7CB0();
                npc_fade_flag = 0;
                if (monster_info->fInCombat == 0) {
                    npc_fade_flag = record->flag_248;
                }
                character_offset = 0;
                row_offset = 0;
                seen_by_party = false;
                do {
                    W8PartySlotRow* row = reinterpret_cast<W8PartySlotRow*>(
                        reinterpret_cast<char*>(
                            g_status_685170.buffers.party_rows)
                        + row_offset); /* reinterpret-ok: serialized stride */
                    W8Character* character = reinterpret_cast<W8Character*>(
                        reinterpret_cast<char*>(
                            g_status_685170.buffers.characters)
                        + character_offset); /* reinterpret-ok: serialized stride */

                    if (row->occupied != 0 && character->hp_current != 0
                        && character->unknown_0b01 < 0xf) {
                        float threshold = Function505A40(
                            camera_position.x, camera_position.y,
                            camera_position.z, own_x, own_y, own_z, yaw,
                            character->attributes[6].effective,
                            static_cast<unsigned char>(
                                character->skills[15].level),
                            character->condition_turns[12] != 0,
                            character->current_profession == 6,
                            record->missile_value_24f, npc_fade_flag,
                            static_cast<int>(monster_info->party_threat.state_04),
                            g_status_685170.value_232d, distance);

                        if (g_status_685170.flag_238f != 0) {
                            threshold *= g_sight_threat_scale_005ed7f8;
                        }
                        if (threshold >= distance) {
                            seen_by_party = true;
                            break;
                        }
                    }
                    row_offset += 0x106;
                    character_offset += 0x1862;
                } while (row_offset < 0x830);
                monster_info->party_threat.flag_07 = seen_by_party ? 1 : 0;
                if (seen_by_party) {
                    if (GetViewDistance() == g_sight_default_005ec254) {
                        if (record == 0
                            || (SetFactionFlag(
                                    static_cast<char>(record->faction_id_25f),
                                    1),
                                (record->flags_0d0 & 1) == 0)) {
                            unsigned int now = static_cast<unsigned int>(
                                g_object_6598bc->GetValue30());

                            if ((((monster_info->party_threat.last_seen_clock_08 == 0)
                                  || (0x78U < static_cast<unsigned int>(
                                      g_status_685170.world_clock
                                      - monster_info->party_threat.last_seen_clock_08)))
                                 && (ShowMonsterTargetMarker(monster_info) == 0))
                                && (monster_info->flag_16 == 1
                                    || monster_info->flag_16 == 0)
                                && (g_sight_marker_tick_00689b6c == 0
                                    || now - g_sight_marker_tick_00689b6c
                                           > 199)) {
                                g_sight_marker_tick_00689b6c = now;
                                int party_slot =
                                    GetRandomCharacter(0, 0, -1, -1);

                                if (party_slot != -1) {
                                    int effect = g_sight_effect_005ee694;

                                    if (Random(2) == 0) {
                                        effect = g_sight_effect_005ee698;
                                    }
                                    W8StartupStateElement005EE748* notice =
                                        Function52E690(
                                            reinterpret_cast<W8Character*>(
                                                reinterpret_cast<char*>(
                                                    g_status_685170.buffers
                                                        .characters)
                                                + party_slot * 0x1862),
                                            effect, 0,
                                            g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914);

                                    if (notice != 0) {
                                        notice->value_30 = 0x5dc;
                                        notice->clock_34 = GetTickCount();
                                    }
                                }
                            }
                        }
                        else {
                            W8NpcState* npc =
                                GetNpcStateByKind(record->unknown_0cd[0]);

                            if (npc == 0) {
                                srAssertFail("pNPC != NULL", SIGHT_CPP, 0xe9,
                                             0);
                            }
                            else if (npc->unknown_2d == 0
                                     && npc->record->unknown_054 == 0
                                     && ShowMonsterTargetMarker(monster_info)
                                            != 0) {
                                srVector3T<float> own_position;
                                srVector3T<float> delta;

                                own_position.Set(own_x, own_y, own_z);
                                delta = own_position
                                        - g_startup_world_659c0c->GetPosition();
                                if (delta.Length() < 25000.0f
                                    && (npc->unknown_2d = 1,
                                        g_sight_messages_enabled_00683fc5 == 0)
                                    && g_status_685170.current_level != 4) {
                                    int party_slot =
                                        GetRandomCharacter(0, 0, -1, -1);

                                    if (party_slot != -1) {
                                        int effect = g_sight_effect_005ee620;

                                        if (npc->name_style == 0x18
                                            && GetFact(0x2ee) != 0) {
                                            effect = g_sight_effect_005ee66c;
                                        }
                                        W8StartupStateElement005EE748* notice =
                                            Function52E690(
                                                reinterpret_cast<W8Character*>(
                                                    reinterpret_cast<char*>(
                                                        g_status_685170.buffers
                                                            .characters)
                                                    + party_slot * 0x1862),
                                                effect, 0,
                                                g_effect_argument_005ed8c8,
                                                g_effect_argument_005ed914);

                                        if (notice != 0) {
                                            notice->value_30 = 0x5dc;
                                            notice->clock_34 =
                                                GetTickCount();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (monster_info->party_threat.state_04 != 1
                        && record->flag_248 != 0) {
                        unsigned int now = static_cast<unsigned int>(
                            g_object_6598bc->GetValue30());

                        monster->BeginFadeIn004C4F80(5.0f);
                        if (g_sight_messages_enabled_00683fc5 == 0
                            && (g_sight_fade_in_tick_00689b70 == 0
                                || now - g_sight_fade_in_tick_00689b70 > 199)) {
                            g_sight_fade_in_tick_00689b70 = now;
                            Function58AC00(
                                8, gppStringList[0x774 / 4], -1, -1, 0);
                        }
                    }
                    monster_info->party_threat.state_04 = 1;
                    monster_info->party_threat.last_seen_clock_08 = g_status_685170.world_clock;
                    monster_info->party_threat.camera_position_0c = camera_position;
                    monster_info->party_threat.own_position_18.x = own_x;
                    monster_info->party_threat.own_position_18.y = own_y;
                    monster_info->party_threat.own_position_18.z = own_z;
                    goto final_sight_flags;
                }
            }
            (void)seen_by_party;
        }
        if (monster_info->party_threat.state_04 == 1 && record->flag_248 != 0) {
            unsigned int now = static_cast<unsigned int>(
                g_object_6598bc->GetValue30());

            monster->BeginFadeOut004C5150(5.0f);
            if (g_sight_messages_enabled_00683fc5 == 0
                && (g_sight_fade_out_tick_00689b74 == 0
                    || now - g_sight_fade_out_tick_00689b74 > 199)) {
                g_sight_fade_out_tick_00689b74 = now;
                Function58AC00(
                    8, gppStringList[0x778 / 4], -1, -1, 0);
            }
        }
        if (monster_info->party_threat.last_seen_clock_08 == 0) {
            monster_info->party_threat.state_04 = 0;
        }
        else {
            monster_info->party_threat.state_04 = static_cast<unsigned char>(
                (0x78U < static_cast<unsigned int>(
                    g_status_685170.world_clock - monster_info->party_threat.last_seen_clock_08))
                    ? 0 : 2);
        }
        if (record != 0 && (record->flags_0d0 & 1) != 0) {
            W8NpcState* npc = GetNpcStateByKind(record->unknown_0cd[0]);

            if (npc == 0) {
                srAssertFail("pNPC != NULL", SIGHT_CPP, 0x15b, 0);
            }
            else if (npc->marked_e9 != 0) {
                Function50CF70(npc, 0);
            }
        }
    }

final_sight_flags:
    if (monster_info->party_threat.flag_25 == 0) {
        monster_info->party_threat.unknown_05[0] = 0;
        monster_info->party_threat.unknown_05[1] = 0;
        return;
    }
    monster->GetPlayerToMonsterSightFlags004C4A20(
        monster_info->party_threat.unknown_05, monster_info->party_threat.unknown_05 + 1, 0);
}

/* Advance one monster's whole aging cycle by the elapsed minutes: the
   look-around timers, the sight bookkeeping around its last-seen position,
   the per-turn regeneration and fatigue bookkeeping, condition, enchantment
   and effect countdowns, and finally the combat effect slots. */
// FUNCTION: WIZ8 0x00503990
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3)
{
    W8MonsterRecord* record;
    W8Monster* monster;
    bool frost_condition = false;

    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->fInCombat != 0) {
        int args[2];

        args[0] = 3;
        args[1] = monster_info->location_id;
        Function5526F0(monster_info->pCombat->entries_3e, args);
        goto after_early;
    }
    monster = monster_info->monster;
    if (monster->stay_home_291 != 0) {
        srVector3T<float> location;
        srVector3T<float> last_seen;
        srVector3T<float> delta;

        MonsterGetLocation(monster, &location);
        if (monster->formation.x == g_float_005ebb34
            && monster->formation.y == g_float_005ebb34
            && monster->formation.z == g_float_005ebb34) {
            monster->formation = monster->GetPosition();
        }
        last_seen = monster->formation;
        delta = last_seen - location;
        if (delta.Length() > 500.0f) {
            srVector3T<float> probe = last_seen;
            srVector3T<float> camera;

            probe.y += g_float_005ebc64;
            GetCameraPosition(&camera);
            if (ProjectPointThroughCamera004BE940(&probe) == 0
                && g_octree_6598a4->HasLineOfSight(&camera, &probe, 1) == 0) {
                srVector3T<float> notify_position = last_seen;
                unsigned int group_index = GetMonsterGroupIndexByID(
                    0x4d2, SIGHT_CPP, monster_info->monster_group_id, 1);
                W8MonsterGroup* group =
                    GetMonsterGroupByListIndex(group_index);

                Function510CC0(
                    group, &notify_position, monster->GetYaw(), 0, 0, 0, 0);
                for (int index = 0; index < 4; ++index) {
                    if (group->allied_group_ids[index] != 0) {
                        group_index = GetMonsterGroupIndexByID(
                            0x4d9, SIGHT_CPP, group->allied_group_ids[index], 1);
                        group = GetMonsterGroupByListIndex(group_index);
                        Function510CC0(
                            group, &notify_position, monster->GetYaw(), 0, 0, 0,
                            0);
                    }
                }
            }
        }
        goto after_early;
    }
    if (GetViewDistance() != g_sight_default_005ec254) {
        goto after_early;
    }
    if ((monster->linked_navigator_05c == 0 && monster->flag_025 == 0)
        && ((signed char)monster_info->unknown_254 > 1
            || monster->flag_024 == 0)) {
        srVector3T<float> location;
        srVector3T<float> previous;
        srVector3T<float> delta;
        unsigned char cycle;

        MonsterGetLocation(monster, &location);
        previous.x = static_cast<float>(monster_info->runtime_values_338[0]);
        previous.y = static_cast<float>(monster_info->runtime_values_338[1]);
        previous.z = static_cast<float>(monster_info->runtime_values_338[2]);
        monster_info->position_17.y = location.y;
        cycle = monster_info->unknown_254;
        delta.x = location.x - previous.x;
        monster_info->position_17.x = location.x;
        monster_info->position_17.z = location.z;
        delta.y = location.y - previous.y;
        delta.z = location.z - previous.z;
        if ((signed char)cycle > 1) {
            bool cycle_cleared = false;
            bool flags_cleared = false;

            if ((signed char)cycle < 4) {
                bool cleared = false;

                if (delta.Length() < 500.0f) {
                    srVector3T<float> navigator_position =
                        monster->GetPosition();

                    if (g_pathing_00659c60->SnapWaypointPosition00462E60(
                            &navigator_position, 0) == 0
                        && monster_info->party_threat.flag_25 == 0) {
                        srVector3T<float> next_position;

                        monster->movement_0c0.attachment_0ac
                            ->GetNextPosition00456660(&next_position);
                        if (next_position.Length()
                            == static_cast<float>(g_zero_005ebb40)) {
                            next_position = monster->GetPosition();
                        }
                        {
                            srVector3T<float> camera;
                            srVector3T<float> probe = next_position;

                            GetCameraPosition(&camera);
                            if (ProjectPointThroughCamera004BE940(&probe) == 0
                                && g_octree_6598a4->HasLineOfSight(
                                       &camera, &probe, 1) == 0) {
                                srVector3T<float> notify_position =
                                    next_position;
                                unsigned int group_index =
                                    GetMonsterGroupIndexByID(
                                        0x50f, SIGHT_CPP,
                                        monster_info->monster_group_id, 1);
                                W8MonsterGroup* group =
                                    GetMonsterGroupByListIndex(group_index);

                                Function510CC0(
                                    group, &notify_position, monster->GetYaw(),
                                    0, 0, 0, 0);
                                for (int index = 0; index < 4; ++index) {
                                    if (group->allied_group_ids[index] != 0) {
                                        group_index = GetMonsterGroupIndexByID(
                                            0x516, SIGHT_CPP,
                                            group->allied_group_ids[index], 1);
                                        group = GetMonsterGroupByListIndex(
                                            group_index);
                                        Function510CC0(
                                            group, &notify_position,
                                            monster->GetYaw(), 0, 0, 0, 0);
                                    }
                                }
                                cleared = true;
                            }
                        }
                    }
                }
                cycle_cleared = cleared;
                flags_cleared = true;
            }
            else {
                srVector3T<float> camera;
                srVector3T<float> probe;
                srVector3T<float> location2;

                GetCameraPosition(&camera);
                location2 = monster->GetPosition();
                probe = location2;
                probe.y += g_float_005ebc64;
                if (ProjectPointThroughCamera004BE940(&location2) == 0
                    && g_octree_6598a4->HasLineOfSight(&camera, &probe, 1)
                           == 0) {
                    if (monster->formation.x == g_float_005ebb34
                        && monster->formation.y == g_float_005ebb34
                        && monster->formation.z == g_float_005ebb34) {
                        monster->formation = monster->GetPosition();
                    }
                    probe = monster->formation;
                    probe.y += g_float_005ebc64;
                    if (ProjectPointThroughCamera004BE940(&location2) == 0
                        && g_octree_6598a4->HasLineOfSight(
                               &camera, &probe, 1) == 0) {
                        if (monster->formation.x == g_float_005ebb34
                            && monster->formation.y == g_float_005ebb34
                            && monster->formation.z == g_float_005ebb34) {
                            monster->formation = monster->GetPosition();
                        }
                        {
                            srVector3T<float> notify_position =
                                monster->formation;
                            unsigned int group_index =
                                GetMonsterGroupIndexByID(
                                    0x53b, SIGHT_CPP,
                                    monster_info->monster_group_id, 1);
                            W8MonsterGroup* group =
                                GetMonsterGroupByListIndex(group_index);

                            Function510CC0(
                                group, &notify_position, monster->GetYaw(), 0, 0,
                                0, 0);
                            for (int index = 0; index < 4; ++index) {
                                if (group->allied_group_ids[index] != 0) {
                                    group_index = GetMonsterGroupIndexByID(
                                        0x542, SIGHT_CPP,
                                        group->allied_group_ids[index], 1);
                                    group = GetMonsterGroupByListIndex(
                                        group_index);
                                    Function510CC0(
                                        group, &notify_position,
                                        monster->GetYaw(), 0, 0, 0, 0);
                                }
                            }
                        }
                    cycle_cleared = true;
                    flags_cleared = true;
                }
            }
            }
            if (cycle_cleared) {
                monster_info->unknown_254 = 0;
            }
            if (flags_cleared) {
                monster_info->flag_255 = 0;
                monster_info->unknown_246 = 0;
            }
        }
        if ((signed char)monster_info->unknown_254 > 0) {
            ++monster_info->unknown_254;
        }
        monster_info->runtime_values_338[0] =
            static_cast<int>(monster_info->position_17.x);
        monster_info->runtime_values_338[1] =
            static_cast<int>(monster_info->position_17.y);
        monster_info->runtime_values_338[2] =
            static_cast<int>(monster_info->position_17.z);
    }

after_early:
    {
        unsigned int amount =
            monster_info->runtime_block_1db.unknown_07[1];

        if (amount != 0) {
            W8TargetSource source;

            amount *= minutes;
            if (monster_info->fInCombat != 0) {
                amount += amount >> 1;
            }
            ResetTargetSource(&source);
            Function52BB60(
                monster_info, amount, &source, 1, g_in_combat_00683f94, 0, 0, 0);
        }
    }
    if (monster_info->condition_turns[2] != 0) {
        frost_condition = true;
    }
    {
        W8MonsterRecord* data = GetMonsterDataForInfo(monster_info);
        int amount = (static_cast<int>(
                          static_cast<signed char>(data->unknown_150[0x2c]))
                      + static_cast<int>(static_cast<signed char>(
                            monster_info->runtime_block_1db.unknown_07[2])))
                     * static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                W8TargetSource source;

                ResetTargetSource(&source);
                Function52BB60(monster_info, -amount, &source, 0, 0, 0, 0, 0);
            }
        }
        else if (monster_info->hp_current < monster_info->hp_max) {
            HealMonster(monster_info, amount, 0);
        }
    }
    {
        int amount =
            (static_cast<int>(
                 static_cast<signed char>(
                     monster_info->runtime_block_1db.unknown_07[3]))
             + static_cast<int>(static_cast<signed char>(record->unknown_0cd[1])))
            * static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                FatigueMonster(monster_info, -amount, 0);
            }
        }
        else if (monster_info->runtime_stat_current_33
                 < monster_info->runtime_stat_max_2f) {
            RestoreMonsterStamina(monster_info, amount, 0);
        }
    }
    {
        float heal_scale;

        if (g_sight_messages_enabled_00683fc5 == 0 && arg_3 == 0) {
            if (monster_info->fInCombat == 0) {
                heal_scale = 0.5f;
            }
            else {
                heal_scale = 0.0f;
            }
        }
        else {
            heal_scale = 1.0f;
        }
        if (frost_condition) {
            heal_scale *= g_navigator_vertical_phase_step_005ebcc8;
        }
        if (heal_scale > g_float_005ebb34) {
            if (monster_info->hp_current < monster_info->hp_max) {
                monster_info->hp_regen_accumulator_4b =
                    static_cast<float>(minutes)
                        * monster_info->hp_regen_rate_47 * heal_scale
                    + monster_info->hp_regen_accumulator_4b;
                int healed =
                    static_cast<int>(monster_info->hp_regen_accumulator_4b);

                HealMonster(monster_info, healed, 0);
                monster_info->hp_regen_accumulator_4b -=
                    static_cast<float>(healed);
            }
            if (monster_info->runtime_stat_current_33
                < monster_info->runtime_stat_max_2f) {
                monster_info->stamina_regen_accumulator_53 =
                    static_cast<float>(minutes)
                        * monster_info->stamina_regen_rate_4f * heal_scale
                    + monster_info->stamina_regen_accumulator_53;
                int restored =
                    static_cast<int>(monster_info->stamina_regen_accumulator_53);

                RestoreMonsterStamina(monster_info, restored, 0);
                monster_info->stamina_regen_accumulator_53 -=
                    static_cast<float>(restored);
            }
        }
    }
    for (int condition = 0; condition < 0x14; ++condition) {
        if (monster_info->condition_turns[condition] != 0
            && static_cast<unsigned int>(
                   monster_info->condition_turns[condition])
                   < 9999) {
            Function524110(monster_info->location_id, condition, minutes);
        }
    }
    for (int enchant = 0; enchant < 8; ++enchant) {
        float remaining =
            static_cast<float>(monster_info->enchantments[enchant].value_08);

        if (remaining != 0.0f
            && static_cast<unsigned int>(remaining) < 9999) {
            if (minutes < static_cast<unsigned int>(remaining)) {
                monster_info->enchantments[enchant].value_08 =
                    static_cast<int>(remaining) - static_cast<int>(minutes);
            }
            else {
                ClearMonsterEnchantmentSlot(monster_info->location_id, enchant);
            }
        }
    }
    if (monster_info->hp_max <= monster_info->hp_current) {
        monster_info->hp_regen_accumulator_4b = 0.0f;
    }
    if (monster_info->runtime_stat_max_2f
        <= monster_info->runtime_stat_current_33) {
        monster_info->stamina_regen_accumulator_53 = 0.0f;
    }
    {
        W8EffectSlot* slot = monster_info->effect_slots_10f;

        for (int owned_index = 0; owned_index < 0xc; ++owned_index) {
            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d = static_cast<int>(slot->duration_0d)
                                        - static_cast<int>(minutes);
                }
                else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                }
                else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
            ++slot;
        }
    }
    if (monster_info->fInCombat != 0) {
        for (int combat_index = 0; combat_index < 9; ++combat_index) {
            W8EffectSlot* slot =
                &monster_info->pCombat->entries_3e[combat_index];

            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d = static_cast<int>(slot->duration_0d)
                                        - static_cast<int>(minutes);
                }
                else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                }
                else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
        for (int d7_index = 0; d7_index < 6; ++d7_index) {
            W8EffectSlot* slot =
                &monster_info->pCombat->entries_d7[d7_index];

            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d = static_cast<int>(slot->duration_0d)
                                        - static_cast<int>(minutes);
                }
                else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                }
                else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
    }
}
