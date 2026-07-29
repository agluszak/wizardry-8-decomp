#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

/*
 * Engine Code\stModelInstance.cpp.
 *
 * Two model-instance classes, a 3D one and a 2D one, whose registry ids are
 * adjacent. Only their class registry slots are recovered; the interval this
 * unit bounds still holds the rest.
 */

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
