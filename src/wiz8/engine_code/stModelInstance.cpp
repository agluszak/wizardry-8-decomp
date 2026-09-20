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
#include "surrender/srTriMeshPipeline.h"
#include "surrender/srVectorProcessor.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <new>
#include <string.h>

#define ST_MODEL_INSTANCE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\stModelInstance.cpp"

extern srVector3T<float> g_environment_offset_00659cd0;
extern float g_light_scale_0060bfe0;

/* Scratch vertex store shared by every highlight shell submission; grown
   on demand and kept between frames. */
// GLOBAL: WIZ8 0x0065A148
srHeapArray<srVector3T<float> >* g_vertex_scratch_0065a148;

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

/* Retail's Video2, dialogue and radar callers all call this one emitted body,
   which precedes the rest of the stModelInstance2D lifecycle family. It is an
   ordinary stModelInstance.cpp definition, not a header/template emission. */
// FUNCTION: WIZ8 0x0047F0F0
stModelInstance2D::stModelInstance2D(srNode* parent)
    : srClassSupport<stModelInstance2D, srModelInstance, false, 0x10005>(static_cast<srNode*>(0))
{
    render_state_164.display_state = 0;
    render_state_164.left = 0;
    render_state_164.top = 0;
    render_state_164.right = 0;
    render_state_164.bottom = 0;
    state_160 = 0;
    render_state_164.state_0d = 0;
    render_state_164.render_depth = 2000;
    vector_174 = 0;
    vector_178 = 0;
    m_pGlowMaterial = 0;
    if (parent != 0) {
        setParent(parent, 1);
    }
}

// FUNCTION: WIZ8 0x0047F410
stModelInstance2D::~stModelInstance2D()
{
    if (vector_174 != 0) {
        srHeap.free(vector_174);
    }
    if (vector_178 != 0) {
        srHeap.free(vector_178);
    }
    if (m_pGlowMaterial != 0) {
        m_pGlowMaterial->release();
    }
}

