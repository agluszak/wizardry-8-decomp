#pragma once

#include "wiz8/geometry.h"

class W8Monster;
class W8Prop;
struct W8World;
class srScene;
class srNode;
class srModelInstance;
class srLight;
class stLight;
class stMeshModel;
class stModelInstance;
struct W8Item;
struct W8GameData;
struct W8OctRegionVolume;

void SetSceneAmbientLightWhite(srScene* scene);
void RemoveMonsterFromWorldList(W8World* unused, W8Monster* monster);
void AddMonsterToWorld0046E580(W8World* unused, W8Monster* monster);
void AddItemToWorld0046E5C0(W8World* unused, W8Item* item);
void RemoveItemFromWorld0046E5E0(W8World* unused, W8Item* item);
void SetModelInstanceChainExclusionMask(srModelInstance* node, int value);
stModelInstance* DuplicateModelInstance0046F680(stModelInstance* instance);
void ExpandBounds0046F510(srVector3T<float>* minimum, srVector3T<float>* maximum,
                          const srVector3T<float>* candidate_minimum,
                          const srVector3T<float>* candidate_maximum);
void UpdateWorldMonsters0046DD70(W8World* world);
void WorldUpdateProps(W8World* world);
int WorldGetPropCount(W8World* unused);             /* 0x0046E600 */
W8Prop* WorldGetPropAt(W8World* unused, int index); /* 0x0046E620 */
void ForwardThroughMember3C_46E750(W8World* owner, int argument);
void ForwardThroughMember3C_46E640(W8World* owner, int argument);
void FinalizeStaticScene0046F3A0(srScene* scene);
stModelInstance* CreateModelInstance0046F5C0(stMeshModel* model);

unsigned char FinalizeWorldScenes0046F410(srNode* node, srNode* dynamic_scene);
/* Mark an instance lit and bake dynamic-scene lights once (render_flags_178 bit 1). */
unsigned char BakeInstanceVertexLightingIfNeeded0046F4A0(stModelInstance* instance,
                                                         srNode* dynamic_scene); /* 0x0046F4A0 */
unsigned char BakeInstanceVertexLighting0046E8A0(stModelInstance* instance, srNode* lights,
                                                 char walk_chain);
void SetSceneMeshShaderBit3_0046E640(srNode* node, int argument);
void SetSceneMeshShaderLowBits0046E750(srNode* node, int argument);
/* 0x0046F760: assign every live world's mesh vertex-light table index and
   mark the mesh dirty for rebake (automap lighting uses table 1). */
void SetWorldMeshVertexLightTable0046F760(W8World* world, int table);

void BuildPlaneFromPoints0046D660(W8Plane* plane, const srVector3T<float>* first,
                                  const srVector3T<float>* second, const srVector3T<float>* third);
/* Report whether a point satisfies all six frustum planes. */
bool PointInsideFrustum0046D880(const srVector3T<float>* point, const W8Plane* planes);
/* Report whether a sphere of `radius` at `point` reaches all six frustum
   planes (each plane distance may be as low as -radius). */
bool SphereInsideFrustum0046D8D0(const srVector3T<float>* point, float radius,
                                 const W8Plane* planes);
/* Point-in-triangle test via dominant-axis projection: `axis` selects the two
   planar components used. */
bool PointInsideTriangle0046D530(const srVector3T<float>* vertices, short axis,
                                 const srVector3T<float>* point);
/* Build the six frustum planes from the eight sorted corner points. */
void BuildFrustumPlanes0046D7E0(const srVector3T<float>* points, W8Plane* planes);
/* Order a volume's eight corner points into the canonical (y,z,x)-sorted
   sequence the frustum plane builder expects. */
void SortFrustumCorners0046DA20(srVector3T<float>* points);
/* Report whether the six-float bounds box (min xyz, max xyz) intersects the
   region volume's frustum: true when a bounds corner satisfies all six
   planes or a volume corner lands inside the bounds. */
bool BoundsInsideFrustum0046D920(const W8OctRegionVolume* volume, const W8BoundingBox* bounds);
char TraceLineOfSightToBounds0046FAF0(const srVector3T<float>* origin, srVector3T<float>* minimum,
                                      srVector3T<float>* maximum);
bool HasLineOfSightToBounds0046FD70(const srVector3T<float>* origin, srVector3T<float>* minimum,
                                    srVector3T<float>* maximum);
unsigned char ShowTargetMarker(const srVector3T<float>* eye, const srVector3T<float>* lower,
                               const srVector3T<float>* upper); /* 0x0046F820 */

void WorldUpdateLights(W8World* world);
float WorldGetRenderRange(W8World* world);
double WorldGetFarClip(W8World* world);
void WorldSetFarClip(W8World* world, float distance);
void WorldSetRenderRange(W8World* world, float value);
stLight* CreateLight0046DF90(srNode* parent, const char* name);
stLight* CreateWorldLight0046E030(W8World* world, const char* name);
stLight* CreateWorldLight0046E140(W8World* world, const char* name);
void ConfigureWorldLight0046E300(srLight* light, float range);
void WorldRemoveLight(W8World* world, stLight* light); /* 0x0046E250 */
void DestroyWorldLights0046E4A0(W8World* world);
void DetachWorldItemMeshes0046DE40(W8World* world);
extern W8GameData* g_octree_game_data_00652db0;
void __stdcall SetOctreeGameData0046D7D0(W8GameData* value); /* 0x0046D7D0 */
