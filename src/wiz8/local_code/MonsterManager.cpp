#include <stdio.h>
#include "wiz8/engine_code/3d.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/fact_state.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/ThingEditorShared.h"
#include "wiz8/music_playlist.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/fact_state.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/regions.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/utility.h"
#include <math.h>
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/sr_api.h"
#include "DEBUG.H"
#include "random.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
// GLOBAL: WIZ8 0x005ed4f0
float g_monster_record_float_scale = 20.0f;
// GLOBAL: WIZ8 0x00683698
int g_monster_info_iterator_index;

#define MONSTER_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp"
#define MAX_MONSTERS_IN_DATABASE 1000

void DestroyMonsterActionQueue(W8MonsterInfo* monster_info);
// FUNCTION: WIZ8 0x0052A780
int CalculateMonsterFatigueBand(int current, int maximum)
{
    int percentage_lost = 100 - current * 100 / static_cast<unsigned int>(maximum);
    if (percentage_lost < 50)
        return 0;
    if (percentage_lost < 70)
        return 1;
    if (percentage_lost < 85)
        return 2;
    if (percentage_lost < 95)
        return 3;
    return 4;
}

/* The two cycles that always start regardless of the pending one: 0x14 is the
   cycle a motionless monster is still allowed to enter, and 0x15 is death. */
enum { W8_CYCLE_NONE = 0xff, W8_CYCLE_STOP = 0x14, W8_CYCLE_DEATH = 0x15 };
enum { W8_BEHAVIOUR_NEVER_STOP = 3 };
/* The character array the alternate-name form indexes, and the slot it uses. */

// FUNCTION: WIZ8 0x004e3930
W8MonsterInfo* CreateMonsterInfo(W8MonsterGroup* group, W8MonsterRecord* record,
                                 srVector3T<float>* position)
{
    W8MonsterInfo* monster_info = static_cast<W8MonsterInfo*>(malloc(sizeof(W8MonsterInfo)));
    int value;

    if (monster_info == 0) {
        return 0;
    }

    memset(monster_info, 0, sizeof(W8MonsterInfo));
    monster_info->location_id = g_status.next_monster_location_id_234e;
    while (g_status.next_monster_location_id_234e++, monster_info->location_id == 0) {
        monster_info->location_id = g_status.next_monster_location_id_234e;
    }

    monster_info->monster_group_id = group->group_id;
    monster_info->monster_species = group->monster_id;
    monster_info->ubDisposition = group->ubDisposition;
    monster_info->scale_24f = -1.0f;
    monster_info->fActive = 0;
    monster_info->p3D = 0;
    monster_info->fInCombat = false;
    monster_info->pCombat = 0;
    monster_info->position_17 = *position;
    monster_info->derived_23 = GetCameraFacingYaw(position);

    value = RollDice(&record->hit_points_d6);
    monster_info->uiHPMax = value;
    monster_info->hp_current = value;
    value = RollDice(&record->runtime_stat_da);
    monster_info->stamina_max = value;
    monster_info->stamina = value;
    monster_info->fatigue_band = CalculateMonsterFatigueBand(value, value);

    memset(monster_info->uiCondition, 0, sizeof(monster_info->uiCondition));
    memset(monster_info->enchantments, 0, sizeof(monster_info->enchantments));
    monster_info->highest_condition = 0;
    monster_info->condition_argument = 0;
    monster_info->effect_2de = 0;
    memset(&monster_info->modifiers_1db, 0, sizeof(monster_info->modifiers_1db));
    monster_info->fMotionless = 0;
    monster_info->ai_mode_255 = 0;
    monster_info->summoned_2da = 0;
    monster_info->insanity_summon_344 = -1;
    memset(monster_info->movement_watch_position, 0, sizeof(monster_info->movement_watch_position));

    if (PLAdoptAppend(record->unborn_26a != 0 ? gXStatus.plsUnbornMonsterList
                                              : gXStatus.plsMonsterList,
                      monster_info) == -1) {
        free(monster_info);
        return 0;
    }

    IListAdd(group->monsters, monster_info->location_id);
    ++group->member_count;
    ++group->active_member_count;
    RequestRedrawParty();
    g_octree->VisitPointCopy(static_cast<unsigned short>(monster_info->location_id), position);
    return monster_info;
}

/* __stdcall, not __cdecl: 0x0042E650 ends in `ret 0x4`, and both callers here
   clean only three of the four dwords they push across the tail. */

static __inline W8MonsterRecord* MonsterDBFromSpeciesInline(unsigned int monster_species);

/* Materialize one inactive script record in the world. Existing engine
   Monsters are reattached without rebuilding their representation; absent or
   reset ones are activated first and choose either the birth cycle or a random
   idle subcycle according to membership in the unborn list. */
// FUNCTION: WIZ8 0x004e3c70
void ActivateMonsterInWorld(W8MonsterInfo* monster_info)
{
    W8MonsterRecord* record;
    int registry_before;
    int registry_after;
    srVector3T<float> camera_position;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x174, 0);
    }
    if (monster_info->fActive != 0) {
        return;
    }

    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    if (monster_info->p3D == 0 || monster_info->p3D->IsPendingFinalize() != 0) {
        registry_before = GetUsedPageFileBytes();
        ActivateMonster(monster_info, 0);
        MonsterSetLocationId(monster_info->p3D, monster_info->location_id);
        MonsterSetAdjustedPosition(monster_info->p3D, &monster_info->position_17);

        if (PListIndexOf(gXStatus.plsUnbornMonsterList, monster_info) != -1) {
            if (MonsterIsCycleSupported(monster_info->p3D, 0) == 0) {
                srAssertFail("MonsterIsCycleSupported(pMonsterInfo->p3D, CYCLE_BIRTH)",
                             MONSTER_MANAGER_CPP, 0x190, "Unborn monsters must have CYCLE_BIRTH!");
            }
            MonsterSetCycleBehaviour(monster_info->p3D, 1);
            MonsterSetCycle(monster_info->p3D, 0);
            if (MonsterQuery(monster_info->p3D, 0) == -1) {
                srAssertFail("MonsterQuery(pMonsterInfo->p3D, QUERY_NUM_FRAMES) != -1",
                             MONSTER_MANAGER_CPP, 0x196, 0);
            }
            MonsterSetCycleSubCycle(monster_info->p3D, 0);
            MonsterSetAnimating(monster_info->p3D, 0);
            monster_info->p3D->active_088 = 0;
            monster_info->p3D->inactive_215 = 1;
        } else {
            MonsterSetCycle(monster_info->p3D, 1);
            MonsterSetCycleBehaviour(monster_info->p3D, 3);
            if (MonsterQuery(monster_info->p3D, 0) == -1) {
                srAssertFail("MonsterQuery(pMonsterInfo->p3D, QUERY_NUM_FRAMES) != -1",
                             MONSTER_MANAGER_CPP, 0x1a4, 0);
            }
            MonsterSetCycleSubCycle(monster_info->p3D, Random(MonsterQuery(monster_info->p3D, 0)));
            MonsterSetAnimating(monster_info->p3D, monster_info->fMotionless == 0);
        }

        AddMonsterToWorld(GetWorld(), monster_info->p3D);
        MonsterSetFacing(monster_info->p3D, monster_info->derived_23);
        RebuildMonsterDerivedStats(monster_info->location_id);
        monster_info->p3D->movement_0c0.leadership_rank_008 =
            static_cast<unsigned int>(record->effective_level_24f) * 0x10000U +
            monster_info->location_id;
        MonsterSetLocationId(monster_info->p3D, monster_info->location_id);
        monster_info->p3D->pending_finalize_216 = 0;

        registry_after = GetUsedPageFileBytes();
        monster_info->p3D->registry_weight_27c = registry_after - registry_before;
        g_monster_cycle_registry_weight += registry_after - registry_before;
        if (GetFlag68F105() != 0) {
            ShowNoticef(7, L"%dK\n",
                        static_cast<unsigned int>(registry_after - registry_before) >> 10);
        }
        InitializeMonsterRangeCapabilities(
            monster_info, MonsterDBFromSpeciesInline(monster_info->monster_species));
    }

    WorldGetCameraLocation(GetWorld(), &camera_position);
    MonsterForward4A7BE0(monster_info->p3D, &camera_position);
    UpdateCycleRepresentation(monster_info->p3D, GetWorld());
    g_octree->VisitPointCopy(static_cast<unsigned short>(monster_info->location_id),
                             &monster_info->position_17);
    monster_info->fActive = 1;
    ++gXStatus.active_monster_count;
    if (monster_info->p3D != 0) {
        int damage_stage_count = monster_info->p3D->GetDamageStageCount();
        if (damage_stage_count > 1) {
            int damage_stage =
                ((monster_info->uiHPMax - static_cast<int>(monster_info->hp_current)) *
                 damage_stage_count) /
                monster_info->uiHPMax;
            if (damage_stage >= damage_stage_count - 1) {
                damage_stage = damage_stage_count - 1;
            }
            monster_info->p3D->SetDamageStage(damage_stage);
        }
    }
    monster_info->plsVisMonToMon = PLCreate();
    if (monster_info->plsVisMonToMon == 0) {
        srAssertFail("pMonsterInfo->plsVisMonToMon != NULL", MONSTER_MANAGER_CPP, 0x1de, 0);
    }
    RequestRefreshPartyState();
    RefreshFlaggedMainGameState();
    if (record->can_open_doors_0c0 != 0) {
        monster_info->p3D->movement_0c0.flags_000 |= 0x10000000;
    }
    BindNpcToMonster(record->npc_kind_0cd, 1, monster_info->location_id);
}