/* Deliberately not a whole W8ModelInstance3DRenderState assignment. Retail
   copies the 2D fields around the parent/state work and leaves the upper two
   bytes of state_0c untouched; GrCycle's separate whole-block copy is a
   different operation. */
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

    if ((alignment_flags_148.value & 1) == 0) {
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

        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushMatrix();
        renderer->getMatrix(srGERD::MATRIX_MODELVIEW, view);
        world_location = getWorldSpaceLocation();
        world_scale = getWorldSpaceScale();

        srVector3T<float> location;
        location = world_location;
        transformed_location = view.Transform(location);

        srVector3T<float> column_x(view.vectors[0].x, view.vectors[1].x, view.vectors[2].x);
        srVector3T<float> column_y(view.vectors[0].y, view.vectors[1].y, view.vectors[2].y);
        srVector3T<float> column_z(view.vectors[0].z, view.vectors[1].z, view.vectors[2].z);
        basis_x = column_x.Length();
        basis_y = column_y.Length();
        basis_z = column_z.Length();

        float determinant = Det3(view.vectors[0].x, view.vectors[0].y, view.vectors[0].z,
                                 view.vectors[1].x, view.vectors[1].y, view.vectors[1].z,
                                 view.vectors[2].x, view.vectors[2].y, view.vectors[2].z);
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
        if (m_pGlowMaterial == 0) {
            m_pGlowMaterial = new stMaterial;
            if (m_pGlowMaterial == 0) {
                srAssertFail("m_pGlowMaterial", ST_MODEL_INSTANCE_CPP, 926, 0);
            }
            if (mesh.materials_70[0][0] != 0) {
                *m_pGlowMaterial = *mesh.materials_70[0][0];
            }
            if (m_pGlowMaterial == 0) {
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
        m_pGlowMaterial->setEmissive(emissive);
        mesh.materials_70[0][0] = m_pGlowMaterial;
        mesh.shaders_b0[0].value = (mesh.shaders_b0[0].value & ~srShader::MASK_GRADIENT_MODULATE) |
                                   srShader::MASK_GRADIENT_ADD;
    }

render_mesh:
    model->renderTriMesh(*renderer, mesh);
    renderer->popMatrix();
}

/* Scaled 2D extent used by the tooltip and cursor placement code. A unit
   scale returns the stored screen extent directly; otherwise the matching
   axis scale from the node is applied and truncated. */
/* Disabling the glow releases the retained glow material; the render-state
   byte at 0x0d is the glow pass's enable flag. */
// FUNCTION: WIZ8 0x00480EB0
void stModelInstance2D::SetGlowEnabled00480EB0(unsigned char enable)
{
    if (enable == 0 && m_pGlowMaterial != 0) {
        m_pGlowMaterial->release();
        m_pGlowMaterial = 0;
    }
    render_state_164.state_0d = enable;
}

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

/* Lazily allocate the two glow-color vectors and copy the supplied pair; the
   process pass blends between them into the material's emissive term. */
// FUNCTION: WIZ8 0x00480FF0
void stModelInstance2D::SetGlowColors00480FF0(srVector4T<float>* first, srVector4T<float>* second)
{
    if (vector_174 == 0) {
        vector_174 = static_cast<srVector4T<float>*>(srHeap.allocate(sizeof(srVector4T<float>)));
    }
    *vector_174 = *first;
    if (vector_178 == 0) {
        vector_178 = static_cast<srVector4T<float>*>(srHeap.allocate(sizeof(srVector4T<float>)));
    }
    *vector_178 = *second;
}

// FUNCTION: WIZ8 0x00481E30
srClass* stModelInstance2D::vInstance()
{
    return new stModelInstance2D(0);
}

// FUNCTION: WIZ8 0x00481DD0
srClass* stModelInstance::vInstance()
{
    return new stModelInstance(0);
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
    render_state_164.highlight_red = 0.0f;
    render_state_164.highlight_green = 0.0f;
    render_state_164.highlight_blue = 0.0f;
    render_state_164.highlight_alpha = 0.0f;
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
    scale_1a4 = 0.0f;
    flag_1a1 = 0;
    scale_1a8 = 0.0f;
    value_1ac = 0.0f;
}

// FUNCTION: WIZ8 0x0047EDF0
stModelInstance& stModelInstance::operator=(const stModelInstance& other)
{
    srModelInstance::operator=(other);
    render_state_164.highlight_red = 0.0f;
    render_state_164.highlight_green = 0.0f;
    render_state_164.highlight_blue = 0.0f;
    render_state_164.highlight_alpha = 0.0f;
    state_178 = other.state_178;
    state_17c = other.state_17c;
    frame_index_180 = other.frame_index_180;

    damage_stage_tables_188 = other.damage_stage_tables_188;
    damage_stage_184 = other.damage_stage_184;
    value_190 = other.value_190;
    retained_174 = 0;
    scale_194 = other.scale_194;
    scale_1a4 = 0.0f;
    flag_1a0 = 0;
    flag_1a1 = other.flag_1a1;
    scale_1a8 = other.scale_1a8;
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

/* Apply the instance transform for the render pass. Unaligned instances take
   the ordinary world-space matrix; aligned instances rebuild the model-view
   matrix from the camera basis so the model stays screen-facing while
   inheriting the view's translation, scale and handedness. */
// FUNCTION: WIZ8 0x0047F560
void stModelInstance::process(const ProcessInfo& info, e_processType)
{
    srGERD* renderer = info.renderer;

    if ((alignment_flags_148.value & 1) == 0) {
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

        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushMatrix();
        renderer->getMatrix(srGERD::MATRIX_MODELVIEW, view);
        world_location = getWorldSpaceLocation();
        world_scale = getWorldSpaceScale();

        srVector3T<float> location;
        location = world_location;
        transformed_location = view.Transform(location);

        srVector3T<float> column_x(view.vectors[0].x, view.vectors[1].x, view.vectors[2].x);
        srVector3T<float> column_y(view.vectors[0].y, view.vectors[1].y, view.vectors[2].y);
        srVector3T<float> column_z(view.vectors[0].z, view.vectors[1].z, view.vectors[2].z);
        basis_x = column_x.Length();
        basis_y = column_y.Length();
        basis_z = column_z.Length();

        float determinant = Det3(view.vectors[0].x, view.vectors[0].y, view.vectors[0].z,
                                 view.vectors[1].x, view.vectors[1].y, view.vectors[1].z,
                                 view.vectors[2].x, view.vectors[2].y, view.vectors[2].z);
        if (determinant > g_zero_005ebb40) {
            basis_x = -basis_x;
            basis_y = -basis_y;
            basis_z = -basis_z;
        }

        renderer->loadIdentity();
        translation.Set(transformed_location.x, transformed_location.y, transformed_location.z);
        renderer->translate(translation);
        if (align_angle_158 != g_float_005ebb34) {
            renderer->rotate(align_angle_158, align_axis_14c);
        }
        renderer->scale(world_scale.x * basis_x, world_scale.y * basis_y,
                        -(world_scale.z * basis_z));
    }

    if (exclusion_mask_15c != 0) {
        unsigned long previous_mask = renderer->getExclusionMask();
        renderer->setExclusionMask(exclusion_mask_15c | previous_mask);
        RenderMeshes0047F930(*renderer);
        renderer->setExclusionMask(previous_mask);
        renderer->popMatrix();
        return;
    }
    RenderMeshes0047F930(*renderer);
    renderer->popMatrix();
}

/* Submit the instance's linked mesh chain. Pass one fills a detached TriMesh
   per model — animated vertex buffers when the model requests them, the
   damage-stage texture/active-polygon tables, the lazily built highlight
   material or the global material overrides — and pushes each mesh through
   RenderTriMeshWithEquations00470380. When the highlight RGBA is configured
   the chain is submitted again inflated along its normals into a shared
   scratch buffer with reversed winding, skipping the linked "blank" skin. */
// FUNCTION: WIZ8 0x0047F930
void stModelInstance::RenderMeshes0047F930(srGERD& renderer)
{
    srMeshModel::TriMesh mesh;
    mesh.control_flags_0c = 0;

    stMeshModel* model = static_cast<stMeshModel*>(getModel());
    stMeshModel* first_model = model;

    srVector3T<float> center;
    float radius;
    model->getBoundingSphere(center, radius);
    if (((model->control_state_394 & 0x20) == 0) &&
        (renderer.testBoundingSphere(center, radius) == srGERD::VISIBILITY_POSITIONAL_0)) {
        return;
    }
    if (((model->control_state_394 & 0x10) == 0) && (model->vertex_location_count_22c >= 8)) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        model->getBoundingBox(minimum, maximum);
        if (renderer.testBoundingBox(minimum, maximum) == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    // c-style-cast-ok: the pick key is an opaque void* token
    SetPickKey004277F0(((state_178 >> 4) & 1) != 0 ? (void*)0 : (void*)this);

    srVector4T<float> ambient;
    renderer.getAmbientLight(ambient);
    srVector3T<float> ambient_color;
    ambient_color.Set(ambient.x, ambient.y, ambient.z);

    srVector4T<float> light;
    if (model->vertex_lighting_ready_3cd == 0) {
        light.w = 1.0f;
        light.x = (ambient_color.x * scale_194.x + g_environment_offset_00659cd0.x) *
                  g_light_scale_0060bfe0;
        light.y = (ambient_color.y * scale_194.y + g_environment_offset_00659cd0.y) *
                  g_light_scale_0060bfe0;
        light.z = (ambient_color.z * scale_194.z + g_environment_offset_00659cd0.z) *
                  g_light_scale_0060bfe0;
    } else {
        light.x = 0.0f;
        light.y = 0.0f;
        light.z = 0.0f;
        light.w = 0.0f;
    }
    renderer.setAmbientLight(light);

    if (render_state_164.highlight_red != g_float_005ebb34 ||
        render_state_164.highlight_green != g_float_005ebb34 ||
        render_state_164.highlight_blue != g_float_005ebb34 ||
        render_state_164.highlight_alpha != g_float_005ebb34) {
        if (retained_174 == 0) {
            retained_174 = new srMaterial;
            srVector4T<float> zero;
            zero.Set(0.0f, 0.0f, 0.0f, 0.0f);
            retained_174->setAmbient(zero);
            retained_174->setDiffuse(zero);
            retained_174->setSpecular(zero);
            retained_174->setOpacity(0.35);
        }
        retained_174->setEmissive(
            // reinterpret-ok: the render-state block carries the highlight RGBA verbatim.
            *reinterpret_cast<const srVector4T<float>*>(&render_state_164));
    }

    unsigned char first_pass = 1;
    srNode* child;
    while (model != 0) {
        model->SetAmbientColor00472990(ambient_color);
        model->getTriMesh(mesh);
        if (((state_178 >> 3) & 1) != 0 && first_pass != 0) {
            RenderShadow004811D0(renderer, mesh);
        }

        srVector4T<float>* poly_normals;
        if ((model->flags_3a0 >> 2) & 1) {
            mesh.positions_38 = model->GetVertexLocations00471AD0(frame_index_180, 1, value_1ac);
            mesh.normals_3c = model->GetVertexNormals00471CA0(frame_index_180, 1);
            // reinterpret-ok: the equation table aliases the polygon normals.
            poly_normals = reinterpret_cast<srVector4T<float>*>(
                model->GetPolygonNormals00471D00(frame_index_180, 1));
        } else {
            poly_normals = 0;
        }

        if (g_flag_0065a146 != 0) {
            mesh.shaders_b0[0].value &= 0xffff73ff;
            mesh.poly_shaders_100[0] = 0;
            mesh.poly_textures_e0[0][0] = 0;
            mesh.poly_uv_110[0] = 0;
            mesh.texcoords_18[0][0] = 0;
        }
        if (g_flag_0065a0ec != 0) {
            mesh.dig_40[0] = 0;
        }
        srPtr<srTextureIFace>*(*poly_textures)[2] = mesh.poly_textures_e0;
        if (poly_textures != 0 && mesh.active_polygons_14c == 0) {
            long active_count;
            unsigned long* active;
            if (damage_stage_184 >= 0) {
                mesh.poly_textures_e0[0][0] =
                    model->GetTextureTable00473720(damage_stage_tables_188.data[damage_stage_184]);
                active = model->GetActivePolygons00473CD0(
                    &active_count, damage_stage_tables_188.data[damage_stage_184], 1);
            } else {
                active = model->GetActivePolygons00473CD0(&active_count, -1, 1);
            }
            if (active != 0) {
                mesh.active_polygons_14c = active;
                mesh.active_polygon_count_150 = active_count;
            }
        }

        if (((render_state_164.highlight_red == g_float_005ebb34) &&
             (render_state_164.highlight_green == g_float_005ebb34) &&
             (render_state_164.highlight_blue == g_float_005ebb34) &&
             (render_state_164.highlight_alpha == g_float_005ebb34)) ||
            (value_190 != 1)) {
            if (flag_1a0 != 0) {
                g_material_diffuse_scale_0065baa0 = scale_1a4;
                mesh.shaders_b0[0].value = (mesh.shaders_b0[0].value & 0xffffd7bf) | 0x44a0;
                mesh.control_flags_0c |= 0x40;
                g_material_diffuse_scale_enabled_0065ba9e = 1;
            }
            if (flag_1a1 != 0) {
                g_material_emissive_override_0065baa8 = scale_1a8;
                g_material_emissive_override_enabled_0065baa4 = 1;
            }
        } else {
            mesh.vertex_materials_c0[0][0] = 0;
            mesh.materials_70[0][0] = retained_174;
        }

        if (((state_178 >> 4) & 1) != 0 && !renderer.isPickStackEmpty()) {
            srGERD::Pick pick;
            renderer.popPick(pick);
            model->RenderTriMeshWithEquations00470380(renderer, mesh, poly_normals);
            renderer.pushPick(pick);
        } else {
            model->RenderTriMeshWithEquations00470380(renderer, mesh, poly_normals);
        }

        first_pass = 0;
        srNode* previous = child;
        if (child == 0 || child->testFlag(FLAG_TERMINATE) == 0) {
            model = model->next;
            if (previous != 0) {
                child = previous->firstChild();
            }
        } else {
            model = 0;
        }
        if (flag_1a0 != 0) {
            g_material_diffuse_scale_enabled_0065ba9e = 0;
        }
        if (flag_1a1 != 0) {
            g_material_emissive_override_enabled_0065baa4 = 0;
        }
    }

    if (render_state_164.highlight_red != g_float_005ebb34 ||
        render_state_164.highlight_green != g_float_005ebb34 ||
        render_state_164.highlight_blue != g_float_005ebb34 ||
        render_state_164.highlight_alpha != g_float_005ebb34) {
        model = static_cast<stMeshModel*>(getModel());
        if (value_190 == 0) {
            renderer.setWinding(srGERD::WINDING_POSITIONAL_1);
        }
        while (model != 0) {
            model->getTriMesh(mesh);
            if (((model->flags_3a0 & 1) == 0) || (model == first_model)) {
                if (mesh.poly_textures_e0[0][0] != 0 ||
                    ((mesh.textures_90[0][0] != 0) &&
                     (_strnicmp("blank", mesh.textures_90[0][0]->getName(), 5) != 0))) {
                    if (g_vertex_scratch_0065a148 == 0) {
                        g_vertex_scratch_0065a148 = new srHeapArray<srVector3T<float> >;
                    }
                    /* Retail expresses the scratch grow as vertex_count*3
                       floats but stores it as the vec3 element capacity. */
                    unsigned long needed = mesh.vertex_count_00 * 3 * sizeof(float);
                    if (g_vertex_scratch_0065a148->capacity < needed &&
                        g_vertex_scratch_0065a148->capacity != needed) {
                        if (needed == 0) {
                            g_vertex_scratch_0065a148->release();
                        } else {
                            srVector3T<float>* replacement =
                                srHeapArray<srVector3T<float> >::allocate(needed);
                            g_vertex_scratch_0065a148->release();
                            g_vertex_scratch_0065a148->data = replacement;
                            g_vertex_scratch_0065a148->capacity = needed;
                        }
                    }

                    const srVector4T<float>* poly_normals = 0;
                    if ((model->flags_3a0 >> 2) & 1) {
                        mesh.dig_40[0] =
                            model->GetVertexLocations00471AD0(frame_index_180, 1, value_1ac);
                        mesh.dig_40[1] = model->GetVertexNormals00471CA0(frame_index_180, 1);
                        // reinterpret-ok: the equation table aliases the polygon normals.
                        poly_normals = reinterpret_cast<const srVector4T<float>*>(
                            model->GetPolygonNormals00471D00(frame_index_180, 1));
                    }
                    srPtr<srTextureIFace>*(*poly_textures)[2] = mesh.poly_textures_e0;
                    if (poly_textures != 0 && mesh.active_polygons_14c == 0) {
                        long active_count;
                        unsigned long* active;
                        if (damage_stage_184 >= 0) {
                            active = model->GetActivePolygons00473CD0(
                                &active_count, damage_stage_tables_188.data[damage_stage_184], 1);
                        } else {
                            active = model->GetActivePolygons00473CD0(&active_count, -1, 1);
                        }
                        if (active != 0) {
                            mesh.active_polygons_14c = active;
                            mesh.active_polygon_count_150 = active_count;
                        }
                    }

                    mesh.materials_70[0][0] = retained_174;
                    mesh.shaders_b0[1].value = (mesh.shaders_b0[0].value & 0xffff5cb7) | 0x40a0;
                    mesh.poly_shaders_100[1] = 0;
                    mesh.poly_textures_e0[0][1] = 0;
                    mesh.vertex_materials_c0[0][1] = 0;
                    mesh.poly_uv_110[1] = 0;

                    double factor = g_double_005ebc30 / getScale().y * g_double_005ec8d8;
                    float expand = static_cast<float>(
                        (radius * getScale().y * g_double_005ec8d0 + g_double_005ebc30) * factor);
                    srVector3T<float> offsets;
                    offsets.x = expand;
                    offsets.y = expand;
                    offsets.z = expand;

                    if (mesh.vertex_count_00 != 0) {
                        if (offsets.x == g_float_005ebb34 && offsets.y == g_float_005ebb34 &&
                            offsets.z == g_float_005ebb34) {
                            if (mesh.vertex_count_00 * 3 != 0) {
                                srVectorProcessor::copy(
                                    // reinterpret-ok: dword view of the vec3 scratch buffer.
                                    reinterpret_cast<SRDWORD*>(g_vertex_scratch_0065a148->data), 0,
                                    mesh.vertex_count_00 * 3);
                            }
                        } else {
                            srVectorProcessor::mul(g_vertex_scratch_0065a148->data, offsets,
                                                   mesh.normals_3c, mesh.vertex_count_00);
                        }
                    }
                    if (mesh.vertex_count_00 * 3 != 0) {
                        srVectorProcessor::add(
                            // reinterpret-ok: float lanes of the vec3 scratch buffer.
                            reinterpret_cast<float*>(g_vertex_scratch_0065a148->data),
                            reinterpret_cast<const float*>(g_vertex_scratch_0065a148->data),
                            // reinterpret-ok: float lanes of the mesh positions.
                            reinterpret_cast<const float*>(mesh.positions_38),
                            mesh.vertex_count_00 * 3);
                    }
                    mesh.positions_38 = g_vertex_scratch_0065a148->data;
                    mesh.control_flags_0c |= 0x40;

                    if (((state_178 >> 4) & 1) != 0 && !renderer.isPickStackEmpty()) {
                        srGERD::Pick pick;
                        renderer.popPick(pick);
                        model->RenderTriMeshWithEquations00470380(renderer, mesh, poly_normals);
                        renderer.pushPick(pick);
                    } else {
                        model->RenderTriMeshWithEquations00470380(renderer, mesh, poly_normals);
                    }
                }
            }

            srNode* previous = child;
            if (child != 0 && child->testFlag(FLAG_TERMINATE) != 0) {
                break;
            }
            model = model->next;
            if (previous != 0) {
                child = previous->firstChild();
            }
        }
        if (value_190 == 0) {
            renderer.setWinding(srGERD::WINDING_POSITIONAL_0);
        }
    }
    renderer.setAmbientLight(ambient);
}

// GLOBAL: WIZ8 0x005EC8E0
const float g_float_005ec8e0 = 1.0f / 1500.0f;

/* Shared shadow-quad mesh built on first use: two upright triangles on a
   500-unit ground span, lit by a dedicated material so the extruded shadow
   pass submits through the ordinary TriMesh pipeline. */
// GLOBAL: WIZ8 0x0065A14C
srMeshModel::TriMesh* g_shadow_mesh_0065a14c;

// FUNCTION: WIZ8 0x004813F0
void BuildShadowMesh004813F0()
{
    stMaterial* material = SR_NEW(stMaterial)();
    srShader shader;
    shader.value = 0x44b3;
    if (g_shadow_mesh_0065a14c == 0) {
        g_shadow_mesh_0065a14c = new srMeshModel::TriMesh;
        if (g_shadow_mesh_0065a14c != 0) {
            if (material != 0) {
                srVector4T<float> color;
                color.Set(1.0f, 1.0f, 1.0f, 1.0f);
                material->setAmbient(color);
                color.Set(0.0f, 0.0f, 0.0f, 0.0f);
                material->setSpecular(color);
                material->parms_18.shininess = 1.0f;
                material->dirty_74 = 1;
                color.Set(0.0f, 0.0f, 0.0f, 0.0f);
                material->setEmissive(color);
                color.Set(1.0f, 1.0f, 1.0f, 0.0f);
                material->setDiffuse(color);
                material->m_field_78 = 0;
            }
            srVector3i* triangles = static_cast<srVector3i*>(srHeap.allocate(6 * sizeof(long)));
            g_shadow_mesh_0065a14c->poly_vertices_10 = triangles;
            triangles[0].x = 0;
            triangles[0].y = 1;
            triangles[0].z = 2;
            triangles[1].x = 3;
            triangles[1].y = 4;
            triangles[1].z = 5;
            srVector3T<float>* positions = static_cast<srVector3T<float>*>(srHeap.allocate(0x48));
            g_shadow_mesh_0065a14c->positions_38 = positions;
            positions[0].x = -250.0f;
            positions[0].y = 250.0f;
            positions[0].z = 0.0f;
            positions[1].x = 0.0f;
            positions[1].y = -250.0f;
            positions[1].z = 0.0f;
            positions[2].x = 250.0f;
            positions[2].y = 250.0f;
            positions[2].z = 0.0f;
            positions[3].x = 0.0f;
            positions[3].y = 250.0f;
            positions[3].z = -250.0f;
            positions[4].x = 0.0f;
            positions[4].y = -250.0f;
            positions[4].z = 0.0f;
            positions[5].x = 0.0f;
            positions[5].y = 250.0f;
            positions[5].z = 250.0f;
            g_shadow_mesh_0065a14c->normals_3c = 0;
            g_shadow_mesh_0065a14c->control_flags_0c = 0;
            g_shadow_mesh_0065a14c->control_flags_0c |= 0x10;
            g_shadow_mesh_0065a14c->control_flags_0c |= 0x20;
            g_shadow_mesh_0065a14c->control_flags_0c |= 8;
            g_shadow_mesh_0065a14c->polygon_count_04 = 2;
            g_shadow_mesh_0065a14c->vertex_count_00 = 6;
            g_shadow_mesh_0065a14c->bounds_maximum_12c.Set(500.0f, 500.0f, 500.0f);
            g_shadow_mesh_0065a14c->bounds_minimum_120.Set(0.0f, 0.0f, 0.0f);
            g_shadow_mesh_0065a14c->bounds_center_138.Set(250.0f, 250.0f, 250.0f);
            g_shadow_mesh_0065a14c->bounds_radius_144 = 250.0f;
            g_shadow_mesh_0065a14c->pass_count_08 = 1;
            g_shadow_mesh_0065a14c->shaders_b0[0] = shader;
            g_shadow_mesh_0065a14c->poly_equations_14 = 0;
            g_shadow_mesh_0065a14c->texcoords_18[0][0] = 0;
            g_shadow_mesh_0065a14c->materials_70[0][0] = material;
            g_shadow_mesh_0065a14c->textures_90[0][0] = 0;
            g_shadow_mesh_0065a14c->poly_shaders_100[0] = 0;
            g_shadow_mesh_0065a14c->poly_uv_110[0] = 0;
            g_shadow_mesh_0065a14c->active_polygons_14c = 0;
            g_shadow_mesh_0065a14c->dcg_50[0] = 0;
            g_shadow_mesh_0065a14c->dig_40[0] = 0;
            g_shadow_mesh_0065a14c->scg_60[0] = 0;
            g_shadow_mesh_0065a14c->sort_bias_148 = 0.0f;
        }
    }
}

/* Submit the shared shadow-quad mesh extruded over the source mesh's height:
   front-cull the pass, translate to the mesh's mid-height, scale the unit
   quad by a clamped pitch factor, then feed the pipeline slots directly. */
// FUNCTION: WIZ8 0x004811D0
void stModelInstance::RenderShadow004811D0(srGERD& renderer, srMeshModel::TriMesh& mesh)
{
    if (g_shadow_mesh_0065a14c == 0) {
        BuildShadowMesh004813F0();
        if (g_shadow_mesh_0065a14c == 0) {
            return;
        }
    }
    float height = mesh.bounds_maximum_12c.y - mesh.bounds_minimum_120.y;
    renderer.pushEnable();
    renderer.setCullMode(srGERD::CULL_FRONT);
    if (!renderer.isEnabled(srGERD::ENABLE_POSITIONAL_1)) {
        renderer.toggle(srGERD::ENABLE_POSITIONAL_1);
    }
    renderer.matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer.pushMatrix();
    renderer.translate(0.0, height * g_float_005ebc7c, 0.0);
    height = height * g_float_005ec8e0;
    if (height < g_float_005ebc7c) {
        height = g_float_005ebc7c;
    }
    double scale = height;
    renderer.scale(scale, scale, scale);

    srTriMeshPipeline* pipeline = srTriMeshPipeline::Get004750A0(&renderer);
    pipeline->triangle_count_1c = g_shadow_mesh_0065a14c->polygon_count_04;
    pipeline->triangles_34 = g_shadow_mesh_0065a14c->poly_vertices_10;
    pipeline->vertex_count_20 = g_shadow_mesh_0065a14c->vertex_count_00;
    pipeline->positions_38 = g_shadow_mesh_0065a14c->positions_38;
    pipeline->vertex_extras_3c = g_shadow_mesh_0065a14c->normals_3c;
    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_pass_18->shader_14 = 0;
    pipeline->current_pass_18->texture_array_0c = 0;
    pipeline->current_pass_18->value_10 = 0;
    pipeline->material_80 = g_shadow_mesh_0065a14c->materials_70[0][0];
    pipeline->current_record_14->material_08 = pipeline->material_80;
    pipeline->SetFlags004752C0(g_shadow_mesh_0065a14c->shaders_b0[0]);
    pipeline->current_record_14 = &pipeline->records_94[++pipeline->slot_count_84];
    pipeline->current_pass_18 = &pipeline->passes_9c[pipeline->slot_count_84];
    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_record_14->disable_mask_04 = 0;
    pipeline->current_record_14->material_08 = pipeline->material_80;
    pipeline->current_pass_18->texture_00 = pipeline->texture_78;
    pipeline->current_pass_18->pass_value_04 = pipeline->pass_value_7c;
    pipeline->current_pass_18->flags_08.value = pipeline->shader_74.value;
    pipeline->current_pass_18->texture_array_0c = 0;
    pipeline->current_pass_18->value_10 = 0;
    pipeline->current_pass_18->shader_14 = 0;
    pipeline->current_pass_18->st_18 = 0;
    pipeline->current_pass_18->value_1c = 0;
    pipeline->FlushIfCurrent();
    renderer.popMatrix();
    renderer.popEnable();
}
