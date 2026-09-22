#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Noise.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/character_skills.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/startup_world.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/local_code/GroupAttacks.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"

#include "sgp.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/engine_code/GameData.h"

/*
 * Local Code\MonsterAI.cpp.
 *
 * What a monster decides to do. Each monster's combat state owns a queue of
 * decided actions; the bodies here build entries for it, run the per-monster
 * decision over every live monster, and answer the questions the decision
 * itself asks about targets.
 */

#define MONSTER_AI_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterAI.cpp"

/* The special-attack table row value that marks a summon; flee and control
   rules treat those rows differently. */
enum { W8_SPECIAL_ATTACK_EFFECT_SUMMON = 6 };

/* The spell the AI casts when it wants a place rather than a target. */
enum { W8_AI_SPELL_PLACE = 0x77 };

/* The monster action kinds the AI validates. */
enum { W8_MONSTER_ACTION_ATTACK = 0, W8_MONSTER_ACTION_SPELL = 2, W8_MONSTER_ACTION_FLEE = 3 };

/* Gates the out-of-combat check that gives a group whose leader is still up a
   nudge. Only UpdateMonsterGroups reads it. */
// GLOBAL: WIZ8 0x0061CC10
static unsigned char g_flag_0061cc10 = 1;

/* The frame counter the staggered group update rotates through the group
   list, one fifth per frame with a full-distance pass every twentieth. */
// GLOBAL: WIZ8 0x0068D520
static unsigned int g_monster_group_tick = 0;

/* Member info by group list position, defined near the end of the file and
   always inlined into its callers. */
static inline W8MonsterInfo* GetGroupMemberInfo(W8MonsterGroup* group, unsigned int index);
void QueueMonsterAction(W8MonsterInfo* monster_info, int action_kind, int action_detail,
                        int attack_index, W8TargetKind target_kind, int target_value);

/* 125000, the cap on how far a monster will walk to investigate a heard
   noise. */
// GLOBAL: WIZ8 0x00617AE8
const int g_int_00617ae8 = 125000;

struct W8SpellEffectEntry;
/* 0x0061EEFC: two dwords per special-attack kind; only the leading dword is
   read here. */
// GLOBAL: WIZ8 0x0061EEFC
extern const int g_special_attack_table[32][2] = {
    {0, 0}, {1, 0}, {1, 0}, {2, 3}, {4, 0}, {1, 1}, {1, 1}, {1, 0}, {5, 4}, {5, 0}, {1, 5},
    {1, 1}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 1}, {1, 5}, {1, 0},
    {1, 1}, {1, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {5, 0}, {6, 0},
};

/* The spell that fills each being effect slot, walked by
   MonsterSpellTargetOK to tell whether a buff is already running; the
   monster side indexes effect_slots_10f, the party side the matching rows
   in g_status_685170. */
// GLOBAL: WIZ8 0x00616D84
const int g_being_effect_slot_spells_00616d84[12] = {
    0x20, 0x21, 0x11, 0x14, 0x8, 0x28, 0x1a, 0x2d, 0x40, 0, 0, 0,
};

/* The combat-state spell per effect slot, walked against
   W8CombatState::effect_slots and the monster's effect_slots_3e. */
// GLOBAL: WIZ8 0x00616DB4
const int g_combat_effect_slot_spells_00616db4[9] = {
    0x31, 0x30, 0x4c, 0x51, 0x5d, 0x50, 0, 0, 0,
};

/* The same mapping for the second combat effect block, indexed against
   W8CombatState::effect_slots_85a and the monster's effect_slots_d7. One
   retail array: [0..6) are spell ids, [6..23) — the retail sub-table at
   0x00616DF0 — is the spell-point budget/failure percentage indexed by cost
   band that Magic.cpp's failure and power-level readers consume. */
// GLOBAL: WIZ8 0x00616DD8
const int g_combat_effect_slot_spells_and_cast_success_00616dd8[23] = {
    0x2, 0x35, 0x3b, 0x3e, 0,  0,  30,  40,  50,  58,  64,  70,
    76,  81,   86,   90,   94, 97, 100, 102, 105, 107, 110,
};

/* The per-slot weights ChooseMonsterSpell rolls against. */
// GLOBAL: WIZ8 0x0061CC14
static const int g_spell_cast_weights_0061cc14[10] = {5, 5, 5, 10, 10, 10, 10, 15, 15, 15};

/* 0x005EE768: 1500.0, the "close enough" distance for patrol points and heard
   noises. */
// GLOBAL: WIZ8 0x005EE768
extern const double g_double_005ee768 = 1500.0;

/* 0x005EE774: scales the record float into the group-engagement probe
   distance. */
// GLOBAL: WIZ8 0x005EE774
extern const float g_float_005ee774 = 333.33333f;

/* 0x005EE77C: 7500.0, the floor added to the engagement range bound the
   group combat checks compare nearest-member distances against. */
// GLOBAL: WIZ8 0x005EE77C
extern const float g_float_005ee77c = 7500.0f;

/* 0x005EE780: 1.15, the slack the reinforcement check gives a hostile
   monster's distance to the player before it counts as near the group. */
// GLOBAL: WIZ8 0x005EE780
extern const float g_float_005ee780 = 1.15f;

/* Reported once, so a monster missing its special-attack cycle does not flood
   the log. */
// GLOBAL: WIZ8 0x0068D525
static unsigned char g_special_attack_cycle_error_reported;

/* The timed sight service. The dirty flag is raised by the condition and
   enchantment writers whenever what monsters can see of the party may have
   changed; out of combat sight is refreshed unconditionally, in combat only
   when the flag says so, and detection runs while the combat state permits. */
// FUNCTION: WIZ8 0x00530110
void UpdateMonsterSight(void)
{
    if (gXStatus.fCombatMode == 0 || gXStatus.sight_refresh_pending_a03 != 0) {
        if (gXStatus.sight_refresh_pending_a03 != 0) {
            gXStatus.sight_refresh_pending_a03 = 0;
        }
        RefreshOutwardSightForAllMonsters();
        if (gXStatus.fCombatMode != 0 && g_combat_state->round_count_004 == 0) {
            CheckMonsterGroupsEnterCombat();
        }
    }
}

/* The per-frame monster group pass. When `staggered` is set the sight block
   is left to UpdateMonsterSight and the maintenance below is spread across
   the group list a fifth at a time, with a full-distance check every
   twentieth frame; groups further than twice the far clip are detached and
   groups close enough get their members loaded. */
// FUNCTION: WIZ8 0x00530150
void UpdateMonsterGroups(char staggered)
{
    W8MonsterGroup* monster_group;
    W8MonsterInfo* monster_info;
    srVector3T<float> camera_position;
    float nearest_distance;
    unsigned int group_list_index;

    if (AnyCharacterActive() == 0) {
        return;
    }
    if (PLLength(gXStatus.plsMonsterGroupList) == 0) {
        return;
    }
    g_monster_group_tick = g_monster_group_tick + 1;
    if (staggered == 0) {
        if (gXStatus.fCombatMode == 0 || gXStatus.sight_refresh_pending_a03 != 0) {
            if (gXStatus.sight_refresh_pending_a03 != 0) {
                gXStatus.sight_refresh_pending_a03 = 0;
            }
            RefreshOutwardSightForAllMonsters();
            if (gXStatus.fCombatMode != 0 && g_combat_state->round_count_004 == 0) {
                CheckMonsterGroupsEnterCombat();
            }
        }
    }
    WorldGetCameraLocation(GetWorld(), &camera_position);
    for (group_list_index = 0; group_list_index < PLLength(gXStatus.plsMonsterGroupList);
         ++group_list_index) {
        if (staggered != 0 && group_list_index % 5 != g_monster_group_tick % 5) {
            continue;
        }
        monster_group = GetMonsterGroupByListIndex(group_list_index);
        if (MonsterGroupAllMembersDying00511850(monster_group) != 0) {
            continue;
        }
        monster_info = MonsterInfoFromID(113, MONSTER_AI_CPP, monster_group->leader_id_9f, 1);
        if (monster_info == 0 || monster_info->monster == 0) {
            continue;
        }
        nearest_distance = GetGroupNearestDistance(monster_group);
        if (staggered != 0 && group_list_index % 20 != g_monster_group_tick % 20 &&
            (WorldGetFarClip(GetWorld()) < nearest_distance ||
             group_list_index % 5 != g_monster_group_tick % 5)) {
            continue;
        }
        if (monster_group->members_active_28 == 0) {
            if (nearest_distance < WorldGetFarClip(GetWorld()) * g_float_005ec3b8) {
                LoadMonsterGroupMembers(monster_group);
            }
        }
        if (monster_group->members_active_28 != 0 && monster_group->fInCombat == 0) {
            double far_clip;
            if (monster_group->leader_group_id == 0) {
                RefreshMonsterGroupHostility005113A0(monster_group);
            }
            far_clip = WorldGetFarClip(GetWorld());
            if (nearest_distance <= far_clip + far_clip) {
                if (gXStatus.fCombatMode == 0 && MonsterGroupCanEngage(monster_group) != 0) {
                    MonsterGroupEnterCombat(monster_group);
                }
            } else {
                DetachMonsterGroup(monster_group);
            }
        }
        if (g_flag_0061cc10 != 0 && gXStatus.fCombatMode == 0) {
            if (monster_group == 0) {
                srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 207, 0);
            }
            if (monster_group->leader_group_id == 0) {
                monster_info =
                    MonsterInfoFromID(215, MONSTER_AI_CPP, monster_group->leader_id_9f, 1);
                if (monster_info->monster->IsDying() == 0) {
                    DoMonsterRTAI(monster_info, 1);
                }
            }
        }
    }
}

/* How aware the group is of the party: zero while the global gates say the
   group cannot react or its representative record is untargetable, one the
   moment any live member's party-visibility state is one, and two while the
   best state a live member reports is two. Members with no live entry, a
   dying body, no hit points or a condition at 0x0c or higher do not count. */
// FUNCTION: WIZ8 0x00530470
unsigned char GetMonsterGroupPartySightState(W8MonsterGroup* monster_group)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int index;
    unsigned char result;

    result = 0;
    if (g_status_685170.world_suspended_2390 != 0 || GetFlag68F105() != 0) {
        return 0;
    }
    monster_info = MonsterInfoFromID(0xf0, MONSTER_AI_CPP, monster_group->leader_id_9f, 1);
    record = GetMonsterDataForInfo(monster_info);
    if (record != 0 && record->untargetable_24a != 0) {
        return 0;
    }
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        int location_id;

        location_id = IListGetAt(monster_group->monsters, index);
        monster_info = MonsterInfoFromID(0xfe, MONSTER_AI_CPP, location_id, 1);
        if (monster_info->fActive != 0 && monster_info->monster->IsDying() == 0 &&
            monster_info->hp_current != 0 && monster_info->highest_condition < 0xc) {
            if (monster_info->player_visibility.sight_state_04 == W8_SIGHT_SEEN) {
                return 1;
            }
            if (monster_info->player_visibility.sight_state_04 == W8_SIGHT_RECENT) {
                result = 2;
            }
        }
    }
    return result;
}

/* The real-time AI decision for one monster. The `engage` call form is the
   trigger path: while sight is overridden and the monster still has its
   ambush flag set it moves to within its longest out-of-reach attack band of
   the party and activates its group. Otherwise the disposition picks
   a mode: the scripted-orders helper for dispositions zero and two, a
   hit-point retreat check for one. The chosen mode is committed through
   ApplyMonsterRTAIDecision when it differs from what ai_mode_255 already holds. */
