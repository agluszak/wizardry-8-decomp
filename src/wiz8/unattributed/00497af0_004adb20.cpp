#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/geometry.h"

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
        else {
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

// FUNCTION: WIZ8 0x0049E290
void srNode::TraverseInfo::NodeArray::setCapacity(unsigned int new_capacity)
{
    unsigned int copy_capacity;
    unsigned int index;
    srNode** replacement;

    if (capacity != new_capacity) {
        replacement = 0;
        if (new_capacity > 0) {
            replacement = static_cast<srNode**>(
                ::operator new(new_capacity * sizeof(srNode*)));
            if (data != 0 && capacity != 0) {
                copy_capacity = capacity;
                if (new_capacity <= copy_capacity) {
                    copy_capacity = new_capacity;
                }
                for (index = 0; index < copy_capacity; ++index) {
                    replacement[index] = data[index];
                }
            }
        }
        ::operator delete(data);
        data = replacement;
        capacity = new_capacity;
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

// FUNCTION: WIZ8 0x0049DC40
srNode* MonsterLight::vslot7()
{
    srLight* instance = (srLight*)vInstance();
    *instance = *this;
    return instance;
}

// FUNCTION: WIZ8 0x0049DD60
srNode* stLight::vslot7()
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
