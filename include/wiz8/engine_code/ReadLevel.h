#pragma once

struct W8World;
class srModelInstance;

struct W8ReadLevelInfo {
    W8World* world;                      /* 0x00 */
    int hFile;                           /* 0x04 */
    const char* bitmap_folder;           /* 0x08 */
    /* LoadProp zeros this dword before constructing the mesh/anim. */
    int unknown_00c;                     /* 0x0c */
};

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder);
unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
