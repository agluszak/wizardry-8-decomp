#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/float_constants.h"
#include "wiz8/mesh_model.h"
#include "wiz8/render_state.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srNode.h"
#include "surrender/srHeap.h"
#include "surrender/srTriMeshPipeline.h"
#include "surrender/srVectorProcessor.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <new>
#include <string.h>

#define ST_MODEL_INSTANCE_CPP \
    "C:\\Projects\\Wizardry 8\\Engine Code\\stModelInstance.cpp"

extern float g_light_scale_0060bfe0;

float g_model_ambient_offset_x_00659cd0;
float g_model_ambient_offset_y_00659cd4;
float g_model_ambient_offset_z_00659cd8;
unsigned char g_disable_model_textures_0065a146;
unsigned char g_disable_model_values_0065a0ec;
unsigned char g_model_value_1a0_enabled_0065ba9e;
float g_model_value_1a0_0065baa0;
unsigned char g_model_value_1a1_enabled_0065baa4;
int g_model_value_1a1_0065baa8;

static srHeapArray<srVector3T<float> >* g_emissive_vertices_0065a148;
static srMeshModel::TriMesh* g_debug_model_mesh_0065a14c;

// VTABLE: WIZ8 0x005ec89c srClassSupport<srModelInstance, class srNode, 0, 4352>
// VTABLE: WIZ8 0x005ec88c srModel::Client
// class srClassSupport<stModelInstance2D, class srModelInstance, 0, 65541>

// VTABLE: WIZ8 0x005ec814 srClassSupport<stModelInstance, srModelInstance, 0, 65540>
// VTABLE: WIZ8 0x005ec804 srModel::Client

/* 0x0047F260 and 0x00481C50 are compiler-generated scalar deleting
   destructors and have no authored source bodies. */

// TEMPLATE: WIZ8 0x00481A40
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::getClassID

// TEMPLATE: WIZ8 0x00481A50
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::getClassName

// TEMPLATE: WIZ8 0x00481A60
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::getClassNode

// TEMPLATE: WIZ8 0x00481B00
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::clone

// TEMPLATE: WIZ8 0x00481B20
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::~srClassSupport<stModelInstance2D,srModelInstance,0,65541>

/*
 * Engine Code\stModelInstance.cpp.
 *
 * The canonical 3D and 2D model-instance classes have adjacent registry ids.
 * Their operational traversal and rendering paths live in this unit.
 */

/* Find the animated texture assigned to polygons whose runtime name begins
   with "mouth". Damage-stage instances use the stage-specific texture table;
   ordinary instances use the mesh's active polygon texture table. */
// FUNCTION: WIZ8 0x00481080
stTextureAnim* stModelInstance::FindMouthTexture00481080()
{
    stMeshModel* mesh = static_cast<stMeshModel*>(
        static_cast<srModel::Client&>(*this).getModel());

    if (damage_stage_184 == -1) {
        while (mesh != 0) {
            srPtr<srTextureIFace>* textures =
                mesh->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                for (int polygon = 0;
                     polygon < mesh->polygon_count_230;
                     ++polygon) {
                    srTextureIFace* texture = textures[polygon].get();

                    if (texture != 0 &&
                        texture->getClassID() == stTextureAnim::CLASS_ID &&
                        _strnicmp(texture->getName(), "mouth", 5) == 0) {
                        return static_cast<stTextureAnim*>(texture);
                    }
                }
            }
            mesh = mesh->next;
        }
    }
    else {
        while (mesh != 0) {
            srPtr<srTextureIFace>* textures =
                mesh->GetTextureTable00473720(
                    damage_stage_tables_188.data[damage_stage_184]);

            if (textures != 0) {
                for (int polygon = 0;
                     polygon < mesh->polygon_count_230;
                     ++polygon) {
                    srTextureIFace* texture = textures[polygon].get();

                    if (texture != 0 &&
                        texture->getClassID() == stTextureAnim::CLASS_ID &&
                        _strnicmp(texture->getName(), "mouth", 5) == 0) {
                        return static_cast<stTextureAnim*>(texture);
                    }
                }
            }
            mesh = mesh->next;
        }
    }
    return 0;
}

/* Lazily construct the two-triangle marker mesh used by the model debug
   render path. The six vertices form two upright arrowhead planes. */