/* Activate the representation lazily. The mode selects whether all available
   cycles are loaded or only the startup cycle; both paths share the world
   context and preserve the loader's success result for the source assertion. */
// FUNCTION: WIZ8 0x004e4050
void ActivateMonster(W8MonsterInfo* monster_info, int mode)
{
    W8MonsterRecord* record;
    W8GrCycleLoadContext context;
    bool success;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x1f5, 0);
    }
    if (monster_info->fActive != 0) {
        srAssertFail("!pMonsterInfo->fActive", MONSTER_MANAGER_CPP, 0x1f6, 0);
    }
    if (monster_info->p3D != 0) {
        return;
    }

    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    context.world_00 = GetWorld();
    context.bitmap_directory_04 = 0;
    context.directory_08 = "Data\\Monsters";

    if (mode == 0) {
        success = MonsterReadAllCycles004C58E0(&context, record->cycle_name_189, &monster_info->p3D,
                                               1, monster_info->location_id);
        if (success == 0) {
            srAssertFail("fSuccess", MONSTER_MANAGER_CPP, 0x20a,
                         "ActivateMonster: ERROR - MonsterReadAllCycles failed");
        }
    } else if (mode == 1) {
        success = MonsterReadAllCycles004C58E0(&context, record->cycle_name_189, &monster_info->p3D,
                                               0, monster_info->location_id);
        if (success == 0) {
            srAssertFail("fSuccess", MONSTER_MANAGER_CPP, 0x20f,
                         "ActivateMonster: ERROR - MonsterReadAllCycles failed");
        }
        MonsterSetCycle(monster_info->p3D, 1);
    }

    if (monster_info->scale_24f < g_float_005ebb34 ||
        g_status.level_progress[g_status.current_level].visited == 0) {
        monster_info->cycle17_state = MonsterGetMirrorX(monster_info->p3D);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->p3D, monster_info->scale_24f);
    } else {
        MonsterSetScale(monster_info->p3D, monster_info->scale_24f);
        MonsterSetMirrorX(monster_info->p3D, monster_info->cycle17_state);
    }

    ApplyMonsterRepresentationScale(monster_info->p3D);
    MonsterSetCycle(monster_info->p3D, 1);
    RefreshMonsterStandingHeight(monster_info->p3D);
}

// FUNCTION: WIZ8 0x004e4600
void ClearMonsterPathAndResume(W8MonsterInfo* monster_info)
{
    int result;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2f7, 0);
    }
    MonsterReplacePath(monster_info->p3D, 0);
    monster_info->p3D->flags_00c &= 0xdfffffff;
    MonsterForward4537E0(monster_info->p3D);
    if (monster_info->fMotionless == 0) {
        result = MonsterQuery(monster_info->p3D, 6);
        if (result != 1 && result != 2 && monster_info->p3D->m_pRep->pending_cycle == -1) {
            StartMonsterCycle(monster_info, 1, 3);
        }
    }
}

// FUNCTION: WIZ8 0x004e4690
void MonsterStartsDying(W8MonsterInfo* monster_info, char display_message)
{
    if (monster_info->p3D->IsDying() == 0) {
        StartMonsterCycle(monster_info, 0x15, 1);
        DeactivateMonster(monster_info);
        RecordMonsterKill(monster_info, display_message);
        RemoveMonster(
            MonsterGetIndexByLocationID(0x31f, MONSTER_MANAGER_CPP, monster_info->location_id, 1),
            0);
    }
}

/* The kill bookkeeping a monster's death runs: name the kill on the notice
   channel the killer's kind selects, clear the conditions the dead monster
   sourced, and - unless the record opts out - land the faction hit and bank
   the kill count and experience for a monster that fought the party. */
// FUNCTION: WIZ8 0x004E46F0
void RecordMonsterKill(W8MonsterInfo* monster_info, char announce)
{
    int killer_party_slot = -1;
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
    }
    unsigned int monster_species = monster_info->monster_species;
    if (monster_species >= MAX_MONSTERS_IN_DATABASE) {
        srAssertFail("uiMonsterSpecies < MAX_MONSTERS_IN_DATABASE", MONSTER_MANAGER_CPP, 0x5f3, 0);
    }
    W8MonsterRecord* record = gXStatus.monster_record_cache[monster_species];
    if (record == 0) {
        record = static_cast<W8MonsterRecord*>(malloc(sizeof(W8MonsterRecord)));
        if (record != 0) {
            if (LoadMonsterDatabaseRecord(monster_species, record) == 0) {
                free(record);
                record = 0;
            } else {
                gXStatus.monster_record_cache[monster_species] = record;
            }
        }
    }
    unsigned int monster_list_index =
        MonsterGetIndexByLocationID(0x34f, MONSTER_MANAGER_CPP, monster_info->location_id, 1);
    unsigned int notice_channel;
    if (TargetSourceIsCharacter(&monster_info->condition_target_304, 0)) {
        killer_party_slot = monster_info->condition_target_304.iChar;
        if (killer_party_slot != -1 &&
            (killer_party_slot < 0 || killer_party_slot >= W8_PARTY_SLOT_COUNT ||
             !g_status.buffers.XChar[killer_party_slot].fOccupied ||
             g_status.buffers.Char[killer_party_slot].fInParty == 0)) {
            srAssertFail("(iKilledByPC == BAD_INDEX) || VALID_CHAR(iKilledByPC)",
                         MONSTER_MANAGER_CPP, 0x354, 0);
        }
        notice_channel = (killer_party_slot == -1) + 8;
    } else {
        notice_channel = 9;
    }
    if (announce != 0 && monster_info->death_processed_253 == 0 &&
        monster_info->party_threat.sight_state_04 != W8_SIGHT_UNSEEN) {
        ShowNoticef(notice_channel, L"%s %s!", GetMonsterName(monster_info, 0, 0),
                    gppStringList[g_condition_notices[0x49]]);
    }
    ReleaseMonsterConditionBindings(monster_info);
    if (monster_info->summoned_2da == 1) {
        return;
    }
    if (monster_info->death_processed_253 != 0) {
        return;
    }
    if (killer_party_slot == -1) {
        if (!TargetSourceIsMonster(&monster_info->condition_target_304, 0)) {
            goto done;
        }
        unsigned int killer_index = MonsterGetIndexByLocationID(
            0x37d, MONSTER_MANAGER_CPP, monster_info->condition_target_304.iMonsterID, 0);
        if (killer_index == 0xffffffff) {
            goto done;
        }
        W8MonsterInfo* killer_info = MonsterGetScriptPartByLocationIndex(killer_index);
        if (killer_info == 0) {
            srAssertFail("pKillerMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x381, 0);
        }
        if (killer_info->ubDisposition != 2) {
            goto done;
        }
    }
    ApplyFactionChange(0, 1, static_cast<signed char>(record->faction_id_25f), monster_list_index);
done:
    MonsterKilled(record->record_id_187, killer_party_slot);
    if (monster_info->fInCombat != 0 &&
        ((monster_info->ubDisposition == 1 && monster_info->uiCondition[0xd] == 0) ||
         (monster_info->ubDisposition == 2 && monster_info->uiCondition[0xd] != 0))) {
        ++g_combat_state->combat_result_00c;
        if (g_status.current_level < W8_LEVEL_COUNT) {
            ++g_status.level_progress[g_status.current_level].monster_kill_count_03;
        }
        if (killer_party_slot != -1) {
            W8Character* killer = &g_status.buffers.Char[killer_party_slot];
            ++killer->kill_count_09f9;
            if (record->significant_kill_268 != 0) {
                unsigned int level_total = 0;
                int occupied = 0;
                for (int slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
                    if (g_status.buffers.XChar[slot].fOccupied) {
                        level_total += g_status.buffers.Char[slot].uiExpLevel;
                        ++occupied;
                    }
                }
                int share = occupied == 0
                                ? 1
                                : static_cast<int>(level_total / static_cast<double>(occupied));
                if (share + 2 <= static_cast<int>(record->effective_level_24f)) {
                    QueueCharacterEvent(killer, g_effect_005ee61c, 0, g_effect_argument_005ed8c8,
                                        g_effect_argument_005ed914);
                }
            }
        }
        unsigned int experience = record->experience_override_26b;
        if (experience == 0) {
            experience = record->experience_181;
            if (experience == 0) {
                FormatDebugMessage(0, "DATA ERROR: %s is worth 0 XPs", record);
            }
        }
        g_combat_state->experience_pool_010 += experience;
        if (g_status.status_ints_3121[monster_species] == 0) {
            g_status.status_ints_3121[monster_species] = 1;
        }
    }
}

