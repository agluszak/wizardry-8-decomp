#include "combat_actions.h"

#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/startup_world.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/main_game_screen.h"
#include "sgp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HostileEncounterContext {
    srVector3T<float> party_position;
    W8MonsterInfo* info;
    float distance;
    int location_id;
};

static void ProvokeHostileEncounterOnGameThread(void* opaque)
{
    HostileEncounterContext* context = static_cast<HostileEncounterContext*>(opaque);
    srVector3T<float> party_position;
    W8MonsterInfo* provoked_info = 0;
    float provoked_distance = 1e30f;
    GetCameraPosition(&party_position);
    {
        W8PathingService* pathing = g_pathing;
        fprintf(stderr,
                "runtime-test pathing: svc=%p size=%d grid=%f bounds=(%.0f %.0f %.0f)-(%.0f %.0f "
                "%.0f)\n",
                (void*)pathing, pathing != 0 ? pathing->size_004 : -1,
                pathing != 0 ? pathing->grid_scale_01c : 0.0f,
                pathing != 0 ? pathing->level_bounds[0] : 0.0f,
                pathing != 0 ? pathing->level_bounds[1] : 0.0f,
                pathing != 0 ? pathing->level_bounds[2] : 0.0f,
                pathing != 0 ? pathing->level_bounds[3] : 0.0f,
                pathing != 0 ? pathing->level_bounds[4] : 0.0f,
                pathing != 0 ? pathing->level_bounds[5] : 0.0f);
        if (pathing != 0) {
            srVector3T<float> probe = party_position;
            unsigned char snap = pathing->SnapWaypointPosition(&probe, 0);
            fprintf(stderr, "runtime-test pathing: party snap=%d y=%f\n", snap, probe.y);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* mi = MonsterGetScriptPartByLocationIndex(i);
                if (mi != 0 && mi->fActive != 0 && mi->p3D != 0) {
                    srVector3T<float> mp = mi->p3D->GetPosition();
                    unsigned char msnap = pathing->SnapWaypointPosition(&mp, 0);
                    fprintf(stderr,
                            "runtime-test pathing: monster=%u pos=(%.0f %.0f %.0f) snap=%d y=%f\n",
                            i, mp.x, mp.y, mp.z, msnap, mp.y);
                    if (i > 5)
                        break;
                }
            }
        }
    }
    W8MonsterInfo* melee_info = 0;
    float melee_distance = 1e30f;
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info != 0 && info->fActive != 0 && info->p3D != 0 && info->monster_group_id != 0 &&
            info->uiCondition[13] == 0) {
            float dist = (info->p3D->GetPosition() - party_position).Length();
            if (dist < provoked_distance) {
                provoked_distance = dist;
                provoked_info = info;
            }
            /* A monster whose attacks all stop at TOUCH must close inside the
               party's own melee band before it can hit, so both sides' swings
               resolve. Anything with SHORT or better reach can stand off at
               ~1800 and chip the party without ever being hit back. */
            W8MonsterRecord* record = MonsterDBFromSpecies(info->monster_species);
            if (record != 0 && GetBestMonsterAttackRange(record, 0) == W8_RANGE_TOUCH &&
                dist < melee_distance) {
                melee_distance = dist;
                melee_info = info;
            }
        }
    }
    if (melee_info != 0) {
        provoked_info = melee_info;
        provoked_distance = melee_distance;
    }
    if (provoked_info != 0) {
        /* Guarantee the provoked monster drops something on death: retail's
           DropMonsterLoot reads *items.GetAt(0) whenever count <= 1, and an
           all-failed treasure table leaves data[0] uninitialized - a real
           retail crash. Seeding one always-hit slot exercises the adoption
           path instead of reproducing that empty-drop bug. */
        W8MonsterRecord* provoked_record = GetMonsterDataForInfo(provoked_info);
        if (provoked_record != 0) {
            W8MonsterTreasureEntry* treasure = &provoked_record->treasure_1c3.slots[0];
            treasure->type = 0;
            treasure->count = 1;
            treasure->item_id = 0x23c; /* the container item SpawnItem drops */
            treasure->chance = 100;
            treasure->dice.base = 1;
            treasure->dice.count = 0;
            treasure->dice.sides = 0;
        }
        unsigned int provoked_location_id = provoked_info->location_id;
        int provoked_group_id = provoked_info->monster_group_id;
        /* At ~17k units the hostile group's own navigation
           never finds a route to the party, so its combat
           turn can never commit a move. Relocate the group
           beside the camera through
           PositionMonsterGroupNearCamera - the same
           placement GroupAttacks uses for summon encounters -
           so the party stays grounded where it stands. A
           party teleport drops the collision state the frame
           loop needs: without ground contact the camera
           falls below the level bounds, BeginPartyMovement
           reads as a fall death and PumpReviewTransition
           unloads the world before StartCombat ever sees a
           grounded party. */
        W8MonsterGroup* provoked_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(__LINE__, "runtime-test", provoked_info->monster_group_id, 0));
        unsigned char placed = 0;
        if (provoked_group != 0) {
            /* Flag clear moves the group straight onto the camera position -
               the deterministic placement the scatter path falls short of.
               The scatter picks a random heading each call; single spots can
               fail MoveMonsterGroupToPosition, so retry it the way the summon
               path retries its three distances before giving up. */
            placed = PositionMonsterGroupNearCamera(provoked_group, 0.0f, 0.0f, 0);
            for (int attempt = 0; attempt < 32 && placed == 0; ++attempt) {
                static const float distances[3] = {0.0f, 1500.0f, 3000.0f};
                placed =
                    PositionMonsterGroupNearCamera(provoked_group, distances[attempt % 3], 0.0f, 1);
            }
        }
        fprintf(stderr, "runtime-test drop: group=%p placed=%d\n", (void*)provoked_group, placed);
        if (placed == 0 && provoked_info != 0 && provoked_info->p3D != 0 && g_pathing != 0) {
            /* The Monastery start point sits off the pathing grid - the snap
               query finds no path cell under the party, so retail's own
               summon placement has nowhere to land the group. Move the party
               onto navigable ground next to the target instead: snap a cell
               near the monster and reinstall the camera and navigator the
               way WorldSetCameraLocation / save-load do. The next move nudge
               re-latches ground contact. */
            srVector3T<float> anchor = provoked_info->p3D->GetPosition();
            /* Ring-search path-valid spots around the monster at melee
               distance: SnapWaypointPosition with snap_to_cell=0 only tests
               the enclosing cell, and SettlePositionToGround restores the
               ground height WorldSetCameraLocation needs. */
            static const float radii[4] = {400.0f, 600.0f, 800.0f, 300.0f};
            static const float dirs[8][2] = {{1.0f, 0.0f},  {-1.0f, 0.0f}, {0.0f, 1.0f},
                                             {0.0f, -1.0f}, {0.7f, 0.7f},  {-0.7f, 0.7f},
                                             {0.7f, -0.7f}, {-0.7f, -0.7f}};
            int teleported = 0;
            for (int r = 0; r < 4 && !teleported; ++r) {
                for (int d = 0; d < 8 && !teleported; ++d) {
                    srVector3T<float> nav = anchor;
                    nav.x += dirs[d][0] * radii[r];
                    nav.z += dirs[d][1] * radii[r];
                    if (g_pathing->SnapWaypointPosition(&nav, 0) == 0) {
                        continue;
                    }
                    nav.y = anchor.y + 2000.0f;
                    nav.y = SettlePositionToGround00420BD0(&nav, 0);
                    srVector3T<float> cam(nav.x, nav.y + g_default_world_height, nav.z);
                    WorldSetCameraLocation(GetWorld659AB8(), &cam);
                    g_startup_world->SetPositionInternal(&nav);
                    RefreshAllSight();
                    party_position = nav;
                    teleported = 1;
                    fprintf(stderr, "runtime-test teleport: party -> (%.0f %.0f %.0f)\n", nav.x,
                            nav.y, nav.z);
                }
            }
        }
        if (provoked_group != 0 && placed != 0) {
            RefreshAllSight();
            SetMonsterGroupNavigatorDirty(provoked_group, 0);
        }
        /* Placement may drop members that found no scatter
           spot; RemoveMonster detaches their monster and frees
           the info, so re-resolve the chosen member and fall
           back to any surviving member of the same group. */
        {
            unsigned int re_index =
                MonsterGetIndexByLocationID(__LINE__, "runtime-test", provoked_location_id, 0);
            provoked_info =
                re_index != (unsigned int)-1 ? MonsterGetScriptPartByLocationIndex(re_index) : 0;
        }
        if (provoked_info == 0 || provoked_info->p3D == 0) {
            provoked_info = 0;
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info != 0 && info->fActive != 0 && info->p3D != 0 &&
                    info->monster_group_id == provoked_group_id) {
                    provoked_info = info;
                    break;
                }
            }
        }
    }
    if (provoked_info != 0 && provoked_info->p3D != 0) {
        provoked_distance = (provoked_info->p3D->GetPosition() - party_position).Length();
        W8TargetSource source;
        W8CombatSlot target;
        memset(&target, 0, sizeof(target));
        target.iChar = -1;
        target.iMonsterID = -1;
        target.iGroupID = -1;
        SetTargetSourceToCharacter(0, &source);
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = provoked_info->location_id;
        MakeTargetGroupHostile(&source, &target);
        fprintf(stderr,
                "runtime-test provoke: hp=%d cond=%d active=%d incombat=%d "
                "hostile=%u combat=%u\n",
                provoked_info->hp_current, provoked_info->highest_condition, provoked_info->fActive,
                provoked_info->fInCombat, gXStatus.hostile_monster_count, gXStatus.fCombatMode);
    }
    context->party_position = party_position;
    context->info = provoked_info;
    context->distance = provoked_distance;
    context->location_id = provoked_info != 0 ? provoked_info->location_id : -1;
}

