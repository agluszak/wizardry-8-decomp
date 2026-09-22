#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/float_constants.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/layouts/world.h"
#include "wiz8/local_code/Configuration.h"

#include "surrender/srCore.h"
#include "surrender/srMaterial.h"
#include "surrender/srModelInstance.h"
#include "surrender/srStatisticsManager.h"
#include "surrender/srTexture.h"
#include "surrender/srVertexPipe.h"

#include <math.h>
#include <new>

/* The SR.DLL registry string is the original runtime class identity. */

// GLOBAL: WIZ8 0x006834cc
srTexture* g_ground_shadow_texture_006834cc;
// GLOBAL: WIZ8 0x006834d0
srMaterial* g_ground_shadow_material_006834d0;
// GLOBAL: WIZ8 0x006834c8
unsigned long g_ground_shadow_shader_006834c8;

/* The material mapper installed for ground shadows: process projects each
   indexed vertex's world x/z through a rotated scale matrix into the first
   texture-coordinate set. transform_04 starts as the 1/width,1/depth scale
   and is rotated by the shadow's facing each render; center_14/center_18 are
   the shadow's world z/x origins and polygons_20 carries the selected
   triangle indices for the active mesh. No source name survives, so the
   mapper keeps a descriptive name anchored at its constructor. */
// VTABLE: WIZ8 0x005ED3B8
// class W8GroundShadowMapper004D6180
class W8GroundShadowMapper004D6180 : public srVertexProcessor {
public:
    W8GroundShadowMapper004D6180();
    virtual ~W8GroundShadowMapper004D6180() override {}
    /* Retail ICF folds this onto W8NormalTexcoordMapper004B89A0::isActive at
       0x004D6190. */
    virtual int isActive(srVertexPipe&) override
    {
        return 1;
    }
    virtual void process(srVertexPipe& pipe) override;

    srMatrix2T<float> transform_04;
    float center_z_14;
    float center_x_18;
    srVector3T<float>* vertices_1c;
    unsigned long polygons_20[0x1e];
};

static_assert(sizeof(W8GroundShadowMapper004D6180) == 0x98,
              "W8GroundShadowMapper004D6180_must_be_0x98");

// GLOBAL: WIZ8 0x00683430
W8GroundShadowMapper004D6180 g_ground_shadow_material_parameters_00683430;

// FUNCTION: WIZ8 0x004D6180
W8GroundShadowMapper004D6180::W8GroundShadowMapper004D6180() {}

/* Retail ICF folds this class's scalar deleting destructor onto
   W8NormalTexcoordMapper004B89A0's at 0x004B8A50. */

// FUNCTION: WIZ8 0x004D6090
void W8GroundShadowMapper004D6180::process(srVertexPipe& pipe)
{
    srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations_34 +=
        pipe.vertex_count_88;
    pipe.lazy_setup_mask_10 |= 1 << CHANNEL_ST0;

    const unsigned long* index = pipe.avt_70 + pipe.sub_batch_offset_84;
    srVector2T<float>* output =
        pipe.vertex_array_78->st0_0c + pipe.batch_base_80 + pipe.sub_batch_offset_84;
    unsigned long count = pipe.vertex_count_88;
    if (count == 0) {
        return;
    }
    do {
        const srVector3T<float>* vertex = vertices_1c + *index;
        float dz = vertex->z - center_z_14;
        float dx = vertex->x - center_x_18;
        srVector2T<float> coordinate;
        coordinate.x =
            dz * transform_04.vectors[0].x + dx * transform_04.vectors[0].y + g_float_005ebc7c;
        coordinate.y =
            dx * transform_04.vectors[1].x + dz * transform_04.vectors[1].y + g_float_005ebc7c;
        ++index;
        *output++ = coordinate;
    } while (--count != 0);
}

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
        material->setMapper(&g_ground_shadow_material_parameters_00683430);
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

/* Retail ICF folds this onto stSurface2D::traverse at 0x004D6540. No separate
   FUNCTION claim: decomplint rejects FOLDED-before-primary when engine_code
   sorts ahead of surface2d.cpp. */
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

// FUNCTION: WIZ8 0x004D66A0
void stGroundShadow::renderGroundShadow(srGERD* renderer)
{
    srMeshModel::TriMesh mesh;
    srVector3T<float> position;
    srMatrix2T<float> rotation;
    float radius;
    float cosine;
    float sine;
    long saved_offset;
    unsigned long* polygons;
    unsigned long mesh_index;
    stMeshModel* model;
    int count;

    getLocation(position);
    position.y += g_world_scale_005ebc40;
    if (depth_13c <= width_140) {
        radius = width_140;
    } else {
        radius = depth_13c;
    }
    polygons = g_octree_6598a4->CollectPolygonsNearPoint(&position, radius,
                                                         radius + radius + g_world_scale_005ebc40);
    if (*polygons == 0) {
        return;
    }

    saved_offset = renderer->getPolygonOffset();
    renderer->setPolygonOffset(2);

    g_ground_shadow_material_parameters_00683430.transform_04.vectors[0].x =
        g_float_005ebc7c / width_140;
    g_ground_shadow_material_parameters_00683430.transform_04.vectors[0].y = 0;
    g_ground_shadow_material_parameters_00683430.transform_04.vectors[1].x = 0;
    g_ground_shadow_material_parameters_00683430.transform_04.vectors[1].y =
        g_float_005ebc7c / depth_13c;
    cosine = cos(-angle_138);
    sine = sin(-angle_138);
    // The matrix's two row vectors are set through the flat four-float view.
    // reinterpret-ok: deliberate flat-view aliasing of the rotation matrix.
    reinterpret_cast<srVector4T<float>*>(&rotation)->Set(cosine, -sine, sine, cosine);
    g_ground_shadow_material_parameters_00683430.transform_04.MultiplyBy(rotation);
    g_ground_shadow_material_parameters_00683430.center_z_14 = position.z;
    g_ground_shadow_material_parameters_00683430.center_x_18 = position.x;

    while (*polygons != 0) {
        mesh_index = *polygons >> 0x10;
        model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->model());
        model->getTriMesh(mesh);
        count = 0;
        while (*polygons != 0 && (*polygons >> 0x10) == mesh_index) {
            if (count < 0x1e) {
                g_ground_shadow_material_parameters_00683430.polygons_20[count] =
                    *polygons & 0xffff;
                ++count;
            }
            ++polygons;
        }
        mesh.materials_70[0][0] = g_ground_shadow_material_006834d0;
        mesh.poly_textures_e0[0][0] = 0;
        mesh.textures_90[0][0] = g_ground_shadow_texture_006834cc;
        mesh.vertex_materials_c0[0][0] = 0;
        mesh.poly_shaders_100[0] = 0;
        mesh.shaders_b0[0].value = g_ground_shadow_shader_006834c8;
        mesh.poly_uv_110[0] = 0;
        g_ground_shadow_material_parameters_00683430.vertices_1c = model->getVertexLoc();
        model->RenderTriMeshWithEquations00470380(*renderer, mesh, 0);
    }
    renderer->setPolygonOffset(saved_offset);
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