// FUNCTION: WIZ8 0x004813F0
void InitializeDebugModel004813F0()
{
    stMaterial* material = new stMaterial;

    if (g_debug_model_mesh_0065a14c == 0) {
        srMeshModel::TriMesh* mesh = new srMeshModel::TriMesh;
        g_debug_model_mesh_0065a14c = mesh;
        if (mesh == 0) {
            return;
        }

        if (material != 0) {
            srVector4T<float> value;
            value.x = 1.0f;
            value.y = 1.0f;
            value.z = 1.0f;
            value.w = 1.0f;
            material->setAmbient(value);
            value.x = 0.0f;
            value.y = 0.0f;
            value.z = 0.0f;
            value.w = 0.0f;
            material->setSpecular(value);
            material->setShininess(1.0);
            material->setEmissive(value);
            value.x = 1.0f;
            value.y = 1.0f;
            value.z = 1.0f;
            value.w = 0.0f;
            material->setDiffuse(value);
            material->mapper_70 = 0;
        }

        mesh->triangles_010 = static_cast<srVector3i*>(
            srHeap.allocate(2 * sizeof(srVector3i)));
        mesh->triangles_010[0].x = 0;
        mesh->triangles_010[0].y = 1;
        mesh->triangles_010[0].z = 2;
        mesh->triangles_010[1].x = 3;
        mesh->triangles_010[1].y = 4;
        mesh->triangles_010[1].z = 5;

        mesh->vertex_locations_038 = static_cast<srVector3T<float>*>(
            srHeap.allocate(6 * sizeof(srVector3T<float>)));
        mesh->vertex_locations_038[0].method_00421680(-250.0, 250.0, 0.0);
        mesh->vertex_locations_038[1].method_00421680(0.0, -250.0, 0.0);
        mesh->vertex_locations_038[2].method_00421680(250.0, 250.0, 0.0);
        mesh->vertex_locations_038[3].method_00421680(0.0, 250.0, -250.0);
        mesh->vertex_locations_038[4].method_00421680(0.0, -250.0, 0.0);
        mesh->vertex_locations_038[5].method_00421680(0.0, 250.0, 250.0);

        mesh->vertex_normals_03c = 0;
        mesh->flags_00c = 0x38;
        mesh->triangle_count_004 = 2;
        mesh->vertex_count_000 = 6;
        mesh->bounds_maximum_12c.method_00421680(500.0, 500.0, 500.0);
        mesh->bounds_minimum_120.method_00421680(0.0, 0.0, 0.0);
        mesh->bounds_center_138.x = 250.0f;
        mesh->bounds_center_138.y = 250.0f;
        mesh->bounds_center_138.z = 250.0f;
        mesh->bounds_center_138.w = 250.0f;
        mesh->pass_count_008 = 1;
        mesh->shaders_0b0[0].value = 0x44b3;
        mesh->value_014 = 0;
        mesh->value_018 = 0;
        mesh->material_070 = material;
        mesh->texture_090 = 0;
        mesh->values_040 = 0;
        mesh->value_050 = 0;
        mesh->value_060 = 0;
        mesh->value_100 = 0;
        mesh->value_110 = 0;
        mesh->value_148 = 0;
        mesh->polygon_table_14c = 0;
    }
}

/* Draw the marker above the model bounds through the same pipeline state the
   ordinary mesh renderer uses. */
// FUNCTION: WIZ8 0x004811D0
void DebugRenderModel004811D0(
    srGERD* renderer, const srMeshModel::TriMesh& model_mesh)
{
    if (g_debug_model_mesh_0065a14c == 0) {
        InitializeDebugModel004813F0();
        if (g_debug_model_mesh_0065a14c == 0) {
            return;
        }
    }

    float height =
        model_mesh.bounds_maximum_12c.y - model_mesh.bounds_minimum_120.y;
    renderer->pushEnable();
    renderer->setCullMode(srGERD::CULL_MODE_POSITIONAL_2);
    if (!renderer->isEnabled(srGERD::ENABLE_POSITIONAL_1)) {
        renderer->toggle(srGERD::ENABLE_POSITIONAL_1);
    }
    renderer->matrixMode(srGERD::MATRIX_MODE_POSITIONAL_0);
    renderer->pushMatrix();
    srVector3T<float> offset;
    offset.method_00421680(0.0, height * 0.5f, 0.0);
    renderer->translate(offset);
    float scale = height * (1.0f / 1500.0f);
    if (scale < 0.5f) {
        scale = 0.5f;
    }
    renderer->scale(scale, scale, scale);

    srMeshModel::TriMesh& mesh = *g_debug_model_mesh_0065a14c;
    srTriMeshPipeline* pipeline =
        srTriMeshPipeline::Get004750A0(renderer);
    pipeline->value_1c = mesh.triangle_count_004;
    pipeline->value_34 = mesh.triangles_010;
    pipeline->value_20 = mesh.vertex_count_000;
    pipeline->value_38 = mesh.vertex_locations_038;
    pipeline->value_3c = mesh.vertex_normals_03c;
    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_pass_18->value_14 = 0;
    pipeline->current_pass_18->value_0c = 0;
    pipeline->current_pass_18->value_10 = 0;
    pipeline->material_80 = mesh.material_070;
    pipeline->current_record_14->material_08 = mesh.material_070;
    pipeline->SetFlags004752C0(mesh.shaders_0b0[0]);

    ++pipeline->slot_count_84;
    pipeline->current_record_14 =
        &pipeline->records_94[pipeline->slot_count_84];
    pipeline->current_pass_18 =
        &pipeline->passes_9c[pipeline->slot_count_84];
    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_record_14->value_04 = 0;
    pipeline->current_record_14->material_08 = pipeline->material_80;
    pipeline->current_pass_18->value_00 = pipeline->value_78;
    pipeline->current_pass_18->value_04 = pipeline->value_7c;
    pipeline->current_pass_18->flags_08 = pipeline->shader_74;
    pipeline->current_pass_18->value_0c = 0;
    pipeline->current_pass_18->value_10 = 0;
    pipeline->current_pass_18->value_14 = 0;
    pipeline->current_pass_18->value_18 = 0;
    pipeline->current_pass_18->value_1c = 0;
    pipeline->FlushIfCurrent();

    renderer->popMatrix();
    renderer->popEnable();
}

// FUNCTION: WIZ8 0x00480790
int stModelInstance::FindDamageStage00480790(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(model());
    return mesh->FindSkinTable004736D0(name);
}

/* Add a stage by cloning the first stage's table across the complete linked
   mesh chain. The instance stores the table id shared by that chain. */
