#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srNode.h"
#include "surrender/srScene.h"

#include "FileMan.h"

/*
 * Engine Code\3dapi.cpp.
 *
 * The thin layer the rest of the engine calls the renderer through. Most of
 * what is here forwards straight on, which is what makes the file a layer
 * rather than an implementation.
 */

#define THREE_D_API_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3dapi.cpp"

extern void SetRendererReady(void);
extern void Function421090(const float* location);
extern void SetValue60DFAC(void);
extern unsigned char g_renderer_ready_00607d7c;
extern void Function46DC90(srScene* scene);
extern void* ReadGameData00447570(const char* path, void* parent);
extern unsigned char InitializeGameData004497C0(void* game_data);
extern int CheckLevelAssetSet0042CCC0(const char* level_path);
extern void UpdateWorldMeshFromQuads004BAD40(W8World* world);
extern void UpdateWorldMeshFromOctree004BAF50(W8World* world);
extern void Function449BB0(void* owner);
extern void Function426790(void);
extern void Function443A60(W8World* world);
extern void Function479030(void);
extern void Function47A700(void* ambient_sound);
extern void Function4A4210(W8World* world);
extern void Function4AC3D0(W8World* world);
extern void Function46E4A0(W8World* world);
extern unsigned char Function4914C0(void);
extern void Function490B90(void);
extern unsigned char g_world_cleanup_flag_00659757;
extern W8GrowableVector<W8World*> g_worlds_00659a80;
extern void Function46DE40(W8World* world);

