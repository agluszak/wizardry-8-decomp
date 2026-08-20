#include "wiz8/engine_code/MonsterLight.h"

#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/float_constants.h"
#include "surrender/srCore.h"

#include <math.h>

extern W8Object0043A910* g_object_6598bc;
extern float g_monster_light_cycle_rate_005ecd4c;
extern double g_monster_light_cycle_angle_005ec318;

/* Monster's fixed light is a regular srLight specialization.  Its two colours
   are retained for the optional cycle, while the first colour is also the
   initial renderer colour.  The light begins at the origin and records the
   shared engine time used later by both colour cycling and fade-out. */
// FUNCTION: WIZ8 0x0049D500
MonsterLight::MonsterLight(
    srNode* parent,
    unsigned char cycle_color,
    float range,
    const srVector3T<float>* first_color,
    const srVector3T<float>* second_color)
    : srLight(parent, srLight::PRESET_POSITIONAL_1),
      m_vertical_offset_228(0.0f),
      m_color_first_22c(*first_color),
      m_color_second_238(*second_color),
      m_start_time_244(0.0f),
      m_cycle_color_248(cycle_color),
      m_fade_out_249(0)
{
    setName("MonFixedLight");
    m_positional_18 = 2;
    m_positional_flags_5c |= 0x10;
    m_positional_flags_5c |= 4;
    m_range_170 = range;
    m_positional_20 = 0.0f;
    m_positional_28 = 0.0f;
    m_positional_168 = 0.0f;
    m_positional_1d4 = 5000.0f;
    m_positional_24 = 0.0f;
    unknown_2c = 0;
    m_positional_16c = 0.0f;
    setLinearAttenuation(range, 0.0019569471f);
    m_position_78.x = 0.0f;
    m_position_78.y = 0.0f;
    m_position_78.z = 0.0f;
    m_color_6c = *first_color;
    setFlag(srNode::FLAG_POSITIONAL_0);
    m_start_time_244 = g_object_6598bc->GetValue30();
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
        float second_weight = g_float_005ebb38 - first_weight;

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

// FUNCTION: WIZ8 0x0049DC40
srClass* MonsterLight::clone()
{
    srLight* instance = (srLight*)vInstance();
    *instance = *this;
    return instance;
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
