#pragma once

#include "wiz8/geometry.h"

class W8Monster;
struct W8World;
class srScene;
class srNode;
class stMeshModel;
class stModelInstance;
struct W8Item;

void SetSceneAmbientLightWhite(srScene* scene);
void RemoveMonsterFromWorldList(W8World* unused, W8Monster* monster);
void AddMonsterToWorld0046E580(W8World* unused, W8Monster* monster);
void AddItemToWorld0046E5C0(W8World* unused, W8Item* item);
void RemoveItemFromWorld0046E5E0(W8World* unused, W8Item* item);
void SetChainValue15C(char* node, int value);
stModelInstance* DuplicateModelInstance0046F680(stModelInstance* instance);
void ExpandBounds0046F510(srVector3T<float>* minimum, srVector3T<float>* maximum,
                          const srVector3T<float>* candidate_minimum,
                          const srVector3T<float>* candidate_maximum);
void UpdateWorldMonsters0046DD70(W8World* world);
void ForwardThroughMember3C_46E750(W8World* owner, int argument);
void ForwardThroughMember3C_46E640(W8World* owner, int argument);
/* Video2.cpp's prologue for the scene shader walks. */
void __fastcall Function00424A40(unsigned int* destination);
void FinalizeStaticScene0046F3A0(srScene* scene);
stModelInstance* CreateModelInstance0046F5C0(stMeshModel* model);

void FinalizeWorldScenes0046F410(srScene* static_scene, srNode* dynamic_scene);
void SetSceneMeshShaderBit3_0046E640(srNode* node, int argument);
void SetSceneMeshShaderLowBits0046E750(srNode* node, int argument);

void BuildPlaneFromPoints0046D660(srVector4T<float>* plane, const srVector3T<float>* first,
                                  const srVector3T<float>* second, const srVector3T<float>* third);
/* Report whether a point satisfies all six frustum planes. */
unsigned char PointInsideFrustum0046D880(const srVector3T<float>* point,
                                         const srVector4T<float>* planes);
unsigned char HasLineOfSightToBounds0046FD70(const srVector3T<float>* origin,
                                             srVector3T<float>* minimum,
                                             srVector3T<float>* maximum);
unsigned char ShowTargetMarker(void* eye, void* lower, void* upper); /* 0x0046F820 */
