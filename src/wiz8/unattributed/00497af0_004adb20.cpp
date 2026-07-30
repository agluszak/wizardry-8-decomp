#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/geometry.h"

#include <windows.h>
#include <math.h>
#include <new>

/* Address quarantine 00497af0-004adb20; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern "C" {
extern float g_light_scale_0060bfe0;
extern float g_light_scale_identity_005ebb38;
extern double g_double_005ebc70;
extern unsigned char g_byte_0060bfdc;
extern float g_monster_light_fade_rate_005ebc3c;
extern float g_monster_light_cycle_rate_005ecd4c;
extern double g_monster_light_cycle_angle_005ec318;
extern float g_monster_light_half_005ebc7c;
extern const float g_light_time_scale_005ec128;
}

extern W8Object0043A910* g_object_6598bc;

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
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(
                        info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 1;
                ++info.entry_count;
            }
            else {
                if (info.nodes.capacity <= info.node_count) {
                    info.nodes.setCapacity(
                        info.nodes.capacity + 8 + info.node_count);
                }
                info.nodes.data[info.node_count] = this;
                ++info.node_count;
            }

            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }

            if (!testFlag(FLAG_POSITIONAL_2)) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(
                        info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 2;
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
        g_light_scale_0060bfe0 != g_light_scale_identity_005ebb38) {
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
    m_positional_24c = GetTickCount() * g_light_time_scale_005ec128;
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
void MonsterLight::Update0049D990(const W8Position* position)
{
    float elapsed = g_object_6598bc->GetValue30() - m_start_time_244;

    if (m_fade_out_249 != 0) {
        float fade = elapsed * g_monster_light_fade_rate_005ebc3c;
        if (fade > g_light_scale_identity_005ebb38) {
            fade = g_light_scale_identity_005ebb38;
        }
        m_positional_98 = g_light_scale_identity_005ebb38 - fade;
    }
    else if (m_cycle_color_248 != 0) {
        float cycle = elapsed * g_monster_light_cycle_rate_005ecd4c;
        double whole = floor((double)cycle);
        float first_weight = (float)(
            sin(((double)cycle - whole) * g_monster_light_cycle_angle_005ec318)
            + g_light_scale_identity_005ebb38) *
            g_monster_light_half_005ebc7c;
        float second_weight =
            g_light_scale_identity_005ebb38 - first_weight;

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
srNode* MonsterLight::clone()
{
    srLight* instance = (srLight*)vInstance();
    *instance = *this;
    return instance;
}

// FUNCTION: WIZ8 0x0049DD60
srNode* stLight::clone()
{
    stLight* instance = static_cast<stLight*>(vInstance());
    *instance = *this;
    return instance;
}

// FUNCTION: WIZ8 0x0049E3A0
srClass* stLight::vInstance()
{
    return new stLight(0);
}
