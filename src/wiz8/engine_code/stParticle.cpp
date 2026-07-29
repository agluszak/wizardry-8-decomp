#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

/*
 * Engine Code\stParticle.cpp.
 *
 * Only the class registry slots are recovered so far.
 */

// FUNCTION: WIZ8 0x0049b550
const char* stParticle::getClassName() const
{
    return "stParticle";
}

// FUNCTION: WIZ8 0x0049b540
unsigned long stParticle::getClassID() const
{
    return 0x10009;
}

/* Registry node builder in the proven two-level shape: the class hangs
   directly off srNode, whose name comes from its static getter while this
   class's own name is a literal. */
// FUNCTION: WIZ8 0x0049b560
srRegistry::ClassNode* stParticle::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10009);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stParticle", parent, 0x10009, 0);
    }
    return node;
}
