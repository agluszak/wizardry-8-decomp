#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/ClipPlane.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Material.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stScript.h"

// SYNTHETIC: WIZ8 0x004A2200
// stLightDefinition005ECDBC::`scalar deleting destructor'

/* Remaining class methods whose original translation-unit ownership is not proved. */

// FUNCTION: WIZ8 0x004A2220
stLightDefinition::~stLightDefinition()
{
}

// FUNCTION: WIZ8 0x004B9C00
stLevel::stLevel(srNode* parent)
    : srNode(0), m_active(0), m_positional_13c(0)
{
    srRegistry* registry = srCore.getRegistry();
    registry->registerInstance(getClassNode(), this);
    if (parent != 0) {
        setParent(parent, 1);
    }
}

// FUNCTION: WIZ8 0x004B9D10
stLevel::~stLevel()
{
    srRegistry* registry = srCore.getRegistry();
    registry->unregisterInstance(getClassNode(), this);
}

// FUNCTION: WIZ8 0x004519D0
unsigned long W8Node005EC208::getClassID() const
{
    return 0x1000;
}

// FUNCTION: WIZ8 0x00445EF0
srRegistry::ClassNode* W8Node005EC208::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000);
    if (!node) {
        node = registry->registerClass(
            srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
    }
    return node;
}

// FUNCTION: WIZ8 0x004519F0
srNode* W8Node005EC208::clone()
{
    srNode* copy = static_cast<srNode*>(vInstance());
    *copy = *this;
    return copy;
}
// FUNCTION: WIZ8 0x0049DB10
unsigned long W8Illuminator005ECCD8::getClassID() const
{
    return 0x1200;
}
// FUNCTION: WIZ8 0x0049DB30
srRegistry::ClassNode* W8Illuminator005ECCD8::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1200);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass(
            srIlluminator::sGetClassName(), parent, 0x1200, 0);
    }
    return node;
}
// FUNCTION: WIZ8 0x0049DC20
unsigned long MonsterLight::getClassID() const
{
    return 0x1220;
}
// FUNCTION: WIZ8 0x0049DC30
const char* MonsterLight::getClassName() const
{
    return "srLight";
}
// FUNCTION: WIZ8 0x0049DC60
unsigned long stLight::getClassID() const
{
    return 0x10006;
}
// FUNCTION: WIZ8 0x0049DC70
const char* stLight::getClassName() const
{
    return "stLight";
}
// FUNCTION: WIZ8 0x0049DC80
srRegistry::ClassNode* stLight::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10006);

    if (!node) {
        srRegistry* light_registry = srCore.getRegistry();
        srRegistry::ClassNode* light = light_registry->getClassNode(0x1220);

        if (!light) {
            srRegistry* illuminator_registry = srCore.getRegistry();
            srRegistry::ClassNode* illuminator =
                illuminator_registry->getClassNode(0x1200);

            if (!illuminator) {
                srRegistry* node_registry = srCore.getRegistry();
                srRegistry::ClassNode* base = node_registry->getClassNode(0x1000);

                if (!base) {
                    base = node_registry->registerClass(
                        srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
                }
                illuminator = illuminator_registry->registerClass(
                    srIlluminator::sGetClassName(), base, 0x1200, 0);
            }
            light = light_registry->registerClass("srLight", illuminator, 0x1220, 0);
        }
        node = registry->registerClass("stLight", light, 0x10006, 0);
    }
    return node;
}
// FUNCTION: WIZ8 0x0049E300
srRegistry::ClassNode* MonsterLight::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* light = registry->getClassNode(0x1220);

    if (!light) {
        srRegistry* illuminator_registry = srCore.getRegistry();
        srRegistry::ClassNode* illuminator =
            illuminator_registry->getClassNode(0x1200);

        if (!illuminator) {
            srRegistry* node_registry = srCore.getRegistry();
            srRegistry::ClassNode* node = node_registry->getClassNode(0x1000);

            if (!node) {
                node = node_registry->registerClass(
                    srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
            }
            illuminator = illuminator_registry->registerClass(
                srIlluminator::sGetClassName(), node, 0x1200, 0);
        }
        light = registry->registerClass("srLight", illuminator, 0x1220, 0);
    }
    return light;
}
// FUNCTION: WIZ8 0x004BA1B0
unsigned long stLevel::getClassID() const
{
    return 0x10007;
}
// FUNCTION: WIZ8 0x004BA1C0
const char* stLevel::getClassName() const
{
    return "stLevel";
}
// FUNCTION: WIZ8 0x004BA1D0
srRegistry::ClassNode* stLevel::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10007);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stLevel", parent, 0x10007, 0);
    }
    return node;
}
// FUNCTION: WIZ8 0x004BDF00
unsigned long W8ClipPlane005ED180::getClassID() const
{
    return 0x1500;
}
// FUNCTION: WIZ8 0x004BDF10
const char* W8ClipPlane005ED180::getClassName() const
{
    return "srClipPlane";
}
// FUNCTION: WIZ8 0x004BDF20
srRegistry::ClassNode* W8ClipPlane005ED180::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1500);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srClipPlane", parent, 0x1500, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x004BDF90
srNode* W8ClipPlane005ED180::clone()
{
    srClipPlane* copy = static_cast<srClipPlane*>(vInstance());
    *copy = *this;
    return copy;
}
