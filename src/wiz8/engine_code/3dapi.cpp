#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/GDFileIO.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/monster_generators.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "wiz8/engine_code/Level.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/float_constants.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sgp_bridge.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/UpdateMesh.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/startup_world.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "wiz8/world_cursor.h"
#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srNode.h"
#include "surrender/srCamera.h"
#include "surrender/srScene.h"

#include "FileMan.h"
#include "input.h"
#include "soundman.h"

/*
 * Engine Code\3dapi.cpp.
 *
 * The thin layer the rest of the engine calls the renderer through. Most of
 * what is here forwards straight on, which is what makes the file a layer
 * rather than an implementation.
 */

#define THREE_D_API_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3dapi.cpp"

// GLOBAL: WIZ8 0x00607d7c
bool g_renderer_ready = true;

// GLOBAL: WIZ8 0x00607d80
int g_game_data_runtime_pending = 1;

// GLOBAL: WIZ8 0x005ec240
const double g_double_005ec240 = 250000.0;

class W8AmbientSound;

// GLOBAL: WIZ8 0x00659757
bool g_world_cleanup_flag;
// GLOBAL: WIZ8 0x00659a80
W8GrowableVector<W8World*> g_worlds;

// GLOBAL: WIZ8 0x006081f8
bool g_navigator_vertical_enabled = true;

// GLOBAL: WIZ8 0x00607d7d
bool g_world_mesh_update_enabled = true;

// GLOBAL: WIZ8 0x00609c88
float g_float_00609c88 = 60.0f;

// GLOBAL: WIZ8 0x00609c8c
bool g_flag_00609c8c = true;

// GLOBAL: WIZ8 0x006f0530
bool g_shift_held;
// GLOBAL: WIZ8 0x006f0531
bool g_monster_combat_timer_enabled;
// GLOBAL: WIZ8 0x006f0534
bool g_modifier_held;

// FUNCTION: WIZ8 0x00450B10
void ConstructWorldCollections(W8World* world)
{
    world->plsMonsters = PLCreate();
    world->plsItems = PLCreate();
    world->plsProps = PLCreate();
    world->plsCameras = PLCreate();
    world->plsAmbientSounds = PLCreate();
    world->lights_to_update = new W8GrowableVector<stLight*>;
    world->collidable_props = new W8GrowableVector<W8Prop*>;
    world->monster_generators = new W8GrowableVector<MonGen*>;
    world->spell_visuals = new W8GrowableVector<W8SpellVisual*>;
    world->missiles = new W8GrowableVector<W8Missile*>;
    world->triggers = new W8GrowableVector<Trigger*>;
    world->particles = new W8GrowableVector<stParticle*>;
    world->named_positions = new W8GrowableVector<W8NamedPosition*>;

    if (world->plsMonsters == 0) {
        srAssertFail("pWorld->plsMonsters", THREE_D_API_CPP, 0x57e,
                     "Out of memory creating plsMonsters.");
    }
    if (world->plsItems == 0) {
        srAssertFail("pWorld->plsItems", THREE_D_API_CPP, 0x57f,
                     "Out of memory creating plsItems.");
    }
    if (world->plsProps == 0) {
        srAssertFail("pWorld->plsProps", THREE_D_API_CPP, 0x580,
                     "Out of memory creating plsProps.");
    }
    if (world->plsCameras == 0) {
        srAssertFail("pWorld->plsCameras", THREE_D_API_CPP, 0x581,
                     "Out of memory creating plsCameras.");
    }
    if (world->plsAmbientSounds == 0) {
        srAssertFail("pWorld->plsAmbientSounds", THREE_D_API_CPP, 0x582,
                     "Out of memory creating plsAmbientSounds.");
    }
    if (world->lights_to_update == 0) {
        srAssertFail("pWorld->plsLightsToUpdate", THREE_D_API_CPP, 0x583,
                     "Out of memory creating plsLightsToUpdate.");
    }
    if (world->collidable_props == 0) {
        srAssertFail("pWorld->plsCollidableProps", THREE_D_API_CPP, 0x584,
                     "Out of memory creating plsCollidableProps.");
    }
    if (world->monster_generators == 0) {
        srAssertFail("pWorld->plsMonsterGenerators", THREE_D_API_CPP, 0x585,
                     "Out of memory creating plsMonsterGenerators.");
    }
    if (world->missiles == 0) {
        srAssertFail("pWorld->plsMissiles", THREE_D_API_CPP, 0x588,
                     "Out of memory creating plsMissiles.");
    }
    if (world->triggers == 0) {
        srAssertFail("pWorld->plsTriggers", THREE_D_API_CPP, 0x589,
                     "Out of memory creating plsTriggers.");
    }
    if (world->particles == 0) {
        srAssertFail("pWorld->plsParticles", THREE_D_API_CPP, 0x58a,
                     "Out of memory creating plsParticles.");
    }
    if (world->named_positions == 0) {
        srAssertFail("pWorld->plsNamedPositions", THREE_D_API_CPP, 0x58b,
                     "Out of memory creating plsNamedPositions.");
    }
}

/* Load the renderer-facing half of one level. The subordinate OCT, game-data,
   and ReadLevel parsers remain their original owners; this routine establishes
   their order, inputs, and rollback-visible world  */