// FUNCTION: WIZ8 0x004e5550
unsigned int MonsterGetIndexByLocationID(int caller_line, const char* caller_file, int location_id,
                                         unsigned char assert_on_failure)
{
    unsigned int index;
    W8MonsterInfo* monster;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->location_id == location_id) {
            return index;
        }
    }

    for (index = 0; index < PLLength(gXStatus.plsUnbornMonsterList); ++index) {
        monster = (W8MonsterInfo*)PLGet(gXStatus.plsUnbornMonsterList, index);
        if (monster->location_id == location_id) {
            return index + 10000;
        }
    }

    if (assert_on_failure != 0) {
        srAssertFail(
            "FALSE", MONSTER_MANAGER_CPP, 0x5c1,
            reinterpret_cast<const char*>(String("MonsterIndex: ID %d not found (%s line %d)",
                                                 location_id, caller_file, caller_line)));
    }
    return 0xffffffff;
}

// FUNCTION: WIZ8 0x004e5620
W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index)
{
    W8MonsterInfo* result;
    const char* detail;
    int line;

    if (monster_list_index < 10000 || monster_list_index >= 20000) {
        if (monster_list_index >= PLLength(gXStatus.plsMonsterList)) {
            srAssertFail("uiMonsterListIndex < (UINT32) PLLength(gXStatus.plsMonsterList)",
                         MONSTER_MANAGER_CPP, 0x5da, 0);
        }
        result = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, monster_list_index);
        if (result != 0) {
            return result;
        }
        detail = reinterpret_cast<const char*>(
            String("MonsterInfo: ERROR - PLGet failed, index %d, pList %d", monster_list_index,
                   gXStatus.plsMonsterList));
        line = 0x5de;
    } else {
        if (monster_list_index - 10000 >= PLLength(gXStatus.plsUnbornMonsterList)) {
            srAssertFail(
                "(uiMonsterListIndex-10000) < (UINT32) PLLength(gXStatus.plsUnbornMonsterList)",
                MONSTER_MANAGER_CPP, 0x5d1, 0);
        }
        result = (W8MonsterInfo*)PLGet(gXStatus.plsUnbornMonsterList, monster_list_index - 10000);
        if (result != 0) {
            return result;
        }
        detail = reinterpret_cast<const char*>(
            String("MonsterInfo: ERROR - PLGet failed, index %d, pList %d", monster_list_index,
                   gXStatus.plsMonsterList));
        line = 0x5d5;
    }
    /* One tail, reached from both branches with only the line number differing.
       The original carries it as a variable and calls srAssertFail through a
       register it loads before the branch; writing the call out in each branch
       instead duplicates it. */
    srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, line, detail);
    return 0;
}

static __inline W8MonsterRecord* MonsterDBFromSpeciesInline(unsigned int monster_species)
{
    W8MonsterRecord* record;

    if (monster_species >= MAX_MONSTERS_IN_DATABASE) {
        srAssertFail("uiMonsterSpecies < MAX_MONSTERS_IN_DATABASE", MONSTER_MANAGER_CPP, 0x5f3, 0);
    }
    record = gXStatus.monster_record_cache[monster_species];
    if (record == 0) {
        record = (W8MonsterRecord*)malloc(sizeof(W8MonsterRecord));
        if (record == 0) {
            return 0;
        }
        if (!LoadMonsterDatabaseRecord(monster_species, record)) {
            free(record);
            return 0;
        }
        gXStatus.monster_record_cache[monster_species] = record;
    }
    return record;
}

// FUNCTION: WIZ8 0x004e5720
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
    }
    return MonsterDBFromSpeciesInline(monster_info->monster_species);
}

// FUNCTION: WIZ8 0x004e57c0
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species)
{
    return MonsterDBFromSpeciesInline(monster_species);
}

static __inline W8MonsterInfo* MonsterInfoFromIDInline(int caller_line, const char* caller_file,
                                                       int location_id,
                                                       unsigned char assert_on_failure)
{
    W8MonsterInfo* monster = 0;
    unsigned int index;

    index = MonsterGetIndexByLocationID(0x61d, MONSTER_MANAGER_CPP, location_id, 0);
    if (index != 0xffffffff) {
        monster = MonsterGetScriptPartByLocationIndex(index);
    }
    if (monster == 0 && assert_on_failure != 0) {
        srAssertFail(
            "FALSE", MONSTER_MANAGER_CPP, 0x626,
            reinterpret_cast<const char*>(String("MonsterInfoFromID: ID %d not found (%s line %d)",
                                                 location_id, caller_file, caller_line)));
    }
    return monster;
}

// FUNCTION: WIZ8 0x004e5840
W8MonsterInfo* MonsterInfoFromID(int caller_line, const char* caller_file, int location_id,
                                 unsigned char assert_on_failure)
{
    return MonsterInfoFromIDInline(caller_line, caller_file, location_id, assert_on_failure);
}

// FUNCTION: WIZ8 0x004e58b0
W8MonsterRecord* GetMonsterDataByLocationID(int location_id)
{
    W8MonsterInfo* monster;
    unsigned int index;

    index = MonsterGetIndexByLocationID(0x632, MONSTER_MANAGER_CPP, location_id, 1);
    monster = MonsterGetScriptPartByLocationIndex(index);
    if (monster == 0) {
        return 0;
    }
    return MonsterDBFromSpeciesInline(monster->monster_species);
}

// FUNCTION: WIZ8 0x004e5950
W8Monster* GetMonsterByLocationID(int location_id)
{
    W8MonsterInfo* monster_info;
    unsigned int index;

    if (location_id != -1) {
        index = MonsterGetIndexByLocationID(0x64f, MONSTER_MANAGER_CPP, location_id, 0);
        if (index != 0xffffffff) {
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info != 0) {
                return monster_info->p3D;
            }
        }
    }
    return 0;
}

/* Scale the monster database's combat movement range by the shared factor at
   0x005ED4F0 before range/path consumers use it. */
// FUNCTION: WIZ8 0x004e5990
float GetMonsterCombatMoveRange(W8MonsterInfo* monster_info)
{
    W8MonsterRecord* record;
    float result;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
    }
    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    if (record == 0) {
        srAssertFail("pMonsterDB", MONSTER_MANAGER_CPP, 0x66a, 0);
    }
    result = record->combat_move_range_1ba * g_monster_record_float_scale;
    return result;
}

// FUNCTION: WIZ8 0x004e5a50
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info)
{
    W8Monster* monster = monster_info->p3D;

    if (monster != 0) {
        int count = monster->GetDamageStageCount();
        if (count > 1) {
            int value =
                ((monster_info->uiHPMax - static_cast<int>(monster_info->hp_current)) * count) /
                monster_info->uiHPMax;
            if (value >= count - 1) {
                value = count - 1;
            }
            monster->SetDamageStage(value);
        }
    }
}

// FUNCTION: WIZ8 0x004e5aa0
W8MonsterInfo* GetNextMonsterInfo(unsigned char reset_iterator)
{
    W8MonsterInfo* result = 0;
    int index;

    if (reset_iterator != 0) {
        g_monster_info_iterator_index = 0;
    }
    if (g_monster_info_iterator_index < (int)PLLength(gXStatus.plsMonsterList)) {
        index = g_monster_info_iterator_index++;
        result = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, index);
    }
    return result;
}

// FUNCTION: WIZ8 0x004e5af0
int GetMonsterQuadrant(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x6b9, 0);
    }
    return GetQuadrantForPosition(monster_info->p3D->GetPosition());
}

// FUNCTION: WIZ8 0x004e5b50
int GetMonsterCycleFallbackValue(unsigned int monster_species)
{
    W8MonsterRecord* record;

    record = MonsterDBFromSpeciesInline(monster_species);
    if (record == 0) {
        return -1;
    }
    if (FindFirstGrCycleByName(record->cycle_name_189) != 0) {
        return 0;
    }
    if (GetRenderOptionState(0xe) != 0) {
        return record->alternate_model_index_257;
    }
    return record->model_index_253;
}

