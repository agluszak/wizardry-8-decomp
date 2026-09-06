#pragma once

#include "surrender/srMath.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"

class srCamera;
class srLight;
class srModelInstance;
class srNode;
class srScene;
class stLevel;
class W8Octree;
unsigned char TraceToBounds(void* eye, const float* lower, const float* upper);
struct W8Quad;
class W8Missile;
class W8SpellVisual;
class W8MonsterGenerator;
class stLight;
class W8Prop;
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
    srVector3T<float> position;
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
    float environment_range_start_014;
    float environment_range_end_018;
    float m_positional_01c;
    float view_distance_020;
    float environment_intensity_024;
    unsigned char m_positional_028[4];
    EnvironmentColour environment_colour_02c;
    stLevel* level;
    srScene* static_scene;
    srNode* dynamic_scene;
    srCamera* camera;
    srModelInstance** psrMeshes;
    void* m_owned_04c;
    W8Octree* octree;
    srLight* camera_light;
    unsigned char m_positional_058[0x11];
    unsigned char m_loaded;
    unsigned char m_padding_06a[2];
    W8Quad* m_owned_06c;
    srModelInstance* update_mesh_source;
    float value_74;
    float value_78;
    unsigned char m_positional_07c[0x20];
    W8PList m_list_09c;
    W8PList m_list_0a8;
    W8GrowableVector<W8SpellVisual*>* spell_visuals;
    W8GrowableVector<W8Missile*>* missiles;
    W8GrowableVector<stLight*>* lights_to_update;
    W8GrowableVector<W8Prop*>* collidable_props;
    W8GrowableVector<W8MonsterGenerator*>* monster_generators;
    W8GrowableVector<Trigger*>* triggers;
    W8GrowableVector<stParticle*>* particles;
    W8GrowableVector<W8NamedPosition*>* named_positions;
    unsigned char m_positional_0d4[8];

};

extern "C" {
extern W8World* g_world;
extern W8World* g_world_659ab8;
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
W8World* GetWorld(void);
W8World* GetWorld659AB8(void);
void MarkRendererReady(void);
void WorldUpdateProps(W8World* world);
}

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
void DetachAllWorldItems(void);
void WorldUpdateLights(W8World* world);
float WorldGetValue78(W8World* world);
double WorldGetFarClip(W8World* world);
void WorldGetCameraRotation(W8World* world, srMatrix3T<float>* rotation);
void WorldGetCameraLocation(W8World* world, srVector3T<float>* location);
void WorldGetCameraLocation00451160(
    W8World* world, srVector3T<float>* location);
void SetWorldScenePosition004511D0(
    W8World* world, const srVector3T<float>* position);
stParticle* FindParticleByName(W8World* world, const char* name);
stLight* CreateLight0046DF90(srNode* parent, const char* name);
stLight* CreateWorldLight0046E030(W8World* world, const char* name);
stLight* CreateWorldLight0046E140(W8World* world, const char* name);
void ConfigureWorldLight0046E300(srLight* light, float range);
void WorldRemoveLight(W8World* world, srNode* light); /* 0x0046E250 */

static_assert(sizeof(W8World) == 0xdc, "W8World_must_be_0xdc");