// FUNCTION: WIZ8 0x0044F5F0
unsigned char LoadWorld(W8World* world, char* level_file_name, const char* level_folder,
                        const char* asset_folder, unsigned char use_octree)
{
    char extension[4];
    char level_path[1024];
    char pvl_path[1024];
    char oct_path[1024];
    char game_data_path[1024];
    char material_path[1024];
    char material_folder[1024];

    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x15f, 0);
    }

    SetValue60DFAC();
    sprintf(level_path, "%s\\%s", level_folder, level_file_name);
    strcpy(extension, level_file_name + strlen(level_file_name) - 3);
    level_file_name[strlen(level_file_name) - 4] = '\0';

    strcpy(pvl_path, level_path);
    pvl_path[strlen(pvl_path) - 3] = 'p';
    sprintf(game_data_path, "%s\\%s.wgd", level_folder, level_file_name);
    sprintf(oct_path, "%s\\%s.oct", level_folder, level_file_name);
    sprintf(material_path, "%s\\%s.mat", level_folder, level_file_name);
    sprintf(material_folder, "%s", asset_folder);

    g_worlds.Add(world);

    world->m_loaded = true;
    PListInit(&world->m_list_09c);
    PListInit(&world->m_lights_0a8);
    world->m_owned_04c = 0;

    if (CheckLevelAssetSet(oct_path) >= 0) {
        world->octree = new W8Octree(oct_path, &world->m_owned_04c);
        if (world->octree != 0 && world->octree->HasLoadError()) {
            delete world->octree;
            world->octree = 0;
        } else if (world->octree != 0) {
            use_octree = 0;
        }
    }

    if (game_data_path[0] != '\0' && world->m_owned_04c == 0) {
        world->m_owned_04c = ReadGameData00447570(game_data_path, false);
        if (world->m_owned_04c != 0 && InitializeGameData(world->m_owned_04c) == 0) {
            return 0;
        }
    }

    SetSceneAmbientLightWhite(world->static_scene);
    world->camera = CreateOrSetGameCamera(world->static_scene, 0);
    world->camera_light = CreateWorldLight0046E140(world, "CameraLight");
    world->camera_light->ambient_198.SetZero();
    world->camera_light->diffuse_1a4.Set(1.0f, 0.85f, 0.39f);
    world->camera_light->specular_1b0.SetZero();
    world->camera_light->intensity_1d0 = 1.0f;
    world->camera_light->setGroupMask(2);
    ConfigureWorldLight(world->camera_light, 4000.0f);

    if (world->octree == 0 || world->octree->GetMeshCount() == 0) {
        world->psrMeshes = 0;
    } else {
        unsigned long mesh_count = world->octree->GetMeshCount();
        world->psrMeshes =
            static_cast<srModelInstance**>(malloc((mesh_count + 1) * sizeof(srModelInstance*)));
        if (world->psrMeshes == 0) {
            srAssertFail("pWorld->psrMeshes", THREE_D_API_CPP, 0x1be,
                         "LoadWorld: Couldn't allocate psrMeshes.");
        }
        for (unsigned long index = 0; index <= mesh_count; ++index) {
            world->psrMeshes[index] = 0;
        }
        strcpy(level_path, pvl_path);
    }

    world->m_owned_06c = 0;
    world->update_mesh_source = 0;
    memset(world->m_padding_07c, 0, 0x10);

    int handle = FileOpen(level_path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    if (handle == 0) {
        srAssertFail("hFile", THREE_D_API_CPP, 0x1d8, "Could not open level file.");
    }
    unsigned char success = ReadLevel(world, handle, use_octree, material_folder);
    FileClose(handle);
    if (success == 0) {
        srAssertFail("fSuccess", THREE_D_API_CPP, 0x1dd,
                     "Problem loading level, please check files and versions.");
    }
    /* ReadLevel failure is assertion-only in the canonical body. LoadWorld
       still performs the mesh update and returns success. */

    world->m_padding_0d4[0] = 0;
    if (world->octree != 0) {
        UpdateWorldOctree(world);
    } else if (world->m_owned_06c != 0) {
        world->m_owned_06c->dirty = 1;
        UpdateWorldMeshFromQuads(world);
    }
    return 1;
}

// FUNCTION: WIZ8 0x0044F1C0
W8World* CreateWorld()
{
    W8World* world = static_cast<W8World*>(malloc(sizeof(W8World)));
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0xa0, 0);
    }
    memset(world, 0, sizeof(*world));

    world->static_scene = SR_NEW(srScene)(static_cast<srNode*>(0));
    if (world->static_scene == 0) {
        free(world);
        return 0;
    }
    world->static_scene->setName("Sir-Tech Scene Static Scene");

    world->level = new stLevel(world->static_scene);
    if (world->level == 0) {
        delete world->static_scene;
        free(world);
        return 0;
    }
    world->level->setName("Sir-Tech Level");
    world->level->m_active = 1;

    world->dynamic_scene = SR_NEW(srNode)(world->static_scene);
    if (world->dynamic_scene == 0) {
        delete world->static_scene;
        delete world->level;
        free(world);
        return 0;
    }
    world->dynamic_scene->setName("Sir-Tech Dynamic Scene");
    SetSceneAmbientLightWhite(world->static_scene);
    ConstructWorldCollections(world);
    world->environment_range_end_018 = 1.0f;
    world->environment_range_blue_01c = 1.0f;
    world->environment_range_start_014 = 0.75f;
    return world;
}

