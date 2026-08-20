#pragma once

struct W8World;
class srModelInstance;
class srNode;
class stParticle;
template <class T> class W8GrowableVector;

struct W8ReadLevelInfo {
    W8World* world;                      /* 0x00 */
    int hFile;                           /* 0x04 */
    const char* bitmap_folder;           /* 0x08 */
    const char* mesh_filename;           /* 0x0c */
};

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder);
unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
unsigned char SkipSingleLevelMesh00487BD0(W8ReadLevelInfo* info);
unsigned char ReadWorldParticles004BD0D0(
    W8ReadLevelInfo* info,
    srNode* scene,
    W8GrowableVector<stParticle*>* particles);
