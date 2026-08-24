#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/combat_state.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/float_constants.h"
#include "wiz8/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/utility.h"
#include "wiz8/screen_state.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/npc_item_lists.h"
#include "wiz8/sr_api.h"
#include "DEBUG.H"
#include "random.h"
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define MONSTER_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp"
#define MAX_MONSTERS_IN_DATABASE 1000

unsigned char GetRenderOptionState(int index);
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating);
unsigned char MonsterIsAnimating(W8Monster* monster);
void MonsterSetPendingCycle(W8Monster* monster, int cycle);
void Function4C5B10(W8Monster* monster, int value);
int MonsterQuery(W8Monster* monster, int query);
void MonsterForward4537E0(W8Monster* monster);
void MonsterSetBehaviour(W8Monster* monster, int behavior);
void MonsterSetSubCycle(W8Monster* monster, int subcycle);
unsigned char RemoveMonster(
    unsigned int monster_list_index,
    unsigned char destroy_monster);
void MonsterInfoEnterCombat(W8MonsterInfo* monster_info);
void MonsterInfoLeaveCombat(W8MonsterInfo* monster_info);
void RequestRedrawParty(void);
void DestroyMonsterGroup(W8MonsterGroup* monster_group, W8MonsterInfo* monster_info);
void Function5103E0(W8MonsterGroup* monster_group);
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group);
void ClearEffectSlot(W8MonsterInfo* monster_info, W8MonsterCombatEntry* entry);
void DestroyMonsterActionQueue(W8MonsterInfo* monster_info);
void Function546E70(void);
void ResetCombatSlot(W8CombatSlot* combat_slot);   /* 0x00536170 */
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag);
void EndMonsterTurn(W8MonsterInfo* monster_info);
extern int g_dword_6850be;
void DeactivateMonster(W8MonsterInfo* monster_info);
void Function4ACF90(W8Monster* monster);
void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
/* Writes the monster's world position through an out-parameter; __cdecl, since
   0x004C5750 ends in a bare `ret`. */
void Function4C5750(W8Monster* monster, srVector3T<float>* position);
void RefreshAllSight(void);
void SetTargetToMonster(int location_id, int value);
void Function593330(void);
void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour);
void MonsterForward4A84A0(W8Monster* monster);
float Function4BE5C0(srVector3T<float>* position);
int Function52A780(int first, int second);

/* The two cycles that always start regardless of the pending one: 0x14 is the
   cycle a motionless monster is still allowed to enter, and 0x15 is death. */
enum { W8_CYCLE_NONE = 0xff, W8_CYCLE_STOP = 0x14, W8_CYCLE_DEATH = 0x15 };
enum { W8_BEHAVIOUR_NEVER_STOP = 3 };
void MonsterDies(W8MonsterInfo* monster_info, int display_message);
void __fastcall Function452C90(W8Navigator* navigator);
void __fastcall Function4537E0(W8Navigator* navigator);
W8WideChar* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                           unsigned char name_form);
/* The character array the alternate-name form indexes, and the slot it uses. */

// FUNCTION: WIZ8 0x004e3930
W8MonsterInfo* CreateMonsterInfo(
    W8MonsterGroup* group,
    W8MonsterRecord* record,
    srVector3T<float>* position)
{
    W8MonsterInfo* monster_info =
        static_cast<W8MonsterInfo*>(malloc(sizeof(W8MonsterInfo)));
    int value;

    if (monster_info == 0) {
        return 0;
    }

    memset(monster_info, 0, sizeof(W8MonsterInfo));
    monster_info->location_id = g_status_685170.next_monster_location_id_234e;
    while (g_status_685170.next_monster_location_id_234e++,
           monster_info->location_id == 0) {
        monster_info->location_id = g_status_685170.next_monster_location_id_234e;
    }

    monster_info->monster_group_id = group->group_id;
    monster_info->monster_species = group->monster_id;
    monster_info->flag_16 = group->flag_2a;
    monster_info->scale_24f = -1.0f;
    monster_info->flag_14 = 0;
    monster_info->monster = 0;
    monster_info->fInCombat = 0;
    monster_info->pCombat = 0;
    monster_info->position_17 = *position;
    monster_info->derived_23 = Function4BE5C0(position);

    value = RollDice(&record->hit_points_d6);
    monster_info->hp_max = value;
    monster_info->hp_current = value;
    value = RollDice(&record->runtime_stat_da);
    monster_info->runtime_stat_max_2f = value;
    monster_info->runtime_stat_current_33 = value;
    monster_info->runtime_value_242 = Function52A780(value, value);

    memset(monster_info->condition_turns, 0, sizeof(monster_info->condition_turns));
    memset(monster_info->enchantments, 0, sizeof(monster_info->enchantments));
    monster_info->value_107 = 0;
    monster_info->condition_argument = 0;
    monster_info->effect_2de = 0;
    memset(&monster_info->runtime_block_1db, 0,
           sizeof(monster_info->runtime_block_1db));
    monster_info->motionless = 0;
    monster_info->flag_255 = 0;
    monster_info->value_2da = 0;
    monster_info->value_344 = -1;
    memset(monster_info->runtime_values_338, 0,
           sizeof(monster_info->runtime_values_338));

    if (PLAdoptAppend(
            record->flag_26a != 0 ? g_unborn_monster_list : g_monster_list,
            monster_info) == -1) {
        free(monster_info);
        return 0;
    }

    IListAdd(group->monsters, monster_info->location_id);
    ++group->member_count;
    ++group->active_member_count;
    RequestRedrawParty();
    g_octree_6598a4->VisitPointCopy0042E620(
        static_cast<unsigned short>(monster_info->location_id), position);
    return monster_info;
}

/* The one record id that is displayed as a character's name with a prefix
   rather than out of the monster database. */
void Function5248D0(W8MonsterInfo* monster_info);
void Function58AB60(int value_1, int value_2, void* notice, W8WideChar* name);
void Function4C59C0(W8Monster* monster, W8World* world);
W8World* GetWorld(W8Monster* monster);
void Function46E5A0(W8World* world);
void DeleteMonster004C5860(W8Monster* monster);
/* __stdcall, not __cdecl: 0x0042E650 ends in `ret 0x4`, and both callers here
   clean only three of the four dwords they push across the tail. */
