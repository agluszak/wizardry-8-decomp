#pragma once

#include "wiz8/geometry.h"

class W8Monster;
struct W8World;
class srScene;
class srNode;
class stMeshModel;
class stModelInstance;

void SetSceneAmbientLightWhite(srScene* scene);
void RemoveMonsterFromWorldList(W8World* unused, W8Monster* monster);
void WorldAddToList00(W8World* unused, void* entry);
void WorldRemoveFromList04(W8World* unused, void* entry);
void SetChainValue15C(char* node, int value);
stModelInstance* DuplicateModelInstance0046F680(
    stModelInstance* instance);
void ExpandBounds0046F510(
    srVector3T<float>* minimum,
    srVector3T<float>* maximum,
    const srVector3T<float>* candidate_minimum,
    const srVector3T<float>* candidate_maximum);
void UpdateWorldMonsters0046DD70(W8World* world);
void ForwardThroughMember3C_46E750(W8World* owner, int argument);
void ForwardThroughMember3C_46E640(W8World* owner, int argument);
void FinalizeStaticScene0046F3A0(srScene* scene);
stModelInstance* CreateModelInstance0046F5C0(stMeshModel* model);

void FinalizeWorldScenes0046F410(
    srScene* static_scene, srNode* dynamic_scene);
void Function46E640(srScene* scene, int argument);
void Function46E750(srScene* scene, int argument);


void BuildPlaneFromPoints0046D660(
    srVector4T<float>* plane,
    const srVector3T<float>* first,
    const srVector3T<float>* second,
    const srVector3T<float>* third);
unsigned char Function0046D880(
    const srVector3T<float>* point, const unsigned char* filter);
unsigned char HasLineOfSightToBounds0046FD70(
    const srVector3T<float>* origin,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum);
unsigned char ShowTargetMarker(
    void* eye, void* lower, void* upper);                               /* 0x0046F820 */