static void ReadHostileEngagementOnGameThread(void* opaque)
{
    HostileSnapshotQuery* query = static_cast<HostileSnapshotQuery*>(opaque);
    HostileEngagementSnapshot* s = &query->snapshot;
    srVector3T<float> party_position;
    memset(s, 0, sizeof(*s));
    GetCameraPosition(&party_position);
    /* The camera rides the environ's world_height above the party's feet;
       monster distances are ground distances, so measure from the feet. */
    party_position.y -= g_environ != 0 ? g_environ->world_height_30 : g_default_world_height;
    s->screen = g_current_screen_state.id;
    s->pending = g_pending_screen_state.id;
    s->combat_mode = gXStatus.fCombatMode != 0;
    s->hostile_count = gXStatus.hostile_monster_count;
    s->nearest_engaged_distance = 1e30f;
    s->provoked_hp = -1;
    s->provoked_condition = -1;
    s->provoked_in_combat = -1;
    s->provoked_distance = -1.0f;
    s->aim_hp = -1;
    s->aim_dead = 0;
    s->report_target_type = -1;
    s->report_target_monster = -1;
    if (gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info == 0 || info->fActive == 0 || info->p3D == 0)
                continue;
            ++s->active_monsters;
            float distance = (info->p3D->GetPosition() - party_position).Length();
            if (info->uiCondition[W8_CONDITION_HOSTILE] != 0)
                ++s->hostile_condition_monsters;
            if (info->fInCombat != 0) {
                ++s->engaged_hostiles;
                if (distance < s->nearest_engaged_distance)
                    s->nearest_engaged_distance = distance;
                if (info->hp_current == 0 || info->uiCondition[W8_CONDITION_DEAD] != 0) {
                    ++s->engaged_dead;
                } else {
                    s->engaged_hp_total += info->hp_current;
                }
            }
            if (info->location_id == query->location_id) {
                s->provoked_active = 1;
                s->provoked_hp = static_cast<int>(info->hp_current);
                s->provoked_condition = static_cast<int>(info->highest_condition);
                s->provoked_in_combat = info->fInCombat;
                s->provoked_distance = distance;
                s->provoked_dead = info->uiCondition[W8_CONDITION_DEAD] != 0;
                s->provoked_threat_state = info->party_threat.sight_state_04;
            }
            if (info->location_id == query->aim_location_id) {
                s->aim_active = 1;
                s->aim_hp = static_cast<int>(info->hp_current);
                s->aim_dead = info->uiCondition[W8_CONDITION_DEAD] != 0 || info->hp_current == 0;
            }
        }
    }
    if (g_status.buffers.Char != 0) {
        for (int slot = 0; slot < 8; ++slot) {
            const W8Character* character = &g_status.buffers.Char[slot];
            if (character->fInParty != 0) {
                s->party_hp_total += character->hp_current;
                if (character->hp_current == 0 ||
                    character->highest_condition >= W8_CONDITION_UNCONSCIOUS)
                    ++s->incapacitated_members;
            }
        }
    }
    if (g_status.buffers.XChar != 0) {
        s->first_target_type = -1;
        s->first_target_monster = -1;
        for (int slot = 0; slot < 8; ++slot) {
            const W8PartySlotRow* row = &g_status.buffers.XChar[slot];
            if (row->fOccupied != 0 && row->action_03d == W8_ACTION_ATTACK) {
                ++s->queued_attacks;
                if (s->first_target_type < 0) {
                    s->first_target_type = row->target_in_combat.iType;
                    s->first_target_monster = row->target_in_combat.iMonsterID;
                }
            }
        }
    }
    s->round_active = g_combat_state != 0 ? g_combat_state->combat_over_000 : 0;
    s->action_status = g_combat_state != 0 ? g_combat_state->eCombatActionStatus : 0;
    s->action_monster = g_combat_state != 0 && g_combat_state->pActionMonsterInfo != 0
                            ? g_combat_state->pActionMonsterInfo->location_id
                            : -1;
    s->action_char = g_combat_state != 0 ? g_combat_state->iActionChar : -1;
    if (g_combat_state != 0) {
        s->report_count = g_combat_state->attack_report.count;
        s->report_amount = g_combat_state->attack_report.amount;
        s->report_missed = g_combat_state->attack_report.missed;
        s->report_target_type = g_combat_state->attack_report.target.iType;
        s->report_target_monster = g_combat_state->attack_report.target.iMonsterID;
    }
}

