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
    : srLight(parent, srLight::PRESET_POINT), m_vertical_offset(0.0f),
      m_color_first(*first_color), m_color_second(*second_color), m_start_time(0.0f),
      m_cycle_color(cycle_color), m_fade_out(0)
{
    setName("MonFixedLight");
    attenuation_model = srLight::ATTENUATION_3DSTUDIO_MAX;
    enable_flags |= 0x10; /* ENABLE_RANGE_FAR */
    enable_flags |= 4;    /* ENABLE_BOUNDING_SPHERE */
    far_end = range;
    near_start = 0.0;
    near_end = 0.0;
    far_start = 0.0;
    safe_range = 5000.0f;
    setLinearAttenuation(range, 0.0019569471f);
    specular_1b0.SetZero();
    diffuse_1a4 = *first_color;
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
    ambient_198 = other.ambient_198;
    diffuse_1a4 = other.diffuse_1a4;
    specular_1b0 = other.specular_1b0;
    spot_direction = other.spot_direction;
    spot_angle = other.spot_angle;
    spot_exponent = other.spot_exponent;
    intensity_1d0 = other.intensity_1d0;
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
    m_fade_out = 0;

    setParent(other.getParent(), 1);
    intensity_1d0 = 1.0f;
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
    if (visible != 0) {
        clearFlag(srNode::FLAG_DISABLE);
    } else {
        setFlag(srNode::FLAG_DISABLE);
    }
}

// FUNCTION: WIZ8 0x0049D990
void MonsterLight::Update(const srVector3T<float>* position)
{
    float elapsed = g_game_time_accumulator->GetElapsed() - m_start_time;

    if (m_fade_out != 0) {
        float fade = elapsed * g_float_005ebc3c;
        if (fade > g_float_one) {
            fade = g_float_one;
        }
        intensity_1d0 = g_float_one - fade;
    } else if (m_cycle_color != 0) {
        float cycle = elapsed * g_monster_light_cycle_rate;
        double whole = floor(cycle);
        float first_weight =
            static_cast<float>(sin((cycle - whole) * g_double_005ec318) + g_float_one) *
            g_float_005ebc7c;
        float second_weight = g_float_one - first_weight;

        diffuse_1a4.x = m_color_first.x * first_weight + m_color_second.x * second_weight;
        diffuse_1a4.y = m_color_first.y * first_weight + m_color_second.y * second_weight;
        diffuse_1a4.z = m_color_first.z * first_weight + m_color_second.z * second_weight;
    }

    srVector3T<double> location;
    location.Set(position->x, position->y + m_vertical_offset, position->z);
    setLocation(location);
}

// FUNCTION: WIZ8 0x0049DAF0
void MonsterLight::StartFadeOut()
{
    m_fade_out = 1;
    m_start_time = g_game_time_accumulator->GetElapsed();
}
