#include "wiz8/ground_shadow.h"

#include "surrender/srCore.h"

#include <new>

/* The SR.DLL registry string is the original runtime class identity. The
   constructor's material subobject remains unrecovered, so this unit starts
   with the independent vtable identity, clone, and factory bodies. */

extern unsigned char g_ground_shadow_enabled_00685110;

// FUNCTION: WIZ8 0x004d6430
stGroundShadow::stGroundShadow(const stGroundShadow& other)
    : srNode(0)
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
    registry->registerInstance(node, this);

    setParent(other.parentNode(), 1);
    setName(other.getName());
    field_138 = other.field_138;
    value_13c = other.value_13c;
    value_140 = other.value_140;
}

// FUNCTION: WIZ8 0x004d6540
void stGroundShadow::traverse(TraverseInfo& info)
{
    unsigned int old_capacity;
    unsigned int new_capacity;
    unsigned int copy_count;
    unsigned int index;
    TraverseInfo::Entry* replacement;

    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_POSITIONAL_0)) {
        old_capacity = info.entries.capacity;
        if (old_capacity <= info.entry_count) {
            new_capacity = old_capacity + 8 + info.entry_count;
            if (old_capacity != new_capacity) {
                replacement = 0;
                if (new_capacity != 0) {
                    replacement = static_cast<TraverseInfo::Entry*>(
                        ::operator new(new_capacity * sizeof(TraverseInfo::Entry)));
                    if (info.entries.data != 0 && old_capacity != 0) {
                        copy_count = new_capacity < old_capacity
                            ? new_capacity : old_capacity;
                        for (index = 0; index < copy_count; ++index) {
                            replacement[index] = info.entries.data[index];
                        }
                    }
                }
                ::operator delete(info.entries.data);
                info.entries.data = replacement;
                info.entries.capacity = new_capacity;
            }
        }
        info.entries.data[info.entry_count].node = this;
        info.entries.data[info.entry_count].value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: WIZ8 0x004d6640
void stGroundShadow::process(const ProcessInfo& info, e_processType)
{
    if (g_ground_shadow_enabled_00685110 != 0) {
        if (info.renderer->hasPickState()) {
            srGERD::Pick pick;
            info.renderer->popPick(pick);
            renderGroundShadow(info.renderer);
            info.renderer->pushPick(pick);
            return;
        }
        renderGroundShadow(info.renderer);
    }
}

// FUNCTION: WIZ8 0x004d69a0
unsigned long stGroundShadow::getClassID() const
{
    return 0x10010;
}

// FUNCTION: WIZ8 0x004d69b0
const char* stGroundShadow::getClassName() const
{
    return g_stGroundShadowClassName;
}

// FUNCTION: WIZ8 0x004d69c0
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

// FUNCTION: WIZ8 0x004d6a30
srNode* stGroundShadow::vslot7()
{
    stGroundShadow* instance = static_cast<stGroundShadow*>(vInstance());

    *static_cast<srNode*>(instance) = *this;
    instance->field_138 = field_138;
    instance->value_13c = value_13c;
    instance->value_140 = value_140;
    return instance;
}

// FUNCTION: WIZ8 0x004d6bf0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