/* Queue the ordinary melee attack for every living party slot - the same
   ChooseAction call the ATTACK keyboard command dispatches. Slots that
   already hold an attack keep it so the repeat does not churn targeting. */
static void QueuePartyAttacksOnGameThread(void* opaque)
{
    CombatAttackQuery* query = static_cast<CombatAttackQuery*>(opaque);
    query->eligible = 0;
    query->queued = 0;
    query->aimed = 0;
    if (g_status.buffers.XChar == 0) {
        return;
    }
    /* Aim each slot at a live target: once the provoked monster dies the
       queued attack needs to retarget or the round swings at a corpse. Fall
       back to the nearest live in-combat monster. */
    int aim_location_id = query->location_id;
    if (gXStatus.plsMonsterList != 0) {
        bool alive = false;
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->fActive != 0 && info->fInCombat != 0 && info->hp_current != 0 &&
                info->uiCondition[W8_CONDITION_DEAD] == 0 && info->location_id == aim_location_id) {
                alive = true;
                break;
            }
        }
        if (!alive) {
            srVector3T<float> camera;
            float best = 1e30f;
            GetCameraPosition(&camera);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info == 0 || info->fActive == 0 || info->fInCombat == 0 || info->p3D == 0 ||
                    info->hp_current == 0 || info->uiCondition[W8_CONDITION_DEAD] != 0) {
                    continue;
                }
                float distance = (info->p3D->GetPosition() - camera).Length();
                if (distance < best) {
                    best = distance;
                    aim_location_id = info->location_id;
                }
            }
        }
    }
    query->aim_location_id = aim_location_id;
    query->aim_hp = -1;
    if (aim_location_id >= 0 && gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->location_id == aim_location_id) {
                query->aim_hp = static_cast<int>(info->hp_current);
                break;
            }
        }
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        ++query->eligible;
        /* The fixture party never passes through the equip paths that run
           CalcAttacks, so hand_attacks[].in_play stays clear and
           CanAnyHandReachTarget refuses every swing. Recompute through the
           product's own entry point. */
        CalcAttacks(character);
        if (row->action_03d != W8_ACTION_ATTACK) {
            ChooseAction(slot, W8_ACTION_ATTACK, -1, 0, 0, 1);
        }
        /* ChooseAction only records the action; the swing resolves against
           target_in_combat, which the player path fills through AimAtTarget.
           Without it the queued attack swings at nothing and can never
           land. */
        if (aim_location_id >= 0 && row->action_03d == W8_ACTION_ATTACK) {
            if (row->target_in_combat.iType != W8_TARGET_KIND_MONSTER ||
                row->target_in_combat.iMonsterID != aim_location_id) {
                W8CombatSlot target;
                memset(&target, 0, sizeof(target));
                target.iChar = -1;
                target.iMonsterID = -1;
                target.iGroupID = -1;
                target.iType = W8_TARGET_KIND_MONSTER;
                target.iMonsterID = aim_location_id;
                AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_IN_COMBAT);
            }
            /* ResolveCharacterAttack validates the swing against the
               out-of-combat block, so both contexts need the target. */
            if (row->target_out_of_combat.iType != W8_TARGET_KIND_MONSTER ||
                row->target_out_of_combat.iMonsterID != aim_location_id) {
                W8CombatSlot target;
                memset(&target, 0, sizeof(target));
                target.iChar = -1;
                target.iMonsterID = -1;
                target.iGroupID = -1;
                target.iType = W8_TARGET_KIND_MONSTER;
                target.iMonsterID = aim_location_id;
                AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            }
            if (row->target_in_combat.iType == W8_TARGET_KIND_MONSTER &&
                row->target_out_of_combat.iType == W8_TARGET_KIND_MONSTER) {
                ++query->aimed;
            }
        }
        if (row->action_03d == W8_ACTION_ATTACK) {
            ++query->queued;
        }
    }
}

/* Queue a single-enemy combat spell on every living slot: the fixture grants
   the learned flag and spell points directly (the party-builder path never
   produced a caster), then runs the product's own SetCharacterSpell so the
   cast lands in the same state a player-queued one would. The spell_target
   block comes out of AimAtTarget the same way combat melee aims do. */