// FUNCTION: WIZ8 0x00480560
int stModelInstance::AddDamageStage00480560(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(model());

    if (mesh->FindSkinTable004736D0(name) != -1) {
        return -1;
    }

    int stage = damage_stage_tables_188.capacity;
    int new_count = stage + 1;
    int* replacement = static_cast<int*>(
        srHeap.allocate(new_count * sizeof(int)));
    for (int index = 0; index < (int)damage_stage_tables_188.capacity; ++index) {
        replacement[index] = damage_stage_tables_188.data[index];
    }
    if (damage_stage_tables_188.data != 0) {
        srHeap.free(damage_stage_tables_188.data);
    }
    damage_stage_tables_188.data = replacement;
    damage_stage_tables_188.capacity = new_count;

    int base_table = stage > 0 ? damage_stage_tables_188.data[0] : -1;
    damage_stage_tables_188.data[stage] =
        mesh->CreateSkinTable00473260(name, base_table);
    for (mesh = mesh->next; mesh != 0; mesh = mesh->next) {
        mesh->CreateSkinTable00473260(name, base_table);
    }
    return stage;
}

// FUNCTION: WIZ8 0x00480670
int stModelInstance::AddExistingDamageStage00480670(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(model());
    int table = mesh->FindSkinTable004736D0(name);

    if (table == -1) {
        return -1;
    }

    int stage = damage_stage_tables_188.capacity;
    int new_count = stage + 1;
    int* replacement = static_cast<int*>(
        srHeap.allocate(new_count * sizeof(int)));
    for (int index = 0; index < (int)damage_stage_tables_188.capacity; ++index) {
        replacement[index] = damage_stage_tables_188.data[index];
    }
    if (damage_stage_tables_188.data != 0) {
        srHeap.free(damage_stage_tables_188.data);
    }
    damage_stage_tables_188.data = replacement;
    damage_stage_tables_188.capacity = new_count;
    damage_stage_tables_188.data[stage] = table;
    return stage;
}

// FUNCTION: WIZ8 0x004807b0
unsigned char stModelInstance::ReplaceDamageStageTexture004807B0(
    int stage, const char* old_name, srTextureIFace* replacement)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(model());
    unsigned char replaced = 0;

    if (replacement != 0) {
        if (replacement->getClassID() == stTextureAnim::CLASS_ID) {
            static_cast<stTextureAnim*>(replacement)->Prepare004857B0();
        }
        replacement->getClassID();
    }

    for (; mesh != 0; mesh = mesh->next) {
        srPtr<srTextureIFace>* textures =
            mesh->GetTextureTable00473720(damage_stage_tables_188.data[stage]);
        if (textures == 0) {
            continue;
        }

        for (int polygon = 0; polygon < mesh->polygon_count_230; ++polygon) {
            srTextureIFace* texture = textures[polygon].get();
            if (texture == 0 ||
                (texture->getClassID() != 0x10001 &&
                 texture->getClassID() != stTextureAnim::CLASS_ID)) {
                continue;
            }

            if (_stricmp(texture->getName(), old_name) == 0) {
                replaced = 1;
                while (polygon < mesh->polygon_count_230 &&
                       textures[polygon].get() == texture) {
                    textures[polygon] = replacement;
                    ++polygon;
                }
            }
            else {
                while (polygon < mesh->polygon_count_230 &&
                       textures[polygon].get() == texture) {
                    ++polygon;
                }
            }
            --polygon;
        }
    }
    return replaced;
}

// TEMPLATE: WIZ8 0x00481860
// srClassSupport<stModelInstance,srModelInstance,0,65540>::getClassID

// TEMPLATE: WIZ8 0x00481870
// srClassSupport<stModelInstance,srModelInstance,0,65540>::getClassName

// TEMPLATE: WIZ8 0x00481880
// srClassSupport<stModelInstance,srModelInstance,0,65540>::getClassNode

// TEMPLATE: WIZ8 0x00481920
// srClassSupport<stModelInstance,srModelInstance,0,65540>::clone

// TEMPLATE: WIZ8 0x00481940
// srClassSupport<stModelInstance,srModelInstance,0,65540>::~srClassSupport

// FUNCTION: WIZ8 0x0047F410
stModelInstance2D::~stModelInstance2D()
{
    if (vector_174 != 0) {
        srHeap.free(vector_174);
    }
    if (vector_178 != 0) {
        srHeap.free(vector_178);
    }
    if (m_pGlowMaterial_17c != 0) {
        m_pGlowMaterial_17c->release();
    }
}

