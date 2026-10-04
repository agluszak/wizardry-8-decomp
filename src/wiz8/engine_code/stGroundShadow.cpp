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
static srTexture* g_ground_shadow_texture;
// GLOBAL: WIZ8 0x006834d0
static srMaterial* g_ground_shadow_material;
// GLOBAL: WIZ8 0x006834c8
static unsigned long g_ground_shadow_shader;

/* The material mapper installed for ground shadows: process projects each
   indexed vertex's world x/z through a rotated scale matrix into the first
   texture-coordinate set. transform starts as the 1/width,1/depth scale
   and is rotated by the shadow's facing each render; center_14/center_18 are
   the shadow's world z/x origins and polygons_20 carries the selected
   triangle indices for the active mesh. No source name survives, so the
   mapper keeps a descriptive name anchored at its constructor. */
// VTABLE: WIZ8 0x005ED3B8
// class W8GroundShadowMapper
class W8GroundShadowMapper : public srVertexProcessor {
public:
    /* Retail ICF folds this onto W8NormalTexcoordMapper::isActive at
       0x004D6190. */
    virtual int isActive(srVertexPipe&) override
    {
        return 1;
    }
    virtual void process(srVertexPipe& pipe) override;

    srMatrix2T<float> transform;
    float center_z;
    float center_x;
    srVector3T<float>* vertices_1c;
    unsigned long polygons_20[0x1e];
};

static_assert(sizeof(W8GroundShadowMapper) == 0x98, "W8GroundShadowMapper004D6180_must_be_0x98");

// GLOBAL: WIZ8 0x00683430
static W8GroundShadowMapper g_ground_shadow_material_parameters;

/* Retail ICF folds this class's scalar deleting destructor onto
   W8NormalTexcoordMapper's at 0x004B8A50. */

// FUNCTION: WIZ8 0x004D6090
void W8GroundShadowMapper::process(srVertexPipe& pipe)
{
    srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations +=
        pipe.vertex_count_88;
    pipe.lazy_setup_mask |= 1 << CHANNEL_ST0;

    const unsigned long* index = pipe.avt + pipe.sub_batch_offset;
    srVector2T<float>* output =
        pipe.vertex_array->st0 + pipe.batch_base + pipe.sub_batch_offset;
    unsigned long count = pipe.vertex_count_88;
    if (count == 0) {
        return;
    }
    do {
        const srVector3T<float>* vertex = vertices_1c + *index;
        float dz = vertex->z - center_z;
        float dx = vertex->x - center_x;
        srVector2T<float> coordinate;
        coordinate.Set(
            dz * transform.vectors[0].x + dx * transform.vectors[0].y + g_float_005ebc7c,
            dx * transform.vectors[1].x + dz * transform.vectors[1].y + g_float_005ebc7c);
        ++index;
        *output++ = coordinate;
    } while (--count != 0);
}

// VTABLE: WIZ8 0x005ed3f8
// class srClassSupport<stGroundShadow,srNode,0,65552>

// FUNCTION: WIZ8 0x004D61B0
stGroundShadow::stGroundShadow(srNode* parent)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    angle = 0;
    depth = 500.0f;
    width_140 = 500.0f;
    setParent(parent, 1);

    if (g_ground_shadow_texture == 0) {
        g_ground_shadow_texture =
            LoadTextureFromFolder("Data\\Monsters\\Bitmaps\\", "Shadow.tga", 1);
        g_ground_shadow_texture->addReference();
        g_ground_shadow_texture->setMipmap(srTextureIFace::MIPMAP_NONE);
        g_ground_shadow_texture->setWrapS(srTextureIFace::WRAP_CLAMP);
        g_ground_shadow_texture->setWrapT(srTextureIFace::WRAP_CLAMP);

        srMaterial* material = SR_NEW(srMaterial);
        g_ground_shadow_material = material;
        material->setMapper(&g_ground_shadow_material_parameters);
        g_ground_shadow_shader = (g_ground_shadow_shader & 0xfeff9277UL) | 0x00808260UL;
    }
}

// FUNCTION: WIZ8 0x004d6430
stGroundShadow::stGroundShadow(const stGroundShadow& other)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    setParent(other.parent_, 1);
    setName(other.getName());
    angle = other.angle;
    depth = other.depth;
    width_140 = other.width_140;
}

/* Retail ICF folds this onto stSurface2D::traverse at 0x004D6540. No separate
   FUNCTION claim: decomplint rejects FOLDED-before-primary when engine_code
   sorts ahead of surface2d.cpp. */
void stGroundShadow::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }

    if (!testFlag(FLAG_DISABLE)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_TERMINATE) && first_child_ != 0) {
        first_child_->traverse(info);
    }
}

// FUNCTION: WIZ8 0x004d6640
void stGroundShadow::process(const ProcessInfo& info, e_processType)
{
    if (g_settings.monster_shadows != 0) {
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
    position.y += g_world_scale;
    if (depth <= width_140) {
        radius = width_140;
    } else {
        radius = depth;
    }
    polygons =
        g_octree->CollectPolygonsNearPoint(&position, radius, radius + radius + g_world_scale);
    if (*polygons == 0) {
        return;
    }

    saved_offset = renderer->getPolygonOffset();
    renderer->setPolygonOffset(2);

    g_ground_shadow_material_parameters.transform.vectors[0].Set(g_float_005ebc7c / width_140,
                                                                    0);
    g_ground_shadow_material_parameters.transform.vectors[1].Set(0,
                                                                    g_float_005ebc7c / depth);
    cosine = cos(-angle);
    sine = sin(-angle);
    rotation.vectors[0].Set(cosine, -sine);
    rotation.vectors[1].Set(sine, cosine);
    g_ground_shadow_material_parameters.transform.MultiplyBy(rotation);
    g_ground_shadow_material_parameters.center_z = position.z;
    g_ground_shadow_material_parameters.center_x = position.x;

    while (*polygons != 0) {
        mesh_index = *polygons >> 0x10;
        model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->getModel());
        model->getTriMesh(mesh);
        count = 0;
        while (*polygons != 0 && (*polygons >> 0x10) == mesh_index) {
            if (count < 0x1e) {
                g_ground_shadow_material_parameters.polygons_20[count] = *polygons & 0xffff;
                ++count;
            }
            ++polygons;
        }
        mesh.materials_70[0][0] = g_ground_shadow_material;
        mesh.poly_textures_e0[0][0] = 0;
        mesh.textures_90[0][0] = g_ground_shadow_texture;
        mesh.vertex_materials_c0[0][0] = 0;
        mesh.poly_shaders[0] = 0;
        mesh.shaders[0].value = g_ground_shadow_shader;
        mesh.poly_uv[0] = 0;
        g_ground_shadow_material_parameters.vertices_1c = model->getVertexLoc();
        model->RenderTriMeshWithEquations(*renderer, mesh, 0);
    }
    renderer->setPolygonOffset(saved_offset);
}

// FUNCTION: WIZ8 0x004d6bf0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
