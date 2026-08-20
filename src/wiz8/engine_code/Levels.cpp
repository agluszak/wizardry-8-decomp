#include <cmath>
#include <cstdio>
#include <cstring>

#include "wiz8/combat_state.h"
#include "surrender/srClipPlane.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/game_state.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/music_playlist.h"
#include "wiz8/spell_effect.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

#include "FileMan.h"

extern void Function4EA310(int mode);
extern void Function426790(void);
extern void Function50DA00(void);
extern unsigned char ReleaseItemLists(void);
extern unsigned char ShutdownMonsterManager(void);
extern unsigned char Function5B1740(void);
extern void Function48DB30(void);
extern void ClearValue689FAC(void);
extern void Function4909C0(void);
extern void Function489920(void);
extern void ClearValue6834D4(void);
extern unsigned char SaveLevelStatus(const char* path);
extern int Function4D9700(int level);
extern void Function427440(void);
extern unsigned char Function42B020(int level, W8LevelInfo* info);
extern void InitializeItemManagerState(void);
extern void Function443A50(void);
extern void Function5817D0(void);
extern unsigned char LoadLevelStatus(const char* path, int level);
extern void BuildLevelStatusPath(char* path, int level);
extern float Function420BD0(const srVector3T<float>* position, unsigned char* hit);
extern float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);
extern void MoveWorldToPoint(
    W8World* destination, W8World* source, const srVector3T<float>* point);
extern void Function5115B0(void);
extern void ResetMonsterGroupTurnState(void);
extern unsigned char LoadAmbientSoundList0047AB40(char* filename);
extern void Function41AA40(void);
extern void Function482410(void);
extern void Function4D6C50(int level);
extern void Function50AC60(void);
extern void Function48ED70(unsigned char value);
extern void Function50E700(void);
extern void Function50DB50(void);
extern void Function50C270(void);
extern void Function50C2E0(void);
extern void Function5777C0(void);
extern void UpdateRandomEncounterBudget(unsigned char first_visit);
extern void Function48C9F0(void);
extern void Function5060C0(void);
extern unsigned int AgeAllMonsterSight(void);
extern void RebuildAllWorldItemInstances(void);
extern void Function451020(void);
extern void SetSkyNodeVisible(char visible);

extern unsigned char g_world_cleanup_flag_00659757;
extern float g_runtime_world_scale_6081e8;
extern unsigned char g_flag_00659756;
extern float g_default_world_height_00603ac8;
extern float g_position_height_epsilon_005ebfdc;
// GLOBAL: WIZ8 0x00687417
srVector3T<float> g_saved_world_position_00687417;
extern unsigned char g_environment_load_flag_00603ad0;
extern unsigned char g_level_runtime_flag_0065ba70;
extern unsigned char g_value_0068f0fd;
extern int g_value_006850b0;
extern unsigned char g_flag_00687607;

extern char g_ambient_sound_filename_006059e0[];
extern const char* g_sky_names_00605880[];

/* Resolve a database level number into all level and sky resource paths. The
   regular forty-seven levels use the database row directly; the ten test
   slots synthesize level1..level9 and DefaultLevel. A missing LVL file is
   valid only when both its OCT and PVL replacements exist. */