// FUNCTION: WIZ8 0x00450B10
void ConstructWorldCollections(W8World* world)
{
    world->plsMonsters = PListCreate();
    world->plsItems = PListCreate();
    world->plsProps = PListCreate();
    world->plsCameras = PListCreate();
    world->plsAmbientSounds = PListCreate();
    world->lights_to_update = new W8LightVector;
    world->collidable_props = new W8GrowableVector<W8Prop*>;
    world->monster_generators = new W8GrowableVector<W8MonsterGenerator*>;
    world->m_vector_b4 = new W8GrowableVector<W8VectorElement005EC280*>;
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
   their order, inputs, and rollback-visible world fields. */
// FUNCTION: WIZ8 0x0044F5F0
unsigned char LoadWorld(
    W8World* world, char* level_file_name, const char* level_folder,
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
    sprintf(game_data_path, "%s\\%s.wgd", asset_folder, level_file_name);
    sprintf(oct_path, "%s\\%s.oct", asset_folder, level_file_name);
    sprintf(material_path, "%s\\%s.mat", asset_folder, level_file_name);
    sprintf(material_folder, "%s", asset_folder);

    g_worlds_00659a80.Add(world);

    world->m_loaded = 1;
    PListInit(&world->m_list_09c);
    PListInit(&world->m_list_0a8);
    world->m_owned_04c = 0;

    if (CheckLevelAssetSet0042CCC0(oct_path) >= 0) {
        world->octree = new W8Octree(oct_path, &world->m_owned_04c);
        if (world->octree != 0 && world->octree->HasLoadError()) {
            delete world->octree;
            world->octree = 0;
        }
        else if (world->octree != 0) {
            use_octree = 0;
        }
    }

    if (game_data_path[0] != '\0' && world->m_owned_04c == 0) {
        world->m_owned_04c = ReadGameData00447570(game_data_path, 0);
        if (world->m_owned_04c != 0 &&
            InitializeGameData004497C0(world->m_owned_04c) == 0) {
            return 0;
        }
    }

    Function46DC90(world->static_scene);
    world->camera = CreateOrSetGameCamera(world->static_scene, 0);
    world->camera_light = CreateWorldLight0046E140(world, "CameraLight");
    world->camera_light->m_direction_60.x = 0.0f;
    world->camera_light->m_direction_60.y = 0.0f;
    world->camera_light->m_direction_60.z = 0.0f;
    world->camera_light->m_color_6c.x = 1.0f;
    world->camera_light->m_color_6c.y = 0.85f;
    world->camera_light->m_color_6c.z = 0.39f;
    world->camera_light->m_position_78.x = 0.0f;
    world->camera_light->m_position_78.y = 0.0f;
    world->camera_light->m_position_78.z = 0.0f;
    world->camera_light->m_positional_98 = 1.0f;
    world->camera_light->setGroupMask(2);
    ConfigureWorldLight0046E300(world->camera_light, 4000.0f);

    if (world->octree == 0 || world->octree->GetMeshCount() == 0) {
        world->psrMeshes = 0;
    }
    else {
        unsigned long mesh_count = world->octree->GetMeshCount();
        world->psrMeshes = static_cast<srModelInstance**>(
            malloc((mesh_count + 1) * sizeof(srModelInstance*)));
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
    memset(world->m_positional_07c, 0, 0x10);

    int handle = FileOpen(
        level_path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    if (handle == 0) {
        srAssertFail("hFile", THREE_D_API_CPP, 0x1d8,
                     "Could not open level file.");
    }
    unsigned char success =
        ReadLevel(world, handle, use_octree, material_folder);
    CloseVirtualFile(handle);
    if (success == 0) {
        srAssertFail("fSuccess", THREE_D_API_CPP, 0x1dd,
                     "Problem loading level, please check files and versions.");
    }
    /* ReadLevel failure is assertion-only in the canonical body. LoadWorld
       still performs the mesh update and returns success. */

    world->m_positional_0d4[0] = 0;
    if (world->octree != 0) {
        UpdateWorldMeshFromOctree004BAF50(world);
    }
    else if (world->m_owned_06c != 0) {
        world->m_owned_06c->dirty = 1;
        UpdateWorldMeshFromQuads004BAD40(world);
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

    world->static_scene = new W8Scene(0);
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

    world->dynamic_scene = new W8Node005EC208(world->static_scene);
    if (world->dynamic_scene == 0) {
        delete world->static_scene;
        delete world->level;
        free(world);
        return 0;
    }
    world->dynamic_scene->setName("Sir-Tech Dynamic Scene");
    Function46DC90(world->static_scene);
    ConstructWorldCollections(world);
    world->environment_range_end_018 = 1.0f;
    world->m_positional_01c = 1.0f;
    world->environment_range_start_014 = 0.75f;
    return world;
}

// SYNTHETIC: WIZ8 0x0044F3D0
// W8Node005EC208::`scalar deleting destructor'
W8Node005EC208::~W8Node005EC208()
{
}

/* Detach the item meshes in every registered world before the renderer-side
   resource transition. */
// FUNCTION: WIZ8 0x0044f5b0
void DetachAllWorldItems(void)
{
    int count = g_worlds_00659a80.GetCount();

    for (int index = 0; index < count; ++index) {
        Function46DE40(*g_worlds_00659a80.GetAt(index));
    }
}

// FUNCTION: WIZ8 0x004507A0
void DestroyWorldCollections(W8World* world)
{
    if (world == 0) {
        return;
    }

    if (world->plsMonsters != 0) {
        while (PListGetCount(world->plsMonsters) != 0) {
            W8Monster* object = static_cast<W8Monster*>(
                PListGetAt(world->plsMonsters, 0));
            PListRemoveAt(world->plsMonsters, 0);
            delete object;
        }
        PListDestroy(world->plsMonsters);
        world->plsMonsters = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    if (world->plsItems != 0) {
        while (PListGetCount(world->plsItems) != 0) {
            W8Item* object = static_cast<W8Item*>(
                PListGetAt(world->plsItems, 0));
            PListRemoveAt(world->plsItems, 0);
            delete object;
        }
        PListDestroy(world->plsItems);
        world->plsItems = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    if (world->plsProps != 0) {
        while (PListGetCount(world->plsProps) != 0) {
            W8Prop* object = static_cast<W8Prop*>(
                PListGetAt(world->plsProps, 0));
            PListRemoveAt(world->plsProps, 0);
            delete object;
        }
        PListDestroy(world->plsProps);
        world->plsProps = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();

    Function443A60(world);
    if (world->triggers != 0) {
        delete world->triggers;
        world->triggers = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();

    if (world->plsCameras != 0) {
        while (PListGetCount(world->plsCameras) != 0) {
            W8WorldCameraEntry* entry = static_cast<W8WorldCameraEntry*>(
                PListGetAt(world->plsCameras, 0));
            PListRemoveAt(world->plsCameras, 0);
            DestroyPathAI004A9810(entry->path);
            free(entry);
        }
        PListDestroy(world->plsCameras);
        world->plsCameras = 0;
    }

    Function479030();
    if (world->plsAmbientSounds != 0) {
        while (PListGetCount(world->plsAmbientSounds) != 0) {
            void* ambient_sound = PListGetAt(world->plsAmbientSounds, 0);
            PListRemoveAt(world->plsAmbientSounds, 0);
            Function47A700(ambient_sound);
        }
        PListDestroy(world->plsAmbientSounds);
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
            operator delete(*world->named_positions->GetAt(0));
            world->named_positions->RemoveAt(0);
        }
    }

    Function4A4210(world);
    Function4AC3D0(world);
    Function46E4A0(world);
    PListFreeData(&world->m_list_0a8);
    PListFreeData(&world->m_list_09c);

    delete world->lights_to_update;
    delete world->collidable_props;
    delete world->monster_generators;
    delete world->m_vector_b4;
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
        Function449BB0(world->m_owned_04c);
        operator delete(world->m_owned_04c);
        world->m_owned_04c = 0;
    }
    if (world->octree != 0) {
        delete world->octree;
        world->octree = 0;
    }
    if (world->m_owned_06c != 0) {
        DestroyWorldQuad004BE0A0(world->m_owned_06c);
        world->m_owned_06c = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    DestroyWorldCollections(world);
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    if (Function4914C0() != 0) Function490B90();

    index = g_worlds_00659a80.IndexOf(world);
    if (index >= 0) {
        g_worlds_00659a80.RemoveAt(index);
    }
    if (world->static_scene != 0) {
        world->static_scene->release();
        world->static_scene = 0;
    }
    free(world);
}

/* Note that the renderer is up. Eight bytes and no branch. */
// FUNCTION: WIZ8 0x00451010
void MarkRendererReady(void)
{
    g_renderer_ready_00607d7c = 1;
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
void WorldGetCameraLocation00451160(W8World* world, W8Position* location)
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
    location->x = 0.0f;
    location->y = 0.0f;
    location->z = 0.0f;
}

/* Set the view position after loading or traversing a portal. The camera light
   follows the camera, while only the camera move publishes the new game-space
   position. */
// FUNCTION: WIZ8 0x004511D0
void SetWorldScenePosition004511D0(
    W8World* world, const W8Position* location)
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
        render_position.x = position.x;
        render_position.y = position.y;
        render_position.z = position.z;
        static_cast<srNode*>(world->camera)->setLocation(render_position);
        Function421090(&position.x);
    }
    if (world->camera_light != 0) {
        render_position.x = position.x;
        render_position.y = position.y;
        render_position.z = position.z;
        static_cast<srNode*>(world->camera_light)->setLocation(render_position);
    }
}

// FUNCTION: WIZ8 0x00451110
unsigned char ForwardLoadWorld(
    W8World* world, char* level_file_name, const char* level_folder,
    const char* asset_folder, unsigned char use_octree)
{
    return LoadWorld(
        world, level_file_name, level_folder, asset_folder, use_octree);
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
        Function421090(location);
    }
    if (world->camera_light != 0) {
        position.x = location[0];
        position.y = location[1];
        position.z = location[2];
        ((srNode*)world->camera_light)->setLocation(position);
    }
}

/* Read the camera node's double-precision renderer position back into the
   world's float position type. */
// FUNCTION: WIZ8 0x00450750
void WorldGetCameraLocation(W8World* world, W8Position* location)
{
    location->x = (float)world->camera->getLocationX();
    location->y = (float)world->camera->getLocationY();
    location->z = (float)world->camera->getLocationZ();
}

// SYNTHETIC: WIZ8 0x00423e50
// srMaterial::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423e80
// W8Camera::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423eb0
// W8Scene::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423f00
// W8ColorSurface::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00424a50
// W8MeshModel005EBE98::`scalar deleting destructor'

/* The second world, read straight out of the global with no guard. Its type is
   settled by the viewport, which reads a camera member through the same
   object. */
// FUNCTION: WIZ8 0x004512a0
W8World* GetWorld659AB8(void)
{
    return g_world_659ab8;
}