// FUNCTION: WIZ8 0x004e5c00
void ProcessMonstersAtCombatEnd(unsigned char forced_cleanup)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->fActive != 0 && monster_info->hp_current > 0 &&
            monster_info->uiCondition[W8_CONDITION_DEAD] == 0 && monster_info->summoned_2da != 0) {
            if (forced_cleanup == 0) {
                FormatNotice(9, 0, gppStringList[W8_NOTICE_MONSTER_SLAIN],
                             GetMonsterName(monster_info, 0, 0));
            }
            ReleaseMonsterConditionBindings(monster_info);
            if (forced_cleanup == 0) {
                monster_info->death_processed_253 = 1;
                if (monster_info->p3D->IsDying() == 0) {
                    StartMonsterCycle(monster_info, 0x15, 1);
                    DeactivateMonster(monster_info);
                    RecordMonsterKill(monster_info, 1);
                    RemoveMonster(MonsterGetIndexByLocationID(0x31f, MONSTER_MANAGER_CPP,
                                                              monster_info->location_id, 1),
                                  0);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004e5d00
void ConvertMonsterAttributes(W8MonsterInfo* monster_info)
{
    unsigned int monster_attribute = 0;

    do {
        W8MonsterRecord* record;
        int attribute_index;
        int value;

        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = record->attribute_values_d1[monster_attribute];
        attribute_index = 0;

        if (monster_attribute >= 5) {
            srAssertFail("uiMonsterAttribute < MONSTER_ATTR_COUNT", MONSTER_MANAGER_CPP, 0x78d, 0);
        }
        switch (monster_attribute) {
        case 0:
            attribute_index = 0;
            break;
        case 1:
            attribute_index = 1;
            break;
        case 2:
            attribute_index = 4;
            break;
        case 3:
            attribute_index = 5;
            break;
        case 4:
            attribute_index = 6;
            break;
        default:
            srAssertFail("FALSE", MONSTER_MANAGER_CPP, 0x798,
                         "ConvertMonsterAttribute: ERROR - Invalid monster attribute");
        }

        value += monster_info->modifiers_1db.attribute_adjustments[attribute_index];
        if (value > 125) {
            value = 125;
        } else if (value < 1) {
            value = 1;
        }
        monster_info->attributes[monster_attribute] = static_cast<unsigned char>(value);
        ++monster_attribute;
    } while (monster_attribute < 5);
}

// FUNCTION: WIZ8 0x004e5e50
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->monster_species == monster_species) {
            return monster_info;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004e5ea0
void ResetLivingMonstersAfterCombat(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->hp_current > 0) {
            monster_info->p3D->ResetMovementAndGroupState();
            if (monster_info->ai_mode_255 > 0 && monster_info->ai_mode_255 <= 3) {
                monster_info->ai_mode_255 = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004e5f00
void DestroyUngroupedMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->monster_group_id == 0) {
            W8Monster* monster = monster_info->p3D;

            if (monster != 0) {
                if ((monster->flags_1dc & W8_MONSTER_REMOVE_AFTER_FADE) != 0 &&
                    monster->removal_state_22e != 0) {
                    monster->ApplyRemovalStateEffects();
                }
            }
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            DeactivateMonster(monster_info);
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
            }
            if (monster_info->p3D != 0) {
                DetachMonsterRepresentation(monster_info->p3D, GetWorld());
                RemoveMonsterFromWorldList(GetWorld(), monster_info->p3D);
                DeleteMonster(monster_info->p3D);
                monster_info->p3D = 0;
            }
            g_octree->UnregisterLocationObjects(
                static_cast<unsigned short>(monster_info->location_id));
            ReleaseNpcBinding(monster_info->bound_npc_index);
            void* removed = PLRemoveAt(gXStatus.plsMonsterList, index);
            if (removed != 0) {
                free(removed);
            }
            --index;
        }
    }
}

// FUNCTION: WIZ8 0x004e6020
void SetMonsterControlState(W8MonsterInfo* monster_info, int control_state)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x85b, 0);
    }
    switch (control_state) {
    case 0:
    case 2:
        if (monster_info->control_state == 1 && monster_info->p3D->linked_navigator_05c == 0) {
            monster_info->p3D->ClearMovement();
            monster_info->ai_mode_255 = 0;
        }
        break;
    }
    monster_info->control_state = control_state;
    RecountActiveMonsterGroupMembers(GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0x872, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1)));
}

// FUNCTION: WIZ8 0x004e60b0
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, unsigned char motionless)
{
    unsigned char previous = monster_info->fMotionless;
    W8Monster* monster = monster_info->p3D;

    monster_info->fMotionless = motionless;
    if (motionless == 0) {
        if (previous != 0) {
            MonsterSetAnimating(monster, 1);
            if (monster_info->p3D->m_pRep->pending_cycle == -1) {
                StartMonsterCycle(monster_info, 1, 3);
            }
        }
    } else if (previous == 0) {
        signed char cycle_value = monster->m_pRep->pending_cycle;

        if (cycle_value != -1) {
            if (cycle_value == 0x14) {
                return;
            }
            MonsterSetPendingCycle(monster, -1);
        }
        MonsterSetAnimating(monster, 0);
    }
}

// FUNCTION: WIZ8 0x004e6130
void MoveMonsterToLiveList(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x8b8, 0);
    }
    if (PListRemove(gXStatus.plsUnbornMonsterList, monster_info) == 0) {
        return;
    }

    PLAdoptAppend(gXStatus.plsMonsterList, monster_info);
    if (MonsterIsCycleSupported(monster_info->p3D, 0) != 0) {
        MonsterSetCycle(monster_info->p3D, 0);
        MonsterSetCycleBehaviour(monster_info->p3D, 1);
    } else {
        MonsterSetCycle(monster_info->p3D, 1);
        MonsterSetCycleBehaviour(monster_info->p3D, 3);
    }
    MonsterSetCycleSubCycle(monster_info->p3D, 0);
    MonsterSetAnimating(monster_info->p3D, 1);
    monster_info->p3D->active_088 = 1;
    monster_info->p3D->inactive_215 = 0;
}

static inline double DistanceBetweenPositions(const srVector3T<float>* first,
                                              const srVector3T<float>& second)
{
    return (*first - second).Length();
}

// FUNCTION: WIZ8 0x004e61e0
W8MonsterInfo* FindNearestMonsterInfo(const srVector3T<float>* position, double maximum_distance)
{
    W8MonsterInfo* nearest = 0;
    double nearest_distance = 1.0e11;
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, index);
        double distance = DistanceBetweenPositions(position, monster_info->p3D->GetPosition());

        if (distance < nearest_distance &&
            (maximum_distance == 0.0 || distance < maximum_distance)) {
            nearest = monster_info;
            nearest_distance = distance;
        }
    }

    for (index = 0; index < PLLength(gXStatus.plsUnbornMonsterList); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(gXStatus.plsUnbornMonsterList, index);
        double distance = DistanceBetweenPositions(position, monster_info->p3D->GetPosition());

        if (distance < nearest_distance &&
            (maximum_distance == 0.0 || distance < maximum_distance)) {
            nearest = monster_info;
            nearest_distance = distance;
        }
    }
    return nearest;
}

// FUNCTION: WIZ8 0x004e6370
void InitializeMonsterRuntimeStats(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->uiHPMax = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->stamina_max = value;
        monster_info->stamina = value;
        monster_info->fatigue_band = CalculateMonsterFatigueBand(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->p3D, monster_info->scale_24f);
        ApplyMonsterRepresentationScale(monster_info->p3D);
        RefreshMonsterStandingHeight(monster_info->p3D);
    }

    for (index = 0; index < PLLength(gXStatus.plsUnbornMonsterList); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(gXStatus.plsUnbornMonsterList, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->uiHPMax = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->stamina_max = value;
        monster_info->stamina = value;
        monster_info->fatigue_band = CalculateMonsterFatigueBand(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->p3D, monster_info->scale_24f);
        ApplyMonsterRepresentationScale(monster_info->p3D);
        RefreshMonsterStandingHeight(monster_info->p3D);
    }
}

// FUNCTION: WIZ8 0x004e65d0
float CalculateMonsterScale(W8MonsterInfo* monster_info)
{
    float minimum;
    float maximum;

    MonsterGetScaleRange(monster_info->p3D, &minimum, &maximum);
    if (minimum == 0.0f || maximum == 0.0f) {
        return MonsterGetScale(monster_info->p3D);
    }

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
    }
    W8MonsterRecord* record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    int minimum_hp = record->hit_points_d6.base + record->hit_points_d6.count;
    int maximum_hp =
        record->hit_points_d6.base + record->hit_points_d6.count * record->hit_points_d6.sides;
    float scale =
        ((maximum - minimum) * (static_cast<unsigned int>(monster_info->uiHPMax) - minimum_hp)) /
            (maximum_hp - minimum_hp) +
        minimum;
    float result = (maximum - minimum) * (Random(1000) * 0.0004f - 0.2f) + scale;

    if (result < minimum) {
        return minimum;
    }
    if (result > maximum) {
        return maximum;
    }
    return result;
}

