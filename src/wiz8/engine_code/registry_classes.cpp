#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/ClipPlane.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Material.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/vector.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

// SYNTHETIC: WIZ8 0x004A2200
// stLightDefinition005ECDBC::`scalar deleting destructor'

extern void ReleaseSoundHandle00408F70(int handle);
extern void GetPosition421070(W8Position* position);
extern unsigned char g_master_ambient_volume_6850f6;
extern const double g_zero_005ebb40;
extern float g_light_scale_identity_005ebb38;

W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

/* Closed family of source-declared registry class methods. */

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

// TEMPLATE: WIZ8 0x00429CC0
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::getClassID

// TEMPLATE: WIZ8 0x00429CE0
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::getClassNode

// TEMPLATE: WIZ8 0x00429D50
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::clone

// FUNCTION: WIZ8 0x00429E80
unsigned long W8Registered005EBF94::getClassID() const
{
    return 0x2200;
}
// TEMPLATE: WIZ8 0x0042A010
// srClassSupport<srCamera,srNode,0,5120>::getClassID

// TEMPLATE: WIZ8 0x0042A020
// srClassSupport<srCamera,srNode,0,5120>::getClassName

// TEMPLATE: WIZ8 0x0042A030
// srClassSupport<srCamera,srNode,0,5120>::getClassNode

// TEMPLATE: WIZ8 0x0042A0A0
// srClassSupport<srCamera,srNode,0,5120>::clone
// TEMPLATE: WIZ8 0x0042A0C0
// srClassSupport<srScene,srNode,0,4112>::getClassID

// TEMPLATE: WIZ8 0x0042A0D0
// srClassSupport<srScene,srNode,0,4112>::getClassName

// TEMPLATE: WIZ8 0x0042A0E0
// srClassSupport<srScene,srNode,0,4112>::getClassNode

// TEMPLATE: WIZ8 0x0042A150
// srClassSupport<srScene,srNode,0,4112>::clone
// FUNCTION: WIZ8 0x0042A360
srVertexProcessor::~srVertexProcessor()
{
}

// SYNTHETIC: WIZ8 0x0042B890
// srVertexProcessor::`scalar deleting destructor'
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
// VTABLE: WIZ8 0x005ecfb0
// class stSound3D

// VTABLE: WIZ8 0x005ecfe4
// class srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AEA70
// stSound3D::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004AF3D0
// srClassSupport<stSound3D,srNode,0,65547>::getClassID

// TEMPLATE: WIZ8 0x004AF3E0
// srClassSupport<stSound3D,srNode,0,65547>::getClassName

// TEMPLATE: WIZ8 0x004AF3F0
// srClassSupport<stSound3D,srNode,0,65547>::getClassNode

// TEMPLATE: WIZ8 0x004AF460
// srClassSupport<stSound3D,srNode,0,65547>::clone

// TEMPLATE: WIZ8 0x004AF5A0
// srClassSupport<stSound3D,srNode,0,65547>::~srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AF660
// srClassSupport<stSound3D,srNode,0,65547>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004AE6D0
stSound3D::stSound3D(const char* name, srNode* parent)
    : srClassSupport<stSound3D, srNode, 0, 0x1000b>(
          static_cast<srNode*>(0)),
      unknown_138(0),
      sound_handle_13c(-1),
      value_140(0x7f),
      value_144(25000.0f),
      sound_name_148(0),
      flag_14c(0)
{
    if (parent != 0) {
        setParent(parent, 0);
    }
    if (name != 0) {
        sound_name_148 = static_cast<char*>(malloc(strlen(name) + 1));
        strcpy(sound_name_148, name);
    }
    g_sound3d_instances_65be40.Add(this);
}

// FUNCTION: WIZ8 0x004AEAA0
stSound3D::~stSound3D()
{
    if (sound_name_148 != 0) {
        free(sound_name_148);
    }
    if (sound_handle_13c != -1) {
        ReleaseSoundHandle00408F70(sound_handle_13c);
    }
    int index = g_sound3d_instances_65be40.IndexOf(this);
    if (index != -1) {
        g_sound3d_instances_65be40.RemoveAt(index);
    }
}

// FUNCTION: WIZ8 0x004AE8B0
srClass* stSound3D::vInstance()
{
    return new stSound3D(0, 0);
}

// FUNCTION: WIZ8 0x004AEBF0
unsigned char stSound3D::Play004AEBF0(
    unsigned char flatten, unsigned char flag)
{
    SOUND3DPARMS options;
    W8Position listener;

    if (sound_name_148 == 0) {
        return 0;
    }
    GetPosition421070(&listener);
    BuildSoundOptions004AECC0(&listener, &options);
    if (flatten != 0) {
        options.uiLoop = 0;
    }
    sound_handle_13c = Sound3DPlay(sound_name_148, &options);
    flag_14c = flag;
    return sound_handle_13c != -1;
}

// FUNCTION: WIZ8 0x004AECC0
void stSound3D::BuildSoundOptions004AECC0(
    const W8Position* listener, SOUND3DPARMS* options)
{
    float angle = -GetCameraAngleRadians420DD0();
    unsigned int volume =
        (value_140 * g_master_ambient_volume_6850f6) / 0x7f;
    srMatrix3T<float> rotation;
    srVector3T<float> node_position;
    srVector3T<float> offset;

    rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
    rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
    rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);
    if ((double)angle != g_zero_005ebb40) {
        double cosine = cos(angle);
        double sine = sin(angle);
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        srMatrix3T<float> camera_rotation;

        first.method_00421680(cosine, 0.0, sine);
        second.method_00421680(0.0, 1.0, 0.0);
        third.method_00421680(-sine, 0.0, cosine);
        camera_rotation.method_004219F0(first, second, third);
        rotation.method_00421A40(camera_rotation);
    }

    getLocation(node_position);
    offset.method_00421650(
        node_position.x - listener->x,
        node_position.y - listener->y,
        node_position.z - listener->z);
    float x = Function4218E0(rotation.vectors[0], offset);
    float y = Function4218E0(rotation.vectors[1], offset);
    float z = Function4218E0(rotation.vectors[2], offset);

    memset(options, 0xff, sizeof(*options));
    srVector3T<float> listener_offset;
    listener_offset.method_00421650(
        listener->x - node_position.x,
        listener->y - node_position.y,
        listener->z - node_position.z);
    options->uiVolume = static_cast<unsigned int>(
        (g_light_scale_identity_005ebb38 -
         listener_offset.method_00421700() / value_144) * volume);
    options->uiLoop = 1;
    options->Pos.flX = x;
    options->Pos.flY = y;
    options->Pos.flZ = z;
    options->Pos.flVelX = 0.0f;
    options->Pos.flVelY = 0.0f;
    options->Pos.flVelZ = 0.0f;
    options->Pos.flFaceX = -x;
    options->Pos.flFaceY = -y;
    options->Pos.flFaceZ = -z;
    options->Pos.flUpX = 0.0f;
    options->Pos.flUpY = g_light_scale_identity_005ebb38;
    options->Pos.flUpZ = 0.0f;
    options->Pos.flFalloffMin = value_144;
    options->Pos.flFalloffMax = value_144;
    options->Pos.uiVolume = options->uiVolume;
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
srNode* W8ClipPlane005ED180::clone()
{
    srClipPlane* copy = static_cast<srClipPlane*>(vInstance());
    *copy = *this;
    return copy;
}