// FUNCTION: WIZ8 0x00530560
void DoMonsterRTAI(W8MonsterInfo* monster_info, char engage)
{
    char update;
    unsigned char decision;

    update = 0;
    decision = 0;
    if (engage != 0 && IsSightRangeOverridden() != 0 && monster_info->ai_mode_255 == 1) {
        W8MonsterRecord* record;
        W8MonsterGroup* monster_group;
        W8Navigator* navigator;
        unsigned char best_range;
        unsigned int attack;

        if (monster_info->fInCombat != 0) {
            return;
        }
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MONSTER_AI_CPP, 0x479, 0);
        }
        record = GetMonsterDataForInfo(monster_info);
        best_range = 0;
        for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
            if (RateMonsterAttack(monster_info, record, attack, 0, 0) ==
                    W8_MONSTER_ATTACK_OUT_OF_REACH &&
                best_range < record->attacks[attack].range_category) {
                best_range = record->attacks[attack].range_category;
            }
        }
        if (MonsterApproachStartupNavigator004C5FF0(monster_info->monster,
                                                    CalcRangeDistance((W8RangeCategory)best_range) *
                                                        g_float_005ec390) == 0) {
            return;
        }
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x12b, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
        MonsterGroupEnterCombat(monster_group);
        navigator = monster_info->monster->linked_navigator_05c;
        if (navigator != 0) {
            navigator->group_linked_0bd = 1;
        } else {
            monster_info->monster->group_linked_0bd = 1;
        }
        return;
    }
    if (monster_info->fInCombat != 0) {
        return;
    }
    if (monster_info->hp_current != 0 && monster_info->highest_condition < 0xe) {
        if (monster_info->control_state == 1) {
            decision = 9;
        } else {
            unsigned char alert;

            alert = 0;
            if (engage != 0) {
                alert = GetMonsterGroupPartySightState(
                    GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                        0x14b, MONSTER_AI_CPP, monster_info->monster_group_id, 1)));
            }
            if (alert == 0) {
                if (monster_info->monster->orders_finished_28d == 0) {
                    decision = 0;
                } else {
                    update = ChooseMonsterRTAIMode(monster_info, &decision);
                }
            } else if (alert <= 2) {
                switch (monster_info->ubDisposition) {
                case 0:
                    if (monster_info->monster->orders_finished_28d == 0) {
                        decision = 0;
                    } else {
                        update = ChooseMonsterRTAIMode(monster_info, &decision);
                    }
                    break;
                case 1:
                    decision = static_cast<unsigned char>(
                                   monster_info->hp_current * 100 / monster_info->hp_max <= 0x14) +
                               1;
                    if (monster_info->monster->movement_stopped_024 != 0) {
                        update = 1;
                    }
                    break;
                case 2:
                    if (monster_info->monster->orders_finished_28d == 0) {
                        decision = 0;
                    } else {
                        update = ChooseMonsterRTAIMode(monster_info, &decision);
                    }
                    break;
                default:
                    srAssertFail("FALSE", MONSTER_AI_CPP, 0x185,
                                 "DoMonsterRTAI: ERROR - Invalid disposition");
                    return;
                }
            }
        }
    }
    if ((monster_info->ai_mode_255 & 0x80) != 0) {
        unsigned char mode;

        mode = monster_info->ai_mode_255 & 0xf;
        if (mode != 6 && (monster_info->monster->face_party_290 == 0 ||
                          monster_info->pathing_cooldown_246 != 0 || mode != 0xa ||
                          monster_info->player_visibility.line_of_sight_28 == 0 ||
                          (monster_info->monster->movement_0c0.position_040 -
                           g_startup_world_659c0c->GetPosition())
                                  .Length() >= g_float_005ec2f8)) {
            monster_info->ai_mode_255 &= 0x7f;
        }
        if (decision <= 2 || decision == 9) {
            update = 1;
        }
    }
    if (monster_info->ubDisposition == 1 || monster_info->monster->script_wait_240 != 1 ||
        decision != 0) {
        if (decision != monster_info->ai_mode_255 || update != 0) {
            if (engage == 0 && decision == 1) {
                srAssertFail("ubAIDecision != RT_AI_MODE_CHARGE_PARTY", MONSTER_AI_CPP, 0x1c9, 0);
            }
            ApplyMonsterRTAIDecision(monster_info, decision);
        }
    }
}

/* The orders-driven half of the real-time decision. A monster that should
   face the party and can see it takes mode 0xa straight away; a group with a
   downed member takes nothing. A latched mode keeps running until its patrol
   point is reached, a fresh look-around delay just counts down, and a heard
   noise is investigated while the path to it still looks cheap enough. */
// FUNCTION: WIZ8 0x005308C0
char ChooseMonsterRTAIMode(W8MonsterInfo* monster_info, unsigned char* decision)
{
    W8Monster* monster;
    char changed;
    char mode;

    monster = monster_info->monster;
    mode = 0;
    changed = 0;
    if (monster->face_party_290 != 0 && monster_info->pathing_cooldown_246 == 0 &&
        monster_info->player_visibility.line_of_sight_28 != 0) {
        srVector3T<float> delta;

        delta = monster->movement_0c0.position_040 - g_startup_world_659c0c->GetPosition();
        if (delta.Length() < g_float_005ec2f8) {
            srVector3T<float> camera;
            srVector3T<float> position;

            GetCameraPosition(&camera);
            position = monster->GetPosition();
            *decision = 0xa;
            monster->move_direction_2bc = camera - position;
            return 1;
        }
    }
    if (MonsterGroupHasIncapacitatedMember(monster_info->monster_group_id)) {
        *decision = 0;
        return 0;
    }
    if ((monster_info->ai_mode_255 & 0x80) != 0) {
        if (monster->order_mode_28e == 2 || monster->order_mode_28e == 3) {
            srVector3T<float> delta;
            srVector3T<float> patrol;

            monster->GetPatrolPoint004CA360(&patrol);
            delta = patrol - monster->GetPosition();
            if (delta.Length() >= g_double_005ee768) {
                *decision = monster_info->ai_mode_255;
                return 1;
            }
        } else {
            *decision = monster_info->ai_mode_255;
            return 1;
        }
    }
    if (monster_info->pathing_cooldown_246 != 0) {
        --monster_info->pathing_cooldown_246;
        if (monster_info->pathing_cooldown_246 == 0) {
            monster_info->sp_budget_bonus = 0;
        }
        return 0;
    }
    if (monster_info->ai_mode_255 == 8 &&
        fabsf(monster->movement_0c0.target_yaw - monster->movement_0c0.yaw) >=
            g_camera_transition_epsilon_005ebc84) {
        mode = 8;
        goto commit;
    }
    if (monster_info->heard_noise_radius_43 > 0 && monster->movement_stopped_024 != 0 &&
        monster_info->ai_mode_255 == 4) {
        srVector3T<float> delta;

        delta = monster_info->heard_noise_position_37 - monster->GetPosition();
        if (delta.Length() < g_double_005ee768) {
            monster_info->heard_noise_radius_43 = 0;
        }
    }
    if (monster_info->heard_noise_radius_43 > 0) {
        srVector3T<float> noise_position;
        srVector3T<float> position;
        float range;
        int radius;
        int hops;

        if (monster->movement_stopped_024 == 0 && monster_info->ai_mode_255 == 4) {
            mode = 4;
            goto commit;
        }
        noise_position = monster_info->heard_noise_position_37;
        radius = monster_info->heard_noise_radius_43 * 3 / 2;
        if (radius >= g_int_00617ae8) {
            radius = g_int_00617ae8;
        }
        range = (float)radius;
        position = monster->GetPosition();
        if (g_octree_6598a4->TestNoiseLineOfSight00434220(&position, &noise_position, &range,
                                                          &hops) != 0 &&
            NoiseHearingMargin004F0E50(monster_info->heard_noise_radius_43, static_cast<int>(range),
                                       hops) > 0) {
            mode = 4;
            changed = 1;
            goto commit;
        }
        mode = 8;
        goto commit;
    }
    if (monster_info->look_timer_303 != 0) {
        if (Random(0x14) == 0) {
            double angle;

            mode = 0xa;
            angle = (double)(Random(0x168) << 1) * g_camera_pi_005ec2a0 * g_double_005ed7b0;
            monster->move_direction_2bc.x = (float)(cos(angle) * g_double_005ec150);
            monster->move_direction_2bc.y = 0.0f;
            monster->move_direction_2bc.z = (float)(sin(angle) * g_double_005ec150);
        } else {
            mode = 0;
        }
        goto commit;
    }
    switch (monster->order_mode_28e) {
    case 0: {
        srVector3T<float> delta;
        srVector3T<float> patrol;

        monster->GetPatrolPoint004CA360(&patrol);
        delta = patrol - monster->GetPosition();
        if (delta.Length() < g_double_005ee768) {
            mode = 0;
        } else {
            mode = 7;
        }
        goto commit;
    }
    case 1:
        mode = 6;
        if (monster->movement_stopped_024 == 0) {
            goto commit;
        }
        goto set_changed;
    case 2:
    case 3: {
        srVector3T<float> delta;
        srVector3T<float> patrol;
        int count;
        int next;

        mode = 7;
        monster->GetPatrolPoint004CA360(&patrol);
        delta = patrol - monster->GetPosition();
        if (delta.Length() >= g_double_005ee768) {
            goto commit;
        }
        count = monster->vector_29c.GetCount();
        if (monster->order_mode_28e == 2) {
            next = monster->patrol_index_2ac + 1;
            if (next < count) {
                monster->patrol_index_2ac = (signed char)next;
            } else {
                monster->patrol_index_2ac = 0;
            }
        } else if (count < 2) {
            monster->patrol_index_2ac = 0;
        } else if ((monster_info->ai_mode_255 & 0xf) == 7) {
            do {
                next = (signed char)Random(count);
            } while (next == monster->patrol_index_2ac);
            monster->patrol_index_2ac = (signed char)next;
        } else {
            monster->patrol_index_2ac = (signed char)Random(count);
        }
        goto set_changed;
    }
    case 4:
        monster->move_direction_2bc.x = monster->direction_x_2b0;
        monster->move_direction_2bc.y = monster->direction_y_2b4;
        monster->move_direction_2bc.z = monster->direction_z_2b8;
        mode = 0xa;
        goto set_changed;
    default:
        goto commit;
    }
set_changed:
    changed = 1;
commit:
    if (mode != static_cast<char>(monster_info->ai_mode_255)) {
        changed = 1;
    }
    *decision = (unsigned char)mode;
    return changed;
}

/* Carry out the real-time mode ChooseMonsterRTAIMode picked. Each case does
   the movement or aiming that mode needs; when a mode cannot run the decision
   is folded back to zero so ai_mode_255 records what actually happened. Bit 0x80
   of the decision rides in alongside the mode and only case 6 consumes it. */
