#ifndef WIZ8_ENGINE_CODE_ANI_MESH_H
#define WIZ8_ENGINE_CODE_ANI_MESH_H

class stModelInstance005EC7D0;

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
    float vector_08[3];                  /* 0x08 */
    float vector_14[3];                  /* 0x14 */
    float radius_20;                     /* 0x20 */
    unsigned int loaded_bytes_24;        /* 0x24 */
    signed char list_index_28;           /* 0x28 */
    unsigned char unknown_29[3];
    char* bitmap_directory_2c;           /* 0x2c: strBitmapDir */
    char* filename_30;                   /* 0x30: strFilename */
    int file_offset_34;                  /* 0x34 */
    int load_value_38;                   /* 0x38 */
    int last_used_3c;                    /* 0x3c */
};                                       /* 0x40 */

static_assert(sizeof(W8AniMesh) == 0x40, "W8AniMesh_minimum_size_must_be_0x40");

W8AniMesh* CreateAniMesh004B57E0();
W8AniMesh* CopyAniMesh004B58D0(const W8AniMesh* other);
unsigned char UnloadAniMesh004B63F0(W8AniMesh* mesh, unsigned char force);
stModelInstance005EC7D0* GetAniMeshFrame004B6550(
    W8AniMesh* mesh, unsigned char frame);
void DestroyAniMesh004B5880(W8AniMesh* mesh);
unsigned char AniMeshValue004B64F0(W8AniMesh* mesh);
unsigned char AniMeshRadius004B66E0(W8AniMesh* mesh, float* radius);
void AniMeshSetFlag10004B6860(W8AniMesh* mesh, int unused, signed char enabled);

#endif