// FUNCTION: WIZ8 0x0042A370
unsigned char LevelBuildInfoByID(int level, W8LevelInfo* info)
{
    char oct_path[1020];
    char pvl_path[1020];

    if ((unsigned int)level >= 57) {
        srAssertFail(
            "ulLevel < TEST_LEVEL_COUNT",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Levels.cpp",
            237, 0);
    }

    if (level < 47) {
        sprintf(info->level_folder, "%s\\%s", "Levels",
                g_level_folders[level].folder_name);
        sprintf(info->level_file_name, "%s.%s",
                g_level_folders[level].level_name, "LVL");
        if (g_level_folders[level].sky_index == -1) {
            info->sky_file_name[0] = '\0';
        }
        else {
            sprintf(info->sky_file_name, "%s.%s",
                    g_sky_names_00605880[g_level_folders[level].sky_index],
                    "LVL");
        }
    }
    else {
        sprintf(info->level_folder, "%s\\Test", "Levels");
        if (level == 56) {
            sprintf(info->level_file_name, "DefaultLevel.%s", "LVL");
            sprintf(info->sky_file_name, "%s.%s",
                    g_sky_names_00605880[0], "LVL");
        }
        else {
            char test_level = static_cast<char>(level + 2);
            sprintf(info->level_file_name, "level%c.%s", test_level, "LVL");
            sprintf(info->sky_file_name, "sky%c.%s", test_level, "LVL");
        }
    }

    strcpy(info->sky_folder, info->level_folder);
    sprintf(info->level_bitmap_folder, "%s\\Bitmaps", info->level_folder);
    sprintf(info->sky_bitmap_folder, "%s\\Bitmaps", info->sky_folder);
    sprintf(info->level_path, "%s\\%s", info->level_folder,
            info->level_file_name);

    strcpy(oct_path, info->level_path);
    strcpy(oct_path + strlen(oct_path) - 3, "oct");
    strcpy(pvl_path, info->level_path);
    strcpy(pvl_path + strlen(pvl_path) - 3, "pvl");
    if (!FileExists(info->level_path)
        && (!FileExists(oct_path) || !FileExists(pvl_path))) {
        return 0;
    }

    sprintf(info->sky_path, "%s\\%s", info->sky_folder,
            info->sky_file_name);
    if (level < 47) {
        if (g_level_folders[level].sky_index != -1
            && !FileExists(info->sky_path)) {
            sprintf(info->sky_folder, "%s\\Test", "Levels");
            sprintf(info->sky_file_name, "%s.%s",
                    g_sky_names_00605880[0], "LVL");
            sprintf(info->sky_bitmap_folder, "%s\\Bitmaps",
                    info->sky_folder);
            sprintf(info->sky_path, "%s\\%s", info->sky_folder,
                    info->sky_file_name);
            if (!FileExists(info->sky_path)) {
                return 0;
            }
        }
    }
    else if (!FileExists(info->sky_path)) {
        info->sky_file_name[0] = '\0';
    }
    return 1;
}

/* Build the complete live level around the current world. The subordinate
   loaders remain in their original units; this body owns their ordering,
   rollback boundary, entry positioning, first-visit work and final renderer
   publication. */
