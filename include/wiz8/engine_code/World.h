#pragma once

#include "wiz8/layouts/world.h"

class srNode;
class stParticle;
class Trigger;
class W8Prop;

W8World* GetWorld(void);
W8World* GetWorld659AB8(void);
void MarkRendererReady(void);

W8World* CreateWorld();
W8World* ForwardCreateWorld00451100(void);
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
bool FindEntityByName(const char* name, srVector3T<float>* position, float* angle,
                      srVector3T<float>* direction);
unsigned char AdjustWorldCollisionPosition00451390(float radius, srVector3T<float>* position,
                                                   unsigned char check_items,
                                                   unsigned char check_monsters);
unsigned char FindNearbyFreePosition00451800(float radius, srVector3T<float>* position,
                                             unsigned char check_items,
                                             unsigned char check_monsters);

static_assert(sizeof(W8World) == 0xdc, "W8World_must_be_0xdc");

void SetWorld659AB8(W8World* world);

/* APST chunk: serialize every world prop's animation state. The record is a
   fixed 64-byte name plus the six rep bytes LoadAnimationState0044DBD0 reads. */
void SaveWorldProps0044E830(W8World* world, int handle);
/* APST chunk: restore saved prop animation state. The 0xDEADD00D signature
   selects the name-keyed format; older saves carry a bare count plus the
   object's id_008 key. Unmatched records are consumed by a scratch prop. */
void LoadWorldProps0044E9A0(W8World* world, int handle);
void UpdateCameraPathStateByName(W8World* world, const char* name, int active);