void __stdcall Function42E650(unsigned short location_id);
void Function509EA0(int value);
void Function508D70(unsigned int monster_list_index);
void Function4F8CB0(W8MonsterInfo* monster_info, int value);
void Function4C5ED0(W8Monster* monster);
void __cdecl Function58AC00(int channel, void* message, int first, int second,
                            int flag);
void RequestRedraw(int mask);
unsigned char Function531920(W8MonsterGroup* monster_group);
W8CombatActor* NextEngagedCharacter(int restart);
unsigned char Function4A5790(void);
void StartCombat(int surprise);
void EndCombat(unsigned char reason);
void Function595570(void);
int Function428E20(void);
void Function50E8C0(int location_id);
void Function51B420(W8MonsterInfo* monster_info, W8MonsterRecord* record);
void Function509CD0(unsigned char value, int enabled, int location_id);
void WorldAddToList00(W8World* world, void* entry);
void MonsterPropagateValue004C5870(W8Monster* monster, int value);
void MonsterForward4A7BE0(
    W8Monster* monster,
    const srVector3T<float>* position);
void MonsterCallSlot10(void* object, int argument);
void RequestRefreshPartyState(void);
unsigned char GetFlag68F105(void);
extern int g_monster_cycle_registry_weight_0065ba4c;
extern float g_float_005ec52c;
extern unsigned char g_flag_689b32;
extern unsigned char g_flag_68517c;
extern unsigned char g_flag_6850d2;

static __inline W8MonsterRecord* MonsterDBFromSpeciesInline(
    unsigned int monster_species);

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
    if (monster_info->flag_14 != 0) {
        return;
    }

    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    if (monster_info->monster == 0 ||
        monster_info->monster->GetFlag216004CA290() != 0) {
        registry_before = Function428E20();
        ActivateMonster(monster_info, 0);
        MonsterPropagateValue004C5870(
            monster_info->monster, monster_info->location_id);
        MonsterSetAdjustedPosition004C5F00(
            monster_info->monster, &monster_info->position_17);

        if (PListIndexOf(gXStatus.plsUnbornMonsterList, monster_info) != -1) {
            if (MonsterIsCycleSupported(monster_info->monster, 0) == 0) {
                srAssertFail(
                    "MonsterIsCycleSupported(pMonsterInfo->p3D, CYCLE_BIRTH)",
                    MONSTER_MANAGER_CPP,
                    0x190,
                    "Unborn monsters must have CYCLE_BIRTH!");
            }
            MonsterSetBehaviour(monster_info->monster, 1);
            MonsterSetCycle(monster_info->monster, 0);
            if (MonsterQuery(monster_info->monster, 0) == -1) {
                srAssertFail(
                    "MonsterQuery(pMonsterInfo->p3D, QUERY_NUM_FRAMES) != -1",
                    MONSTER_MANAGER_CPP,
                    0x196,
                    0);
            }
            MonsterSetSubCycle(monster_info->monster, 0);
            MonsterSetAnimating(monster_info->monster, 0);
            monster_info->monster->state_088 = 0;
            monster_info->monster->flag_215 = 1;
        }
        else {
            MonsterSetCycle(monster_info->monster, 1);
            MonsterSetBehaviour(monster_info->monster, 3);
            if (MonsterQuery(monster_info->monster, 0) == -1) {
                srAssertFail(
                    "MonsterQuery(pMonsterInfo->p3D, QUERY_NUM_FRAMES) != -1",
                    MONSTER_MANAGER_CPP,
                    0x1a4,
                    0);
            }
            MonsterSetSubCycle(
                monster_info->monster,
                Random(MonsterQuery(monster_info->monster, 0)));
            MonsterSetAnimating(
                monster_info->monster, monster_info->motionless == 0);
        }

        WorldAddToList00(GetWorld(), monster_info->monster);
        MonsterSetFacing004C5B60(
            monster_info->monster, monster_info->derived_23);
        Function50E8C0(monster_info->location_id);
        monster_info->monster->movement_0c0.value_008 =
            static_cast<unsigned int>(record->missile_value_24f) * 0x10000U +
            monster_info->location_id;
        MonsterPropagateValue004C5870(
            monster_info->monster, monster_info->location_id);
        monster_info->monster->flag_216 = 0;

        registry_after = Function428E20();
        monster_info->monster->registry_weight_27c =
            registry_after - registry_before;
        g_monster_cycle_registry_weight_0065ba4c +=
            registry_after - registry_before;
        if (GetFlag68F105() != 0) {
            WriteGameLog(
                7,
                L"%dK\n",
                static_cast<unsigned int>(registry_after - registry_before) >> 10);
        }
        Function51B420(
            monster_info,
            MonsterDBFromSpeciesInline(monster_info->monster_species));
    }

    WorldGetCameraLocation(GetWorld(), &camera_position);
    MonsterForward4A7BE0(monster_info->monster, &camera_position);
    MonsterCallSlot10(
        monster_info->monster, reinterpret_cast<int>(GetWorld()));
    g_octree_6598a4->VisitPointCopy0042E620(
        static_cast<unsigned short>(monster_info->location_id),
        &monster_info->position_17);
    monster_info->flag_14 = 1;
    ++gXStatus.active_monster_count;
    if (monster_info->monster != 0) {
        int damage_stage_count =
            monster_info->monster->GetDamageStageCount004C6A50();
        if (damage_stage_count > 1) {
            int damage_stage =
                ((monster_info->hp_max - monster_info->hp_current) *
                 damage_stage_count) /
                monster_info->hp_max;
            if (damage_stage >= damage_stage_count - 1) {
                damage_stage = damage_stage_count - 1;
            }
            monster_info->monster->SetDamageStage004C6990(damage_stage);
        }
    }
    monster_info->mon_to_mon_visibility = PLCreate();
    if (monster_info->mon_to_mon_visibility == 0) {
        srAssertFail(
            "pMonsterInfo->plsVisMonToMon != NULL",
            MONSTER_MANAGER_CPP,
            0x1de,
            0);
    }
    RequestRefreshPartyState();
    Function593330();
    if (record->unknown_0c0[0] != 0) {
        monster_info->monster->movement_0c0.unknown_000 |= 0x10000000;
    }
    Function509CD0(
        record->unknown_0cd[0], 1, monster_info->location_id);
}

