#pragma once

#include "surrender/srMaterial.h"
#include "wiz8/texture_animation.h"

class srMaterialIFace;
class srMeshModel;
class srShader;
class srModelInstance;
class srTexture;
class srTextureIFace;
class srShader;
class stTextureAnim;

/* Combinable flags in level materials; distinct from srShader's packed word. */
enum { W8_MATERIAL_TWO_SIDED = 1u, W8_MATERIAL_NORMAL_TEXCOORD_MASK = 0x1feu };

/* The materials.cpp assertions name the pointer ppstMaterial. The constructor
   at 0x004925B0 has unresolved TU ownership and registers the class with
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
    virtual srClass* vClone() override;
    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) override;

protected:
    virtual ~stMaterial() override;

public:
    int m_surface_flags; /* 0x78 */
};

static_assert((sizeof(stMaterial) == 0x7C), "stMaterial_must_be_0x7c");

#pragma pack(push, 1)

/* Engine Code\materials.cpp consumes this serialized material record from
   level particles and animated-texture descriptors. The four 40-byte texture
   names and the unaligned tail fields are fixed by 0x004B8A70/0x004B98F0. */
struct W8MaterialRecord {
    unsigned char version;                 /* 0x000 */
    char texture_name[0x28];               /* 0x001 */
    char texture_names[4][0x28];           /* 0x029 */
    srVector3T<float> ambient;             /* 0x0c9 */
    srVector3T<float> diffuse;             /* 0x0d5 */
    srVector3T<float> emissive_colour;     /* 0x0e1 */
    srVector3T<float> specular;            /* 0x0ed */
    float shininess;                       /* 0x0f9 */
    float opacity;                         /* 0x0fd */
    float emission;                        /* 0x101 */
    unsigned char padding_105[8];          /* 0x105 */
    W8TextureAnimationMode animation_mode; /* 0x10d */
    int animation_frame;                   /* 0x10e */
    float animation_rate;                  /* 0x112 */
    unsigned long surface_flags;           /* 0x116 */
    float texture_modes[4];                /* 0x11a */
};

#pragma pack(pop)

static_assert(sizeof(W8MaterialRecord) == 0x12a, "W8MaterialRecord_size_must_be_0x12a");

/* Per-draw material override switches consumed by stMaterial::getMaterialInfo;
   stModelInstance's mesh submit arms them around each chained model. */
extern bool g_material_diffuse_scale_enabled;
extern float g_material_diffuse_scale;
extern bool g_material_emissive_override_enabled;
extern float g_material_emissive_override;

unsigned char LoadMaterial(const char* bitmap_folder, const W8MaterialRecord* source,
                           srMaterialIFace** material, srTextureIFace** texture,
                           srShader* render_flags, int positional_unused);
srTexture* LoadTextureFromFolder(const char* folder, const char* name, bool required);
stTextureAnim* LoadAnimatedTexture(const char* folder, const char* name,
                                   const W8MaterialRecord* source, bool required);
bool MeshHasAnimatedTexture(srMeshModel* model);
void SetModelAnimatedTextureFrame(srModelInstance* instance, int frame);
stTextureAnim* GetModelAnimatedTexture(srModelInstance* instance);

unsigned char CreateDefaultMaterial(srMaterialIFace** material, srTextureIFace** texture,
                                    srShader* render_flags);
srTextureIFace* LoadTextureFromPath(const char* path, const W8MaterialRecord* source,
                                    bool required);

struct W8OctPreTreeVertex;
struct W8OctRegionPolygon;

extern W8OctPreTreeVertex* g_gd_vertices;
extern W8OctRegionPolygon* g_gd_polygons;

void ReportBuildStatus(short channel, const char* message);
void ReportStartupMessage(const char* message);
char* TrimAndLowercaseString(char* text);
struct W8OctPreTreeVertex;
struct W8OctRegionPolygon;
