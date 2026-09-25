#pragma once

#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/geometry.h"

struct W8World;
class srModelInstance;
class srNode;
class stParticle;
template <class T> class W8GrowableVector;

struct W8ReadLevelInfo {
    W8World* world;            /* 0x00 */
    int hFile;                 /* 0x04 */
    const char* bitmap_folder; /* 0x08 */
    const char* mesh_filename; /* 0x0c */
};

unsigned char ReadLevel(W8World* world, int handle, unsigned char use_octree,
                        const char* bitmap_folder);
unsigned char ReadWorldParticles(W8ReadLevelInfo* info, srNode* scene,
                                 W8GrowableVector<stParticle*>* particles);

#include "wiz8/engine_code/ReadMesh.h"