// FUNCTION: WIZ8 0x00530f10
void ApplyMonsterRTAIDecision(W8MonsterInfo* monster_info, unsigned char decision)
{
    W8Monster* monster = monster_info->monster;
    W8MonsterGroup* monster_group;
    W8MonsterRecord* record;
    W8MonsterInfo* member;
    W8SpellEffectEntry* effect;
    W8SpellVisual* visual;
    srVector3T<float> position;
    srVector3T<float> patrol_point;
    unsigned char best_range;
    unsigned int attack;
    unsigned int index;

    switch (decision & 0xf) {
    case 1:
        best_range = 0;
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MONSTER_AI_CPP, 1145, 0);
        }
        record = GetMonsterDataForInfo(monster_info);
        for (attack = 0; attack < 3; ++attack) {
            if (RateMonsterAttack(monster_info, record, attack, 0, 0) == 3 &&
                best_range < record->attacks[attack].range_category) {
                best_range = record->attacks[attack].range_category;
            }
        }
        if (MonsterApproachStartupNavigator004C5FF0(
                monster, CalcRangeDistance((W8RangeCategory)best_range) * g_float_005ec390) == 1) {
            if (IsSightRangeOverridden()) {
                monster_group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0x315, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
                MonsterGroupEnterCombat(monster_group);
            }
            break;
        }
        decision = 0;
    case 0:
        ClearMonsterPathAndResume(monster_info);
        break;
    case 2:
        MonsterLinkToStartupNavigator004C6030(monster);
        break;
    case 3:
        if (monster_info->ubDisposition == DISP_HOSTILE) {
            srAssertFail("pMonsterInfo->ubDisposition != DISP_HOSTILE", MONSTER_AI_CPP, 810, 0);
        }
        MonsterApproachStartupNavigator004C5FF0(monster, 3000.0);
        break;
    case 4:
        position = monster_info->heard_noise_position_37;
        MonsterForward452630(monster, &position);
        break;
    case 6:
        if ((decision & 0x80) != 0) {
            monster->SetHeightRange(monster->patrol_distance_294, monster->patrol_variation_298);
            monster_info->ai_mode_255 &= 0x7f;
            break;
        }
        if (monster->formation.x == 0.0f && monster->formation.y == 0.0f &&
            monster->formation.z == 0.0f) {
            monster->formation = monster->GetPosition();
        }
        position = monster->formation;
        if (monster->StartPatrol(&position, monster->patrol_distance_294,
                                 monster->patrol_variation_298)) {
            break;
        }
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x2de, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
        if (monster_group->encounter_registered_c3 != 0) {
            if (g_flag_689b32 != 0 && gfCapturingVideo == 0) {
                FormatDebugMessage(0,
                                   "Monster %d and associated monsters killed because it "
                                   "couldn't patrol",
                                   monster_info->location_id);
            }
            MarkMonsterGroupForRemoval(monster_info->monster_group_id);
        } else {
            if (g_flag_689b32 != 0 && gfCapturingVideo == 0) {
                FormatDebugMessage(0, "%S %d can't path!", GetMonsterName(monster_info, 0, 0),
                                   monster_info->location_id);
            }
            monster_info->pathing_cooldown_246 = 0x14;
            if (monster_info->movement_stall_ticks_254 < 2) {
                monster_info->movement_stall_ticks_254 = 2;
            }
        }
        decision = 0;
        break;
    case 7:
        if (monster->GetPatrolPoint004CA360(&patrol_point) == 0) {
            decision = 0;
            break;
        }
        if (MonsterForward452630(monster, &patrol_point) != 0) {
            break;
        }
        if (monster->IsWithinWorldRange004CA2A0() == 0 &&
            monster_info->party_threat.sight_state_04 == W8_SIGHT_SEEN) {
            monster_group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0x350, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
            position = patrol_point;
            if (MoveMonsterGroupToPosition(monster_group, &position, monster->GetYaw(), 0, 1, 0,
                                           0) != 0) {
                break;
            }
        }
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x2de, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
        if (monster_group->encounter_registered_c3 != 0) {
            if (g_flag_689b32 != 0 && gfCapturingVideo == 0) {
                FormatDebugMessage(0,
                                   "Monster %d and associated monsters killed because it "
                                   "couldn't patrol",
                                   monster_info->location_id);
            }
            MarkMonsterGroupForRemoval(monster_info->monster_group_id);
        } else {
            if (g_flag_689b32 != 0 && gfCapturingVideo == 0) {
                FormatDebugMessage(0, "%S %d can't path!", GetMonsterName(monster_info, 0, 0),
                                   monster_info->location_id);
            }
            monster_info->pathing_cooldown_246 = 0x14;
            if (monster_info->movement_stall_ticks_254 < 2) {
                monster_info->movement_stall_ticks_254 = 2;
            }
        }
        decision = 0;
        break;
    case 8:
        monster->flags_00c &= ~0x20000000;
        monster->ClearMovement();
        position = monster_info->heard_noise_position_37;
        monster_info->heard_noise_radius_43 = 0;
        monster_info->pathing_cooldown_246 = 0x1e;
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x37c, MONSTER_AI_CPP, monster_info->monster_group_id, 1));
        for (index = 0; index < ILLength(monster_group->monsters); ++index) {
            member = MonsterInfoFromID(0x381, MONSTER_AI_CPP,
                                       IListGetAt(monster_group->monsters, index), 1);
            member->monster->AimAtPosition(&position);
        }
        break;
    case 9:
        effect = FindMonsterControlSpellEffect();
        if (effect == 0 || (visual = effect->spell_visuals.data[0]) == 0) {
            break;
        }
        if (monster->SetMovementTargetToNavigator004526C0(visual, 2500.0) == 0) {
            position = visual->GetPosition();
            monster->AimAtPosition(&position);
        }
        break;
    case 10:
        monster->flags_00c &= ~0x20000000;
        monster->ClearMovement();
        position = monster->GetPosition();
        position += monster->move_direction_2bc;
        monster->AimAtPosition(&position);
        break;
    }
    monster_info->ai_mode_255 ^= (monster_info->ai_mode_255 ^ decision) & 0xf;
}

