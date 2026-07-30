#pragma once

#include "surrender/srMath.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

class srCamera;
class srLight;
class srModelInstance;
class srNode;
class stLevel;
class W8Octree;
class W8Node005EC208;
class W8Scene005EBE48;
class W8Missile;
class W8MonsterGenerator;
class stLight;
class W8VectorElement005EC280;
class W8CollidableProp;
class Trigger;
class stParticle;
struct W8PathAI;
struct W8NamedPosition {
    W8NamedPosition()
    {
        name[0] = '\0';
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 0.0f;
        value_08c = 0.0f;
        value_090 = 0.0f;
        value_094 = 0.0f;
        value_098 = 0.0f;
    }

    char name[0x80];
    W8Position position;
    float value_08c;
    float value_090;
    float value_094;
    float value_098;
};

static_assert(sizeof(W8NamedPosition) == 0x9c,
              "W8NamedPosition_must_be_0x9c");

struct W8WorldCameraEntry {
    unsigned char positional_00[0x14];
    unsigned char positional_14[4];
    W8PathAI* path;
};

static_assert(sizeof(W8WorldCameraEntry) == 0x1c,
              "W8WorldCameraEntry_must_be_0x1c");

/* Engine Code\3dapi.cpp. CreateWorld allocates and zeroes exactly 0xdc bytes;
   the list/vector setup and teardown routines prove the owned fields below. */
struct W8World {
    W8PList* plsMonsters;
    W8PList* plsItems;
    W8PList* plsProps;
    W8PList* plsCameras;
    W8PList* plsAmbientSounds;
    float m_positional_014;
    float m_positional_018;
    float m_positional_01c;
    unsigned char m_positional_020[0x18];
    stLevel* level;
    W8Scene005EBE48* static_scene;
    W8Node005EC208* dynamic_scene;
    srCamera* camera;
    void** psrMeshes;
    void* m_owned_04c;
    W8Octree* octree;
    srLight* camera_light;
    unsigned char m_positional_058[0x11];
    unsigned char m_loaded;
    unsigned char m_padding_06a[2];
    void* m_owned_06c;
    srModelInstance* update_mesh_source;
    float value_74;
    float value_78;
    unsigned char m_positional_07c[0x20];
    W8PList m_list_09c;
    W8PList m_list_0a8;
    W8GrowableVector<W8VectorElement005EC280*>* m_vector_b4;
    W8GrowableVector<W8Missile*>* missiles;
    W8LightVector* lights_to_update;
    W8GrowableVector<W8CollidableProp*>* collidable_props;
    W8GrowableVector<W8MonsterGenerator*>* monster_generators;
    W8GrowableVector<Trigger*>* triggers;
    W8GrowableVector<stParticle*>* particles;
    W8GrowableVector<W8NamedPosition*>* named_positions;
    unsigned char m_positional_0d4[8];

};

extern "C" W8World* g_world;

W8World* CreateWorld();
unsigned char LoadWorld(
    W8World* world, char* level_file_name, const char* level_folder,
    const char* asset_folder, unsigned char use_octree);
unsigned char ForwardLoadWorld(
    W8World* world, char* level_file_name, const char* level_folder,
    const char* asset_folder, unsigned char use_octree);
void Forward44FAF0(W8World* world);
void SetCurrentWorld(W8World* world);
void ConstructWorldCollections(W8World* world);
void DestroyWorldCollections(W8World* world);
void DestroyWorld(W8World* world);
float WorldGetValue78(W8World* world);
double WorldGetFarClip(W8World* world);
void WorldGetCameraRotation(W8World* world, srMatrix3T<float>* rotation);
void SetWorldScenePosition004511D0(
    W8World* world, const W8Position* position);
stParticle* FindParticleByName(W8World* world, const char* name);

static_assert(sizeof(W8World) == 0xdc, "W8World_must_be_0xdc");
