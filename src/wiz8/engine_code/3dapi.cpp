#include <cstdlib>
#include <cstring>

#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/sr_api.h"
#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srNode.h"
#include "surrender/srScene.h"

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
extern void Function44F5F0(int a, int b, int c, int d, int e);
extern unsigned char g_renderer_ready_00607d7c;
extern void Function46DC90(srScene* scene);
extern void Function449BB0(void* owner);
extern void Function4BE0A0(void* owner);
extern void Function426790(void);
extern void Function443A60(W8World* world);
extern void Function479030(void);
extern void Function47A700(void* ambient_sound);
extern void Function4A9810(void* camera_owner);
extern void Function4A4210(W8World* world);
extern void Function4AC3D0(W8World* world);
extern void Function46E4A0(W8World* world);
extern unsigned char Function4914C0(void);
extern void Function490B90(void);
extern unsigned char g_world_cleanup_flag_00659757;
extern int g_world_count_00659a84;
extern W8World** g_worlds_00659a8c;

namespace {
struct W8WorldCameraEntry {
    unsigned char positional_00[0x18];
    void* owner_18;
};

} // namespace

// FUNCTION: WIZ8 0x00450B10
void ConstructWorldCollections(W8World* world)
{
    world->plsMonsters = PListCreate();
    world->plsItems = PListCreate();
    world->plsProps = PListCreate();
    world->plsCameras = PListCreate();
    world->plsAmbientSounds = PListCreate();
    world->lights_to_update = new W8LightVector;
    world->collidable_props = new W8GrowableVector<W8CollidableProp*>;
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

// FUNCTION: WIZ8 0x0044F1C0
W8World* CreateWorld()
{
    W8World* world = static_cast<W8World*>(malloc(sizeof(W8World)));
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0xa0, 0);
    }
    memset(world, 0, sizeof(*world));

    world->static_scene = new W8Scene005EBE48(0);
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
    world->m_positional_018 = 1.0f;
    world->m_positional_01c = 1.0f;
    world->m_positional_014 = 0.75f;
    return world;
}

// SYNTHETIC: WIZ8 0x0044F3D0
// W8Node005EC208::`scalar deleting destructor'
W8Node005EC208::~W8Node005EC208()
{
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
            W8Prop005EC1E0* object = static_cast<W8Prop005EC1E0*>(
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
            Function4A9810(entry->owner_18);
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
        Function4BE0A0(world->m_owned_06c);
        world->m_owned_06c = 0;
    }
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    DestroyWorldCollections(world);
    if (g_world_cleanup_flag_00659757 != 0) Function426790();
    if (Function4914C0() != 0) Function490B90();

    for (index = 0; index < g_world_count_00659a84; ++index) {
        if (g_worlds_00659a8c[index] == world) {
            for (; index < g_world_count_00659a84 - 1; ++index) {
                g_worlds_00659a8c[index] = g_worlds_00659a8c[index + 1];
            }
            --g_world_count_00659a84;
            break;
        }
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

// FUNCTION: WIZ8 0x00451110
void Forward44F5F0(int a, int b, int c, int d, int e)
{
    Function44F5F0(a, b, c, d, e);
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
    world->camera->getRotation(rotation);
}

/* Moves the camera and the sky together. Only the camera's move notifies the
   level, which is what separates the two nodes: the sky follows the view but
   nothing else depends on where it is. The pair of doubles is rebuilt for each
   call rather than shared, so the two moves are independent statements. */
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
        ((srNode*)world->camera)->setLocation(position);
        Function421090(location);
    }
    if (world->sky_node != 0) {
        position.x = location[0];
        position.y = location[1];
        ((srNode*)world->sky_node)->setLocation(position);
    }
}

// SYNTHETIC: WIZ8 0x00423e50
// srMaterial::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423e80
// W8Camera005EBE14::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423eb0
// W8Scene005EBE48::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00423f00
// srColorSurface::`scalar deleting destructor'
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
