#include "wiz8/engine_code/3dapi.h"
#include "LibraryDataBase.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/monster_generators.h"
#include "wiz8/render_state.h"
#include <cmath>
#include "wiz8/xstatus.h"
#include <cstdio>
#include <cstring>

#include "wiz8/combat_state.h"
#include "surrender/srClipPlane.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/game_status.h"
#include "wiz8/location_variables.h"
#include "wiz8/screen_state.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/npc_state.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/music_playlist.h"
#include "wiz8/spell_effect.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"
#include "surrender/srLight.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"

#include "FileMan.h"

#define LEVELS_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Levels.cpp"

// GLOBAL: WIZ8 0x00604478
W8LevelFolderRecord g_level_folders[47] = {
    {"Arnika", "Arnika2", "ARN", 1, 3, 2},
    {"Ascension", "Ascension", "ASC", 0, 3, 12},
    {"Bayjin", "Bayjin1", "BA1", -1, 3, 8},
    {"Bayjin", "Bayjin2", "BA2", -1, 3, 8},
    {"Circle", "Circle", "CIR", 10, 3, 14},
    {"Marten", "Marten1", "MR1", -1, 3, 5},
    {"Marten", "Marten2", "MR2", 0, 3, 5},
    {"MountainPass", "MountainPass", "MNT", 2, 3, 6},
    {"Monastery", "Monastery1", "MO1", 0, 3, 1},
    {"Monastery", "Monastery2", "MO2", 0, 3, 1},
    {"MtGigas", "MtGigasWaterCaves", "MGW", -1, 3, 15},
    {"MtGigas", "MtGigasBelowCaves", "MGB", -1, 3, 11},
    {"MtGigas", "MtGigas1", "MG1", -1, 3, 11},
    {"MtGigas", "MtGigas2", "MG2", -1, 3, 11},
    {"MtGigas", "MtGigasOuter", "MGO", 0, 3, 11},
    {"MtGigas", "MtGigasTop", "MGT", 0, 3, 11},
    {"Camp", "Camp", "CMP", 7, 3, 13},
    {"Rapax", "RapaxCellar", "RAC", 1, 3, 9},
    {"Rapax", "RapaxMainFloor", "RAM", 1, 3, 9},
    {"Rapax", "RapaxUpperFloor", "RAU", 1, 3, 9},
    {"Rapax", "RapaxExterior", "RAE", 1, 3, 9},
    {"Rift", "Rift1", "RIF", 3, 3, 10},
    {"SeaCaves", "SeaCave1", "SC1", 0, 3, 7},
    {"SeaCaves", "SeaCave2", "SC2", 0, 3, 7},
    {"Swamp", "Swamp", "SWM", -1, 3, 4},
    {"Trynnie", "Trynnie1", "TY1", -1, 3, 3},
    {"Trynnie", "Trynnie2", "TY2", -1, 3, 3},
    {"ConnectiveTissue", "Arnika_Trynton", "CT1", 0, 3, 16},
    {"ConnectiveTissue", "Trynton_Swamp", "CT2", 0, 3, 0},
    {"ConnectiveTissue2", "SouthEastWilderness", "CT3", 0, 3, 17},
    {"ConnectiveTissue", "Rift_Peak", "CT4", 0, 3, 0},
    {"ConnectiveTissue2", "NorthEastWilderness", "CT5", 0, 3, 0},
    {"ConnectiveTissue", "NorthWilderness", "CT6", 0, 3, 0},
    {"ConnectiveTissue", "Arnika_StarterDungeon", "CT7", 0, 3, 0},
    {"ConnectiveTissue", "Peak_RapaxCastle", "CT8", 0, 3, 0},
    {"Footsteps", "Footsteps", "FS1", 0, 3, 0},
    {"SavantTower", "SavantTower", "SAV", -1, 3, 2},
    {"Trynnie", "Ratkin", "RTK", -1, 3, 3},
    {"Camp", "CampNoRapax", "CNR", 7, 3, 0},
    {"Dungeon", "Dungeon", "DUN", -1, 3, 0},
    {"Spare14", "Spare14", "SPE", 0, -1, 0},
    {"Spare15", "Spare15", "SPF", 0, -1, 0},
    {"Spare16", "Spare16", "SPG", 0, -1, 0},
    {"Spare17", "Spare17", "SPH", 0, -1, 0},
    {"Spare18", "Spare18", "SPI", 0, -1, 0},
    {"Spare19", "Spare19", "SPJ", 0, -1, 0},
    {"Spare20", "Spare20", "SPK", 0, -1, 0},
};