// FUNCTION: WIZ8 0x0044F400
void UpdateWorlds(void)
{
    g_navigator_vertical_enabled =
        !(gfKeyState[0x11] != 0 && g_combat_state != 0 &&
          (g_combat_state->round_active_001 != 0 || gXStatus.fPartyMovementMode != 0));

    {
        int count = g_worlds.GetCount();
        for (int index = 0; index < count; ++index) {
            UpdateWorld(*g_worlds.GetAt(index));
        }
    }

    if (g_renderer_ready != 0 && g_world_mesh_update_enabled != 0) {
        if (g_world->octree != 0) {
            UpdateWorldOctree(g_world);
        } else if (g_world->m_owned_06c != 0) {
            ++g_world->m_owned_06c->dirty;
            UpdateWorldMeshFromQuads(g_world);
        }
        RepositionAmbientSounds(g_world);
        RequestRefreshPartyState();
        g_renderer_ready = false;
    }

    UpdateTimedTriggerEvents();
    UpdateShakeEffects();
    UpdateEnvironment();
    UpdateNpcEvents();
}

// FUNCTION: WIZ8 0x0044F4E0
void UpdateWorld(W8World* world)
{
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x106, 0);
        CreateTriggerShakeEvent(0x6a4, 70.0f, 5000.0f, 1);
    }

    if (world != g_world_659ab8) {
        UpdateWorldMonsters(world);
        UpdateWorldMissiles(world);
        UpdateWorldSpellVisuals(world);
        UpdateAmbientSounds(world);
        WorldUpdateLights(world);
        RunMonsterGenerators();
        UpdateWorldTriggers(world);
        if (world->octree != 0) {
            RunMasterFunctions();
        }
        UpdateSpellEffects();
    }

    W8PList* nodes = &world->m_list_09c;
    int count = static_cast<short>(PLLength(nodes));
    for (int index = 0; index < count; ++index) {
        srNode* node = static_cast<srNode*>(PLGet(nodes, index));
        node->setFlag(srNode::FLAG_DISABLE);
    }
    PListClear(nodes);
}

// VTABLE: WIZ8 0x005EC208
// class srClientSupport<srNode,4096>

// TEMPLATE: WIZ8 0x004519D0
// srClientSupport<srNode,4096>::getClassID

// TEMPLATE: WIZ8 0x004519F0
// srClientSupport<srNode,4096>::clone

// SYNTHETIC: WIZ8 0x0044F3D0
// srClientSupport<srNode,4096>::`scalar deleting destructor'

/* Detach the item meshes in every registered world before the renderer-side
   resource transition. */
// FUNCTION: WIZ8 0x0044f5b0
void DetachAllWorldItems(void)
{
    int count = g_worlds.GetCount();

    for (int index = 0; index < count; ++index) {
        DetachWorldItemMeshes(*g_worlds.GetAt(index));
    }
}

// GLOBAL: WIZ8 0x005ec1f8
const double g_double_005ec1f8 = 3.141592653589793;
// GLOBAL: WIZ8 0x005ebce8
const double g_double_005ebce8 = 180.0;
// GLOBAL: WIZ8 0x00607d84
float g_camera_base_horizontal_fov = 85.0f;
// GLOBAL: WIZ8 0x00607d88
float g_camera_base_vertical_fov = 71.0f;
// GLOBAL: WIZ8 0x00659abc
float g_camera_sway_horizontal_phase;
// GLOBAL: WIZ8 0x00659ac0
float g_camera_sway_vertical_phase;

/* Drives the swaying camera view. A positive mode captures the camera's
   field of view in degrees and enters the mode, a negative mode restores the
   captured view and leaves it, and zero only advances the sway while it is
   active: both phases run in degrees per second, wrap at a full turn and
   offset the view plane through sine. */
// FUNCTION: WIZ8 0x00450080
void SetCameraSwayMode(srCamera* camera, int mode)
{
    float elapsed = MoveTimer(2);

    if (mode != 0) {
        if (mode > 0) {
            g_camera_base_horizontal_fov =
                (float)(camera->getHorizontalFOV() * (g_double_005ebce8 / g_double_005ec1f8));
            g_camera_base_vertical_fov =
                (float)(camera->getVerticalFOV() * (g_double_005ebce8 / g_double_005ec1f8));
            if (!g_camera_sway_active) {
                g_camera_sway_horizontal_phase = 0.0f;
                g_camera_sway_vertical_phase = 0.0f;
                BeginCameraSway();
            }
        } else {
            double radians = g_double_005ec1f8 * g_float_005ebcf8;
            camera->setViewPlane(radians * g_camera_base_horizontal_fov,
                                 radians * g_camera_base_vertical_fov);
            EndCameraSway();
            return;
        }
    }
    if (!g_camera_sway_active) {
        return;
    }

    g_camera_sway_horizontal_phase += elapsed * 12.0f;
    g_camera_sway_vertical_phase += elapsed * 11.0f;
    if (g_camera_sway_horizontal_phase > 360.0f) {
        g_camera_sway_horizontal_phase -= 360.0f;
    }
    if (g_camera_sway_vertical_phase > 360.0f) {
        g_camera_sway_vertical_phase -= 360.0f;
    }

    double radians = g_double_005ec1f8 * g_float_005ebcf8;
    camera->setViewPlane(
        (sin(radians * g_camera_sway_horizontal_phase) * 0.75 + g_camera_base_horizontal_fov) *
            radians,
        (sin(radians * g_camera_sway_vertical_phase) * 0.63 + g_camera_base_vertical_fov) *
            radians);
}