/* Throw away the queue of actions a monster's AI had decided on. */
// FUNCTION: WIZ8 0x00532330
void DestroyMonsterActionQueue(W8MonsterInfo* monster_info)
{
    W8PList* queue = monster_info->pCombat->plsCombatActionList;

    if (queue != 0 && PLDestroy(queue)) {
        monster_info->pCombat->plsCombatActionList = 0;
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
    W8CombatSlot chosen;
    float hp_ratio;
    unsigned char backs_off;
    srVector3T<float> position;

    if (monster_info->highest_condition >= 0xf) {
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
    chance = MonsterAdvanceChance(monster_info, record);
    if (Random(100) < chance) {
        monster_info->action_kind = 4;
        goto validate;
    }
    if (monster_info->ubDisposition == 0) {
        if (record->unknown_249 != 0) {
            monster_info->action_kind = -1;
            monster_info->pCombat->phase = 0;
            monster_info->pCombat->active = 1;
        } else {
            monster_info->action_kind = 6;
        }
        goto validate;
    }
    if (record->prefer_ranged_actions_1b9 == 0 &&
        (range_category = GetBestMonsterAttackRange(record, 1)) != W8_RANGE_NONE &&
        (monster_info->condition_turns[0xc] == 0 || record->kind_0cb == 0xc) &&
        MonsterChooseTarget(monster_info, &chosen, 2) > CalcRangeDistance(range_category)) {
        record = GetMonsterDataForInfo(monster_info);
        hp_ratio =
            static_cast<float>(monster_info->hp_current) / static_cast<float>(monster_info->hp_max);
        backs_off = hp_ratio <= 0.95f && record->prefer_ranged_actions_1b9 == 0;
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
                if (g_special_attack_table[record->special_attack_kind_0e3][0] ==
                    W8_SPECIAL_ATTACK_EFFECT_SUMMON) {
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
                    hp_ratio = static_cast<float>(monster_info->hp_current) /
                               static_cast<float>(monster_info->hp_max);
                    backs_off = hp_ratio <= 0.95f && record->prefer_ranged_actions_1b9 == 0;
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

/* Whether a hostile group still has a member able to engage the party. Only
   hostile groups can; of those, the group engages when a live member has a
   visible target, and a member counts when it can attack, cast one of its
   spells, flee, or - for a touch/short attack with the party close - reach
   the party along the waypoints. The waypoint probe runs once, against the
   group's named member, and is reused for the rest of the sweep. */
// FUNCTION: WIZ8 0x00531920
bool MonsterGroupCanEngage(W8MonsterGroup* monster_group)
{
    W8MonsterInfo* member;
    W8MonsterInfo* leader;
    W8MonsterRecord* record;
    unsigned int index;
    unsigned int spell;
    float distance;
    unsigned char waypoint_checked = 0;
    unsigned char waypoint_result = 0;
    srVector3T<float> source;
    srVector3T<float> destination;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 1178, 0);
    }
    if (monster_group->ubDisposition == DISP_HOSTILE) {
        for (index = 0; index < ILLength(monster_group->monsters); ++index) {
            member = GetGroupMemberInfo(monster_group, index);
            if (member->fActive != 0 && member->hp_current != 0 &&
                member->highest_condition < 0x12 && MonsterHasVisibleTarget(member, 1, 1, 1) != 0) {
                goto members;
            }
        }
    }
    return 0;
members:
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        member = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x4a7, MONSTER_AI_CPP, IListGetAt(monster_group->monsters, index), 1));
        record = GetMonsterDataForInfo(member);
        if (member->fActive == 0 || member->fMotionless != 0 || member->monster->IsDying() != 0 ||
            member->hp_current == 0 || member->highest_condition >= 0xc) {
            continue;
        }
        if (RateMonsterBestAttack(member, record, 0) == 0) {
            return 1;
        }
        if (record->spell_chance_0e0 != 0) {
            if (MonsterIsCycleSupported(member->monster, W8_MONSTER_CYCLE_SPELL) == 0) {
                if (g_spell_cycle_error_reported == 0) {
                    FormatDebugMessage(0, "ERROR: %ls is missing a SPELL animation cycle", record);
                    g_spell_cycle_error_reported = 1;
                }
            } else {
                for (spell = 0; spell < 10; ++spell) {
                    if (MonsterCanAimSpell005474B0(record->spells_14d[spell]) != 0 &&
                        IsSpellUsableByMonster(member, record->spells_14d[spell], 1) != 0) {
                        return 1;
                    }
                }
            }
        }
        if (CanMonsterFlee(member, record, 1) != 0) {
            return 1;
        }
        if (GetBestMonsterAttackRange(record, 0) <= W8_RANGE_SHORT) {
            distance = member->monster->GetDistanceToPlayer004C7CB0();
            if (GetMonsterCombatMoveRange(member) * g_float_005ee774 > distance) {
                if (waypoint_checked == 0) {
                    leader =
                        MonsterInfoFromID(0x4cb, MONSTER_AI_CPP, monster_group->leader_id_9f, 1);
                    if (leader != 0 && leader->fActive != 0) {
                        destination = g_startup_world_659c0c->GetPosition();
                        source = leader->monster->GetPosition();
                        waypoint_result = g_octree_6598a4->pathing_180->TestWaypointSpan0045A1B0(
                            &source, &destination, 0, 0);
                    }
                    waypoint_checked = 1;
                }
                if (waypoint_result != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The percentage chance the monster chooses the advance action this round. A
   badly hurt NPC-disposition monster always presses on, a monster already
   committed keeps advancing until an enemy is inside short range, and a
   record without an explicit advance chance advances unless its behavior byte
   says otherwise. */
// FUNCTION: WIZ8 0x00531c00
unsigned int MonsterAdvanceChance(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    unsigned int hp_percent;
    unsigned int result = 0;
    W8CombatSlot chosen;
    float distance;

    hp_percent = monster_info->hp_current * 100 / monster_info->hp_max;
    if (monster_info->fInCombat == 0) {
        srAssertFail("pMonsterInfo->fInCombat", MONSTER_AI_CPP, 1272, 0);
    }
    if ((record->flags_0d0 & 1) != 0 && monster_info->ubDisposition != DISP_HOSTILE &&
        hp_percent <= 33) {
        return 100;
    }
    if (monster_info->pCombat->advancing_14b != 0) {
        distance = MonsterChooseTarget(monster_info, &chosen, 2);
        if (CalcRangeDistance(W8_RANGE_SHORT) < distance) {
            result = 100;
        } else {
            monster_info->pCombat->advancing_14b = 0;
        }
    } else if (record->advance_chance_0e2 != 0) {
        result = record->advance_chance_0e2;
    } else if (record->combat_morale_1c0 == 0) {
        result = 100;
    }
    return result;
}

/* Refill the monster's pending-action queue with everything it may do this
   round. A fresh pick first rolls the cooperative sweep, queueing a kind-8
   entry for every character and other monster the monster has an attack on,
   and returns early when any of them stuck; a behavior-2 record may then add
   the kind-1 wait action. The rest enumerates attack candidates: each usable
   attack contributes one kind-0 entry per set attack-mode bit for every
   eligible character and monster target. `target_locked` keeps only the
   stored target; `attack_locked` keeps only the committed attack and always
   sweeps every target. A character's evasion skill roll keeps attacks off it
   but flags the slot, and each flagged slot may practice that skill at the
   end. */
// FUNCTION: WIZ8 0x00531CE0
void BuildMonsterActionQueue(W8MonsterInfo* monster_info, char target_locked, char attack_locked)
{
    W8MonsterInfo* other;
    W8MonsterRecord* record;
    unsigned int index;
    unsigned int attack;
    unsigned int mode_bit;
    unsigned int char_lo;
    unsigned int char_hi;
    unsigned int monster_lo;
    unsigned int monster_hi;
    unsigned int attack_lo;
    unsigned int attack_hi;
    unsigned char scan_chars = 0;
    unsigned char scan_monsters = 0;
    unsigned char avoided[8] = {0};
    unsigned char resisted[8] = {0};
    bool hostile_only;
    char disposition_needed;

    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->pCombat->plsCombatActionList != 0 &&
        PLDestroy(monster_info->pCombat->plsCombatActionList) != 0) {
        monster_info->pCombat->plsCombatActionList = 0;
    }
    monster_info->pCombat->plsCombatActionList = PLCreate();
    if (monster_info->pCombat->plsCombatActionList == 0) {
        srAssertFail("pMonsterInfo->pCombat->plsCombatActionList != NULL", MONSTER_AI_CPP, 1385, 0);
    }
    if (target_locked == 0 && attack_locked == 0) {
        if (CanMonsterAttack(monster_info) != 0 && Random(100) < monster_info->attributes[1]) {
            monster_info->action_kind = 8;
            for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                other = MonsterGetScriptPartByLocationIndex(index);
                if (other != monster_info) {
                    ResetCombatSlot(&monster_info->Target);
                    monster_info->Target.iType = W8_TARGET_KIND_MONSTER;
                    monster_info->Target.iMonsterID = other->location_id;
                    if (MonsterHasAttackOn(monster_info, &monster_info->Target) != 0) {
                        QueueMonsterAction(monster_info, 8, 0, 0, W8_TARGET_KIND_MONSTER,
                                           other->location_id);
                    }
                }
            }
            for (index = 0; index < 8; ++index) {
                ResetCombatSlot(&monster_info->Target);
                monster_info->Target.iType = W8_TARGET_KIND_CHARACTER;
                monster_info->Target.iChar = index;
                if (MonsterHasAttackOn(monster_info, &monster_info->Target) != 0) {
                    QueueMonsterAction(monster_info, 8, 0, 0, W8_TARGET_KIND_CHARACTER, index);
                }
            }
            if (static_cast<int>(PLLength(monster_info->pCombat->plsCombatActionList)) > 0) {
                return;
            }
        }
        if (record->combat_morale_1c0 == 2 && record->combat_behavior_1bf != 3 &&
            Random(100) < monster_info->attributes[1]) {
            QueueMonsterAction(monster_info, 1, -1, 0, W8_TARGET_KIND_NONE, 0);
        }
    }
    monster_info->action_kind = 0;
    if (attack_locked == 0) {
        attack_lo = 0;
        attack_hi = W8_MAX_MONSTER_ATTACKS;
    } else {
        attack_lo = monster_info->pCombat->attack_index_11;
        attack_hi = attack_lo + 1;
    }
    if (target_locked == 0 || attack_locked != 0 || monster_info->pCombat->berserk_015 != 0 ||
        monster_info->attributes[2] >= 0x4b) {
        char_lo = 0;
        char_hi = 8;
        scan_chars = 1;
        scan_monsters = 1;
        monster_lo = 0;
        monster_hi = PLLength(gXStatus.plsMonsterList);
    } else {
        if (IsTargetStillPresent(&monster_info->Target) == 0) {
            return;
        }
        if (monster_info->Target.iType == W8_TARGET_KIND_CHARACTER) {
            if (monster_info->Target.iChar == -1) {
                srAssertFail("pTarget->iChar != BAD_INDEX", MONSTER_AI_CPP, 1510, 0);
            }
            char_lo = monster_info->Target.iChar;
            char_hi = char_lo + 1;
            scan_chars = 1;
        } else if (monster_info->Target.iType == W8_TARGET_KIND_MONSTER) {
            if (monster_info->Target.iMonsterID == -1) {
                srAssertFail("pTarget->iMonsterID != BAD_INDEX", MONSTER_AI_CPP, 1519, 0);
            }
            scan_monsters = 1;
            monster_lo = MonsterGetIndexByLocationID(0x5f2, MONSTER_AI_CPP,
                                                     monster_info->Target.iMonsterID, 1);
            monster_hi = monster_lo + 1;
            goto targets_chosen;
        } else {
            return;
        }
    }
    for (index = char_lo; index < char_hi; ++index) {
        if (Random(100) < g_status_685170.buffers.Char[index].skills[0xb].level * 75 / 100) {
            avoided[index] = 1;
        }
    }
targets_chosen:
    hostile_only = monster_info->fInCombat != 0 && monster_info->pCombat->berserk_015 != 0;
    disposition_needed = hostile_only + 1;
    for (attack = attack_lo; attack < attack_hi; ++attack) {
        if (RateMonsterAttack(monster_info, record, attack, 1, hostile_only) != 0) {
            continue;
        }
        if (scan_chars != 0 &&
            monster_info->player_visibility.los_flags_05[RangeCategoryUsesSightCondition(
                monster_info, (W8RangeCategory)record->attacks[attack].range_category)] != 0) {
            for (index = char_lo; index < char_hi; ++index) {
                if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                    g_status_685170.buffers.Char[index].hp_current != 0 &&
                    g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
                    MonsterVsCharDisposition(index, monster_info) == disposition_needed &&
                    MonsterAttackReachesCharacter(monster_info, record, attack, index) != 0) {
                    if (avoided[index] == 0) {
                        for (mode_bit = 0; mode_bit < 9; ++mode_bit) {
                            if ((record->attacks[attack].attack_modes & (1 << mode_bit)) != 0) {
                                QueueMonsterAction(monster_info, W8_MONSTER_ACTION_ATTACK, mode_bit,
                                                   attack, W8_TARGET_KIND_CHARACTER, index);
                            }
                        }
                    } else {
                        resisted[index] = 1;
                    }
                }
            }
        }
        if (scan_monsters != 0) {
            for (index = monster_lo; index < monster_hi; ++index) {
                other = MonsterGetScriptPartByLocationIndex(index);
                if (other != monster_info && other->fActive != 0 && other->hp_current != 0 &&
                    GetMonsterDataForInfo(other)->untargetable_24a == 0 && other->fInCombat != 0 &&
                    MonsterHostility00546F80(monster_info, other) == disposition_needed &&
                    MonsterAttackReachesMonster(monster_info, record, attack, other) != 0) {
                    for (mode_bit = 0; mode_bit < 9; ++mode_bit) {
                        if ((record->attacks[attack].attack_modes & (1 << mode_bit)) != 0) {
                            QueueMonsterAction(monster_info, W8_MONSTER_ACTION_ATTACK, mode_bit,
                                               attack, W8_TARGET_KIND_MONSTER, other->location_id);
                        }
                    }
                }
            }
        }
    }
    if (scan_chars != 0) {
        for (index = char_lo; index < char_hi; ++index) {
            if (resisted[index] != 0 && Random(2) == 0) {
                PracticeCharacterSkill(&g_status_685170.buffers.Char[index], 0xb, 1, 0);
            }
        }
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
    PLAdoptAppend(monster_info->pCombat->plsCombatActionList, entry);
}

/* Build the monster's list of possible actions and take one of them at
   random into its action fields and target. Committing a plain attack also
   commits its attack index and, when asked, restages the round's attack
   counts; a wait action keeps only its detail word. */
// FUNCTION: WIZ8 0x005323F0
unsigned char ChooseRandomMonsterAction(W8MonsterInfo* monster_info, int arg_2, int arg_3,
                                        char set_attack_rate)
{
    W8MonsterAction* entry;
    W8MonsterRecord* record;
    unsigned int count;

    record = GetMonsterDataForInfo(monster_info);
    BuildMonsterActionQueue(monster_info, arg_2, arg_3);
    count = PLLength(monster_info->pCombat->plsCombatActionList);
    if (count == 0) {
        return 0;
    }
    entry = static_cast<W8MonsterAction*>(
        PLGet(monster_info->pCombat->plsCombatActionList, static_cast<int>(Random(count))));
    if (entry == 0) {
        return 0;
    }
    monster_info->action_kind = entry->action_kind;
    switch (entry->action_kind) {
    case W8_MONSTER_ACTION_ATTACK:
        monster_info->pCombat->attack_index_11 = entry->attack_index;
        monster_info->action_detail = entry->action_detail;
        if (set_attack_rate != 0) {
            monster_info->pCombat->unknown_005 = record->attacks_per_round_0e5;
            monster_info->pCombat->attacks_per_round = record->attacks_per_round_0e5;
        }
        break;
    case 1:
        monster_info->action_detail = entry->action_detail;
        break;
    }
    monster_info->Target = entry->target;
    return 1;
}

/* Whether the monster may cast the spell now: the record allows monsters to
   cast it, nothing is blocking the cast, the special cases for spell 0x3c
   and fire spells under camera sway pass, and `needs_target` also demands a
   target to aim at. */
// FUNCTION: WIZ8 0x00532550
bool IsSpellUsableByMonster(W8MonsterInfo* monster_info, int spell_id, char needs_target)
{
    if (spell_id == 0) {
        return 0;
    }
    if (g_spell_records[spell_id].monster_castable == 0) {
        return 0;
    }
    if (IsSpellBlockedForMonster(monster_info, spell_id)) {
        return 0;
    }
    if (DispatchWorldCursorNodeCommand004D9080(monster_info, 4, 0) != 0) {
        return 0;
    }
    if (spell_id == 0x3c && monster_info->insanity_summon_344 != -1) {
        return 0;
    }
    if (g_spell_records[spell_id].realm == W8_SPELL_REALM_FIRE &&
        g_camera_sway_active_652da4 != 0) {
        return 0;
    }
    if (needs_target != 0) {
        W8GrowableVector<W8CombatSlot> targets;
        CollectMonsterSpellTargets(monster_info, spell_id, &targets);
        if (targets.GetCount() == 0) {
            return 0;
        }
    }
    return 1;
}

/* Pick where the spell lands: collect every slot the spell may be cast at
   and take one at random into the monster's stored target. */
// FUNCTION: WIZ8 0x005326F0
bool AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id)
{
    W8GrowableVector<W8CombatSlot> targets;

    CollectMonsterSpellTargets(monster_info, spell_id, &targets);
    if (targets.GetCount() == 0) {
        return 0;
    }
    monster_info->Target = *targets.GetAt(Random(targets.GetCount()));
    return 1;
}

/* Whether `combat_slot` accepts `spell_id` from this caster. Target type
   eight needs no checking at all; a character or monster slot is checked
   against its own record, and anything else is resolved into its markers
   and accepted when at least half of them take the spell. The per-spell
   switch then vetoes targets the spell would not help or cannot affect. */
// FUNCTION: WIZ8 0x005327E0
bool MonsterSpellTargetOK(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot)
{
    W8MonsterInfo* target = 0;
    W8Character* character = 0;
    W8MonsterRecord* record = 0;
    int hp = 0;
    int hp_max = 0;
    int stat = 0;
    int stat_max = 0;
    unsigned int* condition_turns = 0;
    W8Enchantment* enchantments = 0;
    unsigned int index;
    unsigned int duration;

    if (GetSpellTargetType(spell_id, 0) == W8_TARGET_TYPE_POINT) {
        return 1;
    }
    if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
        character = &g_status_685170.buffers.Char[combat_slot->iChar];
        hp_max = character->uiHPMax;
        hp = character->hp_current;
        stat = character->stamina;
        stat_max = character->uiStaminaMax;
        condition_turns = character->uiCondition;
        enchantments = character->enchantments;
    } else {
        if (combat_slot->iType != W8_TARGET_KIND_MONSTER) {
            W8GrowableVector<int> monster_markers;
            W8GrowableVector<int> party_markers;
            W8TargetSource source;
            W8CombatSlot probe;
            unsigned int valid = 0;
            int total = 0;
            unsigned int power_level;

            SetTargetSourceToMonster(monster_info, &source);
            power_level = ChooseMonsterSpellPowerLevel(
                monster_info, GetMonsterDataForInfo(monster_info), spell_id);
            PopulateSpellTargetMarkers(spell_id, power_level, &source, combat_slot,
                                       &monster_markers, &party_markers, 0);
            for (index = 0; index < (unsigned int)party_markers.GetCount(); ++index) {
                ++total;
                ResetCombatSlot(&probe);
                probe.iType = W8_TARGET_KIND_CHARACTER;
                probe.iChar = *party_markers.GetAt(index);
                if (MonsterSpellTargetOK(monster_info, spell_id, &probe) != 0) {
                    ++valid;
                }
            }
            for (index = 0; index < (unsigned int)monster_markers.GetCount(); ++index) {
                ++total;
                ResetCombatSlot(&probe);
                probe.iType = W8_TARGET_KIND_MONSTER;
                probe.iMonsterID = *monster_markers.GetAt(index);
                if (MonsterSpellTargetOK(monster_info, spell_id, &probe) != 0) {
                    ++valid;
                }
            }
            return valid != 0 && ((total + 1U) >> 1) <= valid;
        }
        target = MonsterInfoFromID(0x7e5, MONSTER_AI_CPP, combat_slot->iMonsterID, 1);
        record = GetMonsterDataForInfo(target);
        stat_max = target->stamina_max;
        stat = target->stamina;
        hp = target->hp_current;
        hp_max = target->hp_max;
        condition_turns = target->condition_turns;
        enchantments = target->enchantments;
        if (monster_info->ubDisposition == DISP_HOSTILE && target->ubDisposition == DISP_FRIENDLY &&
            monster_info->fInCombat != 0 && monster_info->pCombat->reconsider_action_152 != 0) {
            return 0;
        }
    }
    switch (spell_id) {
    case 1:
    case 4:
    case 5:
    case 9:
    case 10:
    case 0x19:
    case 0x1c:
    case 0x24:
    case 0x2a:
    case 0x2b:
    case 0x2f:
    case 0x32:
    case 0x34:
    case 0x37:
    case 0x39:
    case 0x3c:
    case 0x3f:
    case 0x42:
    case 0x46:
    case 0x47:
    case 0x4e:
    case 0x4f:
    case 0x53:
    case 0x56:
    case 0x57:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5e:
    case 0x5f:
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x65:
    case 0x77:
        break;
    case 2:
    case 0x35:
    case 0x3b:
        if (gXStatus.fCombatMode == 0) {
            return 1;
        }
        for (index = 0; index < 9; ++index) {
            if (spell_id == g_combat_effect_slot_spells_and_cast_success_00616dd8[index]) {
                if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
                    duration = g_combat_state->effect_slots_85a[index].duration_0d;
                } else {
                    if (target->fInCombat == 0) {
                        continue;
                    }
                    duration = target->pCombat->effect_slots_d7[index].duration_0d;
                }
                if (duration != 0) {
                    return 0;
                }
            }
        }
        return 1;
    case 6:
    case 0x44:
        if (hp == hp_max) {
            return 0;
        }
        break;
    case 7:
        if (condition_turns[3] != 0) {
            return 0;
        }
        break;
    case 0xb:
    case 0x25:
    case 0x43:
        if (condition_turns[0x10] != 0) {
            return 0;
        }
        break;
    case 0xc:
        if (condition_turns[W8_CONDITION_ASLEEP] != 0) {
            return 0;
        }
        break;
    case 0xd:
    case 0x2c:
        if (stat == stat_max) {
            return 0;
        }
        break;
    case 0xe:
        if (condition_turns[6] != 0) {
            return 0;
        }
        break;
    case 0xf:
        if (condition_turns[0xc] != 0) {
            return 0;
        }
        break;
    case 0x10:
        if (condition_turns[3] == 0 && condition_turns[4] == 0 && condition_turns[6] == 0 &&
            condition_turns[W8_CONDITION_ASLEEP] == 0 && condition_turns[0xc] == 0) {
            return 0;
        }
        break;
    case 0x14:
    case 0x1a:
    case 0x20:
    case 0x28:
        for (index = 0; index < 12; ++index) {
            if (spell_id == g_being_effect_slot_spells_00616d84[index]) {
                if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
                    duration = g_status_685170.effect_slots_17af[index].duration_0d;
                } else {
                    duration = target->effect_slots_10f[index].duration_0d;
                }
                if (duration != 0) {
                    return 0;
                }
            }
        }
        if (spell_id == 0x14 && combat_slot->iType == W8_TARGET_KIND_MONSTER &&
            0x3b < record->spell_chance_0e0) {
            return 0;
        }
        break;
    case 0x15:
        if (enchantments[2].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x1b:
        if (enchantments[3].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x1d:
        if (condition_turns[W8_CONDITION_LOAD_EASED] != 0) {
            return 0;
        }
        break;
    case 0x1f:
        if (condition_turns[0xe] != 0) {
            return 0;
        }
        break;
    case 0x22:
        if (condition_turns[0x10] == 0) {
            return 0;
        }
        break;
    case 0x23:
        if (condition_turns[W8_CONDITION_POISONED] == 0) {
            return 0;
        }
        break;
    case 0x2e:
        if (condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
            return 0;
        }
        if (combat_slot->iType != W8_TARGET_KIND_CHARACTER) {
            if (combat_slot->iType == W8_TARGET_KIND_MONSTER) {
                if (3 < record->kind_0cb && (record->kind_0cb < 6 || record->kind_0cb == 0xd)) {
                    return 0;
                }
                if (record->spell_chance_0e0 != 0) {
                    if (MonsterIsCycleSupported(target->monster, W8_MONSTER_CYCLE_SPELL) == 0) {
                        if (g_spell_cycle_error_reported == 0) {
                            FormatDebugMessage(0, "ERROR: %ls is missing a SPELL animation cycle",
                                               record);
                            g_spell_cycle_error_reported = 1;
                            return 0;
                        }
                    } else {
                        for (index = 0; index < 10; ++index) {
                            if (IsSpellUsableByMonster(target, record->spells_14d[index], 0) != 0) {
                                return 1;
                            }
                        }
                    }
                }
            }
            return 0;
        }
        if (character->skills[0xc].level != 0) {
            return 1;
        }
        for (index = 0; index < 0x72; ++index) {
            if (character->spell_learned[index] == 1) {
                break;
            }
        }
        if (index > 0x71) {
            return 0;
        }
        if (g_spell_records[spell_id].alchemy_spell != 0 && character->skills[0x1a].level != 0) {
            return 0;
        }
        break;
    case 0x30:
    case 0x31:
    case 0x4c:
    case 0x50:
    case 0x51:
    case 0x5d:
        if (gXStatus.fCombatMode == 0) {
            return 1;
        }
        for (index = 0; index < 9; ++index) {
            if (spell_id == g_combat_effect_slot_spells_00616db4[index]) {
                if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
                    duration = g_combat_state->effect_slots[index].duration_0d;
                } else {
                    if (target->fInCombat == 0) {
                        continue;
                    }
                    duration = target->pCombat->effect_slots_3e[index].duration_0d;
                }
                if (duration != 0) {
                    return 0;
                }
            }
        }
        return 1;
    case 0x36:
        if (enchantments[4].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x38:
        if (enchantments[5].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x3d:
        if (enchantments[6].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x41:
        if (enchantments[7].turns_08 != 0) {
            return 0;
        }
        break;
    case 0x45:
        if (condition_turns[9] != 0) {
            return 0;
        }
        break;
    case 0x48:
        if (gXStatus.fCombatMode == 0) {
            return 0;
        }
        for (index = 0; index < 9; ++index) {
            if (g_combat_effect_slot_spells_00616db4[index] != 0x31) {
                if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
                    duration = g_combat_state->effect_slots[index].duration_0d;
                } else {
                    if (target->fInCombat == 0) {
                        continue;
                    }
                    duration = target->pCombat->effect_slots_3e[index].duration_0d;
                }
                if (duration != 0) {
                    return 1;
                }
            }
        }
        return 0;
    case 0x4a:
        if (condition_turns[0xb] == 0 && condition_turns[W8_CONDITION_HOSTILE] == 0) {
            return 0;
        }
        break;
    case 0x4d:
        if (combat_slot->iType == W8_TARGET_KIND_CHARACTER) {
            return 0;
        }
        if (record->kind_0cb != 0x14 && record->kind_0cb != 0x15 && record->kind_0cb != 0x1c &&
            target->summoned_2da == 0) {
            return 0;
        }
        break;
    case 0x55:
        if (condition_turns[6] == 0) {
            return 1;
        }
    case 0x18:
        if (condition_turns[0xb] != 0) {
            return 0;
        }
        break;
    case 0x59:
        if (condition_turns[W8_CONDITION_HOSTILE] != 0) {
            return 0;
        }
        break;
    case 3:
    case 8:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x16:
    case 0x17:
    case 0x1e:
    case 0x21:
    case 0x26:
    case 0x27:
    case 0x29:
    case 0x2d:
    case 0x33:
    case 0x3a:
    case 0x3e:
    case 0x40:
    case 0x49:
    case 0x4b:
    case 0x52:
    case 0x54:
    case 0x58:
    case 0x64:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6a:
    case 0x6b:
    case 0x6c:
    case 0x6d:
    case 0x6e:
    case 0x6f:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    default:
        FormatDebugMessage(0, "WARNING: MonsterSpellTargetOK - unlisted spell %d(%ls)", spell_id,
                           g_spell_records[spell_id].display_name);
        break;
    }
    return 1;
}

/* Whether the spell's area effect would catch a disposition-neutral monster;
   a true return vetoes the cast, since the AI does not turn neutrals hostile
   by accident. */
// FUNCTION: WIZ8 0x005330E0
unsigned char SpellAreaHitsNeutralMonster(W8MonsterInfo* monster_info, int spell_id,
                                          W8CombatSlot* combat_slot)
{
    W8GrowableVector<int> monster_markers;
    W8GrowableVector<int> party_markers;
    W8TargetSource source;
    unsigned int index;
    W8MonsterInfo* target;
    unsigned int power_level;

    if (MonsterCanAimSpell005474B0(spell_id) == 0) {
        return 0;
    }
    SetTargetSourceToMonster(monster_info, &source);
    power_level =
        ChooseMonsterSpellPowerLevel(monster_info, GetMonsterDataForInfo(monster_info), spell_id);
    PopulateSpellTargetMarkers(spell_id, power_level, &source, combat_slot, &monster_markers,
                               &party_markers, 0);
    for (index = 0; index < (unsigned int)monster_markers.GetCount(); ++index) {
        target = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x9de, MONSTER_AI_CPP, *monster_markers.GetAt(index), 1));
        if (target->ubDisposition == DISP_NEUTRAL) {
            return 1;
        }
    }
    return 0;
}

