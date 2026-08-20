#ifndef WIZ8_ENGINE_CODE_ANI_MESH_H
#define WIZ8_ENGINE_CODE_ANI_MESH_H

#include "surrender/srMath.h"

class stModelInstance005EC7D0;
struct W8ReadLevelInfo;
struct W8World;

enum W8AniMeshFlags {
    W8_ANI_MESH_LOADED = 0x01,
    W8_ANI_MESH_FRAME_COUNT_LOADED = 0x02,
    W8_ANI_MESH_RADIUS_LOADED = 0x04,
    W8_ANI_MESH_KEEP_LOADED = 0x08,
    W8_ANI_MESH_FLAG_10 = 0x10,
    W8_ANI_MESH_SINGLE_INSTANCE = 0x20,
};

/* Engine Code\AniMesh.cpp. The class name and string members are
   assertion-backed. Construction, copying, unloading, and frame lookup prove
   the remaining storage roles. */
struct W8AniMesh {
    unsigned char flags_00;              /* 0x00 */
    unsigned char frame_count_01;        /* 0x01 */
    unsigned char unknown_02[2];
    stModelInstance005EC7D0** meshes_04; /* 0x04: ppsrMeshes */
    srVector3T<float> bounds_minimum_08; /* 0x08 */
    srVector3T<float> bounds_maximum_14; /* 0x14 */
    float radius_20;                     /* 0x20 */
    unsigned int loaded_bytes_24;        /* 0x24 */
    signed char list_index_28;           /* 0x28 */
    unsigned char unknown_29[3];
    char* bitmap_directory_2c;           /* 0x2c: strBitmapDir */
    char* filename_30;                   /* 0x30: strFilename */
    int file_offset_34;                  /* 0x34 */
    W8World* world_38;                   /* 0x38 */
    int last_used_3c;                    /* 0x3c */
};                                       /* 0x40 */

static_assert(sizeof(W8AniMesh) == 0x40, "W8AniMesh_minimum_size_must_be_0x40");

W8AniMesh* CreateAniMesh004B57E0();
W8AniMesh* CopyAniMesh004B58D0(const W8AniMesh* other);
float GetAniMeshFrameRadius004B5C10(W8AniMesh* mesh, unsigned char frame);
unsigned char GetAniMeshBounds004B6640(
    W8AniMesh* mesh, srVector3T<float>* minimum, srVector3T<float>* maximum);
unsigned char LoadAniMesh004B5D00(
    int file, W8AniMesh* mesh, unsigned char load_all);
unsigned char LoadAniMeshFromInfo004B5B30(
    W8ReadLevelInfo* info, W8AniMesh* mesh, unsigned char load_all);
unsigned char UnloadAniMesh004B63F0(W8AniMesh* mesh, unsigned char force);
stModelInstance005EC7D0* GetAniMeshFrame004B6550(
    W8AniMesh* mesh, unsigned char frame);
void DestroyAniMesh004B5880(W8AniMesh* mesh);
unsigned char AniMeshValue004B64F0(W8AniMesh* mesh);
unsigned char AniMeshRadius004B66E0(W8AniMesh* mesh, float* radius);
/* Two parameters, not three: the retail body reads its flag from the second
   stack slot, and GrCycle's 0x004A7470 pushes exactly the pair. */
void AniMeshSetFlag10004B6860(W8AniMesh* mesh, signed char enabled);
void EnforceAniMeshMemoryLimit004B6770(W8AniMesh* current);

#endif