static void QueuePartySpellsOnGameThread(void* opaque)
{
    CombatSpellQuery* query = static_cast<CombatSpellQuery*>(opaque);
    query->queued = 0;
    query->aimed = 0;
    if (g_status.buffers.XChar == 0 || g_spell_records == 0) {
        return;
    }
    if (query->spell_id <= 0) {
        for (int id = 1; id < W8_SPELL_COUNT; ++id) {
            const W8SpellRuntimeRecord* record = &g_spell_records[id];
            if (record->target_type == W8_TARGET_TYPE_ENEMY && record->spell_point_cost > 0 &&
                record->spell_level <= 2 && record->usable_when <= W8_SPELL_USABLE_IN_COMBAT &&
                record->effect_dice.count != 0) {
                query->spell_id = id;
                break;
            }
        }
        if (query->spell_id <= 0) {
            return;
        }
    }
    int aim_location_id = query->location_id;
    if (gXStatus.plsMonsterList != 0) {
        bool alive = false;
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->fActive != 0 && info->fInCombat != 0 && info->hp_current != 0 &&
                info->uiCondition[W8_CONDITION_DEAD] == 0 && info->location_id == aim_location_id) {
                alive = true;
                break;
            }
        }
        if (!alive) {
            srVector3T<float> camera;
            float best = 1e30f;
            GetCameraPosition(&camera);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info == 0 || info->fActive == 0 || info->fInCombat == 0 || info->p3D == 0 ||
                    info->hp_current == 0 || info->uiCondition[W8_CONDITION_DEAD] != 0) {
                    continue;
                }
                float distance = (info->p3D->GetPosition() - camera).Length();
                if (distance < best) {
                    best = distance;
                    aim_location_id = info->location_id;
                }
            }
        }
    }
    query->aim_location_id = aim_location_id;
    query->aim_hp = -1;
    if (aim_location_id >= 0 && gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->location_id == aim_location_id) {
                query->aim_hp = static_cast<int>(info->hp_current);
                break;
            }
        }
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        const W8SpellRuntimeRecord* record = &g_spell_records[query->spell_id];
        character->spell_learned[query->spell_id] = 1;
        if (character->iSPLeft[record->realm] < record->spell_point_cost * 8) {
            character->iSPLeft[record->realm] = record->spell_point_cost * 8;
        }
        if (aim_location_id >= 0) {
            W8CombatSlot target;
            memset(&target, 0, sizeof(target));
            target.iChar = -1;
            target.iMonsterID = -1;
            target.iGroupID = -1;
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = aim_location_id;
            /* The spell dialog holds fSpellCastMode and gpSCSV while a cast is
               picked; without them SetCharacterCombatAction clears the current
               target before SetCharacterSpell can copy it, and the SHARED
               branch of ChooseCombatAction dereferences the view. SHARED
               resolves to the spell block for the selected slot, IN_COMBAT
               for the rest. */
            W8SpellCastingView* held_view = gpSCSV;
            unsigned char held_mode = gXStatus.fSpellCastMode;
            if (gpSCSV == 0) {
                gpSCSV = static_cast<W8SpellCastingView*>(calloc(1, sizeof(W8SpellCastingView)));
            }
            gpSCSV->override_spell_104 = query->spell_id;
            gpSCSV->caster = character;
            gXStatus.fSpellCastMode = 1;
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_SPELL);
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_IN_COMBAT);
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            SetCharacterSpell(character, query->spell_id, 1);
            gXStatus.fSpellCastMode = held_mode;
            /* The view block stays allocated: UI code may still read it while
               a cast is in flight, and the real dialog only frees it on close. */
            (void)held_view;
        } else {
            SetCharacterSpell(character, query->spell_id, 1);
        }
        if (row->action_03d == W8_ACTION_CAST_SPELL) {
            ++query->queued;
            if (row->spell_target.iType == W8_TARGET_KIND_MONSTER) {
                ++query->aimed;
            }
        }
    }
}

/* Queue DEFEND on every living party slot - the same ChooseAction call the
   DEFEND command dispatches - so the round resolves monster swings without
   the party killing the attackers first. */
static void QueuePartyDefendOnGameThread(void* opaque)
{
    CombatFleeQuery* query = static_cast<CombatFleeQuery*>(opaque);
    query->eligible = 0;
    query->queued = 0;
    if (g_status.buffers.XChar == 0) {
        return;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied == 0 || character->fInParty == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        ++query->eligible;
        if (row->action_03d != W8_ACTION_DEFEND) {
            ChooseAction(slot, W8_ACTION_DEFEND, -1, 0, 0, 1);
        }
        if (row->action_03d == W8_ACTION_DEFEND) {
            ++query->queued;
        }
    }
}

/* Queue the flee action - the same ChooseAction(W8_ACTION_RUN) the RUN
   combat button dispatches. Party movement is a single party-level action,
   so one eligible slot carries it. */
static void QueuePartyFleeOnGameThread(void* opaque)
{
    CombatFleeQuery* query = static_cast<CombatFleeQuery*>(opaque);
    query->eligible = 0;
    query->queued = 0;
    if (g_status.buffers.XChar == 0) {
        return;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied == 0 || character->fInParty == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        ++query->eligible;
        ChooseAction(slot, W8_ACTION_RUN, -1, 0, 0, 1);
        ++query->queued;
    }
}

/* Pre-wound every living party slot to a single hit point - the fixture
   equivalent of arriving at the encounter already battered - so the first
   landed monster swing crosses the incapacitation threshold and exercises
   the damage-to-condition transition deterministically. */
static void WeakenPartyOnGameThread(void* opaque)
{
    int* weakened = static_cast<int*>(opaque);
    *weakened = 0;
    if (g_status.buffers.XChar == 0 || g_status.buffers.Char == 0) {
        return;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied == 0 || character->fInParty == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        character->hp_current = 1;
        ++*weakened;
    }
}

/* Drop the party on the nearest engaged monster's doorstep: a ring of
   snap-tested offsets around its live position, settled to ground, then the
   WorldSetCameraLocation + navigator reinstall save-load uses. The monster
   AI holds a standoff at its attack-band edge (~1000 for TOUCH monsters),
   and the combat movement phase only moves the party through the scripted
   ResetLevelMovement path, so closing with a queued move does not converge
   inside a test budget. */
static void TeleportPartyNearEngagedOnGameThread(void* opaque)
{
    bool* moved = static_cast<bool*>(opaque);
    srVector3T<float> camera;
    srVector3T<float> anchor;
    float best = 1e30f;

    *moved = false;
    GetCameraPosition(&camera);
    if (gXStatus.plsMonsterList == 0 || g_pathing == 0) {
        return;
    }
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info == 0 || info->fActive == 0 || info->p3D == 0 || info->fInCombat == 0) {
            continue;
        }
        float distance = (info->p3D->GetPosition() - camera).Length();
        if (distance < best) {
            best = distance;
            anchor = info->p3D->GetPosition();
        }
    }
    if (best == 1e30f) {
        return;
    }
    static const float radii[4] = {60.0f, 120.0f, 250.0f, 400.0f};
    static const float dirs[8][2] = {{1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f},  {0.0f, -1.0f},
                                     {0.7f, 0.7f}, {-0.7f, 0.7f}, {0.7f, -0.7f}, {-0.7f, -0.7f}};
    for (int r = 0; r < 4 && !*moved; ++r) {
        for (int d = 0; d < 8 && !*moved; ++d) {
            srVector3T<float> nav = anchor;
            nav.x += dirs[d][0] * radii[r];
            nav.z += dirs[d][1] * radii[r];
            if (g_pathing->SnapWaypointPosition(&nav, 0) == 0) {
                continue;
            }
            nav.y = anchor.y + 2000.0f;
            nav.y = SettlePositionToGround00420BD0(&nav, 0);
            /* SettleFrom-above lands on the highest floor under the start
               point; a raised ledge or roof leaves the party out of every
               band, so only accept landings near the monster's own level. */
            if (nav.y - anchor.y > 400.0f || anchor.y - nav.y > 400.0f) {
                continue;
            }
            srVector3T<float> cam(nav.x, nav.y + g_default_world_height, nav.z);
            WorldSetCameraLocation(GetWorld659AB8(), &cam);
            g_startup_world->SetPositionInternal(&nav);
            *moved = true;
        }
    }
    if (*moved) {
        /* Aim requires party_threat.sight_state_04 == 1 - currently seen - and a
           teleport leaves the sight bookkeeping stale. */
        RefreshAllSight();
        srVector3T<float> after;
        GetCameraPosition(&after);
        fprintf(
            stderr,
            "runtime-test approach-tp: cam=(%.0f %.0f %.0f) anchor=(%.0f %.0f %.0f) dist=%.0f\n",
            after.x, after.y, after.z, anchor.x, anchor.y, anchor.z, (anchor - after).Length());
    }
}

