#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/mesh_model.h"
#include "surrender/srCore.h"
#include "surrender/srMaterial.h"
#include "surrender/srNode.h"
#include "surrender/srHeap.h"

#include <new>
#include <string.h>

// SYNTHETIC: WIZ8 0x0047F260
// stModelInstance2D::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x00481C50
// W8ModelInstance2DRegistry005EC89C::`scalar deleting destructor'

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
                    damage_stage_tables_188[damage_stage_184]);

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

    int stage = damage_stage_count_18c;
    int new_count = stage + 1;
    int* replacement = static_cast<int*>(
        srHeap.allocate(new_count * sizeof(int)));
    for (int index = 0; index < damage_stage_count_18c; ++index) {
        replacement[index] = damage_stage_tables_188[index];
    }
    if (damage_stage_tables_188 != 0) {
        srHeap.free(damage_stage_tables_188);
    }
    damage_stage_tables_188 = replacement;
    damage_stage_count_18c = new_count;

    int base_table = stage > 0 ? damage_stage_tables_188[0] : -1;
    damage_stage_tables_188[stage] =
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

    int stage = damage_stage_count_18c;
    int new_count = stage + 1;
    int* replacement = static_cast<int*>(
        srHeap.allocate(new_count * sizeof(int)));
    for (int index = 0; index < damage_stage_count_18c; ++index) {
        replacement[index] = damage_stage_tables_188[index];
    }
    if (damage_stage_tables_188 != 0) {
        srHeap.free(damage_stage_tables_188);
    }
    damage_stage_tables_188 = replacement;
    damage_stage_count_18c = new_count;
    damage_stage_tables_188[stage] = table;
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
            mesh->GetTextureTable00473720(damage_stage_tables_188[stage]);
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

// FUNCTION: WIZ8 0x00481870
const char* stModelInstance::getClassName() const
{
    return "stModelInstance";
}

// FUNCTION: WIZ8 0x00481860
unsigned long stModelInstance::getClassID() const
{
    return 0x10004;
}

// FUNCTION: WIZ8 0x00481a50
const char* W8ModelInstance2DRegistry005EC89C::getClassName() const
{
    return "stModelInstance2D";
}

// FUNCTION: WIZ8 0x00481a40
unsigned long W8ModelInstance2DRegistry005EC89C::getClassID() const
{
    return 0x10005;
}

/* Three-level registry builder: the class registers under srModelInstance,
   which registers under srNode, which registers under srClass. Only srNode
   supplies its name through a static getter; the two below it are literals,
   which is the same split every other variant shows. */
// FUNCTION: WIZ8 0x00481880
srRegistry::ClassNode* stModelInstance::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10004);

    if (!node) {
        srRegistry* instance_registry = srCore.getRegistry();
        srRegistry::ClassNode* instance = instance_registry->getClassNode(0x1100);

        if (!instance) {
            srRegistry* node_registry = srCore.getRegistry();
            srRegistry::ClassNode* base = node_registry->getClassNode(0x1000);

            if (!base) {
                base = node_registry->registerClass(
                    srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
            }
            instance = instance_registry->registerClass(
                "srModelInstance", base, 0x1100, 0);
        }
        node = registry->registerClass("stModelInstance", instance, 0x10004, 0);
    }
    return node;
}

/* The derived instance has no first-party resource to release here. Its
   responsibility is the registry edge; srModelInstance performs the base
   teardown automatically after this body. The registry helper is defined
   first because its complete body is expanded into the original destructor. */
// FUNCTION: WIZ8 0x00481940
stModelInstance::~stModelInstance()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10004);

    if (node == 0) {
        srRegistry* instance_registry = srCore.getRegistry();

        node = instance_registry->getClassNode(0x1100);
        if (node == 0) {
            srRegistry* node_registry = srCore.getRegistry();

            node = node_registry->getClassNode(0x1000);
            if (node == 0) {
                node = node_registry->registerClass(
                    srNode::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x1000,
                    1);
            }
            node = instance_registry->registerClass(
                "srModelInstance", node, 0x1100, 0);
        }
        node = registry->registerClass(
            "stModelInstance", node, 0x10004, 0);
    }
    registry->unregisterInstance(node, this);
}