// FUNCTION: WIZ8 0x004507A0
void DestroyWorldCollections(W8World* world)
{
    if (world == 0) {
        return;
    }

    if (world->plsMonsters != 0) {
        while (PLLength(world->plsMonsters) != 0) {
            W8Monster* object = static_cast<W8Monster*>(PLGet(world->plsMonsters, 0));
            PLRemoveAt(world->plsMonsters, 0);
            delete object;
        }
        PLDestroy(world->plsMonsters);
        world->plsMonsters = 0;
    }
    if (g_world_cleanup_flag != 0)
        RenderFrame();
    if (world->plsItems != 0) {
        while (PLLength(world->plsItems) != 0) {
            W8Item* object = static_cast<W8Item*>(PLGet(world->plsItems, 0));
            PLRemoveAt(world->plsItems, 0);
            delete object;
        }
        PLDestroy(world->plsItems);
        world->plsItems = 0;
    }
    if (g_world_cleanup_flag != 0)
        RenderFrame();
    if (world->plsProps != 0) {
        while (PLLength(world->plsProps) != 0) {
            W8Prop* object = static_cast<W8Prop*>(PLGet(world->plsProps, 0));
            PLRemoveAt(world->plsProps, 0);
            delete object;
        }
        PLDestroy(world->plsProps);
        world->plsProps = 0;
    }
    if (g_world_cleanup_flag != 0)
        RenderFrame();

    DestroyAllWorldTriggers(world);
    if (world->triggers != 0) {
        delete world->triggers;
        world->triggers = 0;
    }
    if (g_world_cleanup_flag != 0)
        RenderFrame();

    if (world->plsCameras != 0) {
        while (PLLength(world->plsCameras) != 0) {
            W8WorldCameraEntry* entry =
                static_cast<W8WorldCameraEntry*>(PLGet(world->plsCameras, 0));
            PLRemoveAt(world->plsCameras, 0);
            DestroyPathAI(entry->path);
            free(entry);
        }
        PLDestroy(world->plsCameras);
        world->plsCameras = 0;
    }

    StopAllAmbientSounds();
    if (world->plsAmbientSounds != 0) {
        while (PLLength(world->plsAmbientSounds) != 0) {
            W8AmbientSound* ambient_sound =
                static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, 0));
            PLRemoveAt(world->plsAmbientSounds, 0);
            DestroyAmbientSound(ambient_sound);
        }
        PLDestroy(world->plsAmbientSounds);
        world->plsAmbientSounds = 0;
    }

    if (world->particles != 0) {
        while (world->particles->GetCount() != 0) {
            (*world->particles->GetAt(0))->release();
            world->particles->RemoveAt(0);
        }
    }
    if (world->named_positions != 0) {
        while (world->named_positions->GetCount() != 0) {
            delete *world->named_positions->GetAt(0);
            world->named_positions->RemoveAt(0);
        }
    }

    DestroyAllMissiles(world);
    DestroyAllSpellVisuals(world);
    DestroyWorldLights(world);
    PListFreeData(&world->m_lights_0a8);
    PListFreeData(&world->m_list_09c);

    delete world->lights_to_update;
    delete world->collidable_props;
    delete world->monster_generators;
    delete world->spell_visuals;
    delete world->missiles;
    delete world->particles;
    delete world->named_positions;
}

// FUNCTION: WIZ8 0x0044FAF0
void DestroyWorld(W8World* world)
{
    int index;

    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x1ff, 0);
    }
    if (world->psrMeshes != 0) {
        free(world->psrMeshes);
        world->psrMeshes = 0;
    }
    if (world->update_mesh_source != 0) {
        world->update_mesh_source = 0;
    }
    if (world->m_owned_04c != 0) {
        delete world->m_owned_04c;
        world->m_owned_04c = 0;
    }
    if (world->octree != 0) {
        delete world->octree;
        world->octree = 0;
    }
    if (world->m_owned_06c != 0) {
        DestroyWorldQuad(world->m_owned_06c);
        world->m_owned_06c = 0;
    }
    if (g_world_cleanup_flag != 0)
        RenderFrame();
    DestroyWorldCollections(world);
    if (g_world_cleanup_flag != 0)
        RenderFrame();
    if (IsWorldCursorVisible() != 0)
        HideWorldCursor();

    index = g_worlds.IndexOf(world);
    if (index >= 0) {
        g_worlds.RemoveAt(index);
    }
    if (world->static_scene != 0) {
        world->static_scene->release();
        world->static_scene = 0;
    }
    free(world);
}

/* Per-frame camera / camera-path update for one world. Main-game frames the
   primary world, then the sky world with flag 0x40 so the motion pass is
   skipped there. Function-local statics keep the last refresh position and the
   last applied rotation; their atexit thunks are at 0x00450070 / 0x00450060. */