/* Activate the representation lazily. The mode selects whether all available
   cycles are loaded or only the startup cycle; both paths share the world
   context and preserve the loader's success result for the source assertion. */
// FUNCTION: WIZ8 0x004e4050
void ActivateMonster(W8MonsterInfo* monster_info, int mode)
{
    W8MonsterRecord* record;
    W8GrCycleLoadContext context;
    unsigned char success;

    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x1f5, 0);
    }
    if (monster_info->flag_14 != 0) {
        srAssertFail(
            "!pMonsterInfo->fActive", MONSTER_MANAGER_CPP, 0x1f6, 0);
    }
    if (monster_info->monster != 0) {
        return;
    }

    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    context.world_00 = GetWorld();
    context.value_04 = 0;

    if (mode == 0) {
        success = MonsterReadAllCycles004C58E0(
            &context,
            record->cycle_name_189,
            &monster_info->monster,
            1,
            monster_info->location_id);
        if (success == 0) {
            srAssertFail(
                "fSuccess",
                MONSTER_MANAGER_CPP,
                0x20a,
                "ActivateMonster: ERROR - MonsterReadAllCycles failed");
        }
    }
    else if (mode == 1) {
        success = MonsterReadAllCycles004C58E0(
            &context,
            record->cycle_name_189,
            &monster_info->monster,
            0,
            monster_info->location_id);
        if (success == 0) {
            srAssertFail(
                "fSuccess",
                MONSTER_MANAGER_CPP,
                0x20f,
                "ActivateMonster: ERROR - MonsterReadAllCycles failed");
        }
        MonsterSetCycle(monster_info->monster, 1);
    }

    if (monster_info->scale_24f < g_float_005ebb34 ||
        g_status_685170.level_progress[g_status_685170.current_level].visited == 0) {
        monster_info->unknown_301[0] =
            MonsterGetCycle17State(monster_info->monster);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
    }
    else {
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
        MonsterSetCycle17State(
            monster_info->monster, monster_info->unknown_301[0]);
    }

    Function4C5810(monster_info->monster);
    MonsterSetCycle(monster_info->monster, 1);
    Function4C5ED0(monster_info->monster);
}

// FUNCTION: WIZ8 0x004e4600
void Function4E4600(W8MonsterInfo* monster_info)
{
    int result;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2f7, 0);
    }
    Function4C5B10(monster_info->monster, 0);
    monster_info->monster->flags_00c &= 0xdfffffff;
    MonsterForward4537E0(monster_info->monster);
    if (monster_info->motionless == 0) {
        result = MonsterQuery(monster_info->monster, 6);
        if (result != 1 && result != 2 &&
            monster_info->monster->m_pRep->pending_cycle == -1) {
            StartMonsterCycle(monster_info, 1, 3);
        }
    }
}

// FUNCTION: WIZ8 0x004e4690
void MonsterStartsDying(W8MonsterInfo* monster_info, int display_message)
{
    if (monster_info->monster->IsDying() == 0) {
        StartMonsterCycle(monster_info, 0x15, 1);
        DeactivateMonster(monster_info);
        MonsterDies(monster_info, display_message);
        RemoveMonster(
            MonsterGetIndexByLocationID(
                0x31f,
                MONSTER_MANAGER_CPP,
                monster_info->location_id,
                1),
            0);
    }
}

// FUNCTION: WIZ8 0x004e5550
unsigned int MonsterGetIndexByLocationID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure)
{
    unsigned int index;
    W8MonsterInfo* monster;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->location_id == location_id) {
            return index;
        }
    }

    for (index = 0; index < PLLength(g_unborn_monster_list); ++index) {
        monster = (W8MonsterInfo*)PLGet(g_unborn_monster_list, index);
        if (monster->location_id == location_id) {
            return index + 10000;
        }
    }

    if (assert_on_failure != 0) {
        srAssertFail(
            "FALSE",
            MONSTER_MANAGER_CPP,
            0x5c1,
            reinterpret_cast<const char*>(String(
                "MonsterIndex: ID %d not found (%s line %d)",
                location_id,
                caller_file,
                caller_line)));
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
            srAssertFail(
                "uiMonsterListIndex < (UINT32) PLLength(gXStatus.plsMonsterList)",
                MONSTER_MANAGER_CPP,
                0x5da,
                0);
        }
        result = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, monster_list_index);
        if (result != 0) {
            return result;
        }
        detail = reinterpret_cast<const char*>(String(
            "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
            monster_list_index,
            gXStatus.plsMonsterList));
        line = 0x5de;
    }
    else {
        if (monster_list_index - 10000 >= PLLength(gXStatus.plsUnbornMonsterList)) {
            srAssertFail(
                "(uiMonsterListIndex-10000) < (UINT32) PLLength(gXStatus.plsUnbornMonsterList)",
                MONSTER_MANAGER_CPP,
                0x5d1,
                0);
        }
        result = (W8MonsterInfo*)PLGet(
            gXStatus.plsUnbornMonsterList, monster_list_index - 10000);
        if (result != 0) {
            return result;
        }
        detail = reinterpret_cast<const char*>(String(
            "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
            monster_list_index,
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
        srAssertFail(
            "uiMonsterSpecies < MAX_MONSTERS_IN_DATABASE",
            MONSTER_MANAGER_CPP,
            0x5f3,
            0);
    }
    record = g_monster_record_cache[monster_species];
    if (record == 0) {
        record = (W8MonsterRecord*)malloc(sizeof(W8MonsterRecord));
        if (record == 0) {
            return 0;
        }
        if (!LoadMonsterDatabaseRecord(monster_species, record)) {
            free(record);
            return 0;
        }
        g_monster_record_cache[monster_species] = record;
    }
    return record;
}

// FUNCTION: WIZ8 0x004e5720
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    return MonsterDBFromSpeciesInline(monster_info->monster_species);
}

