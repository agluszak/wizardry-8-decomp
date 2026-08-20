#pragma once

#include "surrender/srMaterial.h"

class srMaterialIFace;
class srMeshModel;
class srModelInstance;
class srTexture;
class srTextureIFace;
class stTextureAnim;

/* Engine Code\materials.cpp. Its canonical assertions name the pointer
   ppstMaterial, and the constructor at 0x004925B0 registers the class with
   SurRender's registry under the literal "stMaterial" and the class id
   0x10002, whose parent chain the same body spells out: srMaterialIFace
   0x2200, then srMaterial 0x2210, then this. So the name is the original's
   own, not a descriptive one.

   The vtable at 0x005ECB6C lines up with srMaterial's slot for slot: slots 3,
   4, 6 and 8 through 12 are import thunks into SR.DLL, and the four overridden
   here plus the destructor are the five SurRender does not export. The object
   is 0x7C bytes, which is what the constructor's callers allocate through
   srHeap, and the only field past srMaterial's extent is at 0x78. */
class stMaterial : public srMaterial {
public:
    stMaterial();
    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual ~stMaterial() override;
    virtual srClass* clone() override;

    int m_field_78;                          /* 0x78 */
};

static_assert((sizeof(stMaterial) == 0x7C), "stMaterial_must_be_0x7c");

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
