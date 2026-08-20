#include "wiz8/float_constants.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/geometry.h"

#include <windows.h>
#include <math.h>
#include <new>

/* Address quarantine 0049ba51-0049e5cf. The previous 00497af0-004adb20 bounds
   spanned eight units that carry their own assertion-backed intervals -
   stParticle, OctSubMesh, Item, AnimObj, Missile, GrCycle, PathAI and Spells -
   and every body actually in this file belongs to the single unnamed unit that
   sits between them. The upper bound is OctSubMesh.cpp's interval lower bound
   less one; the lower bound is one past stParticle.cpp's last emission at
   0x0049BA50, since that unit runs past its own assertion interval. No CPP path
   string in the image names this unit, so it stays address-bounded. */

extern "C" {
extern float g_light_scale_0060bfe0;
extern double g_double_005ebc70;
extern unsigned char g_byte_0060bfdc;
extern float g_monster_light_cycle_rate_005ecd4c;
extern double g_monster_light_cycle_angle_005ec318;
}

extern W8Object0043A910* g_object_6598bc;

/* The parent-taking constructor never forwards the parent to srLight: the base
   runs with its own defaults and the node is linked afterwards, which is why
   the emitted body carries an explicit null test around setParent. Everything
   below +0x228 is stLight's own state, and both timestamps start from the same
   converted tick. */
// FUNCTION: WIZ8 0x0049C2C0
stLight::stLight(srNode* parent)
{
    m_positional_23a = 0;
    m_prop_254 = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    m_positional_18 = 2;
    m_owned_244 = 0;
    m_positional_239 = 1;
    m_positional_248 = 0;
    m_positional_250 = 1;
    m_positional_228.x = 0.0f;
    m_positional_228.y = 0.0f;
    m_positional_228.z = 0.0f;
    m_positional_240 = 0;
    m_definition_234 = 0;
    m_positional_238 = 0;
    m_positional_23c = m_positional_24c = GetTickCount() * 0.0025f;
}

/* Exactly two owned members. The definition is released through its own
   virtual destructor, so ownership of every stLightDefinition form is
   stLight's; the path is released through the kind-guarded PathAI helper
   rather than the general one. Nothing else here is owned: the prop at +0x254
   and the parent link are both left to their owners. */
// FUNCTION: WIZ8 0x0049C430
stLight::~stLight()
{
    delete m_definition_234;
    if (m_owned_244 != 0) {
        DestroyOwnedPathAI004A9110(m_owned_244);
    }
}

/* Assignment deep-copies both owned members - the definition through its
   virtual Clone slot and the path through the PathAI clone - so a copied light
   shares nothing with its source except the prop reference and the parent it
   is re-linked under. The two timestamps restart from the current tick instead
   of being copied. */
// FUNCTION: WIZ8 0x0049C690
stLight& stLight::operator=(const stLight& other)
{
    srLight::operator=(other);
    setParent(other.parentNode(), 0);
    m_positional_228 = other.m_positional_228;
    if (other.m_definition_234 != 0) {
        m_definition_234 = other.m_definition_234->Clone();
    }
    else {
        m_definition_234 = 0;
    }
    m_positional_238 = other.m_positional_238;
    m_positional_239 = other.m_positional_239;
    if (other.m_owned_244 != 0) {
        m_owned_244 = ClonePathAI004A98C0(other.m_owned_244);
    }
    else {
        m_owned_244 = 0;
    }
    m_positional_248 = other.m_positional_248;
    m_positional_250 = other.m_positional_250;
    m_positional_240 = other.m_positional_240;
    m_positional_23c = m_positional_24c = GetTickCount() * 0.0025f;
    m_prop_254 = other.m_prop_254;
    m_positional_23a = other.m_positional_23a;
    return *this;
}

// FUNCTION: WIZ8 0x0049C7A0
void stLight::traverse(srNode::TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_POSITIONAL_1)) {
        if (testFlag(FLAG_POSITIONAL_0) ||
            fabs(m_positional_98) <= g_double_005ebc70 ||
            (g_byte_0060bfdc & 1) == 0) {
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
        }
        else if (m_definition_234 != 0) {
            if (!testFlag(FLAG_POSITIONAL_2)) {
                srNode::TraverseInfo::Entry& entry =
                    info.entries[info.entry_count];
                entry.node = this;
                entry.value = 1;
                ++info.entry_count;
            }
            else {
                info.nodes[info.node_count] = this;
                ++info.node_count;
            }

            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }

            if (!testFlag(FLAG_POSITIONAL_2)) {
                srNode::TraverseInfo::Entry& entry =
                    info.entries[info.entry_count];
                entry.node = this;
                entry.value = 2;
                ++info.entry_count;
            }
        }
    }
}