// FUNCTION: WIZ8 0x004e57c0
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species)
{
    return MonsterDBFromSpeciesInline(monster_species);
}

static __inline W8MonsterInfo* MonsterInfoFromIDInline(
    int caller_line,
    const char* caller_file,
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
            "FALSE",
            MONSTER_MANAGER_CPP,
            0x626,
            reinterpret_cast<const char*>(String(
                "MonsterInfoFromID: ID %d not found (%s line %d)",
                location_id,
                caller_file,
                caller_line)));
    }
    return monster;
}

// FUNCTION: WIZ8 0x004e5840
W8MonsterInfo* MonsterInfoFromID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure)
{
    return MonsterInfoFromIDInline(
        caller_line,
        caller_file,
        location_id,
        assert_on_failure);
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
                return monster_info->monster;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004e5990
float GetMonsterRecordScaledFloat1BA(W8MonsterInfo* monster_info)
{
    W8MonsterRecord* record;
    float result;

    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    if (record == 0) {
        srAssertFail(
            "pMonsterDB",
            MONSTER_MANAGER_CPP,
            0x66a,
            0);
    }
    result = record->float_1ba * g_monster_record_float_scale;
    return result;
}

// FUNCTION: WIZ8 0x004e5a50
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info)
{
    W8Monster* monster = monster_info->monster;

    if (monster != 0) {
        int count = monster->GetDamageStageCount004C6A50();
        if (count > 1) {
            int value = ((monster_info->hp_max - monster_info->hp_current) * count) /
                        monster_info->hp_max;
            if (value >= count - 1) {
                value = count - 1;
            }
            monster->SetDamageStage004C6990(value);
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
    if (g_monster_info_iterator_index < (int)PLLength(g_monster_list)) {
        index = g_monster_info_iterator_index++;
        result = (W8MonsterInfo*)PLGet(g_monster_list, index);
    }
    return result;
}

// FUNCTION: WIZ8 0x004e5af0
int GetMonsterQuadrant(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x6b9, 0);
    }
    return GetQuadrantForPosition(monster_info->monster->GetPosition());
}

// FUNCTION: WIZ8 0x004e5b50
int Function4E5B50(unsigned int monster_species)
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
        return record->value_257;
    }
    return record->value_253;
}

// FUNCTION: WIZ8 0x004e5c00
void ProcessMonstersAtCombatEnd(unsigned char forced_cleanup)
{
    unsigned int index;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->flag_14 != 0 &&
            static_cast<unsigned int>(monster_info->hp_current) > 0 &&
            monster_info->condition_turns[W8_CONDITION_EQUIPMENT_UNLOCKED] == 0 &&
            monster_info->value_2da != 0) {
            if (forced_cleanup == 0) {
                Function58AB60(
                    9,
                    0,
                    gppStringList[W8_NOTICE_MONSTER_SLAIN],
                    GetMonsterName(monster_info, 0, 0));
            }
            Function5248D0(monster_info);
            if (forced_cleanup == 0) {
                monster_info->flag_253 = 1;
                if (monster_info->monster->IsDying() == 0) {
                    StartMonsterCycle(monster_info, 0x15, 1);
                    DeactivateMonster(monster_info);
                    MonsterDies(monster_info, 1);
                    RemoveMonster(
                        MonsterGetIndexByLocationID(
                            0x31f,
                            MONSTER_MANAGER_CPP,
                            monster_info->location_id,
                            1),
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
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = record->attribute_values_d1[monster_attribute];
        attribute_index = 0;

        if (monster_attribute >= 5) {
            srAssertFail(
                "uiMonsterAttribute < MONSTER_ATTR_COUNT",
                MONSTER_MANAGER_CPP,
                0x78d,
                0);
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
            srAssertFail(
                "FALSE",
                MONSTER_MANAGER_CPP,
                0x798,
                "ConvertMonsterAttribute: ERROR - Invalid monster attribute");
        }

        value += monster_info->runtime_block_1db.attribute_adjustments[attribute_index];
        if (value > 125) {
            value = 125;
        }
        else if (value < 1) {
            value = 1;
        }
        monster_info->converted_attributes_247[monster_attribute] =
            static_cast<unsigned char>(value);
        ++monster_attribute;
    } while (monster_attribute < 5);
}

// FUNCTION: WIZ8 0x004e5e50
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
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

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (static_cast<unsigned int>(monster_info->hp_current) > 0) {
            Function452C90(monster_info->monster);
            if (monster_info->flag_255 > 0 && monster_info->flag_255 <= 3) {
                monster_info->flag_255 = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004e5f00
void DestroyUngroupedMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->monster_group_id == 0) {
            W8Monster* monster = monster_info->monster;

            if (monster != 0) {
                unsigned int flags = monster->flags_1dc;
                flags >>= 8;
                if ((flags & 1) != 0 &&
                    monster->state_22e != 0) {
                    monster->ApplyRemovalStateEffects();
                }
            }
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            DeactivateMonster(monster_info);
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
            }
            if (monster_info->monster != 0) {
                Function4C59C0(monster_info->monster, GetWorld());
                Function46E5A0(GetWorld(monster_info->monster));
                DeleteMonster004C5860(monster_info->monster);
                monster_info->monster = 0;
            }
            (void)g_octree_6598a4;
            Function42E650(static_cast<unsigned short>(monster_info->location_id));
            Function509EA0(monster_info->runtime_value_2f1);
            void* removed = PLRemoveAt(g_monster_list, index);
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
        if (monster_info->control_state == 1 &&
            monster_info->monster->linked_navigator_05c == 0) {
            Function4537E0(monster_info->monster);
            monster_info->flag_255 = 0;
        }
        break;
    }
    monster_info->control_state = control_state;
    RecountActiveMonsterGroupMembers(
        GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(
                0x872,
                MONSTER_MANAGER_CPP,
                monster_info->monster_group_id,
                1)));
}