// FUNCTION: WIZ8 0x004e67a0
void TryStartMonsterCycle2(W8MonsterInfo* monster_info, W8Monster* monster, int query_state)
{
    if (monster_info->fActive != 0 && monster_info->hp_current > 0 &&
        monster_info->uiCondition[W8_CONDITION_DEAD] == 0 &&
        monster_info->within_viewing_distance != 0 && query_state == 1) {
        int result = MonsterQuery(monster, 2);

        if (result != 0 && monster_info->fMotionless == 0) {
            monster->flags_1dc |= 0x80;
            if (MonsterIsCycleSupported(monster, 2) != 0) {
                signed char cycle = monster->m_pRep->pending_cycle;

                if (cycle == 2 || monster->IsCycleInterruptable(cycle) == 0 ||
                    (gXStatus.fCombatMode != 0 && g_combat_state->eCombatActionStatus != 0 &&
                     g_combat_state->pActionMonsterInfo == monster_info)) {
                    return;
                }
                W8MonsterGroup* group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0x987, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1));
                unsigned int chance = group->member_count * 20;

                if (gXStatus.fNpcDialogueMode == 0 && Random(chance) == 0) {
                    StartMonsterCycle(monster_info, 2, 1);
                }
            }
        }
    }
}

/* Return the monster experience value exposed by Monster Info. Retail prefers
   the optional +0x26b override and otherwise uses the +0x181 base. The same
   effective value is also used as an HP-weighted combat-difficulty proxy. */
// FUNCTION: WIZ8 0x004e6780
unsigned int GetMonsterExperience(const W8MonsterRecord* record)
{
    unsigned int value = record->experience_override_26b;

    if (value <= 0) {
        value = record->experience_181;
    }
    return value;
}

// FUNCTION: WIZ8 0x004e68c0
bool AnyMonsterDying(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info != 0 && monster_info->p3D->IsDying() != 0) {
            return true;
        }
    }
    return false;
}

/* Reset the two generated runtime IDs and the live manager counts, then create
   or clear the four lists owned by gXStatus. The final two success checks are
   deliberately asymmetric: retail rechecks plsMonsterList after initializing
   plsUnbornMonsterList, and plsMonsterGroupList after the encounter list. */
// FUNCTION: WIZ8 0x004e3720
bool InitializeMonsterManagerState(void)
{
    g_status.next_monster_location_id_234e = 1;
    g_status.next_group_id_234a = 1;
    gXStatus.active_monster_count = 0;
    gXStatus.hostile_monster_count = 0;
    gXStatus.hostile_group_count = 0;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->selected_item = -1;
    }
    if (gXStatus.plsMonsterList == 0) {
        gXStatus.plsMonsterList = PLCreate();
    } else {
        PListClear(gXStatus.plsMonsterList);
    }
    if (gXStatus.plsMonsterList == 0) {
        return false;
    }
    if (gXStatus.plsMonsterGroupList == 0) {
        gXStatus.plsMonsterGroupList = PLCreate();
    } else {
        PListClear(gXStatus.plsMonsterGroupList);
    }
    if (gXStatus.plsMonsterGroupList == 0) {
        return false;
    }
    if (gXStatus.plsUnbornMonsterList == 0) {
        gXStatus.plsUnbornMonsterList = PLCreate();
    } else {
        PListClear(gXStatus.plsUnbornMonsterList);
    }
    if (gXStatus.plsMonsterList == 0) {
        return false;
    }
    if (gXStatus.plsMonsterGroupEncounterList != 0) {
        PListClear(gXStatus.plsMonsterGroupEncounterList);
        return gXStatus.plsMonsterGroupList != 0;
    }
    gXStatus.plsMonsterGroupEncounterList = PLCreate();
    return gXStatus.plsMonsterGroupList != 0;
}

/* The manager teardown: it drains the monster list by repeatedly destroying
   entry zero rather than walking it, then releases the four gXStatus lists and
   the species-indexed record cache. The cache walk is a pointer sweep against
   the address one past the last slot, which is how the original spells it. */
// FUNCTION: WIZ8 0x004e3820
unsigned char ShutdownMonsterManager(void)
{
    W8MonsterRecord** slot;

    if (gXStatus.plsMonsterGroupList == 0) {
        srAssertFail("gXStatus.plsMonsterGroupList != NULL", MONSTER_MANAGER_CPP, 0x5c, 0);
    }
    if (gXStatus.plsMonsterList == 0) {
        srAssertFail("gXStatus.plsMonsterList != NULL", MONSTER_MANAGER_CPP, 0x5d, 0);
    }
    while (static_cast<int>(PLLength(gXStatus.plsMonsterList)) > 0) {
        if (RemoveMonster(0, 1) == 0) {
            return 0;
        }
    }
    if (PLDestroy(gXStatus.plsMonsterList) == 0) {
        return 0;
    }
    gXStatus.plsMonsterList = 0;
    if (PLDestroy(gXStatus.plsMonsterGroupList) == 0) {
        return 0;
    }
    gXStatus.plsMonsterGroupList = 0;
    if (PLDestroy(gXStatus.plsUnbornMonsterList) == 0) {
        return 0;
    }
    gXStatus.plsUnbornMonsterList = 0;
    if (PLDestroy(gXStatus.plsMonsterGroupEncounterList) == 0) {
        return 0;
    }
    gXStatus.plsMonsterGroupEncounterList = 0;
    for (slot = gXStatus.monster_record_cache;
         slot < gXStatus.monster_record_cache + MAX_MONSTERS_IN_DATABASE; ++slot) {
        if (*slot != 0) {
            free(*slot);
            *slot = 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004e3af0
bool RemoveMonster(unsigned int monster_list_index, unsigned char destroy_monster)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);

    if (gXStatus.fCombatMode != 0 && monster_info->fInCombat != 0 && monster_info->pCombat != 0) {
        MonsterInfoLeaveCombat(monster_info);
    }
    if (monster_info->monster_group_id != 0) {
        W8MonsterGroup* monster_group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
            0x125, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1));

        IListRemove(monster_group->monsters, monster_info->location_id);
        --monster_group->member_count;
        RequestRedrawParty();
        if (monster_group->member_count == 0) {
            DestroyMonsterGroup(monster_group, monster_info);
        } else {
            RecountActiveMonsterGroupMembers(monster_group);
            if (monster_group->leader_location_id == monster_info->location_id) {
                ElectGroupLeaderMember(monster_group);
                if (monster_group->leader_group_id == 0) {
                    RefreshMonsterGroupAndAllies(monster_group);
                }
            }
        }
        monster_info->monster_group_id = 0;
    }
    DeactivateMonster(monster_info);
    if (destroy_monster != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        DeactivateMonster(monster_info);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
        }
        if (monster_info->p3D != 0) {
            DetachMonsterRepresentation(monster_info->p3D, GetWorld());
            RemoveMonsterFromWorldList(GetWorld(), monster_info->p3D);
            DeleteMonster(monster_info->p3D);
            monster_info->p3D = 0;
        }
        g_octree->UnregisterLocationObjects(static_cast<unsigned short>(monster_info->location_id));
        ReleaseNpcBinding(monster_info->bound_npc_index);
        void* removed = PLRemoveAt(gXStatus.plsMonsterList, monster_list_index);
        if (removed != 0) {
            free(removed);
        }
    }
    return 1;
}

/* Retires one entry from the live world: it parks the entry at the sentinel
   location 9999, zeroes its two current stats, resets the live Monster's flag
   word to the deactivated value, captures the position the Monster ends at, and
   lowers the live count. In combat it also clears the two selection slots the
   global at 0x006836A8 holds if this entry occupied them. The p3D name for the
   Monster pointer at +0x0c comes from the MonsterManager.cpp:585 assertion. */
// FUNCTION: WIZ8 0x004e4280
void DeactivateMonster(W8MonsterInfo* monster_info)
{
    srVector3T<float> position;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x240, 0);
    }
    if (monster_info->fActive != 0) {
        if (monster_info->p3D == 0) {
            srAssertFail("pMonsterInfo->p3D != NULL", MONSTER_MANAGER_CPP, 0x249, 0);
        }
        monster_info->uiCondition[W8_CONDITION_DEAD] = 9999;
        monster_info->highest_condition = 0x12;
        monster_info->hp_current = 0;
        monster_info->stamina = 0;
        monster_info->p3D->active_088 = 0;
        monster_info->p3D->flags_00c = 0x200000;
        ClearMonsterSpellIcons(monster_info->p3D);
        ReleaseMonToMonVisibilityList(monster_info);
        MonsterGetLocalLocation(monster_info->p3D, &position);
        monster_info->position_17 = position;
        monster_info->fActive = 0;
        --gXStatus.active_monster_count;
        if (gXStatus.fCombatMode != 0) {
            RefreshAllSight();
            SetTargetToMonster(monster_info->location_id, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            RefreshFlaggedMainGameState();
            RecountCombatMonsters();
            if (g_combat_state->pActionMonsterInfo == monster_info) {
                g_combat_state->eCombatActionStatus = 0;
                g_combat_state->pActionMonsterInfo = 0;
            }
        }
    }
}