// FUNCTION: WIZ8 0x0047F290
stModelInstance2D& stModelInstance2D::operator=(const stModelInstance2D& other)
{
    srModelInstance::operator=(other);
    state_170 = other.state_170;
    left_168 = other.left_168;
    top_16a = other.top_16a;
    right_16c = other.right_16c;
    bottom_16e = other.bottom_16e;
    state_160 = other.state_160;
    if (other.parentNode() != 0) {
        setParent(other.parentNode(), 1);
    }
    state_171 = other.state_171;
    render_depth_164 = other.render_depth_164;
    if (other.vector_174 != 0) {
        vector_174 = static_cast<srVector4T<float>*>(
            srHeap.allocate(sizeof(srVector4T<float>)));
        *vector_174 = *other.vector_174;
    }
    if (other.vector_178 != 0) {
        vector_178 = static_cast<srVector4T<float>*>(
            srHeap.allocate(sizeof(srVector4T<float>)));
        *vector_178 = *other.vector_178;
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047F3A0
void stModelInstance2D::SetModel0047F3A0(srModel* model)
{
    assignModel(model);
    if (model != 0) {
        static_cast<srMeshModel*>(model)->enableStartupControls();
    }
}

/* Render the instance through SurRender's detached TriMesh value. Aligned
   instances retain their screen-facing orientation while preserving the
   current view matrix's translation, scale and handedness. The optional glow
   path clones the mesh material once, oscillates between the two configured
   emissive colors, and overrides only this draw's material and first shader. */
// FUNCTION: WIZ8 0x00480920
void stModelInstance2D::process(const ProcessInfo& info, e_processType)
{
    srMeshModel::TriMesh mesh;
    srGERD* renderer = info.renderer;

    if ((alignment_flags_148 & 1) == 0) {
        applyWorldSpaceMatrix(*renderer);
    }
    else {
        srMatrix4T<float> view;
        srVector3T<double> world_location;
        srVector3T<double> world_scale;
        srVector4T<float> transformed_location;
        srVector3T<float> translation;
        float basis_x;
        float basis_y;
        float basis_z;

        renderer->matrixMode(srGERD::MATRIX_MODE_POSITIONAL_0);
        renderer->pushMatrix();
        renderer->getMatrix(srGERD::MATRIX_MODE_POSITIONAL_0, view);
        world_location = getWorldSpaceLocation();
        world_scale = getWorldSpaceScale();

        transformed_location.x =
            view.vectors[0].x * (float)world_location.x
            + view.vectors[0].y * (float)world_location.y
            + view.vectors[0].z * (float)world_location.z
            + view.vectors[0].w;
        transformed_location.y =
            view.vectors[1].x * (float)world_location.x
            + view.vectors[1].y * (float)world_location.y
            + view.vectors[1].z * (float)world_location.z
            + view.vectors[1].w;
        transformed_location.z =
            view.vectors[2].x * (float)world_location.x
            + view.vectors[2].y * (float)world_location.y
            + view.vectors[2].z * (float)world_location.z
            + view.vectors[2].w;
        transformed_location.w =
            view.vectors[3].x * (float)world_location.x
            + view.vectors[3].y * (float)world_location.y
            + view.vectors[3].z * (float)world_location.z
            + view.vectors[3].w;

        basis_x = (float)sqrt(
            view.vectors[0].x * view.vectors[0].x
            + view.vectors[1].x * view.vectors[1].x
            + view.vectors[2].x * view.vectors[2].x);
        basis_y = (float)sqrt(
            view.vectors[0].y * view.vectors[0].y
            + view.vectors[1].y * view.vectors[1].y
            + view.vectors[2].y * view.vectors[2].y);
        basis_z = (float)sqrt(
            view.vectors[0].z * view.vectors[0].z
            + view.vectors[1].z * view.vectors[1].z
            + view.vectors[2].z * view.vectors[2].z);

        float determinant =
            (view.vectors[1].y * view.vectors[2].z
             - view.vectors[1].z * view.vectors[2].y)
                * view.vectors[0].x
            + view.vectors[2].x
                * (view.vectors[0].y * view.vectors[1].z
                   - view.vectors[0].z * view.vectors[1].y)
            + view.vectors[1].x
                * (view.vectors[0].z * view.vectors[2].y
                   - view.vectors[0].y * view.vectors[2].z);
        if (determinant > g_zero_005ebb40) {
            basis_x = -basis_x;
            basis_y = -basis_y;
            basis_z = -basis_z;
        }

        renderer->loadIdentity();
        translation.x = transformed_location.x;
        translation.y = transformed_location.y;
        translation.z = transformed_location.z;
        renderer->translate(translation);
        if (align_angle_158 != g_float_005ebb34) {
            renderer->rotate((double)align_angle_158, align_axis_14c);
        }
        renderer->scale(
            world_scale.x * basis_x,
            world_scale.y * basis_y,
            -(world_scale.z * basis_z));
    }

    srMeshModel* model = static_cast<srMeshModel*>(
        static_cast<srModel::Client&>(*this).getModel());
    model->getTriMesh(mesh);

    if (state_171 != 0) {
        if (m_pGlowMaterial_17c == 0) {
            m_pGlowMaterial_17c = new stMaterial;
            if (m_pGlowMaterial_17c == 0) {
                srAssertFail(
                    "m_pGlowMaterial",
                    ST_MODEL_INSTANCE_CPP,
                    926,
                    0);
            }
            if (mesh.material_070 != 0) {
                *m_pGlowMaterial_17c = *mesh.material_070;
            }
            if (m_pGlowMaterial_17c == 0) {
                goto render_mesh;
            }
        }

        float glow_weight = (float)fabs(sin(
            ((double)(GetTickCount() % render_depth_164)
             / (double)(int)render_depth_164)
            * g_camera_angle_period_005ec014));
        float base_weight = g_float_005ebb38 - glow_weight;
        srVector4T<float> emissive;
        emissive.x =
            vector_174->x * base_weight + vector_178->x * glow_weight;
        emissive.y =
            vector_174->y * base_weight + vector_178->y * glow_weight;
        emissive.z =
            vector_174->z * base_weight + vector_178->z * glow_weight;
        emissive.w = g_float_005ebb38;
        m_pGlowMaterial_17c->setEmissive(emissive);
        mesh.material_070 = m_pGlowMaterial_17c;
        mesh.shaders_0b0[0].value =
            (mesh.shaders_0b0[0].value & ~0x400UL) | 0x800UL;
    }

render_mesh:
    model->renderTriMesh(*renderer, mesh);
    renderer->popMatrix();
}

/* Disabling the glow also drops the material cloned by process(); enabling it
   leaves an existing clone available for the next draw. */
// FUNCTION: WIZ8 0x00480EB0
void stModelInstance2D::SetGlowEnabled00480EB0(unsigned char enabled)
{
    if (enabled == 0 && m_pGlowMaterial_17c != 0) {
        m_pGlowMaterial_17c->release();
        m_pGlowMaterial_17c = 0;
    }
    state_171 = enabled;
}

// FUNCTION: WIZ8 0x00480EF0
unsigned short stModelInstance2D::GetScaledWidth00480EF0() const
{
    srVector3T<double> scale = getScale();
    if (scale.x == g_float_005ebb38 &&
        scale.y == g_float_005ebb38 &&
        (float)scale.z == g_float_005ebb38) {
        return static_cast<unsigned short>(left_168);
    }
    return static_cast<unsigned short>(
        scale.x * static_cast<unsigned short>(left_168));
}

// FUNCTION: WIZ8 0x00480F70
unsigned short stModelInstance2D::GetScaledHeight00480F70() const
{
    srVector3T<double> scale = getScale();
    if (scale.x == g_float_005ebb38 &&
        (float)scale.y == g_float_005ebb38 &&
        (float)scale.z == g_float_005ebb38) {
        return static_cast<unsigned short>(top_16a);
    }
    return static_cast<unsigned short>(
        scale.z * static_cast<unsigned short>(top_16a));
}

// FUNCTION: WIZ8 0x00480FF0
void stModelInstance2D::SetGlowColors00480FF0(
    const srVector4T<float>* first,
    const srVector4T<float>* second)
{
    if (vector_174 == 0) {
        vector_174 = static_cast<srVector4T<float>*>(
            srHeap.allocate(sizeof(srVector4T<float>)));
    }
    *vector_174 = *first;

    if (vector_178 == 0) {
        vector_178 = static_cast<srVector4T<float>*>(
            srHeap.allocate(sizeof(srVector4T<float>)));
    }
    *vector_178 = *second;
}

// FUNCTION: WIZ8 0x00481E30
srClass* stModelInstance2D::vInstance()
{
    return new stModelInstance2D(0);
}

// TEMPLATE: WIZ8 0x00481C80
// srArray<srNode::TraverseInfo::Entry>::setCapacity

// TEMPLATE: WIZ8 0x00481D00
// srClassSupport<srModelInstance,srNode,0,4352>::sGetClassNode

// TEMPLATE: WIZ8 0x00481D70
// srArray<srTriMeshPipeline::Record>::operator[]

// TEMPLATE: WIZ8 0x00481DA0
// srArray<srTriMeshPipeline::Pass>::operator[]

// SYNTHETIC: WIZ8 0x0047EDC0
// stModelInstance::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0047EC80
stModelInstance::stModelInstance(srNode* parent)
    : srClassSupport<stModelInstance, srModelInstance, false, 0x10004>(
          static_cast<srNode*>(0))
{
    render_depth_164 = 0;
    state_168 = 0;
    state_16c = 0;
    state_170_173 = 0;
    state_178 = 0;
    state_17c = static_cast<unsigned long>(-1);
    frame_index_180 = 0;
    value_190 = 0;
    if (parent != 0) {
        setParent(parent, 1);
    }
    damage_stage_184 = -1;
    material_174 = 0;
    scale_194.x = 1.0f;
    scale_194.y = 1.0f;
    scale_194.z = 1.0f;
    flag_1a0 = 0;
    value_1a4 = 0;
    flag_1a1 = 0;
    value_1a8 = 0;
    value_1ac = 0.0f;
}

// FUNCTION: WIZ8 0x0047EDF0
stModelInstance& stModelInstance::operator=(const stModelInstance& other)
{
    srModelInstance::operator=(other);
    render_depth_164 = 0;
    state_168 = 0;
    state_16c = 0;
    state_170_173 = 0;
    state_178 = other.state_178;
    state_17c = other.state_17c;
    frame_index_180 = other.frame_index_180;

    if (this != &other) {
        if (damage_stage_tables_188.data != 0) {
            srHeap.free(damage_stage_tables_188.data);
        }
        damage_stage_tables_188.data = 0;
        damage_stage_tables_188.capacity = 0;
        if (other.damage_stage_tables_188.capacity != 0) {
            damage_stage_tables_188.data = static_cast<int*>(srHeap.allocate(
                other.damage_stage_tables_188.capacity * sizeof(int)));
            damage_stage_tables_188.capacity =
                other.damage_stage_tables_188.capacity;
            for (int stage = 0;
                 stage < (int)damage_stage_tables_188.capacity;
                 ++stage) {
                damage_stage_tables_188.data[stage] =
                    other.damage_stage_tables_188.data[stage];
            }
        }
    }
    damage_stage_184 = other.damage_stage_184;
    value_190 = other.value_190;
    material_174 = 0;
    scale_194 = other.scale_194;
    value_1a4 = 0;
    flag_1a0 = 0;
    flag_1a1 = other.flag_1a1;
    value_1a8 = other.value_1a8;
    value_1ac = 0.0f;
    return *this;
}

/* Meshes with the low traversal flag clear are submitted before the sibling
   chain; meshes with it set are submitted afterward. Children remain last,
   matching the scene graph's positional-flag ordering. */
// FUNCTION: WIZ8 0x004803F0
void stModelInstance::traverse(TraverseInfo& info)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(model());

    if (!testFlag(FLAG_POSITIONAL_0) &&
        mesh != 0 &&
        (mesh->flags_3a0 & 1) == 0) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_POSITIONAL_0) &&
        mesh != 0 &&
        (mesh->flags_3a0 & 1) != 0) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

