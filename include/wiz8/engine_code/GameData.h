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

/* Proven prefix of the processed GameData owner.  GDFileIO.cpp constructs a
   larger record; these fields are the bounds, vertex table and first surface
   bank consumed by GameData.cpp and OctBuildTree.cpp. */
struct W8GameData {
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
};

#pragma pack(pop)

static_assert(sizeof(W8LevelDataRecord) == 0xac,
              "W8LevelDataRecord_must_be_0xac");

extern W8LevelDataRecord* g_level_data_00652dac;
extern W8GameData* g_octree_game_data_00652db0;

void Function41EF50(void);
void Function41F260();
unsigned char LoadSurfaceVertices004214D0(
    srVector3T<float>* output, const int* vertex_indices);
unsigned char InitializeGameData004497C0(W8GameData* game_data);