// FUNCTION: WIZ8 0x0044FC20
void UpdateWorldCameraAndPaths(W8World* world, unsigned int flags)
{
    static srVector3T<float> s_last_automap_refresh_position;
    static srMatrix3T<float> s_saved_camera_rotation;
    srVector3T<float> camera_position;
    srVector3T<float> navigator_position;
    srVector3T<float> delta;
    srVector3T<double> render_position;
    srMatrix3T<float> rotation;
    srMatrix3T<float> path_rotation;
    srMatrix3T<float> motion_saved;
    W8CameraPath* camera_path;
    W8PathAI* path;
    char sound_environment;
    char sound_environment_secondary;
    float yaw;
    float pitch;
    int camera_count;
    int index;
    float dx;
    float dy;
    float dz;

    RefreshDirtyAutomap();
    GetCameraPosition(&camera_position);
    if (g_game_data_runtime_pending != 0 && world->m_owned_04c != 0) {
        UpdateGameDataRuntime();
        g_game_data_runtime_pending = 0;
    }
    if (world->m_owned_04c != 0) {
        if (g_level_flags != 0) {
            *g_level_flags &= ~0x200u;
        }
        if (world->m_owned_04c != 0 && (flags & 0x40) == 0) {
            world->camera->getRotation(rotation);
            world->m_owned_04c->ApplyCameraMotionFlags(flags, &rotation, &motion_saved);
            world->camera->setRotation(rotation);
            s_saved_camera_rotation = rotation;
            yaw = GetCameraYawInDegrees();
            pitch = GetCameraPitchInDegrees();
            g_startup_world->SetAngles(yaw);
            g_startup_world->SetPitch(pitch);
            if (world->m_owned_04c->ApplyCameraMotion(flags, &camera_position, &delta,
                                                              &motion_saved) != 0) {
                camera_position.x += delta.x;
                camera_position.y += delta.y;
                camera_position.z += delta.z;
                navigator_position.x = camera_position.x;
                navigator_position.y = camera_position.y - g_default_world_height;
                navigator_position.z = camera_position.z;
                if (world->camera_light != 0) {
                    render_position.x = camera_position.x;
                    render_position.y = camera_position.y;
                    render_position.z = camera_position.z;
                    static_cast<srNode*>(world->camera_light)->setLocation(render_position);
                }
                render_position.x = camera_position.x;
                render_position.y = camera_position.y;
                render_position.z = camera_position.z;
                static_cast<srNode*>(world->camera)->setLocation(render_position);
                g_startup_world->SetPositionInternal(&navigator_position);
                dx = camera_position.x - s_last_automap_refresh_position.x;
                dy = camera_position.y - s_last_automap_refresh_position.y;
                dz = camera_position.z - s_last_automap_refresh_position.z;
                if (dx * dx + dy * dy + dz * dz > g_double_005ec240) {
                    s_last_automap_refresh_position = camera_position;
                    camera_position.y -= g_default_world_height;
                    if (AutomapHasCellAt(&camera_position) != 0) {
                        SetWorldMeshVertexLightTable(g_world, 1);
                        UpdateAutomapBounds();
                        SetWorldMeshVertexLightTable(g_world, 0);
                    }
                }
                DispatchWorldCursorNodeCommand(0, 0, 0);
                GetLevelSoundEnvironment(&sound_environment, &sound_environment_secondary);
                if (sound_environment >= 0) {
                    Sound3DSetEnvironment(sound_environment);
                }
            }
        } else {
            world->camera->setRotation(s_saved_camera_rotation);
        }
    } else {
        world->camera->setRotation(s_saved_camera_rotation);
    }
    SetCameraSwayMode(world->camera, 0);
    camera_count = static_cast<int>(PLLength(world->plsCameras));
    if (world->plsCameras != 0 && camera_count != 0) {
        for (index = 0; index < camera_count; ++index) {
            camera_path = static_cast<W8CameraPath*>(PLGet(world->plsCameras, index));
            if (camera_path != 0 && camera_path->active_14 != 0) {
                path = camera_path->path_18;
                PathAITick(path, 1);
                PathAIApply(path, world->camera);
                {
                    srVector3T<double> location = world->camera->getLocation();
                    srVector3T<float> party_point;
                    party_point.x = static_cast<float>(location.x);
                    party_point.y = static_cast<float>(location.y);
                    party_point.z = static_cast<float>(location.z);
                    PlacePartyAtPoint(&party_point);
                }
                world->camera->getRotation(path_rotation);
                if (g_world_659ab8 != 0 && g_world_659ab8->camera != 0) {
                    g_world_659ab8->camera->setRotation(path_rotation);
                }
                ApplyCameraRotation(&path_rotation);
                if (path->discrete_mode_1c != 0) {
                    if (path->position >= path->nodes_0c->GetCount() - g_float_005ebb38) {
                        UpdateCameraPathState(world, camera_path, 0);
                    }
                } else if (path->position >= g_double_005ebc30) {
                    UpdateCameraPathState(world, camera_path, 0);
                }
            }
        }
    }
}