/* Which of the record's ten spells the monster casts: each usable slot adds
   its weight from the fixed table, then a roll under the total walks them
   down. The assertion names the original local, uiCastableSpellCnt. */
// FUNCTION: WIZ8 0x00533260
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    int weights[10] = {0};
    int spell_ids[10];
    unsigned int count = 0;
    int total = 0;
    int slot;
    int roll;
    unsigned int index;

    for (slot = 0; slot < 10; ++slot) {
        int spell_id = record->spells_14d[slot];
        if (IsSpellUsableByMonster(monster_info, spell_id, 1) != 0) {
            weights[count] = g_spell_cast_weights_0061cc14[slot];
            spell_ids[count] = spell_id;
            total += g_spell_cast_weights_0061cc14[slot];
            ++count;
        }
    }
    if (count == 0) {
        srAssertFail("uiCastableSpellCnt > 0", MONSTER_AI_CPP, 0xa03, 0);
    }
    roll = Random(total);
    index = 0;
    while (index < count && roll >= weights[index]) {
        roll -= weights[index];
        ++index;
    }
    return spell_ids[index];
}

/* Fill `targets` with the combat slots `spell_id` may be cast at by this
   monster. The monster's action fields carry the action being probed while
   the reachability helpers run and are restored afterwards; spell 0x77 asks
   for a place to flee to, everything else is a cast. */