struct CombatModeRequest {
    bool enabled;
    bool achieved;
    bool waiting_on_ground;
    bool input_blocked;
};

/* Manual combat entry has a one-frame ground-contact precondition in retail.
   Do the readiness check and command dispatch on the game thread in one
   callback; polling flag4 from the driver and injecting the key afterward
   races ApplyCameraMotion clearing the flag on the next frame. */
static void RequestCombatModeOnGameThread(void* opaque)
{
    CombatModeRequest* request = static_cast<CombatModeRequest*>(opaque);
    request->achieved = (gXStatus.fCombatMode != 0) == request->enabled;
    request->waiting_on_ground = false;
    request->input_blocked = IsScreenInputBlocked() != 0;
    if (request->achieved || request->input_blocked) {
        return;
    }
    if (request->enabled && GetLevelDataFlag4() == 0) {
        request->waiting_on_ground = true;
        return;
    }
    DispatchMGSCommand(W8_MGS_COMMAND_TOGGLE_COMBAT);
    request->achieved = (gXStatus.fCombatMode != 0) == request->enabled;
}

int EngageHostile(RuntimeCase& test)
{
    HostileEncounterContext context;
    if (!test.on_game_thread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                             240000)) {
        return -1;
    }
    if (context.location_id < 0) {
        test.fail("hostile-fixture", "active-monster-not-found");
        return -1;
    }
    if (!WaitForCombatMode(test, true)) {
        return -1;
    }
    test.step("combat-aggroed");
    return context.location_id;
}

bool SnapshotEngagement(RuntimeCase& test, HostileSnapshotQuery& query, const char* step)
{
    return test.on_game_thread(step, ReadHostileEngagementOnGameThread, &query, 60000);
}

static bool CombatModeMatches(const GameplaySnapshot& state, void* ctx)
{
    return state.combat == *static_cast<const bool*>(ctx);
}

bool WaitForCombatMode(RuntimeCase& test, bool enabled, unsigned long timeout_ms)
{
    bool wanted = enabled;
    return test.wait_until(enabled ? "combat-started" : "combat-ended", timeout_ms,
                           CombatModeMatches, &wanted);
}

bool RequestCombatMode(RuntimeCase& test, bool enabled, unsigned long timeout_ms)
{
    const char* step = enabled ? "combat-start" : "combat-end";
    unsigned int started = GetTickCount();
    HeldCommand nudge(test, W8_MGS_COMMAND_MOVE_FORWARD);
    bool nudging = false;
    CombatModeRequest last = {enabled, false, false, false};
    while (GetTickCount() - started < timeout_ms && gfProgramIsRunning) {
        CombatModeRequest request = {enabled, false, false, false};
        unsigned long elapsed = GetTickCount() - started;
        if (elapsed >= timeout_ms) {
            break;
        }
        if (!test.on_game_thread(step, RequestCombatModeOnGameThread, &request,
                                 timeout_ms - elapsed)) {
            return false;
        }
        last = request;
        if (request.achieved) {
            nudge.release();
            test.step(enabled ? "combat-started" : "combat-ended");
            return true;
        }
        if (enabled && request.waiting_on_ground) {
            if (!nudging) {
                nudging = nudge.begin();
            } else {
                nudge.repeat();
            }
        }
        Sleep(20);
    }
    nudge.release();
    fprintf(stderr,
            "runtime-test combat-mode: desired=%d achieved=%d waiting_on_ground=%d "
            "input_blocked=%d\n",
            enabled, last.achieved, last.waiting_on_ground, last.input_blocked);
    return test.fail(step, enabled ? "combat-not-entered" : "combat-not-ended");
}

bool QueuePartyAttack(RuntimeCase& test, CombatAttackQuery& out, const char* step)
{
    if (!test.on_game_thread(step, QueuePartyAttacksOnGameThread, &out, 60000)) {
        return false;
    }
    fprintf(stderr, "runtime-test aim: queued=%d aimed=%d aim=%d hp=%d\n", out.queued, out.aimed,
            out.aim_location_id, out.aim_hp);
    return true;
}

bool QueuePartySpell(RuntimeCase& test, CombatSpellQuery& out, const char* step)
{
    if (!test.on_game_thread(step, QueuePartySpellsOnGameThread, &out, 60000)) {
        return false;
    }
    if (out.spell_id <= 0) {
        return test.fail(step, "no-single-enemy-spell");
    }
    fprintf(stderr, "runtime-test cast: spell=%d queued=%d aimed=%d aim=%d hp=%d\n", out.spell_id,
            out.queued, out.aimed, out.aim_location_id, out.aim_hp);
    return true;
}

int QueuePartyDefend(RuntimeCase& test)
{
    CombatFleeQuery out;
    if (!test.on_game_thread("combat-defend", QueuePartyDefendOnGameThread, &out, 60000)) {
        return -1;
    }
    return out.queued;
}

int WeakenParty(RuntimeCase& test)
{
    int weakened = 0;
    if (!test.on_game_thread("combat-weaken", WeakenPartyOnGameThread, &weakened, 60000)) {
        return -1;
    }
    fprintf(stderr, "runtime-test casualty: weakened=%d\n", weakened);
    return weakened;
}

bool QueuePartyFlee(RuntimeCase& test, CombatFleeQuery& out)
{
    if (!test.on_game_thread("combat-flee", QueuePartyFleeOnGameThread, &out, 60000)) {
        return false;
    }
    fprintf(stderr, "runtime-test flee: queued=%d eligible=%d\n", out.queued, out.eligible);
    return true;
}