/* Combat entry for one monster entry: it stops and re-poses the live Monster,
   allocates the 0x153-byte pCombat block the MonsterManager.cpp:672 assertion
   names, and clears it as 0x54 dwords plus a trailing word and byte - the
   inline `memset` shape VC6 emits for a zero-initialised structure of that
   size, which is also what fixes the block's extent. */
// FUNCTION: WIZ8 0x004e4390
void MonsterInfoEnterCombat(W8MonsterInfo* monster_info)
{
    int query_state;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x299, 0);
    }
    if (monster_info->fInCombat != 0) {
        srAssertFail("!pMonsterInfo->fInCombat", MONSTER_MANAGER_CPP, 0x29a, 0);
    }
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2f7, 0);
    }
    MonsterReplacePath(monster_info->p3D, 0);
    monster_info->p3D->flags_00c &= 0xdfffffff;
    MonsterForward4537E0(monster_info->p3D);
    if (monster_info->fMotionless == 0) {
        query_state = MonsterQuery(monster_info->p3D, 6);
        if (query_state != 1 && query_state != 2 &&
            monster_info->p3D->m_pRep->pending_cycle == -1) {
            StartMonsterCycle(monster_info, 1, 3);
        }
    }
    monster_info->pCombat = static_cast<W8MonsterCombatState*>(malloc(0x153));
    if (monster_info->pCombat == 0) {
        srAssertFail("pMonsterInfo->pCombat != NULL", MONSTER_MANAGER_CPP, 0x2a0, 0);
    }
    memset(monster_info->pCombat, 0, 0x153);
    monster_info->fInCombat = true;
    if (monster_info->player_visibility.sight_state_04 == W8_SIGHT_UNSEEN) {
        monster_info->player_visibility.sight_state_04 = W8_SIGHT_RECENT;
        monster_info->player_visibility.last_seen_clock_0c = g_status.world_clock;
    }
    ResetCombatSlot(&monster_info->Target);
    MonsterSetHighlightMask(monster_info->p3D, 0);
    monster_info->p3D->flags_00c = 0;
    if (monster_info->ubDisposition == 1) {
        RecountCombatMonsters();
    }
    if (gXStatus.fCombatMode != 0) {
        EndMonsterTurn(monster_info);
    }
}

/* Combat exit, the mirror of 0x004E4390: it detaches the entry from the two
   selection slots the global at 0x006836A8 holds, walks pCombat's two record
   runs releasing every occupied one, then frees the block and lowers fInCombat.
   Both runs step by 0x11 bytes; the first starts at +0x3e and the second at
   +0xd7, which is what places them inside the 0x153-byte allocation. */
// FUNCTION: WIZ8 0x004e4500
void MonsterInfoLeaveCombat(W8MonsterInfo* monster_info)
{
    unsigned int index;
    W8EffectSlot* entry;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", MONSTER_MANAGER_CPP, 0x2c6, 0);
    }
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2c7, 0);
    }
    if (monster_info->fInCombat == 0) {
        srAssertFail("pMonsterInfo->fInCombat", MONSTER_MANAGER_CPP, 0x2c8, 0);
    }
    if (g_combat_state->pActionMonsterInfo == monster_info) {
        g_combat_state->eCombatActionStatus = 0;
        g_combat_state->pActionMonsterInfo = 0;
    }
    /* The record cursor is spelled (base + offset) + constant, not
       (base + constant) + offset: the first form leaves the block pointer as
       the LEA's base register, which is the encoding the original uses, while
       the second folds the constant into the displacement and promotes the
       running offset to base instead. */
    for (index = 0; index < 9; ++index) {
        entry = &monster_info->pCombat->effect_slots_3e[index];
        if (entry->active != 0) {
            ClearEffectSlot(monster_info, entry);
        }
    }
    for (index = 0; index < 6; ++index) {
        entry = &monster_info->pCombat->effect_slots_d7[index];
        if (entry->active != 0) {
            ClearEffectSlot(monster_info, entry);
        }
    }
    DestroyMonsterActionQueue(monster_info);
    free(monster_info->pCombat);
    monster_info->pCombat = 0;
    monster_info->fInCombat = false;
    if (monster_info->ubDisposition == 1) {
        RecountCombatMonsters();
    }
}

/* Toggles the party's combat-ready posture. Outside combat it only flips the
   flag; inside it also swaps the two engine bits at +0x001 and +0xa62 of the
   state block, and in engine mode 7 it announces the new posture with the
   message the block at 0x0068C09C holds for that direction. */
// FUNCTION: WIZ8 0x004e6c10
void TogglePartyCombatStance(void)
{
    const wchar_t* message;

    if (g_status.game_started == 0) {
        g_settings.continuous_combat = (g_settings.continuous_combat == 0);
        return;
    }
    if (g_settings.continuous_combat != 0) {
        g_settings.continuous_combat = 0;
        if (gXStatus.fCombatMode != 0) {
            g_combat_state->round_active_001 = (g_combat_state->combat_over_000 == 0);
            g_combat_state->combat_ready_a62 = 1;
        }
        if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
            return;
        }
        message = gppStringList[W8_NOTICE_COMBAT_STANCE_RELAXED];
    } else {
        g_settings.continuous_combat = 1;
        if (gXStatus.fCombatMode != 0 && g_combat_state->combat_ready_a62 != 0) {
            g_combat_state->round_active_001 = 1;
            g_combat_state->combat_ready_a62 = 0;
        }
        if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
            return;
        }
        message = gppStringList[W8_NOTICE_COMBAT_STANCE_READY];
    }
    ShowNotice(0xc, message, -1, -1, 0);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(0x80000);
    }
}

/* The combat toggle the interface drives: out of combat it starts one, and in
   combat it refuses to end it while any loaded group still qualifies, while any
   character is still engaged, or while the state block's leading flag is
   raised, announcing the reason in each case. Only when all three clear does it
   end combat and, if the level record's field at +0x2ca is not -1, run the
   trailing notification. */
// FUNCTION: WIZ8 0x004e6a80
void ToggleCombatMode(void)
{
    unsigned int group_list_index;
    W8MonsterGroup* monster_group;
    W8Missile* missile;

    if (gXStatus.fCombatMode == 0) {
        StartCombat(1);
        return;
    }
    if (gXStatus.hostile_monster_count != 0 || g_combat_state->enemies_engaged_a54 != 0) {
        if (g_combat_state->round_count_004 != 0) {
            ShowNotice(0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END], -1, -1, 0);
            return;
        }
        for (group_list_index = 0; group_list_index < PLLength(gXStatus.plsMonsterGroupList);
             ++group_list_index) {
            monster_group = GetMonsterGroupByListIndex(group_list_index);
            if (monster_group->members_active != 0 && monster_group->fInCombat != 0 &&
                MonsterGroupCanEngage(monster_group) != 0) {
                ShowNotice(0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END], -1, -1, 0);
                return;
            }
        }
    }
    for (missile = NextMissile(1); missile != 0; missile = NextMissile(0)) {
        if ((missile == g_combat_state->engaged_missile ||
             g_missile_table[missile->missile_table_index_1d8].spell_missile_154 != 0) &&
            missile->BlocksEndingCombat() != 0) {
            ShowNotice(0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END_ENGAGED], -1, -1, 0);
            return;
        }
    }
    if (g_combat_state->combat_over_000 != 0) {
        ShowNotice(0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END_PENDING], -1, -1, 1);
        return;
    }
    EndCombat(0);
    ShowNotice(0xc, gppStringList[W8_NOTICE_COMBAT_ENDED], -1, -1, 0);
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
}

/* Starts one animation cycle on a monster, and refuses in three ways.
 
   A NEVER_STOP behaviour on a cycle that can be interrupted is rejected
   outright. So is any new cycle while an uninterruptable one is pending, except
   the stop cycle, which is allowed through silently - the diagnostic there names
   both cycles and the monster, which is what the cycle-name table at 0x0060EA08
   is for. And a motionless monster accepts only the stop cycle, death, and cycle
   zero; anything else fails the third assertion with no message at all.
 
   The combat-mode query on the way in is made for its effect: its result is
   discarded here. */