// FUNCTION: WIZ8 0x004e60b0
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, unsigned char motionless)
{
    unsigned char previous = monster_info->motionless;
    W8Monster* monster = monster_info->monster;

    monster_info->motionless = motionless;
    if (motionless == 0) {
        if (previous != 0) {
            MonsterSetAnimating(monster, 1);
            if (monster_info->monster->m_pRep->pending_cycle == -1) {
                StartMonsterCycle(monster_info, 1, 3);
            }
        }
    }
    else if (previous == 0) {
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
    if (PListRemove(g_unborn_monster_list, monster_info) == 0) {
        return;
    }

    PLAdoptAppend(g_monster_list, monster_info);
    if (MonsterIsCycleSupported(monster_info->monster, 0) != 0) {
        MonsterSetCycle(monster_info->monster, 0);
        MonsterSetBehaviour(monster_info->monster, 1);
    }
    else {
        MonsterSetCycle(monster_info->monster, 1);
        MonsterSetBehaviour(monster_info->monster, 3);
    }
    MonsterSetSubCycle(monster_info->monster, 0);
    MonsterSetAnimating(monster_info->monster, 1);
    monster_info->monster->state_088 = 1;
    monster_info->monster->flag_215 = 0;
}

static __forceinline double DistanceBetweenPositions(
    const srVector3T<float>* first,
    const srVector3T<float>& second)
{
    float x = first->x - second.x;
    float y = first->y - second.y;
    float z = first->z - second.z;
    return sqrt(x * x + y * y + z * z);
}

// FUNCTION: WIZ8 0x004e61e0
W8MonsterInfo* FindNearestMonsterInfo(
    const srVector3T<float>* position,
    double maximum_distance)
{
    W8MonsterInfo* nearest = 0;
    double nearest_distance = 1.0e11;
    unsigned int index;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(g_monster_list, index);
        double distance = DistanceBetweenPositions(
            position,
            monster_info->monster->GetPosition());

        if (distance < nearest_distance &&
            (maximum_distance == 0.0 || distance < maximum_distance)) {
            nearest = monster_info;
            nearest_distance = distance;
        }
    }

    for (index = 0; index < PLLength(g_unborn_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(g_unborn_monster_list, index);
        double distance = DistanceBetweenPositions(
            position,
            monster_info->monster->GetPosition());

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

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(g_monster_list, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->hp_max = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->runtime_stat_max_2f = value;
        monster_info->runtime_stat_current_33 = value;
        monster_info->runtime_value_242 = Function52A780(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
        Function4C5810(monster_info->monster);
        Function4C5ED0(monster_info->monster);
    }

    for (index = 0; index < PLLength(g_unborn_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(g_unborn_monster_list, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->hp_max = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->runtime_stat_max_2f = value;
        monster_info->runtime_stat_current_33 = value;
        monster_info->runtime_value_242 = Function52A780(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
        Function4C5810(monster_info->monster);
        Function4C5ED0(monster_info->monster);
    }
}

// FUNCTION: WIZ8 0x004e65d0
float CalculateMonsterScale(W8MonsterInfo* monster_info)
{
    float minimum;
    float maximum;

    MonsterGetScaleRange(monster_info->monster, &minimum, &maximum);
    if (minimum == 0.0f || maximum == 0.0f) {
        return MonsterGetScale(monster_info->monster);
    }

    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    W8MonsterRecord* record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    int minimum_hp = record->hit_points_d6.base + record->hit_points_d6.count;
    int maximum_hp = record->hit_points_d6.base +
                     record->hit_points_d6.count * record->hit_points_d6.sides;
    float scale = ((maximum - minimum) *
                   ((float)(unsigned int)monster_info->hp_max - (float)minimum_hp)) /
                  ((float)maximum_hp - (float)minimum_hp) + minimum;
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
void TryStartMonsterCycle2(
    W8MonsterInfo* monster_info,
    W8Monster* monster,
    int query_state)
{
    if (monster_info->flag_14 != 0 &&
        static_cast<unsigned int>(monster_info->hp_current) > 0 &&
        monster_info->condition_turns[W8_CONDITION_EQUIPMENT_UNLOCKED] == 0 &&
        monster_info->flag_24d != 0 &&
        query_state == 1) {
        int result = MonsterQuery(monster, 2);

        if (result != 0 && monster_info->motionless == 0) {
            monster->flags_1dc |= 0x80;
            if (MonsterIsCycleSupported(monster, 2) != 0) {
                signed char cycle = monster->m_pRep->pending_cycle;

                if (cycle == 2 ||
                    monster->IsCycleInterruptable(cycle) == 0 ||
                    (g_in_combat_00683f94 != 0 &&
                     g_combat_state->selected_slot != 0 &&
                     g_combat_state->selected_monster == monster_info)) {
                    return;
                }
                W8MonsterGroup* group = GetMonsterGroupByListIndex(
                    GetMonsterGroupIndexByID(
                        0x987,
                        MONSTER_MANAGER_CPP,
                        monster_info->monster_group_id,
                        1));
                unsigned int chance = group->member_count * 20;

                if (g_flag_683f97 == 0 && Random(chance) == 0) {
                    StartMonsterCycle(monster_info, 2, 1);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004e6780
unsigned int GetMonsterCombatValue(const W8MonsterRecord* record)
{
    unsigned int value = record->combat_value_override_26b;

    if (value <= 0) {
        value = record->combat_value_181;
    }
    return value;
}

// FUNCTION: WIZ8 0x004e68c0
unsigned char AnyMonsterDying(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(g_monster_list); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info != 0 && monster_info->monster->IsDying() != 0) {
            return 1;
        }
    }
    return 0;
}

/* Reset the two generated runtime IDs and the live manager counts, then create
   or clear the four lists owned by gXStatus. The final two success checks are
   deliberately asymmetric: retail rechecks plsMonsterList after initializing
   plsUnbornMonsterList, and plsMonsterGroupList after the encounter list. */
// FUNCTION: WIZ8 0x004e3720
bool InitializeMonsterManagerState(void)
{
    g_status_685170.next_monster_location_id_234e = 1;
    g_status_685170.status_count_234a = 1;
    gXStatus.active_monster_count = 0;
    gXStatus.field_02d = 0;
    g_dword_6850be = 0;
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->selected_item = -1;
    }
    if (gXStatus.plsMonsterList == 0) {
        gXStatus.plsMonsterList = PLCreate();
    }
    else {
        PListClear(gXStatus.plsMonsterList);
    }
    if (gXStatus.plsMonsterList == 0) {
        return false;
    }
    if (gXStatus.plsMonsterGroupList == 0) {
        gXStatus.plsMonsterGroupList = PLCreate();
    }
    else {
        PListClear(gXStatus.plsMonsterGroupList);
    }
    if (gXStatus.plsMonsterGroupList == 0) {
        return false;
    }
    if (gXStatus.plsUnbornMonsterList == 0) {
        gXStatus.plsUnbornMonsterList = PLCreate();
    }
    else {
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
        srAssertFail(
            "gXStatus.plsMonsterGroupList != NULL", MONSTER_MANAGER_CPP, 0x5c, 0);
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
    for (slot = g_monster_record_cache;
         slot < g_monster_record_cache + MAX_MONSTERS_IN_DATABASE;
         ++slot) {
        if (*slot != 0) {
            free(*slot);
            *slot = 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004e3af0
unsigned char RemoveMonster(
    unsigned int monster_list_index,
    unsigned char destroy_monster)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);

    if (g_in_combat_00683f94 != 0 && monster_info->fInCombat != 0 && monster_info->pCombat != 0) {
        MonsterInfoLeaveCombat(monster_info);
    }
    if (monster_info->monster_group_id != 0) {
        W8MonsterGroup* monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(
                0x125, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1));

        IListRemove(monster_group->monsters, monster_info->location_id);
        --monster_group->member_count;
        RequestRedrawParty();
        if (monster_group->member_count == 0) {
            DestroyMonsterGroup(monster_group, monster_info);
        } else {
            RecountActiveMonsterGroupMembers(monster_group);
            if (monster_group->value_9f == monster_info->location_id) {
                Function5103E0(monster_group);
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
        if (monster_info->monster != 0) {
            Function4C59C0(monster_info->monster, GetWorld());
            Function46E5A0(GetWorld(monster_info->monster));
            DeleteMonster004C5860(monster_info->monster);
            monster_info->monster = 0;
        }
        (void)g_octree_6598a4;
        Function42E650(static_cast<unsigned short>(monster_info->location_id));
        Function509EA0(monster_info->runtime_value_2f1);
        void* removed = PLRemoveAt(g_monster_list, monster_list_index);
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
    if (monster_info->flag_14 != 0) {
        if (monster_info->monster == 0) {
            srAssertFail("pMonsterInfo->p3D != NULL", MONSTER_MANAGER_CPP, 0x249, 0);
        }
        monster_info->condition_turns[W8_CONDITION_EQUIPMENT_UNLOCKED] = 9999;
        monster_info->value_107 = 0x12;
        monster_info->hp_current = 0;
        monster_info->runtime_stat_current_33 = 0;
        monster_info->monster->state_088 = 0;
        monster_info->monster->flags_00c = 0x200000;
        Function4ACF90(monster_info->monster);
        ReleaseMonToMonVisibilityList(monster_info);
        Function4C5750(monster_info->monster, &position);
        monster_info->position_17.x = position.x;
        monster_info->position_17.y = position.y;
        monster_info->position_17.z = position.z;
        monster_info->flag_14 = 0;
        --g_active_monster_count_683fa1;
        if (g_in_combat_00683f94 != 0) {
            RefreshAllSight();
            SetTargetToMonster(monster_info->location_id, 0);
            Function593330();
            Function546E70();
            if (g_combat_state->selected_monster == monster_info) {
                g_combat_state->selected_slot = 0;
                g_combat_state->selected_monster = 0;
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
    Function4C5B10(monster_info->monster, 0);
    monster_info->monster->flags_00c &= 0xdfffffff;
    MonsterForward4537E0(monster_info->monster);
    if (monster_info->motionless == 0) {
        query_state = MonsterQuery(monster_info->monster, 6);
        if (query_state != 1 && query_state != 2 &&
            monster_info->monster->m_pRep->pending_cycle == -1) {
            StartMonsterCycle(monster_info, 1, 3);
        }
    }
    monster_info->pCombat = static_cast<W8MonsterCombatState*>(malloc(0x153));
    if (monster_info->pCombat == 0) {
        srAssertFail("pMonsterInfo->pCombat != NULL", MONSTER_MANAGER_CPP, 0x2a0, 0);
    }
    memset(monster_info->pCombat, 0, 0x153);
    monster_info->fInCombat = 1;
    if (monster_info->state_34c == 0) {
        monster_info->state_34c = 2;
        monster_info->value_354 = g_world_clock_00686a48;
    }
    ResetCombatSlot(&monster_info->combat_slot_2ba);
    MonsterSetRuntimeFlag5BC(monster_info->monster, 0);
    monster_info->monster->flags_00c = 0;
    if (monster_info->flag_16 == 1) {
        Function546E70();
    }
    if (g_in_combat_00683f94 != 0) {
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
    W8MonsterCombatEntry* entry;

    if (g_in_combat_00683f94 == 0) {
        srAssertFail("gXStatus.fCombatMode", MONSTER_MANAGER_CPP, 0x2c6, 0);
    }
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2c7, 0);
    }
    if (monster_info->fInCombat == 0) {
        srAssertFail("pMonsterInfo->fInCombat", MONSTER_MANAGER_CPP, 0x2c8, 0);
    }
    if (g_combat_state->selected_monster == monster_info) {
        g_combat_state->selected_slot = 0;
        g_combat_state->selected_monster = 0;
    }
    /* The record cursor is spelled (base + offset) + constant, not
       (base + constant) + offset: the first form leaves the block pointer as
       the LEA's base register, which is the encoding the original uses, while
       the second folds the constant into the displacement and promotes the
       running offset to base instead. */
    for (index = 0; index < 9; ++index) {
        entry = &monster_info->pCombat->entries_3e[index];
        if (entry->active != 0) {
            ClearEffectSlot(monster_info, entry);
        }
    }
    for (index = 0; index < 6; ++index) {
        entry = &monster_info->pCombat->entries_d7[index];
        if (entry->active != 0) {
            ClearEffectSlot(monster_info, entry);
        }
    }
    DestroyMonsterActionQueue(monster_info);
    free(monster_info->pCombat);
    monster_info->pCombat = 0;
    monster_info->fInCombat = 0;
    if (monster_info->flag_16 == 1) {
        Function546E70();
    }
}

/* Toggles the party's combat-ready posture. Outside combat it only flips the
   flag; inside it also swaps the two engine bits at +0x001 and +0xa62 of the
   state block, and in engine mode 7 it announces the new posture with the
   message the block at 0x0068C09C holds for that direction. */
// FUNCTION: WIZ8 0x004e6c10
void TogglePartyCombatStance(void)
{
    void* message;

    if (g_flag_68517c == 0) {
        g_flag_6850d2 = (g_flag_6850d2 == 0);
        return;
    }
    if (g_flag_6850d2 != 0) {
        g_flag_6850d2 = 0;
        if (g_in_combat_00683f94 != 0) {
            g_combat_state->flag_001 = (g_combat_state->flag_000 == 0);
            g_combat_state->flag_a62 = 1;
        }
        if (g_screen_state_0068ec78.id != W8_SCREEN_MAIN_GAME) {
            return;
        }
        message = gppStringList[W8_NOTICE_COMBAT_STANCE_RELAXED];
    } else {
        g_flag_6850d2 = 1;
        if (g_in_combat_00683f94 != 0 && g_combat_state->flag_a62 != 0) {
            g_combat_state->flag_001 = 1;
            g_combat_state->flag_a62 = 0;
        }
        if (g_screen_state_0068ec78.id != W8_SCREEN_MAIN_GAME) {
            return;
        }
        message = gppStringList[W8_NOTICE_COMBAT_STANCE_READY];
    }
    Function58AC00(0xc, message, -1, -1, 0);
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
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
    W8CombatActor* character;

    if (g_in_combat_00683f94 == 0) {
        StartCombat(1);
        return;
    }
    if (g_dword_683fa5 != 0 || g_combat_state->flag_a54 != 0) {
        if (g_combat_state->value_004 != 0) {
            Function58AC00(
                0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END], -1, -1, 0);
            return;
        }
        for (group_list_index = 0;
             group_list_index < PLLength(g_monster_group_list);
             ++group_list_index) {
            monster_group = GetMonsterGroupByListIndex(group_list_index);
            if (monster_group->flag_28 != 0 && monster_group->flag_29 != 0 &&
                Function531920(monster_group) != 0) {
                Function58AC00(
                    0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END], -1, -1, 0);
                return;
            }
        }
    }
    for (character = NextEngagedCharacter(1); character != 0;
         character = NextEngagedCharacter(0)) {
        if ((character == g_combat_state->engaged_actor ||
             g_character_class_records[character->class_record_index].flag_154 != 0) &&
            Function4A5790() != 0) {
            Function58AC00(
                0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END_ENGAGED], -1, -1, 0);
            return;
        }
    }
    if (g_combat_state->flag_000 != 0) {
        Function58AC00(
            0xc, gppStringList[W8_NOTICE_COMBAT_CANNOT_END_PENDING], -1, -1, 1);
        return;
    }
    EndCombat(0);
    Function58AC00(0xc, gppStringList[W8_NOTICE_COMBAT_ENDED], -1, -1, 0);
    if (g_level_block->combat_end_notification != -1) {
        Function595570();
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
    W8Monster* monster = monster_info->monster;
    unsigned char pending;
    const char* detail;
    int line;

    if (static_cast<signed char>(behavior) == W8_BEHAVIOUR_NEVER_STOP &&
        monster->IsCycleInterruptable(static_cast<signed char>(cycle)) == 0) {
        detail = "Trying to set a NEVER_STOP behaviour with an uninterruptable cycle!";
        line = 0x46f;
    } else {
        pending = monster->m_pRep->pending_cycle;
        if (g_in_combat_00683f94 != 0) {
            MonsterQuery(monster, 6);
        }
        if (pending != W8_CYCLE_STOP && pending != W8_CYCLE_NONE &&
            monster->IsCycleInterruptable(static_cast<signed char>(pending)) == 0) {
            if (static_cast<signed char>(cycle) == W8_CYCLE_STOP) {
                return;
            }
            srAssertFail(
                "FALSE",
                MONSTER_MANAGER_CPP,
                0x497,
                reinterpret_cast<const char*>(String(
                    "%ls starting new cycle (%s) with an uninterruptable cycle pending (%s)!",
                    GetMonsterName(monster_info, 0, 0),
                    g_cycle_names[static_cast<signed char>(cycle)].name,
                    g_cycle_names[static_cast<signed char>(pending)].name)));
            return;
        }
        if (static_cast<signed char>(cycle) == W8_CYCLE_STOP ||
            static_cast<signed char>(cycle) == W8_CYCLE_DEATH ||
            static_cast<signed char>(cycle) == 0 ||
            monster_info->motionless == 0) {
            MonsterSetAnimating(monster, 1);
            MonsterSetRuntimeBehaviour(monster, static_cast<signed char>(behavior));
            MonsterSetPendingCycle(monster, cycle);
            monster->m_pRep->value_066 = 0;
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

    for (monster_list_index = 0;
         monster_list_index < PLLength(gXStatus.plsMonsterList);
         ++monster_list_index) {
        W8MonsterInfo* monster_info =
            MonsterGetScriptPartByLocationIndex(monster_list_index);
        W8Monster* monster = monster_info->monster;

        if ((monster->flags_1dc & 0x100) != 0) {
            if (monster->fade_state_330 == 0) {
                if (monster->state_22e != 0) {
                    monster->ApplyRemovalStateEffects();
                }
                monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
                DeactivateMonster(monster_info);
                if (monster_info == 0) {
                    srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
                }
                if (monster_info->monster != 0) {
                    Function4C59C0(monster_info->monster, GetWorld());
                    Function46E5A0(GetWorld(monster_info->monster));
                    DeleteMonster004C5860(monster_info->monster);
                    monster_info->monster = 0;
                }
                Function42E650(static_cast<unsigned short>(monster_info->location_id));
                Function509EA0(monster_info->runtime_value_2f1);
                void* removed = PLRemoveAt(gXStatus.plsMonsterList, monster_list_index);
                if (removed != 0) {
                    free(removed);
                }
                --monster_list_index;
            }
        }
        else if ((monster->flags_1dc & 0x200) != 0) {
            RemoveMonster(monster_list_index, 1);
            --monster_list_index;
        }
        else if ((monster->flags_1dc & 0x20) == 0) {
            if ((monster->flags_1dc & 0x40) == 0) {
                int query_state = MonsterQuery(monster, 6);
                TryStartMonsterCycle2(monster_info, monster, query_state);
                if (MonsterQuery(monster, 7) != 0) {
                    if (gXStatus.fCombatMode != 0 && query_state == 0x12) {
                        monster_info->pCombat->unknown_13d[8] = 1;
                    }
                    switch (query_state) {
                    case 1:
                    case 3:
                    case 4:
                    case 0x17:
                    case 0x18:
                        break;
                    case 0x15:
                        if ((monster->flags_1dc & 0x100) == 0) {
                            monster_info =
                                MonsterGetScriptPartByLocationIndex(monster_list_index);
                            Function508D70(monster_list_index);
                            if (monster_info->monster_group_id != 0) {
                                RemoveMonster(monster_list_index, 0);
                            }
                            Function4F8CB0(monster_info, -1);
                            monster_info->monster->BeginDelayedRemoval004C5000();
                        }
                        break;
                    default:
                        if (monster_info->motionless == 0) {
                            if (monster_info->monster->m_pRep->pending_cycle ==
                                W8_CYCLE_NONE) {
                                StartMonsterCycle(monster_info, 1, 3);
                            }
                        }
                        else if (MonsterIsAnimating(monster) != 0) {
                            MonsterSetAnimating(monster, 0);
                        }
                        break;
                    }
                }
            }
        }
        else {
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
W8WideChar* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
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
        swprintf(
            g_monster_name_buffer,
            L"Al-%s",
            g_party_characters[g_alternate_name_slot].name);
        return g_monster_name_buffer;
    }
    if (monster_info->monster_group_id == 0) {
        if (monster_info->monster->IsDying() == 0) {
            FormatDebugMessage(
                1, "ERROR: Monster ID %d has no group", monster_info->location_id);
        }
    } else {
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(
                0x54c, MONSTER_MANAGER_CPP, monster_info->monster_group_id, 1));
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_MANAGER_CPP, 0x54d, 0);
        }
        if (monster_group->flag_2c != 0) {
            return record->name_00 + name_form * 24;
        }
    }
    return record->name_60 + name_form * 24;
}

float Function4EFB60(void);
unsigned int Function554490(int skill_index, int* party_slot);

/* Format the health knowledge the party has earned for one monster. NPC-backed
   records can suppress exact values, and ordinary monsters expose current and
   maximum HP independently at knowledge thresholds ten and five. */
// FUNCTION: WIZ8 0x004e52c0
void FormatMonsterHealth(
    W8MonsterInfo* monster_info,
    W8WideChar* health_text)
{
    unsigned char suppress_exact_health = 0;
    unsigned int health_knowledge;

    if (monster_info->flag_16 != 1) {
        W8MonsterRecord* record;
        W8NPCItemList* npc_item_list;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        if ((record->flags_0d0 & 1) != 0) {
            npc_item_list = GetNPCItemListByID(record->unknown_0cd[0]);
            if (npc_item_list != 0 &&
                npc_item_list->npc_record->unknown_00[0x57] != 0) {
                suppress_exact_health = 1;
            }
        }
    }

    if (monster_info->value_2da == 1) {
        health_knowledge = 125;
    }
    else {
        float average_party_level = Function4EFB60();
        W8MonsterRecord* record;
        int best_party_slot;
        int monster_level;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x5e9, 0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        monster_level = record->unknown_250[1];
        health_knowledge = Function554490(0x15, &best_party_slot);
        if (static_cast<int>(average_party_level) < monster_level) {
            float adjusted_knowledge =
                static_cast<float>(health_knowledge) -
                (static_cast<float>(monster_level) - average_party_level) *
                    g_float_005ec52c +
                g_float_005ebc7c;
            if (adjusted_knowledge < g_float_005ebb34) {
                adjusted_knowledge = g_float_005ebb34;
            }
            health_knowledge = static_cast<unsigned int>(adjusted_knowledge);
        }
    }

    if (g_flag_689b32 != 0) {
        wcscpy(
            health_text,
            FormatWideString(
                L"%d/%d", monster_info->hp_current, monster_info->hp_max));
        return;
    }
    if (health_knowledge < 10 || suppress_exact_health != 0) {
        wcscpy(health_text, L"");
    }
    else {
        wcscpy(health_text, FormatWideString(L"%d", monster_info->hp_current));
    }
    wcscat(health_text, L"/");
    if (health_knowledge > 4 && suppress_exact_health == 0) {
        wcscat(health_text, FormatWideString(L"%d", monster_info->hp_max));
        return;
    }
    wcscat(health_text, L"");
}

// GLOBAL: WIZ8 0x006836B8
W8MonsterManagerState g_monster_manager_state;

// FUNCTION: WIZ8 0x004e6940
W8MonsterManagerState::~W8MonsterManagerState()
{
}

// FUNCTION: WIZ8 0x004e6970
W8MonsterManagerState::W8MonsterManagerState()
{
}

// FUNCTION: WIZ8 0x004e6a10
W8MonsterManagerEntry::~W8MonsterManagerEntry()
{
}

// FUNCTION: WIZ8 0x004e6a30
W8MonsterManagerEntry::W8MonsterManagerEntry()
{
}
