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
// VTABLE: WIZ8 0x005ECB38 stMaterial
// VTABLE: WIZ8 0x005ECB6C srClassSupport<stMaterial, srMaterial, 0, 65538>
class stMaterial : public srClassSupport<stMaterial, srMaterial, false, 0x10002> {
public:
    static const char* sGetClassName()
    {
        return "stMaterial";
    }

    stMaterial();
    virtual srClass* vInstance() override;
    virtual srClass* clone() override;
    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) override;

protected:
    virtual ~stMaterial() override;

public:
    int m_field_78; /* 0x78 */
};

static_assert((sizeof(stMaterial) == 0x7C), "stMaterial_must_be_0x7c");

#pragma pack(push, 1)

/* Engine Code\materials.cpp consumes this serialized material record from
   level particles and animated-texture descriptors. The four 40-byte texture
   names and the unaligned tail fields are fixed by 0x004B8A70/0x004B98F0. */
struct W8MaterialRecord {
    unsigned char version_00;             /* 0x000 */
    unsigned char texture_name_001[0x28]; /* 0x001 */
    char texture_names_029[4][0x28];      /* 0x029 */
    float ambient_0c9[3];                 /* 0x0c9 */
    float diffuse_0d5[3];                 /* 0x0d5 */
    float emissive_colour_0e1[3];         /* 0x0e1 */
    float specular_0ed[3];                /* 0x0ed */
    float shininess_0f9;                  /* 0x0f9 */
    float opacity_0fd;                    /* 0x0fd */
    float emission_101;                   /* 0x101 */
    unsigned char padding_105[8];         /* 0x105 */
    unsigned char animation_mode_10d;     /* 0x10d */
    int animation_frame_10e;              /* 0x10e */
    float animation_rate_112;             /* 0x112 */
    unsigned long shader_flags_116;       /* 0x116 */
    float texture_modes_11a[4];           /* 0x11a */
};

#pragma pack(pop)

static_assert(sizeof(W8MaterialRecord) == 0x12a, "W8MaterialRecord_size_must_be_0x12a");

/* Per-draw material override switches consumed by stMaterial::getMaterialInfo;
   stModelInstance's mesh submit arms them around each chained model. */
extern bool g_material_diffuse_scale_enabled;
extern float g_material_diffuse_scale;
extern bool g_material_emissive_override_enabled;
extern float g_material_emissive_override;

unsigned char LoadMaterial004B8A70(const char* bitmap_folder, const W8MaterialRecord* source,
                                   srMaterialIFace** material, srTextureIFace** texture,
                                   unsigned long* render_flags, int positional_unused);
srTexture* LoadTexture004B95D0(const char* folder, const char* name, unsigned char required);
stTextureAnim* LoadAnimatedTexture(const char* folder, const char* name,
                                   const W8MaterialRecord* source, unsigned char required);
bool MeshHasAnimatedTexture(srMeshModel* model);
void SetModelAnimatedTextureFrame(srModelInstance* instance, int frame);
stTextureAnim* GetModelAnimatedTexture(srModelInstance* instance);

unsigned char CreateDefaultMaterial(srMaterialIFace** material, srTextureIFace** texture,
                                    unsigned long* render_flags);
srTextureIFace* LoadTexture004B9460(const char* path, const W8MaterialRecord* source,
                                    unsigned char required);

struct W8OctPreTreeVertex;
struct W8OctRegionPolygon;

extern W8OctPreTreeVertex* g_gd_vertices;
extern W8OctRegionPolygon* g_gd_polygons;

char BuildPreprocessedFiles(const char* level_path);
void ReportBuildStatus(int channel, const char* message);
void ReportStartupMessage(const char* message);
char* TrimAndLowercaseString(char* text);
struct W8OctPreTreeVertex;
struct W8OctRegionPolygon;