// SYNTHETIC: WIZ8 0x00450060
// `dynamic atexit destructor for 's_saved_camera_rotation''
// SYNTHETIC: WIZ8 0x00450070
// `dynamic atexit destructor for 's_last_automap_refresh_position''

/* Note that the renderer is up. Eight bytes and no branch. */
// FUNCTION: WIZ8 0x00451010
void MarkRendererReady(void)
{
    g_renderer_ready = true;
}

/* Resolve a particle by its runtime name from the current world's particle
   vector. Names are case-insensitive in the original registry-facing lookup. */
// FUNCTION: WIZ8 0x00451080
stParticle* FindParticleByName(W8World* world, const char* name)
{
    if (world != 0 && world->particles != 0) {
        int count = world->particles->GetCount();

        for (int index = 0; index < count; ++index) {
            stParticle* particle = *world->particles->GetAt(index);

            if (_stricmp(particle->getName(), name) == 0) {
                return particle;
            }
        }
    }
    return 0;
}

/* Two forwarders that pass their arguments through unchanged. */
// FUNCTION: WIZ8 0x00451140
void Forward44FAF0(W8World* world)
{
    DestroyWorld(world);
}

/* Read the camera position when one is attached to the world. The sole
   caller preserves this value across a level reload, so a world without a
   camera contributes the zero position. */
// FUNCTION: WIZ8 0x00451160
void WorldGetCameraLocation00451160(W8World* world, srVector3T<float>* location)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 1014, 0);
    }
    if (world->camera != 0) {
        location->x = (float)world->camera->getLocationX();
        location->y = (float)world->camera->getLocationY();
        location->z = (float)world->camera->getLocationZ();
        return;
    }
    location->SetZero();
}

/* Set the view position after loading or traversing a portal. The camera light
   follows the camera, while only the camera move publishes the new game-space
   position. */
// FUNCTION: WIZ8 0x004511D0
void SetWorldScenePosition(W8World* world, const srVector3T<float>* location)
{
    srVector3T<float> position;
    srVector3T<double> render_position;

    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 1043, 0);
    }

    position.x = location->x;
    position.y = location->y;
    position.z = location->z;
    if (world->camera != 0) {
        render_position.SetFromFloat(&position);
        static_cast<srNode*>(world->camera)->setLocation(render_position);
        PlacePartyAtPoint(&position);
    }
    if (world->camera_light != 0) {
        render_position.SetFromFloat(&position);
        static_cast<srNode*>(world->camera_light)->setLocation(render_position);
    }
}

// FUNCTION: WIZ8 0x00451020
void UpdateWorldMeshAfterLoad(void)
{
    if (g_renderer_ready != 0 && g_world_mesh_update_enabled != 0) {
        if (g_world->octree != 0) {
            UpdateWorldOctree(g_world);
        } else if (g_world->m_owned_06c != 0) {
            ++g_world->m_owned_06c->dirty;
            UpdateWorldMeshFromQuads(g_world);
        }
        RepositionAmbientSounds(g_world);
        RequestRefreshPartyState();
        g_renderer_ready = false;
    }
}

// FUNCTION: WIZ8 0x00451110
unsigned char ForwardLoadWorld(W8World* world, char* level_file_name, const char* level_folder,
                               const char* asset_folder, unsigned char use_octree)
{
    return LoadWorld(world, level_file_name, level_folder, asset_folder, use_octree);
}

/* Report a failed assertion with no message of its own, so the expression and
   the site are all the caller has to give. */
// FUNCTION: WIZ8 0x00450780
void ReportAssertion(const char* expression, const char* source_path, long line)
{
    srAssertFail(expression, source_path, line, 0);
}

/* The camera's rotation basis, read straight back out of the renderer node.
   Both assertions belong to this body: 3dapi.cpp:975 names the world and
   3dapi.cpp:976 names its camera member pWorld->psrCamera, which is what puts
   the camera at 0x44 rather than anywhere else. */
// FUNCTION: WIZ8 0x004503c0
void WorldGetCameraRotation(W8World* world, srMatrix3T<float>* rotation)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x3cf, 0);
    }
    if (!world->camera) {
        srAssertFail("pWorld->psrCamera", THREE_D_API_CPP, 0x3d0, 0);
    }
    world->camera->getRotation(*rotation);
}

/* Moves the camera and its camera light together. Only the camera's move
   notifies the level; the light simply follows the view. */
// FUNCTION: WIZ8 0x00450420
void WorldSetCameraLocation(W8World* world, const float* location)
{
    srVector3T<double> position;

    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x422, 0);
    }
    if (world->camera != 0) {
        position.x = location[0];
        position.y = location[1];
        position.z = location[2];
        ((srNode*)world->camera)->setLocation(position);
        PlacePartyAtPoint(reinterpret_cast<const srVector3T<float>*>(location));
    }
    if (world->camera_light != 0) {
        position.x = location[0];
        position.y = location[1];
        position.z = location[2];
        ((srNode*)world->camera_light)->setLocation(position);
    }
}

/* Apply a CamPos record to the world's camera and camera light. A non-null
   source world repeats the orientation write; automap restore passes null and
   recall / LoadLevel pass GetWorld659AB8(). */