bool MovePartyNearTarget(RuntimeCase& test, float nearest_engaged_distance)
{
    if (nearest_engaged_distance <= 800.0f) {
        return true;
    }
    bool moved = false;
    if (!test.on_game_thread("combat-approach", TeleportPartyNearEngagedOnGameThread, &moved,
                             60000)) {
        return false;
    }
    fprintf(stderr, "runtime-test approach: moved=%d dist=%.0f\n", moved, nearest_engaged_distance);
    return true;
}

bool StartCombatRound(RuntimeCase& test, const char* step)
{
    return test.tap(W8_MGS_COMMAND_START_COMBAT_ROUND, step);
}

bool WaitRoundActive(RuntimeCase& test, HostileSnapshotQuery& query, unsigned long budget_ms,
                     const char* step)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < budget_ms && gfProgramIsRunning) {
        Sleep(5);
        if (!SnapshotEngagement(test, query, step)) {
            return false;
        }
        if (query.snapshot.round_active != 0) {
            test.step(step);
            return true;
        }
    }
    return test.fail(step, "round-start-not-consumed");
}

bool WaitRoundFinished(RuntimeCase& test, HostileSnapshotQuery& query, unsigned long budget_ms,
                       const char* step,
                       bool (*observe)(const HostileEngagementSnapshot& state, void* ctx),
                       void* ctx)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < budget_ms && gfProgramIsRunning) {
        Sleep(5);
        if (!SnapshotEngagement(test, query, step)) {
            return false;
        }
        if (observe != 0 && observe(query.snapshot, ctx)) {
            return true;
        }
        if (query.snapshot.round_active == 0 || query.snapshot.combat_mode == 0) {
            return true;
        }
    }
    return test.fail(step, "round-did-not-resolve");
}

bool ExpectTargetDamaged(const HostileEngagementSnapshot& state, int aim_location_id,
                         int baseline_hp)
{
    return aim_location_id >= 0 && baseline_hp >= 0 &&
           (state.aim_dead || !state.aim_active ||
            (state.aim_hp >= 0 && state.aim_hp < baseline_hp));
}

bool MonsterAttackExecuted(const HostileEngagementSnapshot& state)
{
    /* Monsters never own iActionChar: an executing monster action is a
       monster actor resolving its queued swing. Combat.cpp schedules at
       status 1, executes at 2, retires at 3. */
    return state.action_monster >= 0 && state.action_status >= 2;
}

bool PartyTookCasualty(const HostileEngagementSnapshot& state)
{
    return state.incapacitated_members != 0;
}

/* The held forward key only moves the party while combat movement is
   queued: the combat_move branch of the displaced check also requires the
   movement budget to have been spent. */
static bool MovePartyInCombat(RuntimeCase& test, int command)
{
    GameplaySnapshot before, now;
    if (!test.snapshot(before, "combat-walk")) {
        return false;
    }
    HeldCommand held(test, command);
    if (!held.begin()) {
        return test.fail("combat-walk", "binding-missing");
    }
    if (!test.tap(W8_MGS_COMMAND_START_COMBAT_ROUND, "combat-round")) {
        return false;
    }
    bool moved = false;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        held.repeat();
        if (!test.snapshot(now, "combat-walk")) {
            return false;
        }
        srVector3T<float> delta = now.position - before.position;
        delta.y = 0;
        if (delta.Length() > 1.0f && now.movement_budget < before.movement_budget) {
            moved = true;
            break;
        }
    }
    if (!moved && test.has_snapshot()) {
        const GameplaySnapshot& last = test.last_snapshot();
        fprintf(stderr,
                "runtime-test movement: command=%d horizontal=(%.2f %.2f) input=%.2f world=%.2f "
                "budget=%d modal=%u blocked=%u render_flags=%02x held_key=%u held_down=%u\n",
                command, last.position.x - before.position.x, last.position.z - before.position.z,
                last.input_motion, last.world_motion, last.movement_budget,
                last.modal_owner_present, last.world_update_blocked, last.world_render_flags,
                last.held_key, last.held_key_down);
    }
    if (!moved) {
        return test.fail("combat-walk", "action-did-not-move-party");
    }
    return true;
}

static bool CombatMovementUiShown(const GameplaySnapshot& state, void*)
{
    return state.movement_ui != 0;
}

static bool RoundInactive(const GameplaySnapshot& state, void*)
{
    return state.round_active == 0;
}

bool CombatRoundtripCase(RuntimeCase& test)
{
    RT_REQUIRE(test, RequestCombatMode(test, true, 3000));
    RT_REQUIRE(test, test.tap(W8_MGS_COMMAND_PARTY_WALK, "combat-walk"));
    RT_REQUIRE(test, test.wait_until("combat-action-queued", 3000, CombatMovementUiShown, 0));
    RT_REQUIRE(test, MovePartyInCombat(test, W8_MGS_COMMAND_MOVE_FORWARD));
    test.step("combat-party-moved");
    /* START_COMBAT_ROUND finishes the live movement action through
       BeginFreeTurnPhase. */
    RT_REQUIRE(test, test.tap(W8_MGS_COMMAND_START_COMBAT_ROUND, "combat-round"));
    RT_REQUIRE(test, test.wait_until("combat-round", 5000, RoundInactive, 0));
    RT_REQUIRE(test, RequestCombatMode(test, false, 3000));
    return true;
}

bool HostileEncounterCase(RuntimeCase& test)
{
    int location_id = EngageHostile(test);
    RT_REQUIRE(test, location_id >= 0);

    HostileSnapshotQuery query;
    query.location_id = location_id;
    query.aim_location_id = location_id;
    bool round_requested = false;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 10000 && gfProgramIsRunning) {
        Sleep(5);
        if (!SnapshotEngagement(test, query, "hostile-action")) {
            return false;
        }
        const HostileEngagementSnapshot& state = query.snapshot;
        // Combat.cpp schedules at status 1, executes at 2, and retires at 3.
        // Any engaged hostile executing proves the turn pipeline; the provoked
        // monster's allies reach the party first as often as it does.
        if (state.action_monster >= 0 && state.action_status >= 2) {
            test.step("hostile-action-executed");
            return true;
        }
        if (state.round_active)
            round_requested = false;
        if (state.combat_mode && !state.round_active && !round_requested) {
            if (!StartCombatRound(test, "hostile-round")) {
                return false;
            }
            round_requested = true;
        }
    }
    /* First missing product transition report: scheduler, engagement and the
       provoked monster's own state narrow where the frontier actually is. */
    fprintf(stderr,
            "runtime-test hostile-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u cond13=%u nearest=%.0f party_hp=%u provoked active=%u hp=%d "
            "cond=%d incombat=%d dist=%.0f combat=%u round=%u action=%d monster=%d\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.hostile_condition_monsters, query.snapshot.nearest_engaged_distance,
            query.snapshot.party_hp_total, query.snapshot.provoked_active,
            query.snapshot.provoked_hp, query.snapshot.provoked_condition,
            query.snapshot.provoked_in_combat, query.snapshot.provoked_distance,
            query.snapshot.combat_mode, query.snapshot.round_active, query.snapshot.action_status,
            query.snapshot.action_monster);
    return test.fail("hostile-action", "monster-execution-not-observed");
}