// FUNCTION: WIZ8 0x00533320
void CollectMonsterSpellTargets(W8MonsterInfo* monster_info, int spell_id,
                                W8GrowableVector<W8CombatSlot>* targets)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    int saved_detail = monster_info->action_detail;
    int saved_kind = monster_info->action_kind;
    int sight_kind;
    int target_type;
    W8CombatSlot slot;
    W8MonsterGroup* monster_group;
    W8MonsterInfo* member;
    unsigned int index;

    if (spell_id == W8_AI_SPELL_PLACE) {
        sight_kind = 3;
        monster_info->action_kind = W8_MONSTER_ACTION_FLEE;
    } else {
        sight_kind = 5;
        monster_info->action_kind = W8_MONSTER_ACTION_SPELL;
        monster_info->action_detail = spell_id;
    }
    target_type = GetSpellTargetType(spell_id, 0);
    switch (target_type) {
    case W8_TARGET_TYPE_CASTER:
        ResetCombatSlot(&slot);
        slot.iMonsterID = monster_info->location_id;
        slot.iType = W8_TARGET_KIND_MONSTER;
        if (MonsterSpellTargetOK(monster_info, spell_id, &slot) == 0) {
            break;
        }
        targets->Add(slot);
        break;
    case W8_TARGET_TYPE_ALLY:
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            member = MonsterGetScriptPartByLocationIndex(index);
            if (member->fActive != 0 && member->hp_current != 0 &&
                member->highest_condition < 0x12 && member->fInCombat != 0 &&
                MonsterHostility00546F80(monster_info, member) == 2 &&
                MonsterAttackReachesMonster(monster_info, record, 0, member) != 0) {
                ResetCombatSlot(&slot);
                slot.iType = W8_TARGET_KIND_MONSTER;
                slot.iMonsterID = member->location_id;
                if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                    targets->Add(slot);
                }
            }
        }
        if (IsVisibleUnderConditions(monster_info, &monster_info->player_visibility, sight_kind) !=
            0) {
            for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
                if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                    g_status_685170.buffers.Char[index].hp_current != 0 &&
                    g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
                    MonsterVsCharDisposition(index, monster_info) == 2 &&
                    MonsterAttackReachesCharacter(monster_info, record, 0, index) != 0) {
                    ResetCombatSlot(&slot);
                    slot.iType = W8_TARGET_KIND_CHARACTER;
                    slot.iChar = index;
                    if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                        targets->Add(slot);
                    }
                }
            }
        }
        break;
    case W8_TARGET_TYPE_ENEMY:
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            member = MonsterGetScriptPartByLocationIndex(index);
            if (member != monster_info && member->fActive != 0 && member->hp_current != 0 &&
                member->highest_condition < 0x12 && member->fInCombat != 0 &&
                MonsterHostility00546F80(monster_info, member) == 1 &&
                MonsterAttackReachesMonster(monster_info, record, 0, member) != 0) {
                ResetCombatSlot(&slot);
                slot.iType = W8_TARGET_KIND_MONSTER;
                slot.iMonsterID = member->location_id;
                if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                    targets->Add(slot);
                }
            }
        }
        if (IsVisibleUnderConditions(monster_info, &monster_info->player_visibility, sight_kind) !=
            0) {
            for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
                if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                    g_status_685170.buffers.Char[index].hp_current != 0 &&
                    g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
                    MonsterVsCharDisposition(index, monster_info) == 1 &&
                    MonsterAttackReachesCharacter(monster_info, record, 0, index) != 0) {
                    ResetCombatSlot(&slot);
                    slot.iType = W8_TARGET_KIND_CHARACTER;
                    slot.iChar = index;
                    if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                        targets->Add(slot);
                    }
                }
            }
        }
        break;
    case W8_TARGET_TYPE_ENEMY_GROUP:
        for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
            monster_group = GetMonsterGroupByListIndex(index);
            if (monster_group->group_id != monster_info->monster_group_id &&
                monster_group->members_active_28 != 0 && monster_group->fInCombat != 0 &&
                MonsterGroupAllMembersDying00511850(monster_group) == 0 &&
                MonsterGroupHalfSpellTargetsValid(monster_info, spell_id, monster_group) != 0) {
                ResetCombatSlot(&slot);
                slot.iType = W8_TARGET_KIND_GROUP;
                slot.iGroupID = monster_group->group_id;
                if (MonsterActionReachesTarget(monster_info, record, 0, &slot) != 0) {
                    targets->Add(slot);
                }
            }
        }
        if (PartyHalfSpellTargetsValid(monster_info, spell_id) == 0) {
            break;
        }
        ResetCombatSlot(&slot);
        slot.iType = W8_TARGET_KIND_PARTY;
        if (MonsterActionReachesTarget(monster_info, record, 0, &slot) == 0) {
            break;
        }
        targets->Add(slot);
        break;
    case W8_TARGET_TYPE_CONE:
    case W8_TARGET_TYPE_RADIUS:
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            member = MonsterGetScriptPartByLocationIndex(index);
            if (member != monster_info && member->fActive != 0 && member->hp_current != 0 &&
                member->highest_condition < 0x12 && member->fInCombat != 0 &&
                MonsterHostility00546F80(monster_info, member) == 1 &&
                MonsterAttackReachesMonster(monster_info, record, 0, member) != 0) {
                ResetCombatSlot(&slot);
                slot.iType = W8_TARGET_KIND_MONSTER;
                slot.iMonsterID = member->location_id;
                ResolveTargetPoint(&slot, target_type == W8_TARGET_TYPE_RADIUS);
                slot.iType = W8_TARGET_KIND_PLACE;
                if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0 &&
                    SpellAreaHitsNeutralMonster(monster_info, spell_id, &slot) == 0) {
                    targets->Add(slot);
                }
            }
        }
        if (IsVisibleUnderConditions(monster_info, &monster_info->player_visibility, sight_kind) !=
            0) {
            ResetCombatSlot(&slot);
            slot.iType = W8_TARGET_KIND_PARTY;
            ResolveTargetPoint(&slot, target_type == W8_TARGET_TYPE_RADIUS);
            slot.iType = W8_TARGET_KIND_PLACE;
            if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0 &&
                SpellAreaHitsNeutralMonster(monster_info, spell_id, &slot) == 0) {
                for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
                    if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                        g_status_685170.buffers.Char[index].hp_current != 0 &&
                        g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
                        MonsterVsCharDisposition(index, monster_info) == 1 &&
                        MonsterAttackReachesCharacter(monster_info, record, 0, index) != 0) {
                        targets->Add(slot);
                    }
                }
            }
        }
        break;
    case W8_TARGET_TYPE_ALL_ENEMIES:
        ResetCombatSlot(&slot);
        slot.iType = W8_TARGET_KIND_FIVE;
        if (MonsterSpellTargetOK(monster_info, spell_id, &slot) == 0 ||
            SpellAreaHitsNeutralMonster(monster_info, spell_id, &slot) != 0) {
            break;
        }
        targets->Add(slot);
        break;
    case W8_TARGET_TYPE_POINT:
        ResetCombatSlot(&slot);
        slot.point = monster_info->monster->GetPosition();
        targets->Add(slot);
        break;
    default:
        ResetCombatSlot(&slot);
        if (MonsterSpellTargetOK(monster_info, spell_id, &slot) == 0) {
            break;
        }
        targets->Add(slot);
        break;
    }
    monster_info->action_kind = saved_kind;
    monster_info->action_detail = saved_detail;
}

/* Whether a monster can aim the spell it wants to cast. The two area target
   types aim at the world; anything else either needs no aim at all or has to
   pass the slot check. */
// FUNCTION: WIZ8 0x00534290
bool CanMonsterAimSpell(W8MonsterInfo* monster_info, int spell_id)
{
    W8SpellTargetType target_type = GetSpellTargetType(spell_id, 0);

    if (target_type > W8_TARGET_TYPE_ENEMY_GROUP && target_type < W8_TARGET_TYPE_ALL_ENEMIES) {
        return AimMonsterAtSpellTarget(monster_info, spell_id);
    }
    if (g_spell_records[spell_id].needs_aim_13f != 0) {
        return ResolveTargetPoint(&monster_info->Target, 0);
    }
    return 1;
}

