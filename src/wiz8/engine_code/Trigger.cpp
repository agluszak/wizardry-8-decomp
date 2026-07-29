#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "surrender/srCore.h"

/*
 * Engine Code\Trigger.cpp.
 *
 * Only the class registry slots are recovered so far.
 */

// FUNCTION: WIZ8 0x00445ae0
const char* Trigger::getClassName() const
{
    return "Trigger";
}

// FUNCTION: WIZ8 0x00445ad0
unsigned long Trigger::getClassID() const
{
    return 0x10008;
}

/* The shortest of the registry builders: this class hangs straight off srClass
   with no intermediate base, so there is no parent lookup at all - just the
   cache probe and, when it misses, one registerClass with the concrete flag
   set. That single level is the whole of the difference against the 104-byte
   form the srNode-derived classes use. */
// FUNCTION: WIZ8 0x00445f30
srRegistry::ClassNode* Trigger::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10008);

    if (!node) {
        node = registry->registerClass(
            "Trigger", srClass::sGetClassNode(), 0x10008, 1);
    }
    return node;
}
