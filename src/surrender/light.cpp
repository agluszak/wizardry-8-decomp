#include "surrender/srLight.h"

#include <string.h>

// FUNCTION: SURRENDER 0x1004EAB0
srLight::srLight(const srLight& other)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    /* After the assignment, retail re-copies the whole parameter tail
       0x150..0x220 memberwise, including the records operator= skips. */
    *this = other;
    attenuation_model_150 = other.attenuation_model_150;
    near_start_158 = other.near_start_158;
    near_end_160 = other.near_end_160;
    far_start_168 = other.far_start_168;
    far_end_170 = other.far_end_170;
    memcpy(unknown_178_, other.unknown_178_, sizeof(unknown_178_));
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
    memcpy(unknown_1d8_, other.unknown_1d8_, sizeof(unknown_1d8_));
    value_21c_ = other.value_21c_;
    value_220_ = other.value_220_;
}

// FUNCTION: SURRENDER 0x1004DFB0
srLight& srLight::operator=(const srLight& other)
{
    if (this != &other) {
        srIlluminator::operator=(other);
        attenuation_model_150 = other.attenuation_model_150;
        enable_flags_194 = other.enable_flags_194;
        ambient_198 = other.ambient_198;
        diffuse_1a4 = other.diffuse_1a4;
        specular_1b0 = other.specular_1b0;
        opengl_attenuation_188 = other.opengl_attenuation_188;
        spot_direction_1bc = other.spot_direction_1bc;
        spot_angle_1c8 = other.spot_angle_1c8;
        spot_exponent_1cc = other.spot_exponent_1cc;
        intensity_1d0 = other.intensity_1d0;
        near_start_158 = other.near_start_158;
        near_end_160 = other.near_end_160;
        far_start_168 = other.far_start_168;
        far_end_170 = other.far_end_170;
        safe_range_1d4 = other.safe_range_1d4;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004DDA0
srLight::srLight(srNode* parent, e_preset preset)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    setFlag(FLAG_GLOBAL);
    value_21c_ = 0;
    value_220_ = 0;
    enable_flags_194 = 0;
    ambient_198.x = 0.0f;
    ambient_198.y = 0.0f;
    ambient_198.z = 0.0f;
    diffuse_1a4.x = 1.0f;
    diffuse_1a4.y = 1.0f;
    diffuse_1a4.z = 1.0f;
    specular_1b0.x = 1.0f;
    specular_1b0.y = 1.0f;
    specular_1b0.z = 1.0f;
    spot_direction_1bc.z = 1.0f;
    spot_direction_1bc.x = 0.0f;
    spot_direction_1bc.y = 0.0f;
    spot_exponent_1cc = 1.0f;
    intensity_1d0 = 1.0f;
    spot_angle_1c8 = 3.141592653589793 * 0.5;
    safe_range_1d4 = 0.0f;
    attenuation_model_150 = ATTENUATION_OPENGL;
    opengl_attenuation_188.x = 1.0f;
    opengl_attenuation_188.y = 0.0f;
    opengl_attenuation_188.z = 0.0f;
    near_start_158 = 0.0;
    near_end_160 = 0.0;
    far_start_168 = 0.0;
    far_end_170 = 1000.0;
    enable_flags_194 |= 0x10;
    if (preset == PRESET_POSITIONAL_0) {
        enable_flags_194 |= 0x12;
    } else if (preset == PRESET_POSITIONAL_2) {
        enable_flags_194 |= 1;
    }
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x1004DF40
void srLight::setLinearAttenuation(float range, float attenuation)
{
    if (attenuation <= 1e-06f) {
        attenuation = 1e-06f;
    }
    if (range <= 1e-06f) {
        range = 1e-06f;
    }
    opengl_attenuation_188.x = 1.0f;
    opengl_attenuation_188.z = 0.0f;
    opengl_attenuation_188.y = (1.0f - attenuation) / (range * attenuation);
}