/* The 2D form takes the identical chain: both model-instance classes hang off
   srModelInstance, which is what pairs them beyond their adjacent ids. */
// FUNCTION: WIZ8 0x00481a60
srRegistry::ClassNode* W8ModelInstance2DRegistry005EC89C::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10005);

    if (!node) {
        srRegistry* instance_registry = srCore.getRegistry();
        srRegistry::ClassNode* instance = instance_registry->getClassNode(0x1100);

        if (!instance) {
            srRegistry* node_registry = srCore.getRegistry();
            srRegistry::ClassNode* base = node_registry->getClassNode(0x1000);

            if (!base) {
                base = node_registry->registerClass(
                    srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
            }
            instance = instance_registry->registerClass(
                "srModelInstance", base, 0x1100, 0);
        }
        node = registry->registerClass("stModelInstance2D", instance, 0x10005, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x00481B20
W8ModelInstance2DRegistry005EC89C::~W8ModelInstance2DRegistry005EC89C()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10005);

    if (node == 0) {
        srRegistry* instance_registry = srCore.getRegistry();

        node = instance_registry->getClassNode(0x1100);
        if (node == 0) {
            srRegistry* node_registry = srCore.getRegistry();

            node = node_registry->getClassNode(0x1000);
            if (node == 0) {
                node = node_registry->registerClass(
                    srNode::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x1000,
                    1);
            }
            node = instance_registry->registerClass(
                "srModelInstance", node, 0x1100, 0);
        }
        node = registry->registerClass(
            "stModelInstance2D", node, 0x10005, 0);
    }
    registry->unregisterInstance(node, this);
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

// FUNCTION: WIZ8 0x00481E30
srClass* stModelInstance2D::vInstance()
{
    return new stModelInstance2D(0);
}

// FUNCTION: WIZ8 0x00481B00
srNode* stModelInstance2D::clone()
{
    stModelInstance2D* copy = static_cast<stModelInstance2D*>(vInstance());
    *copy = *this;
    return copy;
}

/* Shared parent class-node builder used by the inlined stModelInstance
   construction layer. */
// FUNCTION: WIZ8 0x00481D00
srRegistry::ClassNode* GetSrModelInstanceClassNode00481D00()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1100);

    if (node == 0) {
        srRegistry* node_registry = srCore.getRegistry();

        node = node_registry->getClassNode(0x1000);
        if (node == 0) {
            node = node_registry->registerClass(
                srNode::sGetClassName(),
                srClass::sGetClassNode(),
                0x1000,
                1);
        }
        node = registry->registerClass(
            "srModelInstance", node, 0x1100, 0);
    }
    return node;
}

// SYNTHETIC: WIZ8 0x0047EDC0
// stModelInstance005EC7D0::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0047EC80
stModelInstance005EC7D0::stModelInstance005EC7D0(srNode* parent)
    : stModelInstance()
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
    retained_174 = 0;
    scale_194.x = 1.0f;
    scale_194.y = 1.0f;
    scale_194.z = 1.0f;
    flag_1a0 = 0;
    value_1a4 = 0;
    flag_1a1 = 0;
    value_1a8 = 0;
    value_1ac = 0.0f;
}

// FUNCTION: WIZ8 0x0047EF70
stModelInstance005EC7D0::~stModelInstance005EC7D0()
{
    if (retained_174 != 0) {
        retained_174->release();
    }
    if (damage_stage_tables_188 != 0) {
        srHeap.free(damage_stage_tables_188);
    }
    damage_stage_tables_188 = 0;
    damage_stage_count_18c = 0;
}