// FUNCTION: WIZ8 0x0049C8D0
void stLight::process(
    const srNode::ProcessInfo& info,
    srNode::e_processType type)
{
    if ((type == 1 || type == 3) &&
        g_light_scale_0060bfe0 != g_float_005ebb38) {
        float saved_scale = m_positional_98;
        m_positional_98 = saved_scale * g_light_scale_0060bfe0;
        srLight::process(info, type);
        m_positional_98 = saved_scale;
        return;
    }
    srLight::process(info, type);
}

// FUNCTION: WIZ8 0x0049C940
void stLight::SetDefinitionTime0049C940(float time)
{
    if (m_definition_234 != 0 && m_definition_234->type_04 == 2) {
        static_cast<stLightDefinition005ECDA0*>(m_definition_234)->time_4c =
            time;
    }
}

/* Restart the light-definition driven state when a Monster switches cycles.
   The definition at +0x234 is owned by stLight; type two resets its own pair
   of counters, while the other forms restore intensity and an optional
   colour. */
// FUNCTION: WIZ8 0x0049D070
void stLight::Reset0049D070()
{
    if (m_definition_234 != 0) {
        if (m_definition_234->type_04 == 2) {
            stLightDefinition005ECDA0* definition =
                static_cast<stLightDefinition005ECDA0*>(m_definition_234);
            definition->time_4c = 0.0f;
            definition->value_48 = 0;
            m_positional_248 = 0;
            m_positional_250 = 1;
        }
        else {
            stLightDefinition005ECDBC* definition =
                static_cast<stLightDefinition005ECDBC*>(m_definition_234);
            m_positional_98 = definition->intensity_28;
            if ((definition->flags_08 & 8) != 0) {
                m_color_6c = definition->color_10;
            }
        }
    }

    m_positional_240 = 0;
    m_positional_24c = GetTickCount() * 0.0025f;
    m_positional_23c = m_positional_24c;
}

// FUNCTION: WIZ8 0x0049D970
void MonsterLight::SetVisible0049D970(char visible)
{
    if (visible != 0) {
        clearFlag(srNode::FLAG_POSITIONAL_0);
    }
    else {
        setFlag(srNode::FLAG_POSITIONAL_0);
    }
}

// FUNCTION: WIZ8 0x0049D990
void MonsterLight::Update0049D990(const srVector3T<float>* position)
{
    float elapsed = g_object_6598bc->GetValue30() - m_start_time_244;

    if (m_fade_out_249 != 0) {
        float fade = elapsed * g_float_005ebc3c;
        if (fade > g_float_005ebb38) {
            fade = g_float_005ebb38;
        }
        m_positional_98 = g_float_005ebb38 - fade;
    }
    else if (m_cycle_color_248 != 0) {
        float cycle = elapsed * g_monster_light_cycle_rate_005ecd4c;
        double whole = floor((double)cycle);
        float first_weight = (float)(
            sin(((double)cycle - whole) * g_monster_light_cycle_angle_005ec318)
            + g_float_005ebb38) *
            g_float_005ebc7c;
        float second_weight =
            g_float_005ebb38 - first_weight;

        m_color_6c.x =
            m_color_first_22c.x * first_weight +
            m_color_second_238.x * second_weight;
        m_color_6c.y =
            m_color_first_22c.y * first_weight +
            m_color_second_238.y * second_weight;
        m_color_6c.z =
            m_color_first_22c.z * first_weight +
            m_color_second_238.z * second_weight;
    }

    srVector3T<double> location;
    location.x = position->x;
    location.y = position->y + m_vertical_offset_228;
    location.z = position->z;
    setLocation(location);
}

// FUNCTION: WIZ8 0x0049DAF0
void MonsterLight::StartFadeOut0049DAF0()
{
    m_fade_out_249 = 1;
    m_start_time_244 = g_object_6598bc->GetValue30();
}

// FUNCTION: WIZ8 0x0049DC40
srClass* MonsterLight::clone()
{
    srLight* instance = (srLight*)vInstance();
    *instance = *this;
    return instance;
}

// TEMPLATE: WIZ8 0x0049DD60
// srClassSupport<stLight,srLight,0,65542>::clone

/* The registry base's own destructor, emitted out of line here rather than
   inlined the way 0x0049C430 expands it. */
// TEMPLATE: WIZ8 0x0049DD80
// srClassSupport<stLight,srLight,0,65542>::~srClassSupport

// SYNTHETIC: WIZ8 0x0049E260
// srClassSupport<stLight,srLight,0,65542>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0049E3A0
srClass* stLight::vInstance()
{
    return new stLight(0);
}

// SYNTHETIC: WIZ8 0x0049E400
// stLight::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0049E450
// stLight::`scalar deleting destructor'`adjustor{312}'