// FUNCTION: WIZ8 0x0042A6F0
unsigned char LoadLevel(
    int requested_level, int entrance, unsigned char restoring_game)
{
    int level = Function4D9700(requested_level);
    W8LevelInfo level_info;
    int previous_level;
    unsigned char first_visit = 0;
    char path[260];
    char music_path[260];

    if (level >= 57) {
        srAssertFail(
            "iLevel < TEST_LEVEL_COUNT",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Levels.cpp",
            385, 0);
    }
    if (!LevelBuildInfoByID(level, &level_info)) {
        return 0;
    }

    DisableSky();
    if (GetWorld() != 0) {
        Forward44FAF0(GetWorld());
        SetCurrentWorld(0);
    }
    Function489920();
    Function427440();
    SetCurrentWorld(CreateWorld());

    previous_level = g_current_level;
    g_current_level = level;
    sprintf(
        music_path, "Data\\Music\\%s.MPL",
        g_level_folders[level].folder_name);
    if (FileExists(music_path)) {
        sprintf(
            music_path, "%s.MPL",
            g_level_folders[g_current_level].folder_name);
        Function48FC10(music_path, 1, 1);
    }
    else {
        Function48FC10("", 1, 1);
    }
    Function48F9E0();

    if (!Function42B020(level, &level_info)) {
        return 0;
    }
    InitializeMonsterManagerState();
    InitializeItemManagerState();
    Function443A50();
    if (!ForwardLoadWorld(
            GetWorld(), level_info.level_file_name, level_info.level_folder,
            level_info.level_bitmap_folder, 1)) {
        /* This is the complete canonical rollback here: restore the level ID.
           The already-installed replacement world is not destroyed. */
        g_current_level = previous_level;
        return 0;
    }

    SetSkyNodeVisible(0);
    Function5817D0();
    if (!LoadLevelStatus("Saves\\CurrentGame.SAV", level)) {
        BuildLevelStatusPath(path, level);
        g_flag_00659756 = 1;
        LoadLevelStatus(path, level);
        g_flag_00659756 = 0;
    }

    if (!restoring_game && entrance != -1) {
        Trigger* trigger = 0;

        if (level < 47) {
            char trigger_name[8];
            sprintf(
                trigger_name, "%3s%02d",
                g_level_folders[level].level_name, entrance);
            trigger = FindTriggerByName(trigger_name);
        }
        if (trigger != 0 && trigger->flag_0a0_11) {
            srVector3T<float> trigger_position;
            srVector3T<float> position;

            trigger->GetPosition(&trigger_position);
            position = trigger_position;
            position.y = Function420BD0(&trigger_position, 0)
                       + g_default_world_height_00603ac8;
            if (fabs(position.y - trigger_position.y)
                > g_position_height_epsilon_005ebfdc) {
                position.y = trigger_position.y;
            }
            SetWorldScenePosition004511D0(GetWorld(), &position);

            if (trigger->trigger_kind_018 == 2) {
                srVector3T<float> axis;
                srMatrix3T<float> rotation;

                axis.x = trigger->value_100;
                axis.y = trigger->value_104;
                axis.z = trigger->value_108;
                rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
                rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
                rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);
                if (trigger->angle_0fc != 0.0f) {
                    RotateMatrixAroundAxis0042B910(
                        &rotation.vectors[0].x,
                        sin(trigger->angle_0fc), cos(trigger->angle_0fc),
                        &axis.x);
                }
                ApplyCameraRotation(&rotation);
            }
        }
        else {
            srVector3T<float> position;

            position.x = 0.0f;
            position.y = g_default_world_height_00603ac8;
            position.z = 0.0f;
            SetWorldScenePosition004511D0(GetWorld(), &position);
        }
    }
    else {
        MoveWorldToPoint(
            GetWorld(), GetWorld659AB8(), &g_saved_world_position_00687417);
    }

    if (level < 47 && !g_level_progress[level].visited) {
        ResetMonsterGroupTurnState();
        Function5115B0();
        g_level_progress[level].visited = 1;
        first_visit = 1;
    }

    sprintf(
        path, "%s\\%s\\%s", level_info.level_folder,
        level_info.level_file_name, g_ambient_sound_filename_006059e0);
    LoadAmbientSoundList0047AB40(path);
    if (!g_environment_load_flag_00603ad0) {
        Function41AA40();
    }
    g_level_runtime_flag_0065ba70 = 0;
    Function482410();
    Function4D6C50(level);
    Function50AC60();
    Function48ED70(g_value_0068f0fd);
    Function50E700();

    if (!restoring_game) {
        if (level < 47) {
            Function50DB50();
            Function50C270();
            Function50C2E0();
            Function5777C0();
            g_value_006850b0 = 0;
        }
        if (g_flag_00687607
            && (GetFact(0x4c) || GetFact(0x4b))) {
            Function48C9F0();
        }
        else {
            UpdateRandomEncounterBudget(first_visit);
        }
        if (!first_visit) {
            Function5060C0();
            AgeAllMonsterSight();
        }
        for (int index = g_spell_effects.GetCount() - 1; index >= 0; --index) {
            delete g_spell_effects.RemoveAt(index);
        }
    }
    else {
        W8SpellEffectEntry* effect = FindMonsterControlSpellEffect();

        if (effect != 0) {
            SpawnLureEffects(effect, effect->argument, &effect->target);
        }
    }

    if (first_visit) {
        InitializeMonsterRuntimeStats();
        RebuildAllWorldItemInstances();
    }
    MarkRendererReady();
    Function451020();
    ReleaseReadMeshScratch004881D0();
    return 1;
}

// FUNCTION: WIZ8 0x0042ACE0
unsigned char UnloadLevel(const char* save_directory)
{
    if (g_in_combat_00683f94 != 0) {
        Function4EA310(1);
    }

    if (g_current_level < 47) {
        g_level_progress[g_current_level].sight_clock = g_world_clock_00686a48;
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    Function50DA00();
    if (strcmp(save_directory, "") != 0) {
        SaveLevelStatus("Saves\\CurrentGame.SAV");
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    if (g_active_monster_list_00683fad != 0) {
        if (ReleaseItemLists() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
        if (ShutdownMonsterManager() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
        if (Function5B1740() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
    }

    Function48DB30();
    ClearValue689FAC();
    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    Function4909C0();
    DisableSky();
    W8World* world = GetWorld();
    if (world != 0) {
        Forward44FAF0(world);
        SetCurrentWorld(0);
    }
    Function489920();

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    DisableSky();
    g_current_level = -1;
    ReleaseEnvironmentObjects();
    g_runtime_world_scale_6081e8 = 500.0f;
    ClearValue6834D4();

    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node =
        srClassSupport<srClipPlane, srClipPlane, false, 0x1500>::sGetClassNode();
    srClass* clip_plane = static_cast<srClass*>(registry->find(node, 0, 0));

    while (clip_plane != 0) {
        srClass* next = static_cast<srClass*>(
            registry->find(
                srClassSupport<
                    srClipPlane, srClipPlane, false, 0x1500>::sGetClassNode(),
                0, clip_plane));
        clip_plane->release();
        clip_plane = next;
    }
    return 1;
}
