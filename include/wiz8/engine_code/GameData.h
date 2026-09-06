#pragma once

void CopyLevelDataHandle(int* destination, const int* source);

#include "wiz8/geometry.h"

#pragma pack(push, 1)

/* The current level-data record at 0x00652DAC. GameData.cpp establishes the
   flag word and optional vector; Camera.cpp establishes the two derived
   forward vectors and the scale used to produce the second. */
struct W8LevelDataRecord {
    unsigned int flags;                   /* 0x00 */
    unsigned char unknown_04[0x10];
    float camera_scale_14;                /* 0x14 */
    unsigned char unknown_18[0x28];
    srVector3T<float> vector_40;                 /* 0x40 */
    srVector3T<float> camera_forward_4c;         /* 0x4c */
    unsigned char unknown_58[0x0c];
    srVector3T<float> vector_64;                 /* 0x64 */
    srVector3T<float> vector_70;                 /* 0x70 */
    srVector3T<float> scaled_camera_forward_7c;  /* 0x7c */
    float vector_88[3];                   /* 0x88 */
    unsigned char unknown_94[0x0c];
    srVector3T<float> vector_a0;                 /* 0xa0 */
};

struct W8OctBuildTree00446390;

class BitArray;

/* One environment record: seventeen dwords mixing counters and factors. */
struct W8EnvironRecord {
    int value_00;
    unsigned char value_04;
    unsigned char pad_05[3];
    int value_08;
    int unknown_0c;
    int value_10;
    float value_14;
    int value_18;
    float value_1c;
    float value_20;
    int value_24;
    int value_28;
    int value_2c;
    int value_30;
    float value_34;
    float value_38;
    float value_3c;
    float value_40;
};

static_assert(sizeof(W8EnvironRecord) == 0x44, "W8EnvironRecord_must_be_0x44");

class BitArray;

/* The processed game-data record the octree and world own. The proven prefix
   above is consumed by GameData.cpp and OctBuildTree.cpp; the constructor
   below establishes the rest: two bit sets, paired count/allocation blocks,
   a counted pointer array, the environment count/array pair, and a trailing
   flag. Only straightforward storage is claimed past the prefix. */
struct W8GameData {
    W8GameData(int handle, void* parent);              /* 0x00449010 */
    ~W8GameData();                                     /* 0x00449BB0 */
    unsigned char Function447660(void* file, int index);
    void Function41A9E0();

    W8OctBuildTree00446390* geometry_index_00;
    unsigned long positional_04;
    srVector3T<float> minimum_08;
    srVector3T<float> maximum_14;
    int vertex_count_20;
    srVector3T<float>* vertices_24;
    int surface_count_28;
    unsigned char positional_2c[0x0c];
    W8GDSurface* surfaces_38;
    unsigned char positional_3c[0x0c];
    W8GDSurface* overflow_surfaces_48;
    int value_4c;
    void* block_50;
    int value_54;
    BitArray* bits_58;
    BitArray* bits_5c;
    int value_60;
    void* block_64;
    int value_68;
    void* block_6c;
    int value_70;
    void* block_74;
    int count_78;
    void** array_7c;
    int environ_count_80;
    W8EnvironRecord** environs_84;
    unsigned char value_88;
    unsigned char pad_89[3];
};

static_assert(sizeof(W8GameData) == 0x8c, "W8GameData_must_be_0x8c");

#pragma pack(pop)

static_assert(sizeof(W8LevelDataRecord) == 0xac,
              "W8LevelDataRecord_must_be_0xac");

extern W8LevelDataRecord* g_level_data_00652dac;
extern W8GameData* g_octree_game_data_00652db0;

W8GameData* ReadGameData00447570(const char* path, void* parent); /* 0x00447570 */

void Function41EF50(void);
void Function41F260();
unsigned char LoadSurfaceVertices004214D0(
    srVector3T<float>* output, const int* vertex_indices);
unsigned char InitializeGameData004497C0(W8GameData* game_data);