// FUNCTION: WIZ8 0x004e4db0
void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior)
{
    W8Monster* monster = monster_info->p3D;
    unsigned char pending;
    const char* detail;
    int line;

    if (static_cast<signed char>(behavior) == W8_BEHAVIOUR_NEVER_STOP &&
        monster->IsCycleInterruptable(static_cast<signed char>(cycle)) == 0) {
        detail = "Trying to set a NEVER_STOP behaviour with an uninterruptable cycle!";
        line = 0x46f;
    } else {
        pending = monster->m_pRep->pending_cycle;
        if (gXStatus.fCombatMode != 0) {
            MonsterQuery(monster, 6);
        }
        if (pending != W8_CYCLE_STOP && pending != W8_CYCLE_NONE &&
            monster->IsCycleInterruptable(static_cast<signed char>(pending)) == 0) {
            if (static_cast<signed char>(cycle) == W8_CYCLE_STOP) {
                return;
            }
            srAssertFail(
                "FALSE", MONSTER_MANAGER_CPP, 0x497,
                reinterpret_cast<const char*>(String(
                    "%ls starting new cycle (%s) with an uninterruptable cycle pending (%s)!",
                    GetMonsterName(monster_info, 0, 0),
                    g_cycle_names[static_cast<signed char>(cycle)].name,
                    g_cycle_names[static_cast<signed char>(pending)].name)));
            return;
        }
        if (static_cast<signed char>(cycle) == W8_CYCLE_STOP ||
            static_cast<signed char>(cycle) == W8_CYCLE_DEATH ||
            static_cast<signed char>(cycle) == 0 || monster_info->fMotionless == 0) {
            MonsterSetAnimating(monster, 1);
            MonsterSetRuntimeBehaviour(monster, static_cast<signed char>(behavior));
            MonsterSetPendingCycle(monster, cycle);
            monster->m_pRep->pending_subcycle_066 = 0;
            MonsterForward4A84A0(monster);
            return;
        }
        detail = 0;
        line = 0x4a4;
    }
    srAssertFail("FALSE", MONSTER_MANAGER_CPP, line, detail);
}

/* Advance every live monster once. Removal is done in place, so each branch
   that shortens the PList decrements the unsigned index before the common
   increment. A delayed-removal monster is first detached from combat and the
   world; an ordinary monster advances its cycle state or stops animating while
   motionless. */
// FUNCTION: WIZ8 0x004e4ee0
void ProcessMonsterManagerFrame(void)
{
    unsigned int monster_list_index;

    for (monster_list_index = 0; monster_list_index < PLLength(gXStatus.plsMonsterList);
         ++monster_list_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        W8Monster* monster = monster_info->p3D;

        if ((monster->flags_1dc & W8_MONSTER_REMOVE_AFTER_FADE) != 0) {
            if (monster->fade_state_330 == 0) {
                if (monster->removal_state_22e != 0) {
                    monster->ApplyRemovalStateEffects();
                }
                monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
                DeactivateMonster(monster_info);
                if (monster_info == 0) {
                    srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
                }
                if (monster_info->p3D != 0) {
                    DetachMonsterRepresentation(monster_info->p3D, GetWorld());
                    RemoveMonsterFromWorldList(GetWorld(), monster_info->p3D);
                    DeleteMonster(monster_info->p3D);
                    monster_info->p3D = 0;
                }
                g_octree->UnregisterLocationObjects(
                    static_cast<unsigned short>(monster_info->location_id));
                ReleaseNpcBinding(monster_info->bound_npc_index);
                void* removed = PLRemoveAt(gXStatus.plsMonsterList, monster_list_index);
                if (removed != 0) {
                    free(removed);
                }
                --monster_list_index;
            }
        } else if ((monster->flags_1dc & W8_MONSTER_REMOVE_NOW) != 0) {
            RemoveMonster(monster_list_index, 1);
            --monster_list_index;
        } else if ((monster->flags_1dc & W8_MONSTER_SCRIPT_WAIT) == 0) {
            if ((monster->flags_1dc & W8_MONSTER_PARKED) == 0) {
                int query_state = MonsterQuery(monster, 6);
                TryStartMonsterCycle2(monster_info, monster, query_state);
                if (MonsterQuery(monster, 7) != 0) {
                    if (gXStatus.fCombatMode != 0 && query_state == 0x12) {
                        monster_info->pCombat->special_ready_145 = 1;
                    }
                    switch (query_state) {
                    case 1:
                    case 3:
                    case 4:
                    case 0x17:
                    case 0x18:
                        break;
                    case 0x15:
                        if ((monster->flags_1dc & W8_MONSTER_REMOVE_AFTER_FADE) == 0) {
                            monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
                            HandleScriptedNpcDeath(monster_list_index);
                            if (monster_info->monster_group_id != 0) {
                                RemoveMonster(monster_list_index, 0);
                            }
                            DropMonsterLoot(monster_info, -1);
                            monster_info->p3D->BeginDelayedRemoval();
                        }
                        break;
                    default:
                        if (monster_info->fMotionless == 0) {
                            /* pending_cycle is signed char; the rest of this
                               unit and the representation compare the empty
                               slot to -1. W8_CYCLE_NONE is 0xff as int 255,
                               which a signed char never equals. */
                            if (monster_info->p3D->m_pRep->pending_cycle == -1) {
                                StartMonsterCycle(monster_info, 1, 3);
                            }
                        } else if (MonsterIsAnimating(monster) != 0) {
                            MonsterSetAnimating(monster, 0);
                        }
                        break;
                    }
                }
            }
        } else {
            RemoveMonster(monster_list_index, 1);
            --monster_list_index;
        }
    }
}

/* The display name for one monster. Three things decide it.
 
   Without a record the species' cached database row is fetched, which is the
   same body GetMonsterDataForInfo is, inlined here - both of its assertions
   appear in this function at their own source lines.
 
   One record id is special-cased entirely: it is shown as a party character's
   name with a prefix, formatted into a shared buffer.
 
   Otherwise the monster's group decides which of the record's two name sets is
   used, and name_form picks the variant within it - the sets are twenty-four
   wide characters apart. A monster with no group at all is a bug unless it is
   already dying, and says so on the debug channel rather than asserting. */
// FUNCTION: WIZ8 0x004e5150
wchar_t* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                        unsigned char name_form)
{
    W8MonsterGroup* monster_group;

    if (record == 0) {
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    }
    if (record->record_id_187 == W8_MONSTER_RECORD_ALTERNATE_NAME) {
        swprintf(g_status.monster_name_buffer_2453, L"Al-%s",
                 g_status.buffers.Char[g_status.sedexus_party_slot_247f].name);
        return g_status.monster_name_buffer_2453;
    }
    if (monster_info->monster_group_id == 0) {
        if (monster_info->p3D->IsDying() == 0) {
            FormatDebugMessage(1, "ERROR: Monster ID %d has no group", monster_info->location_id);
        }
    } else {
        monster_group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
            0x54c, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1));
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_MANAGER_CPP, 0x54d, 0);
        }
        if (monster_group->alternate_name != 0) {
            return record->name_00 + name_form * 24;
        }
    }
    return record->name_60 + name_form * 24;
}

// FUNCTION: WIZ8 0x004EFB60
float GetAveragePartyMemberLevel(void)
{
    float total = 0.0f;
    float count = 0.0f;
    for (int party_slot = 0; party_slot < 6; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied != 0) {
            total += g_status.buffers.Char[party_slot].uiExpLevel;
            count += 1.0f;
        }
    }
    if (count == 0.0f)
        return 1.0f;
    return total / count;
}

// FUNCTION: WIZ8 0x00554490
unsigned int GetBestPartySkillLevel(int skill_index, int* party_slot)
{
    unsigned int best_level = 0;
    int best_slot = -1;
    for (int index = 0; index < 8; ++index) {
        W8Character* character = &g_status.buffers.Char[index];
        if (g_status.buffers.XChar[index].fOccupied != 0 && character->hp_current != 0 &&
            character->highest_condition < 0xd &&
            (character->skills[skill_index].level > best_level || best_slot == -1)) {
            best_level = character->skills[skill_index].level;
            best_slot = index;
        }
    }
    if (party_slot != 0)
        *party_slot = best_slot;
    return best_level;
}

/* Format the health knowledge the party has earned for one monster. NPC-backed
   records can suppress exact values, and ordinary monsters expose current and
   maximum HP independently at knowledge thresholds ten and five. */
