#include "wiz8/ground_shadow.h"

#include "surrender/srCore.h"

/* The SR.DLL registry string is the original runtime class identity. The
   constructor's material subobject remains unrecovered, so this unit starts
   with the independent vtable identity, clone, and factory bodies. */

// FUNCTION: WIZ8 0x004D69A0
unsigned long stGroundShadow::getClassID() const
{
    return 0x10010;
}

// FUNCTION: WIZ8 0x004D69B0
const char* stGroundShadow::getClassName() const
{
    return g_stGroundShadowClassName;
}

// FUNCTION: WIZ8 0x004D69C0
srRegistry::ClassNode* stGroundShadow::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10010);

    if (node == 0) {
        srRegistry* parent_registry = srCore.getRegistry();
        node = parent_registry->getClassNode(0x1000);
        if (node == 0) {
            node = parent_registry->registerClass(
                srNode::sGetClassName(),
                srClass::sGetClassNode(),
                0x1000,
                1);
        }
        node = registry->registerClass(
            g_stGroundShadowClassName,
            node,
            0x10010,
            0);
    }
    return node;
}

// FUNCTION: WIZ8 0x004D6A30
srNode* stGroundShadow::vslot7()
{
    stGroundShadow* instance = static_cast<stGroundShadow*>(vInstance());

    *static_cast<srNode*>(instance) = *this;
    instance->field_138 = field_138;
    instance->value_13c = value_13c;
    instance->value_140 = value_140;
    return instance;
}

// FUNCTION: WIZ8 0x004D6BF0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