// GLOBAL: WIZ8 0x00686A70
int g_loaded_level_id;
/* The sky index of the world currently held in g_world_659ab8, or -1 when no
   sky is loaded. Every retail access is a byte access. */
// GLOBAL: WIZ8 0x00604470
signed char g_loaded_sky_index_00604470;
/* The CD volume number of the drive the game-data path finder last matched. */
// GLOBAL: WIZ8 0x00604474
int g_cd_index_00604474;
// GLOBAL: WIZ8 0x00659738
W8MaterialMapper00482010 g_material_mapper_00659738;

extern void Function4EA310(int mode);

// FUNCTION: WIZ8 0x0042b720
int GetLevelCdNumber0042B720(int level)
{
    return g_level_folders[level].cd_number;
}

// FUNCTION: WIZ8 0x0042b6f0
unsigned char IsLevelCdMissing0042B6F0(int level)
{
    return FindGameDataPath0042B590(
               gzCdDirectory, g_level_folders[level].cd_number) == 0;
}

/* Scan every logical drive for the CD whose volume label is WIZ8_<cd_number>,
   write its root path into the caller's buffer and report whether it was
   found. Only a CD-ROM drive is considered, and the volume query temporarily
   suppresses the system's error dialog for a missing disc. */
// FUNCTION: WIZ8 0x0042B590
unsigned char FindGameDataPath0042B590(char* path, int cd_number)
{
    char expected_label[32];
    char volume_name[32];
    char drives[512];
    DWORD length;
    unsigned char found = 0;

    length = GetLogicalDriveStringsA(sizeof(drives), drives);
    if (length == 0) {
        return 0;
    }
    for (DWORD index = 0; index < length; ++index) {
        char drive[4];

        if (drives[index] == '\0') {
            continue;
        }
        drive[0] = drives[index];
        drive[1] = drives[index + 1];
        drive[2] = drives[index + 2];
        drive[3] = '\0';
        index += 2;

        if (GetDriveTypeA(drive) != DRIVE_CDROM) {
            continue;
        }
        strcpy(path, drive);
        sprintf(expected_label, "WIZ8_%d", cd_number);

        UINT previous_mode = SetErrorMode(1);
        if (GetVolumeInformationA(
                path, volume_name, 32, 0, 0, 0, 0, 0) != 0
            && _stricmp(expected_label, volume_name) == 0) {
            found = 1;
            g_cd_index_00604474 = cd_number;
        }
        SetErrorMode(previous_mode);
        if (found != 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0042b740
char Function42B740(int saved_level)
{
    int level = NormalizeMasterFunctionValue004D9700(saved_level);
    if (level >= 0 && level < 48) {
        return g_level_folders[level].unknown_6a;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0042b3e0
void Function42B3E0(void)
{
    W8World* world = GetWorld659AB8();
    if (world != 0) {
        Forward44FAF0(world);
        SetWorld659AB8(0);
        g_loaded_sky_index_00604470 = 0xff;
        ResetEnvironment();
    }
}
/* As in Bink.cpp and PathAI.cpp: retail shares the no-op stub at 0x004023A0
   across arities (this body passes the sky world and two zero arguments), so
   this overload only satisfies the local call and owns no separate address. */
void NoOp(W8World* world, int first, int second)
{
    (void)world;
    (void)first;
    (void)second;
}

/* Load or replace the sky world for a level. A level whose sky index already
   matches the world held in g_world_659ab8 reuses it; a level with no sky
   (-1) only tears the previous one down. The replacement sky receives the
   animated-cloud material: its cloud meshes are marked for the renderer's
   control bits and every CloudsN prop's mesh chain is rebound to it. */
// FUNCTION: WIZ8 0x0042B020
unsigned char LoadSkyWorld0042B020(int level, W8LevelInfo* info)
{
    signed char sky_index;
    W8LevelInfo local_info;
    W8World* sky_world;

    if (level < 47) {
        sky_index = g_level_folders[level].sky_index;
    }
    else {
        sky_index = 0;
    }
    if (sky_index == g_loaded_sky_index_00604470) {
        return 1;
    }
    if (info == 0) {
        if (!LevelBuildInfoByID(level, &local_info)) {
            return 0;
        }
        info = &local_info;
    }
    if (GetWorld659AB8() != 0) {
        Forward44FAF0(GetWorld659AB8());
        SetWorld659AB8(0);
        g_loaded_sky_index_00604470 = 0xff;
        ResetEnvironment();
    }
    if (sky_index == -1) {
        return 1;
    }
    sky_world = CreateWorld();
    g_world_659ab8 = sky_world;
    if (sky_world == 0) {
        return 0;
    }
    if (!LoadWorld(sky_world, info->sky_file_name, info->sky_folder,
                   info->sky_bitmap_folder, 0)) {
        return 0;
    }

    srVector3T<double> position;
    position.SetZero();
    static_cast<srNode*>(sky_world->camera)->setLocation(position);

    stMaterial* material = new stMaterial;
    if (material == 0) {
        srAssertFail("pMat", LEVELS_CPP, 899, 0);
    }
    material->setName("AnimatedCloudMaterial");
    material->autoRelease();
    srVector4T<float> colour;
    colour.Set(1.0f, 1.0f, 1.0f, 1.0f);
    material->setAmbient(colour);
    colour.Set(0.0f, 0.0f, 0.0f, 0.0f);
    material->setDiffuse(colour);
    material->setSpecular(colour);
    material->parms_18.shininess = 1.0f;
    material->parms_18.diffuse.w = 1.0f;
    material->parms_18.emissive = 0.0f;
    material->dirty_74 = 1;
    material->m_field_78 = 0;
    material->setMapper(&g_material_mapper_00659738);

    for (srNode* node = sky_world->level->firstChild();
         node != 0; node = node->nextSibling()) {
        if (node->getClassID() == 0x10004) {
            srModelInstance* instance = static_cast<srModelInstance*>(node);
            srMeshModel* mesh = static_cast<srMeshModel*>(instance->model());

            mesh->control_state_394 |= 0x20;
            if ((mesh->control_state_390 & 8) == 0) {
                unsigned long state = mesh->control_state_390;
                mesh->control_state_390 = state | 8;
                mesh->control_state_390 = state | 8;
            }
            mesh->control_state_394 |= 0x10;
            if ((mesh->control_state_390 & 8) == 0) {
                unsigned long state = mesh->control_state_390;
                mesh->control_state_390 = state | 8;
                mesh->control_state_390 = state | 8;
            }
        }
    }

    for (int index = 1; index < 9; ++index) {
        char prop_name[28];

        sprintf(prop_name, "Clouds0%d", index);
        W8Prop* prop = FindPropByName(sky_world, prop_name);
        if (prop != 0) {
            srModelInstance* instance = prop->ToggleRepAnimation(0);

            if (instance != 0) {
                for (stMeshModel* mesh =
                         static_cast<stMeshModel*>(instance->model());
                     mesh != 0; mesh = mesh->next) {
                    mesh->setMaterial(
                        material, 0, static_cast<srMeshModel::e_side>(0));
                }
            }
        }
    }

    ForwardThroughMember3C_46E750(GetWorld659AB8(), 0);
    ForwardThroughMember3C_46E640(GetWorld659AB8(), 1);
    NoOp(sky_world, 0, 0);
    WorldRemoveLight(sky_world, sky_world->camera_light);
    sky_world->camera_light = 0;
    g_loaded_sky_index_00604470 = sky_index;
    return 1;
}

extern void Function5817D0(void);
extern float Function420BD0(const srVector3T<float>* position, unsigned char* hit);
extern float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);

extern void Function482410(void);
extern void Function4D6C50(int level);
extern void Function50E700(void);
extern void Function5777C0(void);

extern float g_runtime_world_scale_6081e8;

// GLOBAL: WIZ8 0x00605820
unsigned short g_level_name_indices_605820[47] = {
    0x6f7, 0x6f8, 0x6f9, 0x6fa, 0x6fb, 0x6fd, 0x6fc, 0x6fe, 0x6ff, 0x700,
    0x701, 0x719, 0x702, 0x703, 0x704, 0x705, 0x706, 0x707, 0x708, 0x709,
    0x70a, 0x70b, 0x70c, 0x719, 0x70d, 0x70e, 0x70f, 0x710, 0x719, 0x711,
    0x719, 0x712, 0x713, 0x714, 0x719, 0x715, 0x716, 0x717, 0x706, 0x718,
    0x719, 0x719, 0x719, 0x719, 0x719, 0x719, 0x719,
};

// GLOBAL
unsigned char g_flag_00659756;

// GLOBAL: WIZ8 0x00603ac8
float g_default_world_height_00603ac8 = 1000.0f;

// GLOBAL: WIZ8 0x005ebfdc
float g_position_height_epsilon_005ebfdc = 2500.0f;

// GLOBAL: WIZ8 0x00603ad0
unsigned char g_environment_load_flag_00603ad0 = 1;

// GLOBAL: WIZ8 0x0065ba70
unsigned char g_level_runtime_flag_0065ba70;

// GLOBAL: WIZ8 0x0068f0fd
unsigned char g_value_0068f0fd;

// GLOBAL: WIZ8 0x006850b0
int g_value_006850b0;

// GLOBAL: WIZ8 0x00687607
unsigned char g_flag_00687607;

// GLOBAL: WIZ8 0x006059E0
char g_ambient_sound_filename_006059e0[] = "SCF";
// GLOBAL: WIZ8 0x00605880
const char* g_sky_names_00605880[] = {
    "DefaultSky", "RapaxSky",   "MountainPassSky", "RiftSky1",
    "TrynnieSky1", "TrynnieSky2", "RatkinSky1",       "CampSky",
    "BluffSky1",   "BluffSky2",   "CircleSky",
};

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
    int level = NormalizeMasterFunctionValue004D9700(requested_level);
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
    ReleaseRetainedMaterials00489920();
    Function427440();
    SetCurrentWorld(CreateWorld());

    previous_level = g_status_685170.current_level;
    g_status_685170.current_level = level;
    sprintf(
        music_path, "Data\\Music\\%s.MPL",
        g_level_folders[level].folder_name);
    if (FileExists(music_path)) {
        sprintf(
            music_path, "%s.MPL",
            g_level_folders[g_status_685170.current_level].folder_name);
        Function48FC10(music_path, 1, 1);
    }
    else {
        Function48FC10("", 1, 1);
    }
    Function48F9E0();

    if (!LoadSkyWorld0042B020(level, &level_info)) {
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
        g_status_685170.current_level = previous_level;
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
                rotation.vectors[0].Set(1.0, 0.0, 0.0);
                rotation.vectors[1].Set(0.0, 1.0, 0.0);
                rotation.vectors[2].Set(0.0, 0.0, 1.0);
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

            position.Set(0.0f, g_default_world_height_00603ac8, 0.0f);
            SetWorldScenePosition004511D0(GetWorld(), &position);
        }
    }
    else {
        MoveWorldToPoint(
            GetWorld(), GetWorld659AB8(), &g_status_685170.pending_move_location.point);
    }

    if (level < 47 && !g_status_685170.level_progress[level].visited) {
        ResetMonsterGroupTurnState();
        Function5115B0();
        g_status_685170.level_progress[level].visited = 1;
        first_visit = 1;
    }

    sprintf(
        path, "%s\\%s\\%s", level_info.level_folder,
        level_info.level_file_name, g_ambient_sound_filename_006059e0);
    LoadAmbientSoundList0047AB40(path);
    if (!g_environment_load_flag_00603ad0) {
        ResetCurrentEnvironment0041AA40();
    }
    g_level_runtime_flag_0065ba70 = 0;
    Function482410();
    Function4D6C50(level);
    RebindNpcLevelTriggers0050AC60();
    SetWorldCursorNodesVisible0048ED70(g_value_0068f0fd);
    Function50E700();

    if (!restoring_game) {
        if (level < 47) {
            ResetNpcBindingsForParty0050DB50();
            ClearPendingNpcLevelFlags0050C270();
            ReleaseNpcMonsterBindings0050C2E0();
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
            ResetAndRefreshAllSight005060C0();
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
    UpdateWorldMeshAfterLoad00451020();
    ReleaseReadMeshScratch004881D0();
    return 1;
}

// FUNCTION: WIZ8 0x0042ACE0
unsigned char UnloadLevel(const char* save_directory)
{
    if (gXStatus.fCombatMode != 0) {
        Function4EA310(1);
    }

    if (g_status_685170.current_level < 47) {
        g_status_685170.level_progress[g_status_685170.current_level].sight_clock = g_status_685170.world_clock;
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        RenderFrame();
    }

    ReleaseMarkedNpcBindings0050DA00();
    if (strcmp(save_directory, "") != 0) {
        SaveLevelStatus("Saves\\CurrentGame.SAV");
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        RenderFrame();
    }

    if (gXStatus.plsMonsterList != 0) {
        if (ReleaseItemLists() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            RenderFrame();
        }
        if (ShutdownMonsterManager() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            RenderFrame();
        }
        if (ScreenLifecycleSuccess() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            RenderFrame();
        }
    }

    ReleaseWorldCursorNodes0048DB30();
    ClearSearchables005171B0();
    if (g_world_cleanup_flag_00659757 != 0) {
        RenderFrame();
    }

    ReleaseWorldCursor004909C0();
    DisableSky();
    W8World* world = GetWorld();
    if (world != 0) {
        Forward44FAF0(world);
        SetCurrentWorld(0);
    }
    ReleaseRetainedMaterials00489920();

    if (g_world_cleanup_flag_00659757 != 0) {
        RenderFrame();
    }

    DisableSky();
    g_status_685170.current_level = -1;
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
