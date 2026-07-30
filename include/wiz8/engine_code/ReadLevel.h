#pragma once

struct W8World;

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* asset_folder);
