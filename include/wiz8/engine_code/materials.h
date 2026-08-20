#pragma once

class srMaterialIFace;
class srMeshModel;
class srModelInstance;
class srTexture;
class srTextureIFace;
class stTextureAnim;

#pragma pack(push, 1)

/* Engine Code\materials.cpp consumes this serialized material record from
   level particles and animated-texture descriptors. The four 40-byte texture
   names and the unaligned tail fields are fixed by 0x004B8A70/0x004B98F0. */
struct W8MaterialRecord004B8A70 {
    unsigned char version_00;              /* 0x000 */
    unsigned char positional_001[0x28];    /* 0x001 */
    char texture_names_029[4][0x28];       /* 0x029 */
    float ambient_0c9[3];                  /* 0x0c9 */
    float diffuse_0d5[3];                  /* 0x0d5 */
    float positional_0e1[3];               /* 0x0e1 */
    float specular_0ed[3];                 /* 0x0ed */
    float positional_0f9;                  /* 0x0f9 */
    float opacity_0fd;                     /* 0x0fd */
    float emission_101;                    /* 0x101 */
    unsigned char positional_105[8];       /* 0x105 */
    unsigned char animation_mode_10d;      /* 0x10d */
    int animation_frame_10e;               /* 0x10e */
    float animation_rate_112;              /* 0x112 */
    unsigned long shader_flags_116;        /* 0x116 */
    float texture_modes_11a[4];            /* 0x11a */
};

#pragma pack(pop)

static_assert(sizeof(W8MaterialRecord004B8A70) == 0x12a,
              "W8MaterialRecord004B8A70_size_must_be_0x12a");

unsigned char LoadMaterial004B8A70(
    const char* bitmap_folder, const W8MaterialRecord004B8A70* source,
    srMaterialIFace** material, srTextureIFace** texture,
    unsigned long* render_flags, int positional_unused);
srTexture* LoadTexture004B95D0(
    const char* folder, const char* name, unsigned char required);
stTextureAnim* LoadAnimatedTexture004B98F0(
    const char* folder, const char* name,
    const W8MaterialRecord004B8A70* source, unsigned char required);
unsigned char MeshHasAnimatedTexture004B9AA0(srMeshModel* model);
void SetModelAnimatedTextureFrame004B9B00(
    srModelInstance* instance, int frame);
stTextureAnim* GetModelAnimatedTexture004B9B50(srModelInstance* instance);