/* Latch state for the mid-round observe hooks: the case keeps the durable
   evidence it has collected so far. */
struct AttackRoundObserve {
    RuntimeCase* test;
    bool* damaged;
    int aim_id;
    int baseline_hp;
    unsigned int* last_trace;
};

static bool ObserveTargetDamage(const HostileEngagementSnapshot& state, void* opaque)
{
    AttackRoundObserve* observe = static_cast<AttackRoundObserve*>(opaque);
    if (GetTickCount() - *observe->last_trace > 2000) {
        *observe->last_trace = GetTickCount();
        fprintf(stderr,
                "runtime-test trace: engaged=%u nearest=%.0f party_hp=%u "
                "ehp=%u dead=%u combat=%u round=%u action=%d char=%d monster=%d "
                "aim=%d hp=%d active=%u dead=%d "
                "report(hits=%u dmg=%u miss=%u target=%d:%d) "
                "threat=%d target=%d:%d\n",
                state.engaged_hostiles, state.nearest_engaged_distance, state.party_hp_total,
                state.engaged_hp_total, state.engaged_dead, state.combat_mode, state.round_active,
                state.action_status, state.action_char, state.action_monster, observe->aim_id,
                state.aim_hp, state.aim_active, state.aim_dead, state.report_count,
                state.report_amount, state.report_missed, state.report_target_type,
                state.report_target_monster, state.provoked_threat_state, state.first_target_type,
                state.first_target_monster);
    }
    /* This round queued only party melee at aim_id, so durable HP
       loss/deactivation of that monster is the integration proof a queued
       party attack landed; enemy actions and party damage are never
       evidence. */
    if (!*observe->damaged && ExpectTargetDamaged(state, observe->aim_id, observe->baseline_hp)) {
        *observe->damaged = true;
        observe->test->step("party-attack-hit");
        observe->test->step("target-damaged");
        fprintf(stderr,
                "runtime-test hit: target=%d baseline_hp=%d hp=%d active=%u dead=%d "
                "action=%d char=%d report=%u/%u\n",
                observe->aim_id, observe->baseline_hp, state.aim_hp, state.aim_active,
                state.aim_dead, state.action_status, state.action_char, state.report_count,
                state.report_amount);
    }
    /* A kill ends combat in the same frame: the latched success and a
       finished round outrank the combat-mode drop. */
    return *observe->damaged;
}

/* Beyond engagement: queue real melee attacks aimed at a live monster on
   every living slot, drive bounded rounds, and require durable HP loss or
   death of that aimed monster after the round starts. The fixture queues only
   party melee against this target; hostile actions target the party, so this
   is durable evidence of a party hit. attack_report is diagnostic only:
   retail consumes and clears it synchronously before the harness can poll it. */
bool CombatAttackCase(RuntimeCase& test)
{
    int location_id = EngageHostile(test);
    RT_REQUIRE(test, location_id >= 0);

    HostileSnapshotQuery query;
    query.location_id = location_id;
    query.aim_location_id = location_id;
    unsigned int last_trace = 0;
    int aim_id = location_id;
    int baseline_hp = -1;
    bool damaged = false;
    /* Success requires evidence that a party member's attack struck the aimed
       monster and that monster durably lost HP or died. Each round is
       bounded so a stall in the product pipeline fails at the stage that
       stopped, not at one monolithic timeout. */
    const int kMaxRounds = 6;
    for (int round = 0; round < kMaxRounds && gfProgramIsRunning; ++round) {
        if (!SnapshotEngagement(test, query, "combat-round-queue")) {
            return false;
        }
        if (!MovePartyNearTarget(test, query.snapshot.nearest_engaged_distance)) {
            return false;
        }
        CombatAttackQuery attack;
        memset(&attack, 0, sizeof(attack));
        attack.location_id = location_id;
        if (!QueuePartyAttack(test, attack, "combat-round-queue")) {
            return false;
        }
        if (attack.queued == 0) {
            return test.fail("combat-round-queue", "no-attack-queued");
        }
        test.step("combat-attack-queued");
        baseline_hp = attack.aim_hp;
        aim_id = attack.aim_location_id;
        query.aim_location_id = aim_id;

        RT_REQUIRE(test, StartCombatRound(test, "combat-round-start"));
        RT_REQUIRE(test, WaitRoundActive(test, query, 15000, "combat-round-start"));

        /* combat-round-resolve: attack_report is transient scratch state and
           ReportCharacterAttackResult clears it before returning.
           Observe the durable consequence instead. */
        AttackRoundObserve observe = {&test, &damaged, aim_id, baseline_hp, &last_trace};
        if (!WaitRoundFinished(test, query, 90000, "combat-round-resolve", ObserveTargetDamage,
                               &observe)) {
            return false;
        }
        if (damaged) {
            return true;
        }
        if (query.snapshot.combat_mode == 0) {
            return test.fail("combat-attack", "combat-ended-without-party-hit");
        }
    }
    fprintf(stderr,
            "runtime-test attack-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u party_hp=%u nearest=%.0f provoked active=%u hp=%d cond=%d incombat=%d "
            "dead=%d dist=%.0f threat=%d combat=%u round=%u action=%d char=%d monster=%d "
            "aim=%d aim_hp=%d aim_active=%u aim_dead=%d queued=%d target=%d:%d "
            "report(hits=%u dmg=%u miss=%u target=%d:%d)\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.party_hp_total, query.snapshot.nearest_engaged_distance,
            query.snapshot.provoked_active, query.snapshot.provoked_hp,
            query.snapshot.provoked_condition, query.snapshot.provoked_in_combat,
            query.snapshot.provoked_dead, query.snapshot.provoked_distance,
            query.snapshot.provoked_threat_state, query.snapshot.combat_mode,
            query.snapshot.round_active, query.snapshot.action_status, query.snapshot.action_char,
            query.snapshot.action_monster, aim_id, query.snapshot.aim_hp, query.snapshot.aim_active,
            query.snapshot.aim_dead, query.snapshot.queued_attacks,
            query.snapshot.first_target_type, query.snapshot.first_target_monster,
            query.snapshot.report_count, query.snapshot.report_amount, query.snapshot.report_missed,
            query.snapshot.report_target_type, query.snapshot.report_target_monster);
    return test.fail("combat-attack", "party-attack-hit-not-observed");
}