/* The in-combat sweep: refreshes what each monster can see, then drops combat
   for groups that have nothing left to fight. A neutral group stays only
   while a reinforcement is near; a hostile group leaves when no live member
   has a visible enemy, and - when the leader cannot reach - when the nearest
   member sits outside the leader's reach with nothing the group can engage.
   Once no group still has a member fighting, every group leaves at once. */
// FUNCTION: WIZ8 0x00534300
void CheckMonsterGroupsLeaveCombat(void)
{
    W8MonsterGroup* group;
    W8MonsterInfo* member;
    W8MonsterInfo* leader;
    W8MonsterInfo* other;
    unsigned int group_index;
    unsigned int index;
    unsigned int member_index;
    int engaged = 0;
    int sight;
    float nearest;
    float reach;
    float minimum;

    RefreshOutwardSightForAllMonsters();
    for (group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
        group = GetMonsterGroupByListIndex(group_index);
        if (group->members_active_28 == 0 || group->fInCombat == 0 ||
            MonsterGroupAllMembersDying00511850(group) != 0) {
            continue;
        }
        if (group->ubDisposition == DISP_NEUTRAL) {
            if (MonsterGroupHasReinforcement(group) == 0) {
                MonsterGroupLeaveCombat(group);
            }
            continue;
        }
        if (group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 0xc06, 0);
        }
        for (index = 0; index < ILLength(group->monsters); ++index) {
            member = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0xc0b, MONSTER_AI_CPP, IListGetAt(group->monsters, index), 1));
            if (member->fActive == 0 || member->hp_current == 0 ||
                member->highest_condition >= 0x12 || MonsterHasNoVisibleEnemy(member, 0) != 0) {
                continue;
            }
            if (group->ubDisposition != DISP_HOSTILE) {
                break;
            }
            leader = MonsterInfoFromID(0xbcd, MONSTER_AI_CPP, group->leader_id_9f, 1);
            if (GetMonsterGroupFlagC8(group->group_id) != 0 && leader != 0 &&
                leader->fActive != 0) {
                nearest = GetGroupNearestDistance(group);
                reach = CalcRangeDistance(GetMonsterBestRangeCategory(leader, 1, &sight)) +
                        GetMonsterCombatMoveRange(leader) * g_float_005ebc64;
                minimum = GetRangeConstant5EC360() + g_float_005ee77c;
                if (reach <= minimum) {
                    reach = minimum;
                }
                if (reach < nearest && MonsterGroupCanEngage(group) == 0) {
                    MonsterGroupLeaveCombat(group);
                    break;
                }
            }
            if (group == 0) {
                srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 0xc06, 0);
            }
            for (member_index = 0; member_index < ILLength(group->monsters); ++member_index) {
                other = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0xc0b, MONSTER_AI_CPP, IListGetAt(group->monsters, member_index), 1));
                if (other->fActive != 0 && other->hp_current != 0 &&
                    other->highest_condition < 0x12 && MonsterHasNoVisibleEnemy(other, 1) == 0) {
                    ++engaged;
                    break;
                }
            }
            break;
        }
        if (index >= ILLength(group->monsters)) {
            MonsterGroupLeaveCombat(group);
        }
    }
    if (engaged != 0) {
        return;
    }
    for (group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
        group = GetMonsterGroupByListIndex(group_index);
        if (group->members_active_28 != 0 && group->fInCombat != 0 &&
            MonsterGroupAllMembersDying00511850(group) == 0) {
            MonsterGroupLeaveCombat(group);
        }
    }
}

/* Whether the monster sees no living enemy. A raised party-threat flag
   answers at once; a visible hostile party member counts, and `party_only`
   zero also scans the monsters it is hostile to that it can see. */
// FUNCTION: WIZ8 0x00534690
unsigned char MonsterHasNoVisibleEnemy(W8MonsterInfo* monster_info, int party_only)
{
    unsigned int index;
    W8MonsterInfo* other;
    W8VisibilityRecord* record;

    if (monster_info->party_threat.sight_state_04 != W8_SIGHT_UNSEEN) {
        return 0;
    }
    if (monster_info->player_visibility.sight_state_04 != W8_SIGHT_UNSEEN) {
        for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
            if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                g_status_685170.buffers.Char[index].hp_current > 0 &&
                g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
                MonsterVsCharDisposition(index, monster_info) == 1) {
                return 0;
            }
        }
    }
    if (party_only == 0) {
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            other = MonsterGetScriptPartByLocationIndex(index);
            if (other != monster_info && other->fActive != 0 && other->hp_current != 0 &&
                other->highest_condition < 0x12 && other->fInCombat != 0 &&
                MonsterHostility00546F80(monster_info, other) == 1 &&
                (record = FindMonToMonVisibility(monster_info, other)) != 0 &&
                record->sight_state_04 != W8_SIGHT_UNSEEN) {
                return 0;
            }
        }
    }
    return 1;
}

/* Whether any live member of the group has a visible target; the arguments
   forward to MonsterHasVisibleTarget. */
// FUNCTION: WIZ8 0x005347A0
bool MonsterGroupHasVisibleTarget(W8MonsterGroup* monster_group, int party_only, int hostility,
                                  int within_reach)
{
    unsigned int index;
    W8MonsterInfo* member;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 0xc7f, 0);
    }
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        member = GetGroupMemberInfo(monster_group, index);
        if (member->fActive != 0 && member->hp_current > 0 && member->highest_condition < 0x12 &&
            MonsterHasVisibleTarget(member, party_only, hostility, within_reach) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Whether the monster has a living target it can see. `party_only` skips the
   monster scan, `hostility` selects the class - three accepts anything and
   four anything non-neutral - and `within_reach` also requires the target
   inside the engagement range computed from the monster's best attack. */
// FUNCTION: WIZ8 0x00534850
bool MonsterHasVisibleTarget(W8MonsterInfo* monster_info, int party_only, int hostility,
                             int within_reach)
{
    float reach;
    int sight;
    unsigned int index;
    W8MonsterInfo* other;
    W8VisibilityRecord* record;
    char disposition;

    if (within_reach != 0) {
        reach = CalcRangeDistance(GetMonsterBestRangeCategory(monster_info, 1, &sight)) +
                GetMonsterCombatMoveRange(monster_info) * g_float_005ebc64;
        if (reach <= GetRangeConstant5EC360() + g_float_005ee77c) {
            reach = GetRangeConstant5EC360() + g_float_005ee77c;
        }
    }
    if (monster_info->player_visibility.sight_state_04 == W8_SIGHT_SEEN &&
        monster_info->player_visibility.los_flags_05[2] != 0 &&
        (within_reach == 0 || monster_info->monster->GetDistanceToPlayer004C7CB0() <= reach)) {
        for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
            if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                g_status_685170.buffers.Char[index].hp_current > 0 &&
                g_status_685170.buffers.Char[index].highest_condition < 0x12) {
                disposition = MonsterVsCharDisposition(index, monster_info);
                if (hostility == 3) {
                    return 1;
                }
                if (disposition == hostility) {
                    return 1;
                }
                if (hostility == 4 && disposition != 0) {
                    return 1;
                }
            }
        }
    }
    if (party_only == 0) {
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            other = MonsterGetScriptPartByLocationIndex(index);
            if (other != monster_info && other->fActive != 0 && other->hp_current != 0 &&
                other->highest_condition < 0x12 && other->fInCombat != 0) {
                disposition = MonsterHostility00546F80(monster_info, other);
                if (hostility == 3 || disposition == hostility ||
                    (hostility == 4 && disposition != 0)) {
                    record = FindMonToMonVisibility(monster_info, other);
                    if (record != 0 && record->sight_state_04 == W8_SIGHT_SEEN &&
                        record->los_flags_05[2] != 0) {
                        if (within_reach == 0 ||
                            monster_info->monster->GetDistanceToMonster004C7DD0(other->monster) <=
                                reach) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* Whether the monster can flee at all: it has a flee chance, a special-attack
   row, the special-attack cycle the flee uses, enough stamina left, and - for
   non-summoning attacks - somewhere to run. Summoning rows flee at the party
   instead and may not be offered as a special action. */
// FUNCTION: WIZ8 0x00534A40
bool CanMonsterFlee(W8MonsterInfo* monster_info, W8MonsterRecord* record, char exclude_special)
{
    W8GrowableVector<W8CombatSlot> targets;

    if (record->flee_chance_0e1 == 0) {
        return 0;
    }
    if (record->special_attack_kind_0e3 == 0) {
        return 0;
    }
    if (MonsterIsCycleSupported(monster_info->monster, 0x12) == 0) {
        if (g_special_attack_cycle_error_reported == 0) {
            FormatDebugMessage(0, "ERROR: %ls is missing a SPECIAL ATTACK animation cycle", record);
            g_special_attack_cycle_error_reported = 1;
        }
        return 0;
    }
    if (gXStatus.fCombatMode != 0 && monster_info->pCombat->unknown_145[1] != 0) {
        return 0;
    }
    if (static_cast<unsigned int>(monster_info->stamina) <
        static_cast<unsigned int>(monster_info->stamina_max) / 10) {
        return 0;
    }
    if (monster_info->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0 &&
        MonsterSpecialAttackHonorsCastingBlock(record->special_attack_kind_0e3) != 0) {
        return 0;
    }
    if (g_special_attack_table[record->special_attack_kind_0e3][0] ==
        W8_SPECIAL_ATTACK_EFFECT_SUMMON) {
        if (CalcRangeDistance(W8_RANGE_LONG) <
            monster_info->monster->GetDistanceToPlayer004C7CB0()) {
            return 0;
        }
        if (monster_info->ubDisposition == DISP_FRIENDLY) {
            return 0;
        }
        if (exclude_special != 0) {
            return 0;
        }
    } else {
        CollectMonsterSpellTargets(monster_info, W8_AI_SPELL_PLACE, &targets);
        if (targets.GetCount() == 0) {
            return 0;
        }
    }
    return 1;
}

/* Aim a monster that wants to get away. A summoning special-attack row aims
   at where the party is standing instead of at anybody in it. */
// FUNCTION: WIZ8 0x00534cb0
unsigned char AimFleeingMonster(W8MonsterInfo* monster_info, const W8MonsterRecord* record)
{
    srVector3T<float> party;

    if (g_special_attack_table[record->special_attack_kind_0e3][0] ==
        W8_SPECIAL_ATTACK_EFFECT_SUMMON) {
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
   a target of a kind it accepts; fleeing is refused outright for the summoning
   special-attack rows that have nowhere to flee to. */
// FUNCTION: WIZ8 0x00535150
bool IsMonsterActionUsable(W8MonsterInfo* monster_info)
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
        if (g_special_attack_table[GetMonsterDataForInfo(monster_info)->special_attack_kind_0e3]
                                  [0] == W8_SPECIAL_ATTACK_EFFECT_SUMMON) {
            return 0;
        }
        spell_id = W8_AI_SPELL_PLACE;
        break;
    default:
        return 0;
    }
    return MonsterSpellHasPartyTarget(monster_info, spell_id, &monster_info->Target) != 0;
}

/* How near the nearest member of a group has come. The search starts at
   999999 and keeps the smallest player distance any member reports. */
// FUNCTION: WIZ8 0x005324b0
float GetGroupNearestDistance(W8MonsterGroup* group)
{
    unsigned int index;
    int location_id;
    W8MonsterInfo* monster_info;
    float furthest = 999999.0f;
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
   reach. spell_visuals.data sits at W8SpellEffectEntry + 0x10c; the first visual is
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
    visual = effect->spell_visuals.data[0];
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

/* Whether at least half of the group's live in-combat members the caster is
   hostile to accept the spell; the probe is the same MonsterSpellTargetOK
   the cast itself would face. */
// FUNCTION: WIZ8 0x00534DD0
unsigned char MonsterGroupHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id,
                                                W8MonsterGroup* target_group)
{
    unsigned int valid = 0;
    int eligible = 0;
    unsigned int index;
    W8MonsterInfo* member;
    W8CombatSlot slot;

    if (target_group == 0) {
        srAssertFail("pTargetMonsterGroup", MONSTER_AI_CPP, 0xdc4, 0);
    }
    for (index = 0; index < ILLength(target_group->monsters); ++index) {
        int location_id = IListGetAt(target_group->monsters, index);
        member = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xdc9, MONSTER_AI_CPP, location_id, 1));
        if (member->fActive != 0 && member->hp_current != 0 && member->highest_condition < 0x12 &&
            member->fInCombat != 0 && MonsterHostility00546F80(monster_info, member) == 1) {
            ++eligible;
            ResetCombatSlot(&slot);
            slot.iType = W8_TARGET_KIND_MONSTER;
            slot.iMonsterID = location_id;
            if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                ++valid;
            }
        }
    }
    return valid != 0 && ((eligible + 1U) >> 1) <= valid;
}

/* Whether at least half of the party members the caster is hostile to accept
   the spell. */
// FUNCTION: WIZ8 0x00534EF0
unsigned char PartyHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id)
{
    unsigned int valid = 0;
    int eligible = 0;
    unsigned int index;
    W8CombatSlot slot;

    for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
        if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
            g_status_685170.buffers.Char[index].hp_current != 0 &&
            g_status_685170.buffers.Char[index].highest_condition < 0x12 &&
            MonsterVsCharDisposition(index, monster_info) == 1) {
            ++eligible;
            ResetCombatSlot(&slot);
            slot.iType = W8_TARGET_KIND_CHARACTER;
            slot.iChar = index;
            if (MonsterSpellTargetOK(monster_info, spell_id, &slot) != 0) {
                ++valid;
            }
        }
    }
    return valid != 0 && ((eligible + 1U) >> 1) <= valid;
}