// FUNCTION: WIZ8 0x004504B0
void SetWorldCameraState(W8World* world, W8World* source_world, W8WorldCameraState* state)
{
    srVector3T<float> location;
    srVector3T<double> position;
    srMatrix3T<float> rotation;

    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x43f, 0);
    } else {
        if (state == 0) {
            srAssertFail("CamPos", THREE_D_API_CPP, 0x444, 0);
        }
        location.x = state->position.x;
        location.y = state->position.y;
        location.z = state->position.z;
        if (world->camera != 0) {
            position.x = location.x;
            position.y = location.y;
            position.z = location.z;
            ((srNode*)world->camera)->setLocation(position);
            PlacePartyAtPoint(&location);
        }
        if (world->camera_light != 0) {
            position.x = location.x;
            position.y = location.y;
            position.z = location.z;
            ((srNode*)world->camera_light)->setLocation(position);
        }
        world->camera->getRotation(rotation);
        SetCameraOrientation(state->yaw, state->pitch, &rotation);
        world->camera->setRotation(rotation);
    }
    if (source_world != 0) {
        if (state == 0) {
            srAssertFail("CamPos", THREE_D_API_CPP, 0x44e, 0);
        }
        world->camera->getRotation(rotation);
        SetCameraOrientation(state->yaw, state->pitch, &rotation);
        world->camera->setRotation(rotation);
    }
}

/* Reinstall a CamPos record, then copy the resulting camera pose onto the
   startup navigator in degrees with the default world height removed. */
// FUNCTION: WIZ8 0x00450610
void RestoreWorldCameraState(W8World* world, W8World* source_world, W8WorldCameraState* state)
{
    srVector3T<double> camera_location;
    srVector3T<float> navigator_position;

    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x46b, 0);
    }
    SetWorldCameraState(world, source_world, state);
    world->camera->getLocation(camera_location);
    navigator_position.x = (float)camera_location.x;
    navigator_position.y = (float)camera_location.y;
    navigator_position.z = (float)camera_location.z;
    g_startup_world->SetAngles(GetCameraYawInDegrees());
    g_startup_world->SetPitch(GetCameraPitchInDegrees());
    navigator_position.y -= g_default_world_height;
    g_startup_world->SetPositionInternal(&navigator_position);
}

/* Snapshot the world's camera location and the live yaw/pitch into a CamPos
   record. A null world is a no-op; a world without a camera zeros the point. */
// FUNCTION: WIZ8 0x004506C0
void GetWorldCameraState(W8World* world, W8WorldCameraState* state)
{
    if (world != 0) {
        if (state == 0) {
            srAssertFail("CamPos", THREE_D_API_CPP, 0x488, 0);
        }
        if (world->camera != 0) {
            state->position.x = (float)world->camera->getLocationX();
            state->position.y = (float)world->camera->getLocationY();
            state->position.z = (float)world->camera->getLocationZ();
        } else {
            state->position.SetZero();
        }
        GetCameraOrientation(state->yaw, state->pitch);
    }
}

/* Read the camera node's double-precision renderer position back into the
   world's float position type. */
// FUNCTION: WIZ8 0x00450750
void WorldGetCameraLocation(W8World* world, srVector3T<float>* location)
{
    location->x = (float)world->camera->getLocationX();
    location->y = (float)world->camera->getLocationY();
    location->z = (float)world->camera->getLocationZ();
}

/* The second world, read straight out of the global with no guard. Its type is
   settled by the viewport, which reads a camera member through the same
   object. */
// FUNCTION: WIZ8 0x004512a0
W8World* GetWorld659AB8(void)
{
    return g_world_659ab8;
}

/* Runtime/debug adjustment dispatcher for the active world's clipping and
   environment controls. Bits not handled here are intentionally ignored. */
// FUNCTION: WIZ8 0x00450210
void ApplyWorldUpdateFlags(W8World* world, unsigned int flags)
{
    float scale;

    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x321, 0);
    }
    if ((flags & 1) != 0) {
        WorldSetFarClip(world,
                        static_cast<float>(WorldGetFarClip(world)) + g_position_height_epsilon);
        scale = 2.0f;
        if (WorldGetFarClip(world) >= 50000.0f) {
            scale = 1.5f;
        }
        WorldSetRenderRange(world, static_cast<float>(WorldGetFarClip(world)) * scale);
    }
    if ((flags & 2) != 0) {
        WorldSetFarClip(world,
                        static_cast<float>(WorldGetFarClip(world)) - g_position_height_epsilon);
        scale = 2.0f;
        if (WorldGetFarClip(world) >= 50000.0f) {
            scale = 1.5f;
        }
        WorldSetRenderRange(world, static_cast<float>(WorldGetFarClip(world)) * scale);
    }
    if ((flags & 4) != 0 && g_float_00609c88 < 180.0f) {
        g_float_00609c88 += g_float_005ebc88;
    }
    if ((flags & 8) != 0 && g_float_00609c88 > g_float_005ebc88) {
        g_float_00609c88 -= g_float_005ebc88;
    }
    if ((flags & 0x10) != 0) {
        g_flag_00609c8c = true;
    }
    if ((flags & 0x40) != 0) {
        world->m_loaded = world->m_loaded == 0;
    }
    if ((flags & 0x100) != 0) {
        SetWorldEnvironmentValue(g_world, GetWorldValue24(g_world) + 0.02f);
    }
    if ((flags & 0x200) != 0) {
        SetWorldEnvironmentValue(g_world, GetWorldValue24(g_world) - 0.02f);
    }
}