// FUNCTION: WIZ8 0x004e52c0
void FormatMonsterHealth(W8MonsterInfo* monster_info, wchar_t* health_text)
{
    unsigned char suppress_exact_health = 0;
    unsigned int health_knowledge;

    if (monster_info->ubDisposition != 1) {
        W8MonsterRecord* record;
        W8NpcState* npc;

        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        if ((record->flags_0d0 & 1) != 0) {
            npc = GetNpcStateByKind(record->npc_kind_0cd);
            if (npc != 0 && npc->record->has_group != 0) {
                suppress_exact_health = 1;
            }
        }
    }

    if (monster_info->summoned_2da == 1) {
        health_knowledge = 125;
    } else {
        float average_party_level = GetAveragePartyMemberLevel();
        W8MonsterRecord* record;
        int best_party_slot;
        int monster_level;

        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        monster_level = record->display_level_251;
        health_knowledge = GetBestPartySkillLevel(0x15, &best_party_slot);
        if (static_cast<int>(average_party_level) < monster_level) {
            float adjusted_knowledge = health_knowledge -
                                       (monster_level - average_party_level) * g_float_005ec52c +
                                       g_float_005ebc7c;
            if (adjusted_knowledge < g_float_005ebb34) {
                adjusted_knowledge = g_float_005ebb34;
            }
            health_knowledge = static_cast<unsigned int>(adjusted_knowledge);
        }
    }

    if (g_dev_mode != 0) {
        wcscpy(health_text,
               FormatWideString(L"%d/%d", monster_info->hp_current, monster_info->uiHPMax));
        return;
    }
    if (health_knowledge < 10 || suppress_exact_health != 0) {
        wcscpy(health_text, L"");
    } else {
        wcscpy(health_text, FormatWideString(L"%d", monster_info->hp_current));
    }
    wcscat(health_text, L"/");
    if (health_knowledge > 4 && suppress_exact_health == 0) {
        wcscat(health_text, FormatWideString(L"%d", monster_info->uiHPMax));
        return;
    }
    wcscat(health_text, L"");
}

// FUNCTION: WIZ8 0x004e6970
W8XStatus::W8XStatus() {}

// FUNCTION: WIZ8 0x004e6940
W8XStatus::~W8XStatus() {}

// FUNCTION: WIZ8 0x004e6a10
W8MonsterManagerEntry::~W8MonsterManagerEntry() {}

// FUNCTION: WIZ8 0x004e6a30
W8MonsterManagerEntry::W8MonsterManagerEntry() {}

// FUNCTION: WIZ8 0x004e4ab0
void DetectMonsterGroups(void)
{
    bool noticed = false;

    for (unsigned int group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList);
         ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
        if (group->members_active == 0 ||
            (group->alternate_name != 0 && group->group_state[0] != 0) ||
            (gXStatus.fCombatMode != 0 && group->fInCombat == 0) ||
            MonsterGroupHasRenderableMember(group, 0) == 0) {
            continue;
        }
        if (group->alternate_name == 0) {
            W8MonsterRecord* record = MonsterGroupGetRecord(group);
            int best_slot = -1;
            unsigned int best_margin = 0;
            for (unsigned int slot = 0; slot < 8; ++slot) {
                W8Character* character = &g_status.buffers.Char[slot];
                if (g_status.buffers.XChar[slot].fOccupied == 0 || character->hp_current == 0 ||
                    character->highest_condition >= 0xb ||
                    character->uiCondition[W8_CONDITION_BLIND] != 0) {
                    continue;
                }
                int score = (character->monster_awareness_12b6[group->monster_id] -
                             record->effective_level_24f) *
                                3 +
                            character->attributes[W8_ATTRIBUTE_INTELLIGENCE].effective / 10 +
                            character->skills[W8_SKILL_MYTHOLOGY].level;
                int roll = Random(100);
                if (roll >= score) {
                    continue;
                }
                unsigned int margin = score - roll;
                if (character->skills[W8_SKILL_MYTHOLOGY].active_00 != 0) {
                    PracticeCharacterSkill(character, W8_SKILL_MYTHOLOGY, 10, 0);
                }
                if (margin > best_margin) {
                    best_slot = slot;
                    best_margin = margin;
                }
            }
            if (best_slot != -1) {
                PostCharacterNotice(best_slot, gppStringList[0x1ca], GetMonsterGroupName(group));
                group->alternate_name = 1;
                bool vowel;
                switch (towupper(*GetMonsterGroupName(group))) {
                case L'A':
                case L'E':
                case L'I':
                case L'O':
                case L'U':
                    vowel = true;
                    break;
                default:
                    vowel = false;
                    break;
                }
                wchar_t* article;
                if (group->member_count == 1) {
                    article = vowel ? gppStringList[0x1cc] : gppStringList[0x1cb];
                } else {
                    article = gppStringList[0x1cd];
                }
                ShowNoticef(8, L"%s %s!", article, GetMonsterGroupName(group));
                noticed = true;
            }
        }
        if (group->group_state[0] == 0) {
            for (unsigned int slot = 0; slot < 8; ++slot) {
                W8Character* character = &g_status.buffers.Char[slot];
                if (g_status.buffers.XChar[slot].fOccupied == 0 || character->hp_current == 0 ||
                    character->highest_condition >= 0xb ||
                    character->uiCondition[W8_CONDITION_BLIND] != 0) {
                    continue;
                }
                unsigned int awareness =
                    character->monster_awareness_12b6[group->monster_id] + group->member_count;
                if (awareness > 0xff) {
                    awareness = 0xff;
                }
                character->monster_awareness_12b6[group->monster_id] =
                    static_cast<unsigned char>(awareness);
            }
            group->group_state[0] = 1;
        }
    }
    if (noticed) {
        RequestRedrawParty();
    }
}

// FUNCTION: WIZ8 0x004E6CE0
void EvaluateCombatDifficulty(void)
{
    unsigned int hostile_experience = 0;
    unsigned int eligible_count = 0;
    unsigned int party_levels = 0;
    unsigned int monster_index;
    int slot;

    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList); ++monster_index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster->fInCombat && monster->ubDisposition == DISP_HOSTILE &&
            monster->hp_current > 0 && monster->highest_condition < 0x12) {
            unsigned int health_percent = monster->hp_current * 100 / monster->uiHPMax;
            hostile_experience +=
                GetMonsterExperience(GetMonsterDataForInfo(monster)) * health_percent / 100;
        }
    }
    unsigned int threat_level = EstimateCombatThreatLevel(hostile_experience);
    if (threat_level == 0) {
        threat_level = 1;
    }

    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (!row->fOccupied || character->hp_current == 0 || character->highest_condition >= 0x12) {
            continue;
        }
        bool count_character = true;
        if (slot < 2) {
            W8NpcState* npc = GetNpcState(row->npc_index);
            if (npc != 0) {
                int faction = npc->record->faction_5f;
                if (faction != 0 && faction != 1) {
                    for (unsigned int group_index = 0;
                         group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
                        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
                        if (group->members_active && group->fInCombat &&
                            group->ubDisposition == DISP_HOSTILE &&
                            MonsterGroupGetRecord(group)->faction_id_25f == faction) {
                            count_character = false;
                            break;
                        }
                    }
                }
            }
        }
        if (count_character) {
            ++eligible_count;
            party_levels += character->uiExpLevel;
        }
    }

    unsigned int party_power = 0;
    if (eligible_count > 0) {
        party_power = static_cast<unsigned int>(
            party_levels /
                pow(static_cast<double>(eligible_count) * g_float_005ebca0, g_double_005ebe80) +
            g_double_005ebe80);
    }
    int relative_strength = static_cast<int>(party_power * 100 / threat_level) - 100;
    unsigned char difficulty;
    if (relative_strength >= 20) {
        difficulty = 0;
    } else if (relative_strength <= -20) {
        difficulty = 2;
    } else {
        difficulty = 1;
    }

    switch (difficulty) {
    case 0:
        StartMusicResource("CombatEasy.MPL", 0, 1);
        break;
    case 1:
        StartMusicResource("Combat.MPL", 0, 1);
        break;
    case 2:
        StartMusicResource("CombatLousy.MPL", 0, 1);
        break;
    }
    if ((g_combat_state != 0 && g_combat_state->party_surprised_a52) ||
        ClockIsTicking(gXStatus.combat_countdown) == 0 ||
        difficulty != gXStatus.combat_difficulty) {
        unsigned int event_type;
        switch (difficulty) {
        case 0:
            event_type = g_effect_005ee5fc;
            break;
        case 1:
            event_type = g_effect_005ee600;
            break;
        default:
            event_type = g_effect_005ee604;
            break;
        }
        ApplyItemEffectToRandomCharacter(event_type, -1, 0, g_effect_argument_005ed8c8);
    }
    ++g_status.combat_difficulty_counts[difficulty];
    gXStatus.combat_difficulty = difficulty;

    for (slot = 0; slot < 2; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        if (row->fOccupied) {
            W8NpcState* npc = GetNpcState(row->npc_index);
            if (npc != 0) {
                W8Character* character = &g_status.buffers.Char[slot];
                if (character->highest_condition == 0x12) {
                    npc->item_assist_f1 = 0;
                } else if (npc->name_style == W8_NPC_VI_DOMINA ||
                           npc->name_style == W8_NPC_DRAZIC || npc->name_style == W8_NPC_RODAN) {
                    npc->healer_assist_f0 = 1;
                } else {
                    npc->item_assist_f1 = 1;
                }
            }
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status.buffers.XChar[slot];
        W8Character* character = &g_status.buffers.Char[slot];
        if (row->fOccupied && character->uiCondition[0x12] == 0) {
            gXStatus.monster_manager_entries[slot].condition_19_latch =
                character->uiCondition[0x13] != 0;
        }
    }
}
