#include "wiz8/engine_code/materials.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/local_code/Configuration.h"

#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/world.h"

#include "surrender/srMaterial.h"
#include "surrender/srModelInstance.h"
#include "surrender/srTexture.h"

#include <math.h>
#include <new>

/* The SR.DLL registry string is the original runtime class identity. */

// GLOBAL: WIZ8 0x006834cc
srTexture* g_ground_shadow_texture_006834cc;
// GLOBAL: WIZ8 0x006834d0
srMaterial* g_ground_shadow_material_006834d0;
// GLOBAL: WIZ8 0x006834c8
unsigned long g_ground_shadow_shader_006834c8;
/* 0x00683430: the vertex-processor parameters the material mapper is
   installed with; the next recovered global begins at 0x006834C8. */
// GLOBAL: WIZ8 0x00683430
unsigned char g_ground_shadow_material_parameters_00683430[0x98] = {0};

// SYNTHETIC: WIZ8 0x004D6340
// stGroundShadow::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ed3f8
// class srClassSupport<stGroundShadow,srNode,0,65552>

// SYNTHETIC: WIZ8 0x004D6B50
// srClassSupport<stGroundShadow,srNode,0,65552>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004D6A70
// srClassSupport<stGroundShadow,srNode,0,65552>::~srClassSupport

// FUNCTION: WIZ8 0x004D61B0
stGroundShadow::stGroundShadow(srNode* parent)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    angle_138 = 0;
    depth_13c = 500.0f;
    width_140 = 500.0f;
    setParent(parent, 1);

    if (g_ground_shadow_texture_006834cc == 0) {
        g_ground_shadow_texture_006834cc =
            LoadTexture004B95D0("Data\\Monsters\\Bitmaps\\", "Shadow.tga", 1);
        g_ground_shadow_texture_006834cc->addReference();
        g_ground_shadow_texture_006834cc->setMipmap(srTextureIFace::MIPMAP_NONE);
        g_ground_shadow_texture_006834cc->setWrapS(srTextureIFace::WRAP_CLAMP);
        g_ground_shadow_texture_006834cc->setWrapT(srTextureIFace::WRAP_CLAMP);

        srMaterial* material = SR_NEW(srMaterial);
        g_ground_shadow_material_006834d0 = material;
        material->setMapper(
            reinterpret_cast<srVertexProcessor*>(g_ground_shadow_material_parameters_00683430));
        g_ground_shadow_shader_006834c8 =
            (g_ground_shadow_shader_006834c8 & 0xfeff9277UL) | 0x00808260UL;
    }
}

// FUNCTION: WIZ8 0x004d6430
stGroundShadow::stGroundShadow(const stGroundShadow& other)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    setParent(other.parentNode(), 1);
    setName(other.getName());
    angle_138 = other.angle_138;
    depth_13c = other.depth_13c;
    width_140 = other.width_140;
}

/* Retail ICF folds this onto stSurface2D::traverse at 0x004D6540. */
void stGroundShadow::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_DISABLE)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_TERMINATE) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: WIZ8 0x004d6640
void stGroundShadow::process(const ProcessInfo& info, e_processType)
{
    if (g_settings_6850c8.monster_shadows != 0) {
        if (!info.renderer->isPickStackEmpty()) {
            srGERD::Pick pick;
            info.renderer->popPick(pick);
            renderGroundShadow(info.renderer);
            info.renderer->pushPick(pick);
            return;
        }
        renderGroundShadow(info.renderer);
    }
}

/* The shadow vertex-processor parameters hold two packed 2D vectors that the
   mapper rotates by the shadow facing; retail 0x004D6B80 is a thiscall whose
   `this` is the float[4] inside the parameter block. Clang inlines Rotate, so
   no retail claim is made for the standalone emission. */
struct W8GroundShadowRotation {
    float values[4];

    void Rotate(const float* matrix)
    {
        float rotated[4];
        for (int i = 0; i < 2; ++i) {
            rotated[i] = values[0] * matrix[i] + values[1] * matrix[i + 2];
            rotated[i + 2] = values[2] * matrix[i] + values[3] * matrix[i + 2];
        }
        values[0] = rotated[0];
        values[1] = rotated[1];
        values[2] = rotated[2];
        values[3] = rotated[3];
    }
};

