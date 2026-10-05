#include "wiz8/engine_code/MonsterLight.h"

#include "wiz8/engine_code/GameTimeAccumulator.h"
#include "wiz8/float_constants.h"
#include "surrender/srCore.h"

#include <math.h>
#include <string.h>

// GLOBAL: WIZ8 0x005ecd4c
const float g_monster_light_cycle_rate = 0.025f;
// GLOBAL: WIZ8 0x005ec318
const double g_double_005ec318 = 6.2831852;

/* The light-deletion path emitted this vftable slot emission ahead of the
   class's authored bodies. */

/* Monster's fixed light is a regular srLight specialization.  Its two colours
   are retained for the optional cycle, while the first colour is also the
   initial renderer colour.  The light begins at the origin and records the
   shared engine time used later by both colour cycling and fade-out. */
// FUNCTION: WIZ8 0x0049D500
MonsterLight::MonsterLight(srNode* parent, bool cycle_color, float range,
                           const srVector3T<float>* first_color,
                           const srVector3T<float>* second_color)
    : srLight(parent, srLight::PRESET_POINT), m_vertical_offset(0.0f), m_color_first(*first_color),
      m_color_second(*second_color), m_start_time(0.0f), m_cycle_color(cycle_color), m_fade_out(0)
{
    setName("MonFixedLight");
    attenuation_model = srLight::ATTENUATION_3DSTUDIO_MAX;
    enable_flags |= (1UL << srLight::ENABLE_RANGE_FAR);
    enable_flags |= (1UL << srLight::ENABLE_BOUNDING_SPHERE);
    far_end = range;
    near_start = 0.0;
    near_end = 0.0;
    far_start = 0.0;
    safe_range = 5000.0f;
    setLinearAttenuation(range, 0.0019569471f);
    specular.SetZero();
    diffuse = *first_color;
    setFlag(srNode::FLAG_DISABLE);
    m_start_time = g_game_time_accumulator->GetElapsed();
}

/* A copied monster light preserves the authored light configuration and
   colour-cycle settings, but starts a fresh visible interval at the copied
   node's parent. */
// FUNCTION: WIZ8 0x0049D660
MonsterLight::MonsterLight(const MonsterLight& other) : srLight(0)
{
    srLight::operator=(other);
    attenuation_model = other.attenuation_model;
    near_start = other.near_start;
    near_end = other.near_end;
    far_start = other.far_start;
    far_end = other.far_end;
    scaled_near_start = other.scaled_near_start;
    scaled_far_end = other.scaled_far_end;
    near_attenuation = other.near_attenuation;
    far_attenuation = other.far_attenuation;
    opengl_attenuation = other.opengl_attenuation;
    enable_flags = other.enable_flags;
    ambient = other.ambient;
    diffuse = other.diffuse;
    specular = other.specular;
    spot_direction = other.spot_direction;
    spot_angle = other.spot_angle;
    spot_exponent = other.spot_exponent;
    intensity = other.intensity;
    safe_range = other.safe_range;
    scaled_ambient = other.scaled_ambient;
    scaled_diffuse = other.scaled_diffuse;
    scaled_specular = other.scaled_specular;
    spot_direction_eye = other.spot_direction_eye;
    spot_cutoff = other.spot_cutoff;
    attenuation_range = other.attenuation_range;
    derived_flags = other.derived_flags;
    channel_mask = other.channel_mask;

    m_vertical_offset = other.m_vertical_offset;
    m_color_first = other.m_color_first;
    m_color_second = other.m_color_second;
    m_start_time = other.m_start_time;
    m_cycle_color = other.m_cycle_color;
    m_fade_out = false;

    setParent(other.getParent(), 1);
    intensity = 1.0f;
    setFlag(srNode::FLAG_DISABLE);
    m_start_time = g_game_time_accumulator->GetElapsed();
}

// FUNCTION: WIZ8 0x0049D940
void MonsterLight::SetRange(float range)
{
    far_start = 0.0;
    far_end = range;
    setLinearAttenuation(range, 0.0019569471f);
}

// FUNCTION: WIZ8 0x0049D970
void MonsterLight::SetVisible(bool visible)
{
    if (visible) {
        clearFlag(srNode::FLAG_DISABLE);
    } else {
        setFlag(srNode::FLAG_DISABLE);
    }
}

// FUNCTION: WIZ8 0x0049D990
void MonsterLight::Update(const srVector3T<float>* position)
{
    float elapsed = g_game_time_accumulator->GetElapsed() - m_start_time;

    if (m_fade_out) {
        float fade = elapsed * g_float_005ebc3c;
        if (fade > g_float_one) {
            fade = g_float_one;
        }
        intensity = g_float_one - fade;
    } else if (m_cycle_color) {
        float cycle = elapsed * g_monster_light_cycle_rate;
        double whole = floor(cycle);
        float first_weight =
            static_cast<float>(sin((cycle - whole) * g_double_005ec318) + g_float_one) *
            g_float_005ebc7c;
        float second_weight = g_float_one - first_weight;

        diffuse.x = m_color_first.x * first_weight + m_color_second.x * second_weight;
        diffuse.y = m_color_first.y * first_weight + m_color_second.y * second_weight;
        diffuse.z = m_color_first.z * first_weight + m_color_second.z * second_weight;
    }

    srVector3T<double> location;
    location.Set(position->x, position->y + m_vertical_offset, position->z);
    setLocation(location);
}

// FUNCTION: WIZ8 0x0049DAF0
void MonsterLight::StartFadeOut()
{
    m_fade_out = true;
    m_start_time = g_game_time_accumulator->GetElapsed();
}
