#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srNode.h"
#include "surrender/srHeap.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <new>
#include <string.h>

#define ST_MODEL_INSTANCE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\stModelInstance.cpp"

// VTABLE: WIZ8 0x005ec89c srClassSupport<srModelInstance, class srNode, 0, 4352>
// VTABLE: WIZ8 0x005ec88c srModel::Client
// class srClassSupport<stModelInstance2D, class srModelInstance, 0, 65541>

// SYNTHETIC: WIZ8 0x0047F260
// stModelInstance2D::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00481C50
// srClassSupport<stModelInstance2D,srModelInstance,0,65541>::`scalar deleting destructor'

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
 * Two model-instance classes, a 3D one and a 2D one, whose registry ids are
 * adjacent. Only their class registry slots are recovered; the interval this
 * unit bounds still holds the rest.
 */

/* Find the animated texture assigned to polygons whose runtime name begins
   with "mouth". Damage-stage instances use the stage-specific texture table;
   ordinary instances use the mesh's active polygon texture table. */
// FUNCTION: WIZ8 0x00481080
stTextureAnim* stModelInstance::FindMouthTexture00481080()
{
    stMeshModel* mesh = static_cast<stMeshModel*>(getModel());

    if (damage_stage_184 == -1) {
        while (mesh != 0) {
            srPtr<srTextureIFace>* textures = mesh->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                for (int polygon = 0; polygon < mesh->polygon_count_230; ++polygon) {
                    srTextureIFace* texture = textures[polygon].get();

                    if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID &&
                        _strnicmp(texture->getName(), "mouth", 5) == 0) {
                        return static_cast<stTextureAnim*>(texture);
                    }
                }
            }
            mesh = mesh->next;
        }
    } else {
        while (mesh != 0) {
            srPtr<srTextureIFace>* textures =
                mesh->GetTextureTable00473720(damage_stage_tables_188.data[damage_stage_184]);

            if (textures != 0) {
                for (int polygon = 0; polygon < mesh->polygon_count_230; ++polygon) {
                    srTextureIFace* texture = textures[polygon].get();

                    if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID &&
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

// FUNCTION: WIZ8 0x00480790
int stModelInstance::FindDamageStage00480790(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(getModel());
    return mesh->FindSkinTable004736D0(name);
}

/* Add a stage by cloning the first stage's table across the complete linked
   mesh chain. The instance stores the table id shared by that chain. */
// FUNCTION: WIZ8 0x00480560
int stModelInstance::AddDamageStage00480560(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(getModel());

    if (mesh->FindSkinTable004736D0(name) != -1) {
        return -1;
    }

    int stage = damage_stage_tables_188.capacity;
    damage_stage_tables_188.setCapacity(stage + 1);

    int base_table = stage > 0 ? damage_stage_tables_188.data[0] : -1;
    damage_stage_tables_188.data[stage] = mesh->CreateSkinTable00473260(name, base_table);
    for (mesh = mesh->next; mesh != 0; mesh = mesh->next) {
        mesh->CreateSkinTable00473260(name, base_table);
    }
    return stage;
}

// FUNCTION: WIZ8 0x00480670
int stModelInstance::AddExistingDamageStage00480670(const char* name)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(getModel());
    int table = mesh->FindSkinTable004736D0(name);

    if (table == -1) {
        return -1;
    }

    int stage = damage_stage_tables_188.capacity;
    damage_stage_tables_188.setCapacity(stage + 1);
    damage_stage_tables_188.data[stage] = table;
    return stage;
}

// FUNCTION: WIZ8 0x004807b0
unsigned char stModelInstance::ReplaceDamageStageTexture004807B0(int stage, const char* old_name,
                                                                 srTextureIFace* replacement)
{
    stMeshModel* mesh = static_cast<stMeshModel*>(getModel());
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
            if (texture == 0 || (texture->getClassID() != 0x10001 &&
                                 texture->getClassID() != stTextureAnim::CLASS_ID)) {
                continue;
            }

            if (_stricmp(texture->getName(), old_name) == 0) {
                replaced = 1;
                while (polygon < mesh->polygon_count_230 && textures[polygon].get() == texture) {
                    textures[polygon] = replacement;
                    ++polygon;
                }
            } else {
                while (polygon < mesh->polygon_count_230 && textures[polygon].get() == texture) {
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

// TEMPLATE: WIZ8 0x00481940
// srClassSupport<stModelInstance,srModelInstance,0,65540>::~srClassSupport<stModelInstance,srModelInstance,0,65540>

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
    render_state_164.display_state = other.render_state_164.display_state;
    render_state_164.left = other.render_state_164.left;
    render_state_164.top = other.render_state_164.top;
    render_state_164.right = other.render_state_164.right;
    render_state_164.bottom = other.render_state_164.bottom;
    state_160 = other.state_160;
    if (other.parentNode() != 0) {
        setParent(other.parentNode(), 1);
    }
    render_state_164.state_0d = other.render_state_164.state_0d;
    render_state_164.render_depth = other.render_state_164.render_depth;
    if (other.vector_174 != 0) {
        vector_174 = static_cast<srVector4T<float>*>(srHeap.allocate(sizeof(srVector4T<float>)));
        *vector_174 = *other.vector_174;
    }
    if (other.vector_178 != 0) {
        vector_178 = static_cast<srVector4T<float>*>(srHeap.allocate(sizeof(srVector4T<float>)));
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
    } else {
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

        transformed_location.x = view.vectors[0].x * (float)world_location.x +
                                 view.vectors[0].y * (float)world_location.y +
                                 view.vectors[0].z * (float)world_location.z + view.vectors[0].w;
        transformed_location.y = view.vectors[1].x * (float)world_location.x +
                                 view.vectors[1].y * (float)world_location.y +
                                 view.vectors[1].z * (float)world_location.z + view.vectors[1].w;
        transformed_location.z = view.vectors[2].x * (float)world_location.x +
                                 view.vectors[2].y * (float)world_location.y +
                                 view.vectors[2].z * (float)world_location.z + view.vectors[2].w;
        transformed_location.w = view.vectors[3].x * (float)world_location.x +
                                 view.vectors[3].y * (float)world_location.y +
                                 view.vectors[3].z * (float)world_location.z + view.vectors[3].w;

        basis_x = (float)sqrt(view.vectors[0].x * view.vectors[0].x +
                              view.vectors[1].x * view.vectors[1].x +
                              view.vectors[2].x * view.vectors[2].x);
        basis_y = (float)sqrt(view.vectors[0].y * view.vectors[0].y +
                              view.vectors[1].y * view.vectors[1].y +
                              view.vectors[2].y * view.vectors[2].y);
        basis_z = (float)sqrt(view.vectors[0].z * view.vectors[0].z +
                              view.vectors[1].z * view.vectors[1].z +
                              view.vectors[2].z * view.vectors[2].z);

        float determinant =
            (view.vectors[1].y * view.vectors[2].z - view.vectors[1].z * view.vectors[2].y) *
                view.vectors[0].x +
            view.vectors[2].x *
                (view.vectors[0].y * view.vectors[1].z - view.vectors[0].z * view.vectors[1].y) +
            view.vectors[1].x *
                (view.vectors[0].z * view.vectors[2].y - view.vectors[0].y * view.vectors[2].z);
        if (determinant > g_zero_005ebb40) {
            basis_x = -basis_x;
            basis_y = -basis_y;
            basis_z = -basis_z;
        }

        renderer->loadIdentity();
        translation.Set(transformed_location.x, transformed_location.y, transformed_location.z);
        renderer->translate(translation);
        if (align_angle_158 != g_float_005ebb34) {
            renderer->rotate((double)align_angle_158, align_axis_14c);
        }
        renderer->scale(world_scale.x * basis_x, world_scale.y * basis_y,
                        -(world_scale.z * basis_z));
    }

    srMeshModel* model = static_cast<srMeshModel*>(getModel());
    model->getTriMesh(mesh);

    if (render_state_164.state_0d != 0) {
        if (m_pGlowMaterial_17c == 0) {
            m_pGlowMaterial_17c = new stMaterial;
            if (m_pGlowMaterial_17c == 0) {
                srAssertFail("m_pGlowMaterial", ST_MODEL_INSTANCE_CPP, 926, 0);
            }
            if (mesh.material_070 != 0) {
                *m_pGlowMaterial_17c = *mesh.material_070;
            }
            if (m_pGlowMaterial_17c == 0) {
                goto render_mesh;
            }
        }

        float glow_weight =
            (float)fabs(sin(((double)(GetTickCount() % render_state_164.render_depth) /
                             (double)(int)render_state_164.render_depth) *
                            g_camera_angle_period_005ec014));
        float base_weight = g_float_005ebb38 - glow_weight;
        srVector4T<float> emissive;
        emissive.x = vector_174->x * base_weight + vector_178->x * glow_weight;
        emissive.y = vector_174->y * base_weight + vector_178->y * glow_weight;
        emissive.z = vector_174->z * base_weight + vector_178->z * glow_weight;
        emissive.w = g_float_005ebb38;
        m_pGlowMaterial_17c->setEmissive(emissive);
        mesh.material_070 = m_pGlowMaterial_17c;
        mesh.shaders_0b0[0].value = (mesh.shaders_0b0[0].value & ~0x400UL) | 0x800UL;
    }

render_mesh:
    model->renderTriMesh(*renderer, mesh);
    renderer->popMatrix();
}

/* Scaled 2D extent used by the tooltip and cursor placement code. A unit
   scale returns the stored screen extent directly; otherwise the matching
   axis scale from the node is applied and truncated. */
// FUNCTION: WIZ8 0x00480EF0
int stModelInstance2D::GetWidth00480EF0()
{
    srVector3T<double> scale = getScale();
    if (scale.x == 1.0 && scale.y == 1.0 && scale.z == 1.0) {
        return render_state_164.left;
    }
    return (int)(render_state_164.left * scale.x);
}

// FUNCTION: WIZ8 0x00480F70
int stModelInstance2D::GetHeight00480F70()
{
    srVector3T<double> scale = getScale();
    if (scale.x == 1.0 && scale.y == 1.0 && scale.z == 1.0) {
        return render_state_164.top;
    }
    return (int)(render_state_164.top * scale.y);
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
    : srClassSupport<stModelInstance, srModelInstance, false, 0x10004>(static_cast<srNode*>(0))
{
    render_state_164.render_depth = 0;
    render_state_164.state_04 = 0;
    render_state_164.state_08 = 0;
    render_state_164.state_0c = 0;
    state_178 = 0;
    state_17c = static_cast<unsigned long>(-1);
    frame_index_180 = 0;
    value_190 = 0;
    if (parent != 0) {
        setParent(parent, 1);
    }
    damage_stage_184 = -1;
    retained_174 = 0;
    scale_194 = 1.0f;
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
    render_state_164.render_depth = 0;
    render_state_164.state_04 = 0;
    render_state_164.state_08 = 0;
    render_state_164.state_0c = 0;
    state_178 = other.state_178;
    state_17c = other.state_17c;
    frame_index_180 = other.frame_index_180;

    damage_stage_tables_188 = other.damage_stage_tables_188;
    damage_stage_184 = other.damage_stage_184;
    value_190 = other.value_190;
    retained_174 = 0;
    scale_194 = other.scale_194;
    value_1a4 = 0;
    flag_1a0 = 0;
    flag_1a1 = other.flag_1a1;
    value_1a8 = other.value_1a8;
    value_1ac = 0.0f;
    return *this;
}

// FUNCTION: WIZ8 0x0047EF70
stModelInstance::~stModelInstance()
{
    if (retained_174 != 0) {
        retained_174->release();
    }
}