/* Whether a live hostile monster already fighting stands nearer to one of
   the group's members than to the party: such a reinforcement keeps a
   neutral group out of the leave sweep and brings it into combat. The group
   only counts members already flagged or whose record cannot idle, and only
   while an encounter list exists. */
// FUNCTION: WIZ8 0x00534FC0
bool MonsterGroupHasReinforcement(W8MonsterGroup* monster_group)
{
    unsigned int index;
    unsigned int other_index;
    W8MonsterInfo* member;
    W8MonsterInfo* other;
    float member_distance;
    float other_distance;
    float monster_distance;

    if (gXStatus.plsMonsterGroupEncounterList == 0) {
        return 0;
    }
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        member = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0xe1d, MONSTER_AI_CPP, IListGetAt(monster_group->monsters, index), 1));
        if ((member->ubDisposition == 0 && GetMonsterDataForInfo(member)->unknown_249 != 0) ||
            member->party_threat.los_flags_05[1] == 0) {
            continue;
        }
        member_distance = member->monster->GetDistanceToPlayer004C7CB0();
        if (member_distance > GetRangeConstant5EC360()) {
            continue;
        }
        for (other_index = 0; other_index < PLLength(gXStatus.plsMonsterList); ++other_index) {
            other = MonsterGetScriptPartByLocationIndex(other_index);
            if (other != member && other->party_threat.los_flags_05[1] != 0 &&
                other->fActive != 0 && other->fInCombat != 0 && other->hp_current != 0 &&
                other->ubDisposition == DISP_HOSTILE) {
                other_distance = other->monster->GetDistanceToPlayer004C7CB0();
                monster_distance = member->monster->GetDistanceToMonster004C7DD0(other->monster);
                if (monster_distance + member_distance < other_distance * g_float_005ee780 &&
                    (member_distance < other_distance || monster_distance < other_distance)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Member info by group list position: entry id at the index, resolved through
   the location index to the script part. Assert lines 3199/3204 pin it late
   in the original file. */
static inline W8MonsterInfo* GetGroupMemberInfo(W8MonsterGroup* group, unsigned int index)
{
    if (group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 3199, 0);
    }
    return MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(3204, MONSTER_AI_CPP, IListGetAt(group->monsters, index), 1));
}

/* Recompute each unled hostile group's engagement byte once the combat state
   has settled: a member still finding its feet (value_14c under three) in the
   group or an allied group drops it, and past that the group's own counter
   decides. */
// FUNCTION: WIZ8 0x00535200
void UpdateMonsterGroupEngagement(void)
{
    W8MonsterGroup* group;
    W8MonsterGroup* allied;
    W8MonsterInfo* member;
    unsigned int group_index;
    unsigned int index;
    int allied_index;
    int member_starting;

    if (g_combat_state->combat_update_count <= 2) {
        return;
    }
    for (group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
        group = GetMonsterGroupByListIndex(group_index);
        if (group->members_active_28 == 0 || group->fInCombat == 0 ||
            group->ubDisposition != DISP_HOSTILE || group->leader_group_id != 0) {
            continue;
        }
        member_starting = 0;
        for (index = 0; index < ILLength(group->monsters); ++index) {
            member = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0xee7, MONSTER_AI_CPP, IListGetAt(group->monsters, index), 1));
            if (member->fActive != 0 && member->fInCombat != 0 &&
                (unsigned int)member->pCombat->value_14c < 3) {
                member_starting = 1;
                break;
            }
        }
        for (allied_index = 0; member_starting == 0 && allied_index < 4; ++allied_index) {
            if (group->allied_group_ids[allied_index] != 0) {
                allied = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0xed1, MONSTER_AI_CPP, group->allied_group_ids[allied_index], 1));
                for (index = 0; index < ILLength(allied->monsters); ++index) {
                    member = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                        0xee7, MONSTER_AI_CPP, IListGetAt(allied->monsters, index), 1));
                    if (member->fActive != 0 && member->fInCombat != 0 &&
                        (unsigned int)member->pCombat->value_14c < 3) {
                        member_starting = 1;
                        break;
                    }
                }
            }
        }
        SetMonsterGroupEngagementState(group->group_id,
                                       member_starting == 0 ? group->unknown_c8[1] < 3 : 0);
    }
}

/* Whether the spell's markers catch at least one party member when cast at
   `slot`. */
// FUNCTION: WIZ8 0x005353E0
unsigned char MonsterSpellHasPartyTarget(W8MonsterInfo* monster_info, int spell_id,
                                         W8CombatSlot* slot)
{
    W8GrowableVector<int> monster_markers;
    W8GrowableVector<int> party_markers;
    W8TargetSource source;
    unsigned int power_level;

    SetTargetSourceToMonster(monster_info, &source);
    power_level =
        ChooseMonsterSpellPowerLevel(monster_info, GetMonsterDataForInfo(monster_info), spell_id);
    PopulateSpellTargetMarkers(spell_id, power_level, &source, slot, &monster_markers,
                               &party_markers, 0);
    return party_markers.GetCount() > 0;
}

/* The out-of-combat sweep: refreshes sight, alerts the faction groups of
   every group already fighting, then enters combat for the groups that
   should - promoting a neutral group's disposition when its record says the
   encounter turns it hostile. */
// FUNCTION: WIZ8 0x005354E0
void CheckMonsterGroupsEnterCombat(void)
{
    W8MonsterGroup* group;
    W8MonsterRecord* record;
    unsigned int index;

    RefreshAllSight();
    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group->fInCombat != 0 && group->ubDisposition == DISP_HOSTILE) {
            AlertSameFactionGroups(group);
        }
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group->fInCombat == 0) {
            if (ShouldMonsterGroupEnterCombat(group) != 0) {
                MonsterGroupEnterCombat(group);
                if (group->ubDisposition == DISP_NEUTRAL) {
                    record = MonsterGroupGetRecord(group);
                    if ((record->flags_0d0 & 1) == 0 && record->faction_id_25f == 0 &&
                        record->hostility_radius_25b != 0 && record->hostility_radius_25b != -1) {
                        SetMonsterGroupHostility(group, 1, 0);
                    }
                }
            }
        }
    }
}

/* Whether a group outside combat should join it. Neutral groups need a
   reinforcement; hostile ones need a member with a visible non-neutral
   target inside the leader's reach, the leader itself close enough to walk
   to the party, or a rendered member already near the party. */
// FUNCTION: WIZ8 0x005355D0
bool ShouldMonsterGroupEnterCombat(W8MonsterGroup* monster_group)
{
    W8MonsterInfo* leader;
    W8MonsterInfo* member;
    unsigned int index;
    int sight;
    float reach;
    float minimum;
    float path_distance;
    srVector3T<float> party;

    if (MonsterGroupAllMembersDying00511850(monster_group) != 0 ||
        monster_group->members_active_28 == 0) {
        return 0;
    }
    if (monster_group->leader_id_9f == -0x32323233 ||
        (leader = MonsterInfoFromID(0xf53, MONSTER_AI_CPP, monster_group->leader_id_9f, 1)) == 0 ||
        leader->monster == 0) {
        FormatDebugMessage(1, "ERROR: Group %d is without an active leader",
                           monster_group->group_id);
        return 0;
    }
    if (gXStatus.fCombatMode == 0) {
        return 0;
    }
    if (monster_group->ubDisposition == DISP_NEUTRAL) {
        if (MonsterGroupHasReinforcement(monster_group) != 0) {
            return 1;
        }
    } else {
        for (index = 0; index < ILLength(monster_group->monsters); ++index) {
            member = GetGroupMemberInfo(monster_group, index);
            if (member->fActive != 0 && member->hp_current != 0 &&
                member->highest_condition < 0x12 && MonsterHasVisibleTarget(member, 0, 4, 1) != 0) {
                reach = CalcRangeDistance(GetMonsterBestRangeCategory(leader, 1, &sight)) +
                        GetMonsterCombatMoveRange(leader) * g_float_005ebc64;
                minimum = GetRangeConstant5EC360() + g_float_005ee77c;
                if (reach <= minimum) {
                    reach = minimum;
                }
                party = g_startup_world_659c0c->GetPosition();
                if ((leader->monster->movement_0c0.position_040 - party).Length() <= reach &&
                    leader->monster->FindNavigatorPathDistance(750.0f, &path_distance) &&
                    path_distance <= reach) {
                    return 1;
                }
                break;
            }
        }
        if (monster_group->ubDisposition == DISP_HOSTILE &&
            MonsterGroupHasRenderableMember(monster_group, 1) != 0) {
            if (GetGroupNearestDistance(monster_group) <= CalcRangeDistance(W8_RANGE_EXTREME)) {
                return 1;
            }
        }
    }
    return 0;
}

// TEMPLATE: WIZ8 0x005358D0
// W8GrowableVector<W8CombatSlot>::~W8GrowableVector<W8CombatSlot>

// SYNTHETIC: WIZ8 0x005358F0
// W8GrowableVector<W8CombatSlot>::`scalar deleting destructor'
