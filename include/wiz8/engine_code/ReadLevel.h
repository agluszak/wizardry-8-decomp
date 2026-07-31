#pragma once

struct W8World;
class srModelInstance;

struct W8ReadLevelInfo {
    W8World* world;
    int hFile;
    const char* bitmap_folder;
};

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder);
unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
