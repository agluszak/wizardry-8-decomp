#include "wiz8/engine_code/MonsterLight.h"

#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/float_constants.h"
#include "surrender/srCore.h"

#include <math.h>
#include <string.h>

// GLOBAL: WIZ8 0x005ecd4c
float g_monster_light_cycle_rate_005ecd4c = 0.025f;
// GLOBAL: WIZ8 0x005ec318
double g_double_005ec318 = 6.2831852;

/* The light-deletion path emitted this vftable slot emission ahead of the
   class's authored bodies. */
// SYNTHETIC: WIZ8 0x0049E0A0
// MonsterLight::`scalar deleting destructor'

/* Monster's fixed light is a regular srLight specialization.  Its two colours
   are retained for the optional cycle, while the first colour is also the
   initial renderer colour.  The light begins at the origin and records the
   shared engine time used later by both colour cycling and fade-out. */
// FUNCTION: WIZ8 0x0049D500
MonsterLight::MonsterLight(srNode* parent, unsigned char cycle_color, float range,
                           const srVector3T<float>* first_color,
                           const srVector3T<float>* second_color)
    : srLight(parent, srLight::PRESET_POSITIONAL_1), m_vertical_offset_228(0.0f),
      m_color_first_22c(*first_color), m_color_second_238(*second_color), m_start_time_244(0.0f),
      m_cycle_color_248(cycle_color), m_fade_out_249(0)
{
    setName("MonFixedLight");
    attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
    enable_flags_194 |= 0x10; /* ENABLE_RANGE_FAR */
    enable_flags_194 |= 4;    /* ENABLE_BOUNDING_SPHERE */
    far_end_170 = range;
    near_start_158 = 0.0;
    near_end_160 = 0.0;
    far_start_168 = 0.0;
    safe_range_1d4 = 5000.0f;
    setLinearAttenuation(range, 0.0019569471f);
    specular_1b0.SetZero();
    diffuse_1a4 = *first_color;
    setFlag(srNode::FLAG_DISABLE);
    m_start_time_244 = g_game_time_accumulator_6598bc->GetElapsed();
}

/* A copied monster light preserves the authored light configuration and
   colour-cycle settings, but starts a fresh visible interval at the copied
   node's parent. */
// FUNCTION: WIZ8 0x0049D660
MonsterLight::MonsterLight(const MonsterLight& other) : srLight(0)
{
    srLight::operator=(other);
    attenuation_model_150 = other.attenuation_model_150;
    near_start_158 = other.near_start_158;
    near_end_160 = other.near_end_160;
    far_start_168 = other.far_start_168;
    far_end_170 = other.far_end_170;
    scaled_near_start_178 = other.scaled_near_start_178;
    scaled_far_end_17c = other.scaled_far_end_17c;
    near_attenuation_180 = other.near_attenuation_180;
    far_attenuation_184 = other.far_attenuation_184;
    opengl_attenuation_188 = other.opengl_attenuation_188;
    enable_flags_194 = other.enable_flags_194;
    ambient_198 = other.ambient_198;
    diffuse_1a4 = other.diffuse_1a4;
    specular_1b0 = other.specular_1b0;
    spot_direction_1bc = other.spot_direction_1bc;
    spot_angle_1c8 = other.spot_angle_1c8;
    spot_exponent_1cc = other.spot_exponent_1cc;
    intensity_1d0 = other.intensity_1d0;
    safe_range_1d4 = other.safe_range_1d4;
    scaled_ambient_1d8 = other.scaled_ambient_1d8;
    scaled_diffuse_1e8 = other.scaled_diffuse_1e8;
    scaled_specular_1f8 = other.scaled_specular_1f8;
    spot_direction_eye_208 = other.spot_direction_eye_208;
    spot_cutoff_214 = other.spot_cutoff_214;
    attenuation_range_218 = other.attenuation_range_218;
    derived_flags_21c = other.derived_flags_21c;
    channel_mask_220 = other.channel_mask_220;

    m_vertical_offset_228 = other.m_vertical_offset_228;
    m_color_first_22c = other.m_color_first_22c;
    m_color_second_238 = other.m_color_second_238;
    m_start_time_244 = other.m_start_time_244;
    m_cycle_color_248 = other.m_cycle_color_248;
    m_fade_out_249 = 0;

    setParent(other.getParent(), 1);
    intensity_1d0 = 1.0f;
    setFlag(srNode::FLAG_DISABLE);
    m_start_time_244 = g_game_time_accumulator_6598bc->GetElapsed();
}

// FUNCTION: WIZ8 0x0049E0D0
MonsterLight::~MonsterLight() {}

// FUNCTION: WIZ8 0x0049D940
void MonsterLight::SetRange(float range)
{
    far_start_168 = 0.0;
    far_end_170 = range;
    setLinearAttenuation(range, 0.0019569471f);
}

// FUNCTION: WIZ8 0x0049D970
void MonsterLight::SetVisible0049D970(char visible)
{
    if (visible != 0) {
        clearFlag(srNode::FLAG_DISABLE);
    } else {
        setFlag(srNode::FLAG_DISABLE);
    }
}

// FUNCTION: WIZ8 0x0049D990
void MonsterLight::Update0049D990(const srVector3T<float>* position)
{
    float elapsed = g_game_time_accumulator_6598bc->GetElapsed() - m_start_time_244;

    if (m_fade_out_249 != 0) {
        float fade = elapsed * g_float_005ebc3c;
        if (fade > g_float_005ebb38) {
            fade = g_float_005ebb38;
        }
        intensity_1d0 = g_float_005ebb38 - fade;
    } else if (m_cycle_color_248 != 0) {
        float cycle = elapsed * g_monster_light_cycle_rate_005ecd4c;
        double whole = floor((double)cycle);
        float first_weight =
            (float)(sin(((double)cycle - whole) * g_double_005ec318) + g_float_005ebb38) *
            g_float_005ebc7c;
        float second_weight = g_float_005ebb38 - first_weight;

        diffuse_1a4.x = m_color_first_22c.x * first_weight + m_color_second_238.x * second_weight;
        diffuse_1a4.y = m_color_first_22c.y * first_weight + m_color_second_238.y * second_weight;
        diffuse_1a4.z = m_color_first_22c.z * first_weight + m_color_second_238.z * second_weight;
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
    m_start_time_244 = g_game_time_accumulator_6598bc->GetElapsed();
}

// TEMPLATE: WIZ8 0x0049DC20
// srClassSupport<srLight,srIlluminator,0,4640>::getClassID

// TEMPLATE: WIZ8 0x0049DC30
// srClassSupport<srLight,srIlluminator,0,4640>::getClassName

// TEMPLATE: WIZ8 0x0049DC40
// srClassSupport<srLight,srIlluminator,0,4640>::clone

// TEMPLATE: WIZ8 0x0049E300
// srClassSupport<srLight,srIlluminator,0,4640>::getClassNode

// SYNTHETIC: WIZ8 0x0049E440
// MonsterLight::`vector deleting destructor'`adjustor{312}'
