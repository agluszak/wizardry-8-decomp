#pragma once

#include "wiz8/3d_code/PList.h"
#include "wiz8/vector.h"

class srCamera;
class srNode;
class stLevel;
class W8Octree;
class W8Node005EC208;
class W8Scene005EBE48;
class W8Missile;
class W8MonsterGenerator;
class W8VectorElement005EC294;
class W8VectorElement005EC280;
class W8CollidableProp;
class Trigger;
class stParticle;
class W8NamedPosition;
struct W8UpdateMeshSource;

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
    srNode* sky_node;
    unsigned char m_positional_058[0x11];
    unsigned char m_loaded;
    unsigned char m_padding_06a[2];
    void* m_owned_06c;
    W8UpdateMeshSource* update_mesh_source;
    float value_74;
    float value_78;
    unsigned char m_positional_07c[0x20];
    W8PList m_list_09c;
    W8PList m_list_0a8;
    W8GrowableVector<W8VectorElement005EC280*>* m_vector_b4;
    W8GrowableVector<W8Missile*>* missiles;
    W8GrowableVector<W8VectorElement005EC294*>* lights_to_update;
    W8GrowableVector<W8CollidableProp*>* collidable_props;
    W8GrowableVector<W8MonsterGenerator*>* monster_generators;
    W8GrowableVector<Trigger*>* triggers;
    W8GrowableVector<stParticle*>* particles;
    W8GrowableVector<W8NamedPosition*>* named_positions;
    unsigned char m_positional_0d4[8];

};

W8World* CreateWorld();
void ConstructWorldCollections(W8World* world);
void DestroyWorldCollections(W8World* world);
void DestroyWorld(W8World* world);

static_assert(sizeof(W8World) == 0xdc, "W8World_must_be_0xdc");
