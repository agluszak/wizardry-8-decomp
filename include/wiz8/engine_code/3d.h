#pragma once

#include "wiz8/geometry.h"

class W8Monster;
struct W8World;
class srScene;
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
void FinalizeStaticScene0046F3A0(srScene* scene);
stModelInstance* CreateModelInstance0046F5C0(stMeshModel* model);
