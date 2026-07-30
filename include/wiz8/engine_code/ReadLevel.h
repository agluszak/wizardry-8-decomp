#pragma once

struct W8World;

struct W8ReadLevelInfo {
    W8World* world;
    int hFile;
    const char* bitmap_folder;
};

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder);