/* The 3D instance uses the same billboard transform as its 2D sibling, then
   scopes the instance exclusion bits around the complete mesh-chain render.
   The render helper owns culling, frame selection, damage tables, materials
   and the optional second pass. */
// FUNCTION: WIZ8 0x0047F560
void stModelInstance::process(
    const ProcessInfo& info, e_processType)
{
    srGERD* renderer = info.renderer;

    if ((alignment_flags_148 & 1) == 0) {
        applyWorldSpaceMatrix(*renderer);
    }
    else {
        srMatrix4T<float> view;
        srVector3T<double> world_location;
        srVector3T<double> world_scale;
        srVector4T<float> transformed_location;
        srVector3T<float> translation;
        float basis_x;
        float basis_y;
        float basis_z;

        renderer->matrixMode(srGERD::MATRIX_MODE_POSITIONAL_0);
        renderer->pushMatrix();
        renderer->getMatrix(srGERD::MATRIX_MODE_POSITIONAL_0, view);
        world_location = getWorldSpaceLocation();
        world_scale = getWorldSpaceScale();

        transformed_location.x =
            view.vectors[0].x * (float)world_location.x
            + view.vectors[0].y * (float)world_location.y
            + view.vectors[0].z * (float)world_location.z
            + view.vectors[0].w;
        transformed_location.y =
            view.vectors[1].x * (float)world_location.x
            + view.vectors[1].y * (float)world_location.y
            + view.vectors[1].z * (float)world_location.z
            + view.vectors[1].w;
        transformed_location.z =
            view.vectors[2].x * (float)world_location.x
            + view.vectors[2].y * (float)world_location.y
            + view.vectors[2].z * (float)world_location.z
            + view.vectors[2].w;
        transformed_location.w =
            view.vectors[3].x * (float)world_location.x
            + view.vectors[3].y * (float)world_location.y
            + view.vectors[3].z * (float)world_location.z
            + view.vectors[3].w;

        basis_x = (float)sqrt(
            view.vectors[0].x * view.vectors[0].x
            + view.vectors[1].x * view.vectors[1].x
            + view.vectors[2].x * view.vectors[2].x);
        basis_y = (float)sqrt(
            view.vectors[0].y * view.vectors[0].y
            + view.vectors[1].y * view.vectors[1].y
            + view.vectors[2].y * view.vectors[2].y);
        basis_z = (float)sqrt(
            view.vectors[0].z * view.vectors[0].z
            + view.vectors[1].z * view.vectors[1].z
            + view.vectors[2].z * view.vectors[2].z);

        float determinant =
            (view.vectors[1].y * view.vectors[2].z
             - view.vectors[1].z * view.vectors[2].y)
                * view.vectors[0].x
            + view.vectors[2].x
                * (view.vectors[0].y * view.vectors[1].z
                   - view.vectors[0].z * view.vectors[1].y)
            + view.vectors[1].x
                * (view.vectors[0].z * view.vectors[2].y
                   - view.vectors[0].y * view.vectors[2].z);
        if (determinant > g_zero_005ebb40) {
            basis_x = -basis_x;
            basis_y = -basis_y;
            basis_z = -basis_z;
        }

        renderer->loadIdentity();
        translation.x = transformed_location.x;
        translation.y = transformed_location.y;
        translation.z = transformed_location.z;
        renderer->translate(translation);
        if (align_angle_158 != g_float_005ebb34) {
            renderer->rotate((double)align_angle_158, align_axis_14c);
        }
        renderer->scale(
            world_scale.x * basis_x,
            world_scale.y * basis_y,
            -(world_scale.z * basis_z));
    }

    if (exclusion_mask_15c != 0) {
        unsigned long old_mask = renderer->getExclusionMask();
        renderer->setExclusionMask(old_mask | exclusion_mask_15c);
        Render0047F930(renderer);
        renderer->setExclusionMask(old_mask);
    }
    else {
        Render0047F930(renderer);
    }
    renderer->popMatrix();
}

