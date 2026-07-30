#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/ClipPlane.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/stSound3D.h"

/* Closed family of source-declared registry class methods. */

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

// FUNCTION: WIZ8 0x00429A40
unsigned long W8Registered005EBD10::getClassID() const
{
    return 0x3110;
}
// FUNCTION: WIZ8 0x00429B30
unsigned long W8MeshModel005EBE98::getClassID() const
{
    return 0x2010;
}
// FUNCTION: WIZ8 0x00429B40
const char* W8MeshModel005EBE98::getClassName() const
{
    return "srMeshModel";
}
// FUNCTION: WIZ8 0x00429B50
srRegistry::ClassNode* W8MeshModel005EBE98::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2010);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x2000);

        if (!parent) {
            parent = parent_registry->registerClass(
                "srModel", srClass::sGetClassNode(), 0x2000, 1);
        }
        node = registry->registerClass("srMeshModel", parent, 0x2010, 0);
    }
    return node;
}
// FUNCTION: WIZ8 0x00429BE0
unsigned long W8TextureMap005EBEEC::getClassID() const
{
    return 0x2111;
}
// FUNCTION: WIZ8 0x00429BF0
const char* W8TextureMap005EBEEC::getClassName() const
{
    return "srTextureMap";
}
// FUNCTION: WIZ8 0x00429CC0
unsigned long W8Registered005EBDE0::getClassID() const
{
    return 0x2210;
}
// FUNCTION: WIZ8 0x00429E80
unsigned long W8Registered005EBF94::getClassID() const
{
    return 0x2200;
}
// FUNCTION: WIZ8 0x0042A010
unsigned long W8Camera005EBE14::getClassID() const
{
    return 0x1400;
}
// FUNCTION: WIZ8 0x0042A020
const char* W8Camera005EBE14::getClassName() const
{
    return "srCamera";
}
// FUNCTION: WIZ8 0x0042A030
srRegistry::ClassNode* W8Camera005EBE14::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1400);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srCamera", parent, 0x1400, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x0042A0A0
srNode* W8Camera005EBE14::vslot7()
{
    srCamera* copy = static_cast<srCamera*>(vInstance());
    *copy = *this;
    return copy;
}
// FUNCTION: WIZ8 0x0042A0C0
unsigned long W8Scene005EBE48::getClassID() const
{
    return 0x1010;
}
// FUNCTION: WIZ8 0x0042A0D0
const char* W8Scene005EBE48::getClassName() const
{
    return "srScene";
}
// FUNCTION: WIZ8 0x0042A0E0
srRegistry::ClassNode* W8Scene005EBE48::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1010);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srScene", parent, 0x1010, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x0042A150
srNode* W8Scene005EBE48::vslot7()
{
    srScene* copy = static_cast<srScene*>(vInstance());
    *copy = *this;
    return copy;
}
// FUNCTION: WIZ8 0x0042A360
void W8Object005EBFD0::InstallVtable()
{
    *(void**)this = &g_vtable_005ebfd0;
}
// FUNCTION: WIZ8 0x00445EE0
void W8Object005EC138::InstallVtable()
{
    *(void**)this = &g_vtable_005ec138;
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
srNode* W8Node005EC208::vslot7()
{
    srNode* copy = static_cast<srNode*>(vInstance());
    *copy = *this;
    return copy;
}
// FUNCTION: WIZ8 0x0047CBA0
const char* stBinIStream::getClassName() const
{
    return "stBinIStream";
}
// FUNCTION: WIZ8 0x0047D650
unsigned long W8Registered005EC5D8::getClassID() const
{
    return 0x2900;
}
// FUNCTION: WIZ8 0x0047D6D0
unsigned long stTextureFile::getClassID() const
{
    return 0x10001;
}
// FUNCTION: WIZ8 0x0047D6E0
const char* stTextureFile::getClassName() const
{
    return "stTextureFile";
}
// FUNCTION: WIZ8 0x0047D6F0
srRegistry::ClassNode* stTextureFile::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10001);

    if (!node) {
        srRegistry* texture_registry = srCore.getRegistry();
        srRegistry::ClassNode* texture = texture_registry->getClassNode(0x2110);

        if (!texture) {
            srRegistry* iface_registry = srCore.getRegistry();
            srRegistry::ClassNode* iface = iface_registry->getClassNode(0x2100);

            if (!iface) {
                iface = iface_registry->registerClass(
                    "srTextureIFace", srClass::sGetClassNode(), 0x2100, 1);
            }
            texture = texture_registry->registerClass(
                srTexture::sGetClassName(), iface, 0x2110, 0);
        }
        node = registry->registerClass("stTextureFile", texture, 0x10001, 0);
    }
    return node;
}
// FUNCTION: WIZ8 0x00484700
unsigned long W8Fog005EC94C::getClassID() const
{
    return 0x1210;
}
// FUNCTION: WIZ8 0x00484710
const char* W8Fog005EC94C::getClassName() const
{
    return "srFog";
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
// FUNCTION: WIZ8 0x004A2220
void W8Object005ECDB0::InstallVtable()
{
    *(void**)this = &g_vtable_005ecdb0;
}
// FUNCTION: WIZ8 0x004AF3D0
unsigned long stSound3D::getClassID() const
{
    return 0x1000b;
}
// FUNCTION: WIZ8 0x004AF3E0
const char* stSound3D::getClassName() const
{
    return "stSound3D";
}
// FUNCTION: WIZ8 0x004AF3F0
srRegistry::ClassNode* stSound3D::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000b);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stSound3D", parent, 0x1000b, 0);
    }
    return node;
}

extern unsigned char IsSoundHandleActive00408EF0(int handle);

// FUNCTION: WIZ8 0x004AEC70
unsigned char stSound3D::IsPlaying004AEC70()
{
    if (sound_handle_13c != -1 &&
        IsSoundHandleActive00408EF0(sound_handle_13c) != 0) {
        return 1;
    }
    return 0;
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
srNode* W8ClipPlane005ED180::vslot7()
{
    srClipPlane* copy = static_cast<srClipPlane*>(vInstance());
    *copy = *this;
    return copy;
}
