#pragma once

#include "wiz8/layouts/world.h"

class srLight;
class stLight;
class srNode;
class stParticle;
class Trigger;
class W8Prop;

unsigned char TraceToBounds(void* eye, const float* lower, const float* upper);

W8World* GetWorld(void);
W8World* GetWorld659AB8(void);
void MarkRendererReady(void);

W8World* CreateWorld();
unsigned char LoadWorld(W8World* world, char* level_file_name, const char* level_folder,
                        const char* asset_folder, unsigned char use_octree);
unsigned char ForwardLoadWorld(W8World* world, char* level_file_name, const char* level_folder,
                               const char* asset_folder, unsigned char use_octree);
void Forward44FAF0(W8World* world);
void SetCurrentWorld(W8World* world);
void ConstructWorldCollections(W8World* world);
void DestroyWorldCollections(W8World* world);
void DestroyWorld(W8World* world);
void DetachAllWorldItems(void);
void UpdateWorlds0044F400(void);
void UpdateWorldMeshAfterLoad00451020(void);
void UpdateWorld0044F4E0(W8World* world);
void GetWorldCameraState(W8World* world, W8WorldCameraState* state);
void SetWorldCameraState(W8World* world, W8World* source_world, W8WorldCameraState* state);
void RestoreWorldCameraState(W8World* world, W8World* source_world, W8WorldCameraState* state);
void WorldGetCameraRotation(W8World* world, srMatrix3T<float>* rotation);
void WorldGetCameraLocation(W8World* world, srVector3T<float>* location);
void WorldGetCameraLocation00451160(W8World* world, srVector3T<float>* location);
void SetWorldScenePosition004511D0(W8World* world, const srVector3T<float>* position);
stParticle* FindParticleByName(W8World* world, const char* name);
unsigned char FindEntityByName(const char* name, srVector3T<float>* position, int* location_id,
                               srVector3T<float>* direction);
void SetWorld659AB8(W8World* world);

void Function44E830(W8World* world, int handle);
void Function48F280(W8World* world, const char* name, int active);
