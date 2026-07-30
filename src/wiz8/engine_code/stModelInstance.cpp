#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/mesh_model.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

#include <new>
#include <string.h>

// FUNCTION: WIZ8 0x00481C80
void srNode::TraverseInfo::EntryArray::setCapacity(unsigned int new_capacity)
{
    unsigned int copy_capacity;
    unsigned int index;
    Entry* replacement;

    if (capacity != new_capacity) {
        replacement = 0;
        if (new_capacity > 0) {
            replacement = static_cast<Entry*>(
                ::operator new(new_capacity * sizeof(Entry)));
            if (data != 0 && capacity != 0) {
                copy_capacity = capacity;
                if (new_capacity <= copy_capacity) {
                    copy_capacity = new_capacity;
                }
                for (index = 0; index < copy_capacity; ++index) {
                    replacement[index] = data[index];
                }
            }
        }
        ::operator delete(data);
        data = replacement;
        capacity = new_capacity;
    }
}
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
const char* stModelInstance2D::getClassName() const
{
    return "stModelInstance2D";
}

// FUNCTION: WIZ8 0x00481a40
unsigned long stModelInstance2D::getClassID() const
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
    srCore.getRegistry()->unregisterInstance(getClassNode(), this);
}

/* The 2D form takes the identical chain: both model-instance classes hang off
   srModelInstance, which is what pairs them beyond their adjacent ids. */
// FUNCTION: WIZ8 0x00481a60
srRegistry::ClassNode* stModelInstance2D::getClassNode() const
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