/* Collect the world polygons under the shadow centre and draw them with the
   shadow material, up to 30 polygon indices per mesh. The polygon-offset push
   and pop bracket the pass so the shadow slides under the geometry. */
// FUNCTION: WIZ8 0x004D66A0
void stGroundShadow::renderGroundShadow(srGERD* renderer)
{
    srMeshModel::TriMesh mesh;
    srVector3T<float> position;
    getLocation(position);
    position.y += g_world_scale_005ebc40;

    float radius = width_140;
    if (depth_13c > width_140) {
        radius = depth_13c;
    }
    unsigned long* entry = g_octree_6598a4->CollectPolygonsNearPoint(
        &position, radius, radius + radius + g_world_scale_005ebc40);
    if (*entry == 0) {
        return;
    }

    long offset = renderer->getPolygonOffset();
    renderer->setPolygonOffset(2);

    unsigned char* parameter_block = g_ground_shadow_material_parameters_00683430;
    // reinterpret-ok: external srVertexProcessor parameter block
    float* params = reinterpret_cast<float*>(parameter_block);
    // reinterpret-ok: same external parameter block viewed as dwords
    unsigned long* params_l = reinterpret_cast<unsigned long*>(parameter_block);
    params[1] = g_float_005ebc7c / width_140;
    params_l[2] = 0;
    params_l[3] = 0;
    params[4] = g_float_005ebc7c / depth_13c;
    float sine = sin(-angle_138);
    float cosine = cos(-angle_138);
    srVector2T<float> rotation[2];
    // reinterpret-ok: the 2x2 rotation rows are stored through the vec4 Set helper
    reinterpret_cast<srVector4T<float>*>(rotation)->Set(cosine, -sine, sine, cosine);
    // reinterpret-ok: the 2x2 rotation rows read as flat floats
    const float* rotation_matrix = reinterpret_cast<const float*>(rotation);
    // reinterpret-ok: float[4] view of the vertex-processor parameter block
    reinterpret_cast<W8GroundShadowRotation*>(params + 1)->Rotate(rotation_matrix);
    params[5] = position.y;
    params_l[6] = offset;

    unsigned long key = *entry;
    while (key != 0) {
        unsigned long mesh_index = key >> 0x10;
        stMeshModel* model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->getModel());
        model->getTriMesh(mesh);
        // reinterpret-ok: the polygon index list lives in the parameter block
        mesh.active_polygons_14c = reinterpret_cast<unsigned long*>(parameter_block + 0x20);
        mesh.active_polygon_count_150 = 0;
        while (key != 0 && (key >> 0x10) == mesh_index) {
            if (mesh.active_polygon_count_150 < 0x1e) {
                mesh.active_polygons_14c[mesh.active_polygon_count_150] = key & 0xffff;
                ++mesh.active_polygon_count_150;
            }
            ++entry;
            key = *entry;
        }
        mesh.materials_70[0][0] = g_ground_shadow_material_006834d0;
        mesh.poly_textures_e0[0][0] = 0;
        mesh.textures_90[0][0] = g_ground_shadow_texture_006834cc;
        mesh.vertex_materials_c0[0][0] = 0;
        mesh.poly_shaders_100[0] = 0;
        mesh.shaders_b0[0].value = g_ground_shadow_shader_006834c8;
        mesh.poly_uv_110[0] = 0;
        // reinterpret-ok: the vertex table address is a dword parameter
        params_l[7] = reinterpret_cast<unsigned long>(model->getVertexLoc());
        model->RenderTriMeshWithEquations00470380(*renderer, mesh, 0);
        key = *entry;
    }
    renderer->setPolygonOffset(offset);
}

// TEMPLATE: WIZ8 0x004d69a0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassID

// TEMPLATE: WIZ8 0x004d69b0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassName

// TEMPLATE: WIZ8 0x004d69c0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassNode

// FUNCTION: WIZ8 0x004D6370
stGroundShadow::~stGroundShadow() {}

// TEMPLATE: WIZ8 0x004d6a30
// srClassSupport<stGroundShadow,srNode,0,65552>::clone

// FUNCTION: WIZ8 0x004d6bf0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