/* The spell twin of the attack round-trip: queue a learned single-enemy
   spell on every living slot, drive rounds until a party cast executes and
   the aimed monster durably loses HP or dies - and report which product
   transition never arrived on failure. */
static void EndSpellFixtureCombatOnGameThread(void*)
{
    if (g_combat_state != 0 && gXStatus.fCombatMode != 0) {
        EndCombat(0);
    }
    gfProgramIsRunning = 0;
}

bool CombatSpellCase(RuntimeCase& test)
{
    int location_id = EngageHostile(test);
    RT_REQUIRE(test, location_id >= 0);

    HostileSnapshotQuery query;
    query.location_id = location_id;
    query.aim_location_id = location_id;
    bool round_requested = false;
    bool baseline_taken = false;
    bool cast_queued = false;
    bool cast_executed = false;
    bool damaged = false;
    int spell_id = 0;
    int aim_id = location_id;
    int baseline_hp = -1;
    unsigned int started = GetTickCount();
    unsigned int last_trace = 0;
    while (GetTickCount() - started < 180000 && gfProgramIsRunning) {
        Sleep(5);
        if (!SnapshotEngagement(test, query, "combat-spell")) {
            return false;
        }
        if (GetTickCount() - last_trace > 2000) {
            last_trace = GetTickCount();
            const HostileEngagementSnapshot& t = query.snapshot;
            fprintf(stderr,
                    "runtime-test trace: engaged=%u nearest=%.0f party_hp=%u "
                    "ehp=%u dead=%u combat=%u round=%u action=%d char=%d monster=%d "
                    "aim=%d hp=%d active=%u dead=%d "
                    "report(hits=%u dmg=%u miss=%u) threat=%d target=%d:%d\n",
                    t.engaged_hostiles, t.nearest_engaged_distance, t.party_hp_total,
                    t.engaged_hp_total, t.engaged_dead, t.combat_mode, t.round_active,
                    t.action_status, t.action_char, t.action_monster, aim_id, t.aim_hp,
                    t.aim_active, t.aim_dead, t.report_count, t.report_amount, t.report_missed,
                    t.provoked_threat_state, t.first_target_type, t.first_target_monster);
        }
        const HostileEngagementSnapshot& state = query.snapshot;
        if (!baseline_taken && state.engaged_hostiles != 0) {
            baseline_taken = true;
        }
        /* Monsters never own iActionChar: an executing char action is a party
           actor. The melee attack_report target does not describe spells. */
        if (state.action_char >= 0 && state.action_status >= 2 && !cast_executed) {
            cast_executed = true;
            test.step("party-cast-executed");
        }
        if (cast_executed && !damaged && ExpectTargetDamaged(state, aim_id, baseline_hp)) {
            damaged = true;
            test.step("target-damaged");
        }
        if (baseline_taken && state.screen == W8_SCREEN_MAIN_GAME && state.pending == -1 &&
            state.engaged_hostiles == 0 && !state.combat_mode) {
            /* A cast still queued when combat ends resolves through the
               out-of-combat spell UI, which runs its own modal loop and never
               reaches the quit flag. Dismiss it through the cancel binding. */
            for (int probe = 0; probe < 8; ++probe) {
                GameplaySnapshot modal;
                if (!test.snapshot(modal, "combat-spell")) {
                    return false;
                }
                if (!modal.modal_owner_present) {
                    break;
                }
                if (!test.tap(W8_MGS_COMMAND_CANCEL, "combat-spell")) {
                    return false;
                }
                Sleep(500);
            }
            test.step("combat-ended");
            if (damaged) {
                return true;
            }
            return test.fail("combat-spell", "combat-ended-without-target-damage");
        }
        if (state.round_active)
            round_requested = false;
        if (state.combat_mode && !state.round_active && !round_requested) {
            if (!MovePartyNearTarget(test, state.nearest_engaged_distance)) {
                return false;
            }
            CombatSpellQuery cast;
            memset(&cast, 0, sizeof(cast));
            cast.location_id = location_id;
            cast.spell_id = spell_id;
            if (!QueuePartySpell(test, cast, "combat-spell")) {
                return false;
            }
            spell_id = cast.spell_id;
            baseline_hp = cast.aim_hp;
            aim_id = cast.aim_location_id;
            query.aim_location_id = aim_id;
            if (cast.queued > 0 && !cast_queued) {
                cast_queued = true;
                test.step("combat-cast-queued");
            }
            if (!StartCombatRound(test, "combat-round")) {
                return false;
            }
            round_requested = true;
        }
        if (cast_queued && cast_executed && damaged) {
            /* The spell observation ends while monsters are still taking their
               combat turn. Finish that fixture on the game thread before the
               driver asks WinMain to quit; otherwise the next combat update
               can keep WinMain inside GameLoop past the process deadline. */
            if (!test.on_game_thread("combat-spell-cleanup", EndSpellFixtureCombatOnGameThread, 0,
                                     10000)) {
                return false;
            }
            return true;
        }
    }
    fprintf(stderr,
            "runtime-test cast-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u party_hp=%u nearest=%.0f provoked active=%u hp=%d cond=%d incombat=%d "
            "dead=%d dist=%.0f threat=%d combat=%u round=%u action=%d char=%d monster=%d "
            "spell=%d aim=%d aim_hp=%d aim_active=%u aim_dead=%d queued=%d target=%d:%d "
            "report(hits=%u dmg=%u miss=%u)\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.party_hp_total, query.snapshot.nearest_engaged_distance,
            query.snapshot.provoked_active, query.snapshot.provoked_hp,
            query.snapshot.provoked_condition, query.snapshot.provoked_in_combat,
            query.snapshot.provoked_dead, query.snapshot.provoked_distance,
            query.snapshot.provoked_threat_state, query.snapshot.combat_mode,
            query.snapshot.round_active, query.snapshot.action_status, query.snapshot.action_char,
            query.snapshot.action_monster, spell_id, aim_id, query.snapshot.aim_hp,
            query.snapshot.aim_active, query.snapshot.aim_dead, query.snapshot.queued_attacks,
            query.snapshot.first_target_type, query.snapshot.first_target_monster,
            query.snapshot.report_count, query.snapshot.report_amount,
            query.snapshot.report_missed);
    return test.fail("combat-spell",
                     cast_executed ? "target-damage-not-observed" : "party-cast-not-observed");
}
