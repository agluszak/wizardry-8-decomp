#ifndef WIZ8_ENGINE_CODE_ANI_MESH_H
#define WIZ8_ENGINE_CODE_ANI_MESH_H

/* Engine Code\AniMesh.cpp.  The five consumers in the owning translation
   unit establish these offsets.  The original class name is assertion-backed;
   unresolved members remain positional. */
struct W8AniMesh {
    unsigned char flags_00;              /* 0x00 */
    unsigned char value_01;              /* 0x01 */
    unsigned char unknown_02[0x1e];
    float radius_20;                     /* 0x20 */
    unsigned char unknown_24[4];
    signed char list_index_28;
    unsigned char unknown_29[3];
    void* allocation_2c;                 /* 0x2c */
    void* allocation_30;                 /* 0x30 */
    unsigned char unknown_34[8];
    int counter_3c;                      /* 0x3c */
};                                       /* 0x40 */

static_assert(sizeof(W8AniMesh) == 0x40, "W8AniMesh_minimum_size_must_be_0x40");

class stModelInstance005EC7D0;
stModelInstance005EC7D0* GetAniMeshFrame004B6550(
    W8AniMesh* mesh, unsigned char frame);
void DestroyAniMesh004B5880(W8AniMesh* mesh);
unsigned char AniMeshValue004B64F0(W8AniMesh* mesh);
unsigned char AniMeshRadius004B66E0(W8AniMesh* mesh, float* radius);
void AniMeshSetFlag10004B6860(W8AniMesh* mesh, int unused, signed char enabled);

#endif