// FUNCTION: WIZ8 0x004503B0
int ForwardSelectedPropIndex(W8World*, int, int)
{
    return GetSelectedPropIndex();
}

// FUNCTION: WIZ8 0x00451150
bool ForwardActivateSelectedProp(W8World*, int, int, int)
{
    return ActivateSelectedProp();
}

// FUNCTION: WIZ8 0x00451100
W8World* ForwardCreateWorld(void)
{
    return CreateWorld();
}

/* Resolve the PARTY alias or a named position stored with the current world. */
// FUNCTION: WIZ8 0x004512C0
bool FindEntityByName(const char* name, srVector3T<float>* position, float* angle,
                      srVector3T<float>* direction)
{
    if (name == 0) {
        return false;
    }
    if (_stricmp("PARTY", name) == 0) {
        GetCameraPosition(position);
        return true;
    }

    int count = g_world->named_positions->GetCount();
    for (int index = 0; index < count; ++index) {
        W8NamedPosition* entry = *g_world->named_positions->GetAt(index);
        if (_stricmp(name, entry->name) == 0) {
            *position = entry->position;
            if (angle != 0) {
                *angle = entry->angle;
            }
            if (direction != 0) {
                direction->x = entry->direction_090.x;
                direction->y = entry->direction_090.y;
                direction->z = entry->direction_090.z;
            }
            return true;
        }
    }
    return false;
}

/* Push a candidate position clear of static geometry, then reject it when it
   overlaps another world item or monster. */
// FUNCTION: WIZ8 0x00451390
unsigned char AdjustWorldCollisionPosition(float radius, srVector3T<float>* position,
                                           unsigned char check_items, unsigned char check_monsters)
{
    float threshold = radius * g_path_endpoint_scale;
    float angle = 0.0f;

    do {
        srVector3T<float> first(0.0f, 0.0f, radius);
        first.RotateAboutY(sin(angle), cos(angle));
        first += *position;

        double opposite_angle = angle + g_camera_pi;
        srVector3T<float> second(0.0f, 0.0f, radius);
        second.RotateAboutY(sin(opposite_angle), cos(opposite_angle));
        second += *position;

        g_octree->TraceLineOfSight(position, &first, 1, -3, -3, 1, 0);
        g_octree->TraceLineOfSight(position, &second, 1, -3, -3, 1, 0);

        first -= *position;
        second -= *position;
        float first_distance = first.Length();
        if (first_distance < threshold && second.Length() < threshold) {
            return 0;
        }
        if (first_distance < threshold) {
            first.SetLength(radius - first_distance);
            *position -= first;
        } else {
            second.SetLength(radius - second.Length());
            *position -= second;
        }
        angle += 0.7853981256484985f;
    } while (angle < g_camera_pi);

    if (check_items != 0) {
        W8WorldItem* item = GetNextWorldItem(1);
        while (item != 0) {
            W8Item* owner = item->p3D;
            if (owner != 0) {
                srVector3T<float> center;
                float other_radius;
                owner->GetSearchPosition(&center);
                owner->GetBoundsRadius(&other_radius);
                srVector3T<float> delta(center.x - position->x, center.y - position->y,
                                        center.z - position->z);
                if (delta.Length() < other_radius + radius) {
                    return 0;
                }
            }
            item = GetNextWorldItem(0);
        }
    }

    if (check_monsters != 0) {
        W8MonsterInfo* info = GetNextMonsterInfo(1);
        while (info != 0) {
            W8Monster* monster = info->p3D;
            if (monster != 0) {
                srVector3T<float> center;
                float other_radius;
                monster->GetAnimationCenter(&center);
                monster->GetAnimationRadius(&other_radius);
                srVector3T<float> delta(center.x - position->x, center.y - position->y,
                                        center.z - position->z);
                if (delta.Length() < other_radius + radius) {
                    return 0;
                }
            }
            info = GetNextMonsterInfo(0);
        }
    }
    return 1;
}

/* Try the requested point first, then the two retail half-turn probe points
   around it; only an unobstructed probe is handed to the overlap resolver. */
// FUNCTION: WIZ8 0x00451800
unsigned char FindNearbyFreePosition(float radius, srVector3T<float>* position,
                                     unsigned char check_items, unsigned char check_monsters)
{
    srVector3T<float> candidate = *position;
    if (AdjustWorldCollisionPosition(radius, &candidate, check_items, check_monsters)) {
        *position = candidate;
        return 1;
    }

    float angle = 0.0f;
    float diameter = radius + radius;
    do {
        candidate.x = diameter * sin(angle) + position->x;
        candidate.y = position->y;
        candidate.z = diameter * cos(angle) + position->z;
        srVector3T<float> unblocked = candidate;

        g_octree->TraceLineOfSight(position, &candidate, 1, -3, -3, 1, 0);
        if (candidate == unblocked &&
            AdjustWorldCollisionPosition(radius, &candidate, check_items, check_monsters)) {
            *position = candidate;
            return 1;
        }
        angle += g_float_005ec2a8;
    } while (angle < g_camera_pi);
    return 0;
}