/* Cull and render the complete linked mesh chain. The first pass selects the
   requested animation frame and damage-stage polygon table. A non-zero
   emissive vector creates one retained material and drives the optional
   second pass with reversed winding. */
// FUNCTION: WIZ8 0x0047F930
void stModelInstance::Render0047F930(srGERD* renderer)
{
    stMeshModel* root = static_cast<stMeshModel*>(model());
    srVector3T<float> bounds_center;
    float bounds_radius;

    if (root == 0) {
        return;
    }
    root->getBoundingSphere(bounds_center, bounds_radius);
    if ((root->control_state_394 & 0x20) == 0 &&
        renderer->testBoundingSphere(bounds_center, bounds_radius) ==
            srGERD::VISIBILITY_POSITIONAL_0) {
        return;
    }
    if ((root->control_state_394 & 0x10) == 0 &&
        root->vertex_location_count_22c > 7) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        root->getBoundingBox(minimum, maximum);
        if (renderer->testBoundingBox(minimum, maximum) ==
            srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    SetModelPickKey004277F0(
        (state_178 & 0x10) != 0
            ? 0
            : reinterpret_cast<unsigned long>(this));

    srVector4T<float> saved_ambient;
    srVector4T<float> model_ambient;
    srVector3T<float> mesh_ambient;
    renderer->getAmbientLight(saved_ambient);
    mesh_ambient.x = saved_ambient.x;
    mesh_ambient.y = saved_ambient.y;
    mesh_ambient.z = saved_ambient.z;
    if (root->flag_3cd == 0) {
        model_ambient.x =
            (saved_ambient.x * scale_194.x + g_model_ambient_offset_x_00659cd0)
            * g_light_scale_0060bfe0;
        model_ambient.y =
            (saved_ambient.y * scale_194.y + g_model_ambient_offset_y_00659cd4)
            * g_light_scale_0060bfe0;
        model_ambient.z =
            (saved_ambient.z * scale_194.z + g_model_ambient_offset_z_00659cd8)
            * g_light_scale_0060bfe0;
        model_ambient.w = 1.0f;
    }
    else {
        model_ambient.x = 0.0f;
        model_ambient.y = 0.0f;
        model_ambient.z = 0.0f;
        model_ambient.w = 0.0f;
    }
    renderer->setAmbientLight(model_ambient);

    bool emissive =
        emissive_x_164 != 0.0f || emissive_y_168 != 0.0f ||
        emissive_z_16c != 0.0f || emissive_w_170 != 0.0f;
    if (emissive) {
        if (material_174 == 0) {
            srVector4T<float> zero;
            zero.x = 0.0f;
            zero.y = 0.0f;
            zero.z = 0.0f;
            zero.w = 0.0f;
            material_174 = new srMaterial;
            material_174->setAmbient(zero);
            material_174->setDiffuse(zero);
            material_174->setSpecular(zero);
            material_174->setShininess(0.35);
        }
        srVector4T<float> color;
        color.x = emissive_x_164;
        color.y = emissive_y_168;
        color.z = emissive_z_16c;
        color.w = emissive_w_170;
        material_174->setEmissive(color);
    }

    bool first_mesh = true;
    srNode* mesh_node = this;
    for (stMeshModel* current = root; current != 0; current = current->next) {
        srMeshModel::TriMesh tri_mesh;
        current->SetAmbientColor00472990(mesh_ambient);
        current->getTriMesh(tri_mesh);

        if ((state_178 & 8) != 0 && first_mesh) {
            DebugRenderModel004811D0(renderer, tri_mesh);
        }

        void* frame_values = 0;
        if ((current->flags_3a0 & 4) != 0) {
            tri_mesh.vertex_locations_038 =
                current->GetVertexLocations00471AD0(
                    frame_index_180, 1, value_1ac);
            tri_mesh.vertex_normals_03c =
                current->GetVertexNormals00471CA0(frame_index_180, 1);
            frame_values =
                current->GetFrameValues00471D00(frame_index_180, 1);
        }

        if (g_disable_model_textures_0065a146 != 0) {
            tri_mesh.shaders_0b0[0].value &= 0xffff73ffUL;
            tri_mesh.value_0f0 = 0;
            tri_mesh.polygon_textures_0d0 = 0;
            tri_mesh.value_100 = 0;
            tri_mesh.values_040 = 0;
        }
        if (g_disable_model_values_0065a0ec != 0) {
            tri_mesh.vertex_normals_03c = 0;
        }

        int texture_table = -1;
        if (damage_stage_184 >= 0) {
            texture_table = damage_stage_tables_188.data[damage_stage_184];
            tri_mesh.polygon_textures_0d0 =
                current->GetTextureTable00473720(texture_table);
        }
        if (tri_mesh.polygon_table_count_150 == 0) {
            tri_mesh.polygon_table_14c = current->GetPolygonTable00473CD0(
                &tri_mesh.polygon_table_count_150, texture_table, 1);
        }

        if (emissive && value_190 == 1) {
            tri_mesh.material_070 = material_174;
            tri_mesh.value_0c0 = 0;
        }
        else {
            if (flag_1a0 != 0) {
                g_model_value_1a0_0065baa0 = scale_1a4;
                g_model_value_1a0_enabled_0065ba9e = 1;
                tri_mesh.shaders_0b0[0].value =
                    (tri_mesh.shaders_0b0[0].value & 0xffffd7bfUL) | 0x44a0;
                tri_mesh.flags_00c |= 0x40;
            }
            if (flag_1a1 != 0) {
                g_model_value_1a1_0065baa8 = value_1a8;
                g_model_value_1a1_enabled_0065baa4 = 1;
            }
        }

        srGERD::Pick pick;
        if ((state_178 & 0x10) != 0 && !renderer->isPickStackEmpty()) {
            renderer->popPick(pick);
            current->RenderTriMesh00470380(renderer, tri_mesh, frame_values);
            renderer->pushPick(pick);
        }
        else {
            current->RenderTriMesh00470380(renderer, tri_mesh, frame_values);
        }
        if (flag_1a0 != 0) {
            g_model_value_1a0_enabled_0065ba9e = 0;
        }
        if (flag_1a1 != 0) {
            g_model_value_1a1_enabled_0065baa4 = 0;
        }
        srNode* rendered_node = mesh_node;
        if (rendered_node != 0 && rendered_node->testFlag(FLAG_POSITIONAL_1)) {
            break;
        }
        else if (rendered_node != 0) {
            mesh_node = rendered_node->firstChild();
        }
        first_mesh = false;
    }

    if (emissive) {
        if (value_190 == 0) {
            renderer->setWinding((srGERD::e_winding)1);
        }
        bool first = true;
        srNode* mesh_node = this;
        for (stMeshModel* current = root;
            current != 0;
             current = current->next) {
            if ((current->flags_3a0 & 1) != 0 && !first) {
                srNode* skipped_node = mesh_node;
                if (skipped_node != 0 &&
                    skipped_node->testFlag(FLAG_POSITIONAL_1)) {
                    break;
                }
                if (skipped_node != 0) {
                    mesh_node = skipped_node->firstChild();
                }
                first = false;
                continue;
            }

            srMeshModel::TriMesh tri_mesh;
            current->getTriMesh(tri_mesh);
            if (tri_mesh.polygon_textures_0d0 == 0 &&
                tri_mesh.texture_090 != 0 &&
                _strnicmp(tri_mesh.texture_090->getName(), "blank", 5) == 0) {
                first = false;
                continue;
            }

            void* frame_values = 0;
            if ((current->flags_3a0 & 4) != 0) {
                tri_mesh.vertex_locations_038 =
                    current->GetVertexLocations00471AD0(
                        frame_index_180, 1, value_1ac);
                tri_mesh.vertex_normals_03c =
                    current->GetVertexNormals00471CA0(frame_index_180, 1);
                frame_values =
                    current->GetFrameValues00471D00(frame_index_180, 1);
            }
            int texture_table = damage_stage_184 < 0
                ? -1
                : damage_stage_tables_188.data[damage_stage_184];
            if (tri_mesh.polygon_table_count_150 == 0) {
                tri_mesh.polygon_table_14c = current->GetPolygonTable00473CD0(
                    &tri_mesh.polygon_table_count_150, texture_table, 1);
            }
            tri_mesh.material_070 = material_174;
            tri_mesh.shaders_0b0[0].value =
                (tri_mesh.shaders_0b0[0].value & 0xffff5cb7UL) | 0x40a0;
            tri_mesh.value_0f0 = 0;
            tri_mesh.polygon_textures_0d0 = 0;
            tri_mesh.value_0c0 = 0;
            tri_mesh.value_100 = 0;
            tri_mesh.flags_00c |= 0x40;

            if (g_emissive_vertices_0065a148 == 0) {
                g_emissive_vertices_0065a148 =
                    new srHeapArray<srVector3T<float> >;
            }
            unsigned long vertex_count = tri_mesh.vertex_count_000;
            srVector3T<float>* emissive_vertices =
                g_emissive_vertices_0065a148->ensureExact(
                    vertex_count * sizeof(srVector3T<float>));

            srVector3T<double> node_scale = getScale();
            double inverse_scale = 15.625 / node_scale.z;
            node_scale = getScale();
            float extrusion = static_cast<float>(
                (bounds_radius * node_scale.z * 0.001 + 1.0) *
                inverse_scale);
            srVector3T<float> extrusion_vector;
            extrusion_vector.x = extrusion;
            extrusion_vector.y = extrusion;
            extrusion_vector.z = extrusion;

            if (vertex_count != 0) {
                if (extrusion_vector.x == 0.0f &&
                    extrusion_vector.y == 0.0f &&
                    extrusion_vector.z == 0.0f) {
                    srVectorProcessor::copy(
                        reinterpret_cast<SRDWORD*>(emissive_vertices),
                        0, vertex_count * 3);
                }
                else {
                    srVectorProcessor::multiply(
                        emissive_vertices, extrusion_vector,
                        tri_mesh.vertex_normals_03c, vertex_count);
                }
            }
            if (vertex_count * 3 != 0) {
                srVectorProcessor::add(
                    reinterpret_cast<float*>(emissive_vertices),
                    reinterpret_cast<float*>(emissive_vertices),
                    reinterpret_cast<const float*>(
                        tri_mesh.vertex_locations_038),
                    vertex_count * 3);
            }
            tri_mesh.vertex_locations_038 = emissive_vertices;

            srGERD::Pick pick;
            if ((state_178 & 0x10) != 0 && !renderer->isPickStackEmpty()) {
                renderer->popPick(pick);
                current->RenderTriMesh00470380(
                    renderer, tri_mesh, frame_values);
                renderer->pushPick(pick);
            }
            else {
                current->RenderTriMesh00470380(
                    renderer, tri_mesh, frame_values);
            }
            srNode* rendered_node = mesh_node;
            if (rendered_node != 0 &&
                rendered_node->testFlag(FLAG_POSITIONAL_1)) {
                break;
            }
            else if (rendered_node != 0) {
                mesh_node = rendered_node->firstChild();
            }
            first = false;
        }
        if (value_190 == 0) {
            renderer->setWinding(srGERD::WINDING_POSITIONAL_0);
        }
    }
    renderer->setAmbientLight(saved_ambient);
}

// FUNCTION: WIZ8 0x00481DD0
srClass* stModelInstance::vInstance()
{
    return new stModelInstance(0);
}

// FUNCTION: WIZ8 0x0047EF70
stModelInstance::~stModelInstance()
{
    if (material_174 != 0) {
        material_174->release();
    }
}
