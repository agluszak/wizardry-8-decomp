// Recovery of sr.dll's srLight implementation against the gog-base retail
// binary.

#include "surrender/srLight.h"

#include <math.h>

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

// FUNCTION: SURRENDER 0x1004E8F0
const char* srLight::sGetClassName()
{
    return "srLight";
}

// FUNCTION: SURRENDER 0x1004ED70
srLight::~srLight() {}

/* Derived-state bit meanings recovered from process/isActive: bit0 = pushed
   as an active vertex processor this frame, bit1 = spotlight cone active,
   bit2 = directional, bit3 = OpenGL attenuation active, bit4 = constant-only
   OpenGL attenuation, bit5 = far-range attenuation active. */

// FUNCTION: SURRENDER 0x1004DDA0
srLight::srLight(srNode* parent, e_preset preset)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    enable_flags_194 = 0;
    derived_flags_21c = 0;
    channel_mask_220 = 0;
    setFlag(FLAG_GLOBAL);
    enable_flags_194 = 0;
    ambient_198.x = ambient_198.y = ambient_198.z = 0.0f;
    diffuse_1a4.x = diffuse_1a4.y = diffuse_1a4.z = 1.0f;
    specular_1b0.x = specular_1b0.y = specular_1b0.z = 1.0f;
    spot_direction_1bc.x = spot_direction_1bc.y = 0.0f;
    spot_direction_1bc.z = 1.0f;
    spot_exponent_1cc = 1.0f;
    intensity_1d0 = 1.0f;
    spot_angle_1c8 = (float)(3.141592653589793 * 0.5);
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
        enable_flags_194 |= 0x1;
    }
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x1004EAB0
srLight::srLight(const srLight& other)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    *this = other;
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

// GLOBAL: SURRENDER 0x100A49D0
// Comma-separated light flag-name table, unset on disk; dump prints flag
// indices when it is null.
static const char* const s_light_flag_names = 0;

// FUNCTION: SURRENDER 0x1004E0C0
void srLight::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Control flags: ";
    if (enable_flags_194 == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_light_flag_names;
        for (unsigned long index = 0; index < 0x20; ++index) {
            if ((enable_flags_194 & (1ul << index)) == 0) {
                if (names != 0) {
                    while (*names != '\0' && *names != ',') {
                        ++names;
                    }
                }
            } else {
                if (first) {
                    first = false;
                } else {
                    stream << ',';
                }
                if (names == 0 || *names == '\0') {
                    stream << index;
                } else {
                    while (*names != '\0' && *names != ',') {
                        stream << *names;
                        ++names;
                    }
                }
            }
            if (names != 0 && *names == ',') {
                ++names;
            }
        }
        stream << ']';
    }
    stream << '\n';
    stream.width(0x20);
    stream << "  Intensity: " << intensity_1d0 << '\n';
    stream.width(0x20);
    stream << "  Ambient coeff.: {" << ambient_198.x << ',' << ambient_198.y << ',' << ambient_198.z
           << '}' << '\n';
    stream.width(0x20);
    stream << "  Diffuse coeff.: {" << diffuse_1a4.x << ',' << diffuse_1a4.y << ',' << diffuse_1a4.z
           << '}' << '\n';
    stream.width(0x20);
    stream << "  Specular coeff.: {" << specular_1b0.x << ',' << specular_1b0.y << ','
           << specular_1b0.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Spot direction: {" << spot_direction_1bc.x << ',' << spot_direction_1bc.y << ','
           << spot_direction_1bc.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Spot angle: " << spot_angle_1c8 << '\n';
    stream.width(0x20);
    stream << "  Spot exponent: " << spot_exponent_1cc << '\n';
    stream.width(0x20);
    stream << "  Safe Range: " << safe_range_1d4 << '\n';
    stream.width(0x20);
    stream << "  Attenuation model: ";
    if (attenuation_model_150 == ATTENUATION_OPENGL) {
        stream << "OpenGL" << '\n';
        stream.width(0x20);
        stream << "  Attenuation fact.: {" << opengl_attenuation_188.x << ','
               << opengl_attenuation_188.y << ',' << opengl_attenuation_188.z << '}' << '\n';
    } else if (attenuation_model_150 == ATTENUATION_3DSTUDIO_MAX) {
        stream << "3DStudio Max" << '\n';
        stream.width(0x20);
        stream << "  Near Range : " << near_start_158 << " - " << near_end_160 << '\n';
        stream.width(0x20);
        stream << "  Far Range : " << far_start_168 << " - " << far_end_170 << '\n';
    } else {
        stream << "No attenuation" << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004D650
void srLight::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    /* See srIlluminator::process for the unnamed-enumerator rationale. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
    if (type != 1 && type != 3) {
        if (type != 2 && type != 4) {
            return;
        }
        if ((derived_flags_21c & 0x1) == 0) {
            return;
        }
        renderer->popVertexProcessor();
        return;
    }
#pragma clang diagnostic pop
    applyWorldSpaceMatrix(*renderer);
    srMatrix4T<float> model_view;
    renderer->getMatrix(model_view);
    derived_flags_21c = 0;
    derived_flags_21c = 1;
    channel_mask_220 = 0;
    if (ambient_198.x != 0.0f || ambient_198.y != 0.0f || ambient_198.z != 0.0f) {
        channel_mask_220 |= 0x200;
        scaled_ambient_1d8.x = ambient_198.x * intensity_1d0;
        scaled_ambient_1d8.y = ambient_198.y * intensity_1d0;
        scaled_ambient_1d8.z = ambient_198.z * intensity_1d0;
        scaled_ambient_1d8.w = 0.0f;
    }
    if (diffuse_1a4.x != 0.0f || diffuse_1a4.y != 0.0f || diffuse_1a4.z != 0.0f) {
        channel_mask_220 |= 0x400;
        scaled_diffuse_1e8.x = diffuse_1a4.x * intensity_1d0;
        scaled_diffuse_1e8.y = diffuse_1a4.y * intensity_1d0;
        scaled_diffuse_1e8.z = diffuse_1a4.z * intensity_1d0;
        scaled_diffuse_1e8.w = 0.0f;
    }
    if (specular_1b0.x != 0.0f || specular_1b0.y != 0.0f || specular_1b0.z != 0.0f) {
        channel_mask_220 |= 0x4;
        scaled_specular_1f8.x = specular_1b0.x * intensity_1d0;
        scaled_specular_1f8.y = specular_1b0.y * intensity_1d0;
        scaled_specular_1f8.z = specular_1b0.z * intensity_1d0;
        scaled_specular_1f8.w = 0.0f;
    }
    if (channel_mask_220 == 0) {
        derived_flags_21c &= ~0x1;
    }
    if ((enable_flags_194 & 0x2) != 0) {
        derived_flags_21c |= 0x4;
        srVector3T<float> direction(-model_view.vectors[0].z, -model_view.vectors[1].z,
                                    -model_view.vectors[2].z);
        float length_squared =
            direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
        if (length_squared != 1.0f) {
            float scale = 1.0f / sqrtf(length_squared);
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
        }
        eye_location_140 = direction;
    } else {
        if ((enable_flags_194 & 0x1) != 0 && spot_angle_1c8 > 0.0f && spot_exponent_1cc != 0.0f) {
            srMatrix4T<float> inverse;
            renderer->getInverseModelViewMatrix(inverse);
            derived_flags_21c |= 0x2;
            spot_cutoff_214 = (float)cos(spot_angle_1c8);
            spot_direction_eye_208.x = inverse.vectors[0].x * spot_direction_1bc.x +
                                       inverse.vectors[1].x * spot_direction_1bc.y +
                                       inverse.vectors[2].x * spot_direction_1bc.z;
            spot_direction_eye_208.y = inverse.vectors[0].y * spot_direction_1bc.x +
                                       inverse.vectors[1].y * spot_direction_1bc.y +
                                       inverse.vectors[2].y * spot_direction_1bc.z;
            spot_direction_eye_208.z = inverse.vectors[0].z * spot_direction_1bc.x +
                                       inverse.vectors[1].z * spot_direction_1bc.y +
                                       inverse.vectors[2].z * spot_direction_1bc.z;
            float length_squared = spot_direction_eye_208.x * spot_direction_eye_208.x +
                                   spot_direction_eye_208.y * spot_direction_eye_208.y +
                                   spot_direction_eye_208.z * spot_direction_eye_208.z;
            if (length_squared != 0.0f) {
                float scale = 1.0f / sqrtf(length_squared);
                spot_direction_eye_208.x *= scale;
                spot_direction_eye_208.y *= scale;
                spot_direction_eye_208.z *= scale;
            }
        }
        if (attenuation_model_150 != ATTENUATION_NONE) {
            if (attenuation_model_150 == ATTENUATION_OPENGL) {
                derived_flags_21c |= 0x8;
                if (fabsf(opengl_attenuation_188.y) < 5.9604645e-08f &&
                    fabsf(opengl_attenuation_188.z) < 5.9604645e-08f) {
                    if (opengl_attenuation_188.x == 1.0f ||
                        fabsf(opengl_attenuation_188.x) < 5.9604645e-08f) {
                        derived_flags_21c &= ~0x8;
                    }
                    derived_flags_21c |= 0x10;
                }
            } else {
                float scale = renderer->getMaxModelViewScale();
                scaled_near_start_178 = scale * (float)near_start_158;
                scaled_far_end_17c = scale * (float)far_end_170;
                if (near_start_158 == near_end_160) {
                    near_attenuation_180 = 1.0f;
                } else {
                    near_attenuation_180 = (float)(1.0 / (scale * (near_end_160 - near_start_158)));
                }
                if (far_start_168 == far_end_170) {
                    far_attenuation_184 = 1.0f;
                } else {
                    far_attenuation_184 = (float)(1.0 / (scale * (far_end_170 - far_start_168)));
                }
                if ((enable_flags_194 & 0x10) != 0) {
                    if ((enable_flags_194 & 0x4) != 0) {
                        srVector3T<float> origin(0.0f, 0.0f, 0.0f);
                        if (renderer->testBoundingSphere(origin, safe_range_1d4 +
                                                                     (float)far_end_170) == 0) {
                            derived_flags_21c &= ~0x1;
                        }
                    }
                    attenuation_range_218 = scaled_far_end_17c;
                    derived_flags_21c |= 0x20;
                }
            }
        }
        eye_location_140.x = model_view.vectors[0].w;
        eye_location_140.y = model_view.vectors[1].w;
        eye_location_140.z = model_view.vectors[2].w;
    }
    renderer->popMatrix();
    if ((derived_flags_21c & 0x1) != 0) {
        renderer->pushVertexProcessor(*static_cast<srVertexProcessor*>(this));
    }
}

// FUNCTION: SURRENDER 0x1004CCC0
int srLight::isActive(srVertexPipe& pipe)
{
    const srVertexPipe::Input* input = pipe.input_6c;
    if ((group_mask_13c & input->exclusion_mask_34) != 0) {
        return 0;
    }
    if ((derived_flags_21c & 0x20) != 0) {
        float dx = eye_location_140.x - input->eye_center_18.x;
        float dy = eye_location_140.y - input->eye_center_18.y;
        float dz = eye_location_140.z - input->eye_center_18.z;
        float range = attenuation_range_218 + input->eye_radius_24;
        if (range * range <= dx * dx + dy * dy + dz * dz) {
            return 0;
        }
    }
    if ((derived_flags_21c & 0x2) != 0) {
        float radius = input->eye_radius_24;
        float dx = input->eye_center_18.x - eye_location_140.x;
        float dy = input->eye_center_18.y - eye_location_140.y;
        float dz = input->eye_center_18.z - eye_location_140.z;
        float distance_squared = dx * dx + dy * dy + dz * dz;
        if (radius * radius < distance_squared &&
            (dx * spot_direction_eye_208.x + dy * spot_direction_eye_208.y +
             dz * spot_direction_eye_208.z) /
                        sqrtf(distance_squared) +
                    radius / sqrtf(radius * radius + distance_squared) <
                spot_cutoff_214) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x1004CE00
void srLight::process(srVertexPipe& pipe)
{
    unsigned long channels = channel_mask_220 & pipe.channel_mask_0c;
    if (channels == 0) {
        return;
    }
    /* 0x400 = 1<<CHANNEL_LIGHT_DIFFUSE; 4 = 1<<CHANNEL_SPECULAR: either needs
       the normal dot products below. */
    int need_normals = 0;
    if ((channels & 0x400) != 0 || (channels & 0x4) != 0) {
        need_normals = 1;
    }
    SRDWORD count = pipe.vertex_count_88;
    /* Retail's stack frame aligns a 0x700-byte work area to 32 bytes: five
       64-entry banks (spot factors, attenuation, dot products, distances,
       eye-space directions). */
    float raw[0x1c0 + 8];
    // reinterpret-ok: manual 32-byte alignment of raw VP scratch storage.
    float* work = reinterpret_cast<float*>((reinterpret_cast<unsigned long>(raw) + 0x1f) & ~0x1ful);
    float* spot_factors = work;
    float* attenuation_bank = work + 0x40;
    float* dots = work + 0x80;
    float* distances = work + 0xc0;
    // reinterpret-ok: raw aligned scratch reinterpreted as the direction array.
    srVector3T<float>* directions = reinterpret_cast<srVector3T<float>*>(work + 0x100);
    srVertexPipe::Scratch* scratch = static_cast<srVertexPipe::Scratch*>(pipe.scratch_00);
    float* attenuation = 0;

    if ((derived_flags_21c & 0x4) == 0) {
        srVectorProcessor::copy(
            directions, pipe.eye_space_locations_7c + pipe.batch_base_80 + pipe.sub_batch_offset_84,
            count);
        srVectorProcessor::sub(directions, eye_location_140, directions, count);
        srVectorProcessor::dir(directions, distances, directions, count);
        if (attenuation_model_150 == ATTENUATION_3DSTUDIO_MAX) {
            if ((enable_flags_194 & 0x8) != 0) {
                srVectorProcessor::add(attenuation_bank, -scaled_near_start_178, distances, count);
                srVectorProcessor::mul(attenuation_bank, near_attenuation_180, attenuation_bank,
                                       count);
                srVectorProcessor::clampUnit(attenuation_bank, attenuation_bank, count);
                attenuation = attenuation_bank;
            }
            if ((enable_flags_194 & 0x10) != 0) {
                float* far_bank = attenuation != 0 ? spot_factors : attenuation_bank;
                srVectorProcessor::sub(far_bank, scaled_far_end_17c, distances, count);
                srVectorProcessor::mul(far_bank, far_attenuation_184, far_bank, count);
                srVectorProcessor::clampUnit(far_bank, far_bank, count);
                if (far_bank == spot_factors) {
                    srVectorProcessor::mul(attenuation_bank, attenuation_bank, spot_factors, count);
                } else {
                    attenuation = far_bank;
                }
            }
            if (attenuation != 0 && srVectorProcessor::isZero(attenuation, count)) {
                return;
            }
        } else if (attenuation_model_150 == ATTENUATION_OPENGL) {
            if ((derived_flags_21c & 0x8) != 0) {
                if ((derived_flags_21c & 0x10) != 0) {
                    float constant = 1.0f / opengl_attenuation_188.x;
                    // reinterpret-ok: retail pushes the float bit pattern into
                    // the SRDWORD fill.
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(attenuation_bank),
                                            reinterpret_cast<SRDWORD&>(constant), count);
                } else {
                    srVectorProcessor::invPoly(attenuation_bank, distances, opengl_attenuation_188,
                                               count);
                }
                srVectorProcessor::clampUnit(attenuation_bank, attenuation_bank, count);
                attenuation = attenuation_bank;
            }
        }
        if ((derived_flags_21c & 0x2) != 0) {
            srVector3T<float> negated(-spot_direction_eye_208.x, -spot_direction_eye_208.y,
                                      -spot_direction_eye_208.z);
            srVectorProcessor::dot(spot_factors, negated, directions, count);
            srVectorProcessor::add(spot_factors, -spot_cutoff_214, spot_factors, count);
            srVectorProcessor::mul(spot_factors, 1.0f / (1.0f - spot_cutoff_214), spot_factors,
                                   count);
            srVectorProcessor::clampMin(spot_factors, spot_factors, 0.0f, count);
            if (srVectorProcessor::isZero(spot_factors, count)) {
                return;
            }
            if (spot_exponent_1cc != 1.0f) {
                srVectorProcessor::srSpecularPow(spot_factors, spot_factors, spot_exponent_1cc,
                                                 count);
            }
            if (attenuation != 0) {
                srVectorProcessor::mul(attenuation, attenuation, spot_factors, count);
            } else {
                attenuation = spot_factors;
            }
        }
        if (need_normals != 0) {
            if ((scratch->flags_b00 & 0x8) == 0) {
                pipe.setupEyeSpaceNormal();
            }
            srVectorProcessor::dot(dots, scratch->normals_300 + pipe.sub_batch_offset_84,
                                   directions, count);
            srVectorProcessor::clampMin(dots, dots, 0.0f, count);
        }
    } else if (need_normals != 0) {
        if ((scratch->flags_b00 & 0x8) == 0) {
            pipe.setupEyeSpaceNormal();
        }
        srVectorProcessor::dot(dots, eye_location_140,
                               scratch->normals_300 + pipe.sub_batch_offset_84, count);
        srVectorProcessor::clampUnit(dots, dots, count);
    }

    if ((channels & 0x200) != 0) {
        srVector4T<float> ambient;
        ambient.x = scaled_ambient_1d8.x * pipe.material_info_14.ambient.x;
        ambient.y = scaled_ambient_1d8.y * pipe.material_info_14.ambient.y;
        ambient.z = scaled_ambient_1d8.z * pipe.material_info_14.ambient.z;
        ambient.w = scaled_ambient_1d8.w * pipe.material_info_14.ambient.w;
        if (attenuation != 0) {
            pipe.applyDiffuseLight(attenuation, ambient);
        } else {
            pipe.applyDiffuseLight(ambient);
        }
    }
    if (need_normals == 0) {
        return;
    }
    if (srVectorProcessor::isZero(dots, count)) {
        return;
    }
    if ((channels & 0x400) != 0) {
        srVector4T<float> diffuse;
        diffuse.x = scaled_diffuse_1e8.x * pipe.material_info_14.diffuse.x;
        diffuse.y = scaled_diffuse_1e8.y * pipe.material_info_14.diffuse.y;
        diffuse.z = scaled_diffuse_1e8.z * pipe.material_info_14.diffuse.z;
        diffuse.w = scaled_diffuse_1e8.w * pipe.material_info_14.diffuse.w;
        srCore.getStatisticsManager()->statistics_00.diffuse_operations_24 += count;
        if ((pipe.lazy_setup_mask_10 & 0x2) == 0) {
            pipe.setupDiffuse();
        }
        srVector4T<float>* out =
            pipe.vertex_array_78->diffuse_04 + pipe.batch_base_80 + pipe.sub_batch_offset_84;
        if (attenuation != 0) {
            srVectorProcessor::axpy(out, out, diffuse, attenuation, dots, count);
        } else {
            srVectorProcessor::axpy(out, out, diffuse, dots, count);
        }
    }
    if ((channels & 0x4) == 0) {
        return;
    }
    if ((derived_flags_21c & 0x4) != 0 && count != 0) {
        if (eye_location_140.x == eye_location_140.y && eye_location_140.x == eye_location_140.z) {
            if (count * 3 != 0) {
                // reinterpret-ok: the scalar broadcast fills the v3 array as
                // flat dwords.
                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(directions),
                                        *reinterpret_cast<SRDWORD*>(&eye_location_140.x),
                                        count * 3);
            }
        } else {
            srVectorProcessor::copy(directions, eye_location_140, count);
        }
    }
    if ((scratch->flags_b00 & 0x1) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    if (count * 3 != 0) {
        // reinterpret-ok: elementwise float subtraction across the v3 array
        // (view directions subtracted from the half vectors).
        srVectorProcessor::sub(
            reinterpret_cast<float*>(directions), reinterpret_cast<const float*>(directions),
            reinterpret_cast<const float*>(scratch->dir_000 + pipe.sub_batch_offset_84), count * 3);
    }
    srVectorProcessor::normalize(directions, directions, 1.0f, count);
    if ((scratch->flags_b00 & 0x8) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    srVectorProcessor::dot(distances, scratch->normals_300 + pipe.sub_batch_offset_84, directions,
                           count);
    srVectorProcessor::clampMin(distances, distances, 0.0f, count);
    if (pipe.material_info_14.shininess > 1.0f) {
        srVectorProcessor::srSpecularPow(distances, distances, pipe.material_info_14.shininess,
                                         count);
    }
    srVectorProcessor::mul(distances, distances, dots, count);
    srVector4T<float> specular;
    specular.x = scaled_specular_1f8.x * pipe.material_info_14.specular.x;
    specular.y = scaled_specular_1f8.y * pipe.material_info_14.specular.y;
    specular.z = scaled_specular_1f8.z * pipe.material_info_14.specular.z;
    specular.w = scaled_specular_1f8.w * pipe.material_info_14.specular.w;
    srCore.getStatisticsManager()->statistics_00.specular_operations_28 += count;
    if ((pipe.lazy_setup_mask_10 & 0x4) == 0) {
        pipe.setupSpecular();
    }
    srVector4T<float>* out =
        pipe.vertex_array_78->specular_08 + pipe.batch_base_80 + pipe.sub_batch_offset_84;
    if (attenuation != 0) {
        srVectorProcessor::axpy(out, out, specular, attenuation, distances, count);
    } else {
        srVectorProcessor::axpy(out, out, specular, distances, count);
    }
}

// FUNCTION: SURRENDER 0x1004DC80
void srLight::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE)) {
        return;
    }
    if (testFlag(FLAG_DISABLE) || fabsf(intensity_1d0) <= 0.0001f) {
        if (firstChild() != 0) {
            firstChild()->traverse(info);
        }
        return;
    }
    if (!testFlag(FLAG_GLOBAL)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 1;
        ++info.entry_count;
    } else {
        info.nodes[info.node_count] = this;
        ++info.node_count;
    }
    if (firstChild() != 0) {
        firstChild()->traverse(info);
    }
    if (!testFlag(FLAG_GLOBAL)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 2;
        ++info.entry_count;
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

// FUNCTION: SURRENDER 0x1004E630
void srLight::enable(e_enable flag)
{
    enable_flags_194 |= 1 << flag;
}

// FUNCTION: SURRENDER 0x1004E610
void srLight::disable(e_enable flag)
{
    enable_flags_194 &= ~(1 << flag);
}

// FUNCTION: SURRENDER 0x1004E650
int srLight::isEnabled(e_enable flag) const
{
    return (enable_flags_194 & (1 << flag)) != 0;
}

// FUNCTION: SURRENDER 0x1004E760
void srLight::setAmbient(const srVector3T<float>& ambient)
{
    ambient_198 = ambient;
}

// FUNCTION: SURRENDER 0x1004E670
srVector3T<float> srLight::getAmbient() const
{
    return ambient_198;
}

// FUNCTION: SURRENDER 0x1004E780
void srLight::setDiffuse(const srVector3T<float>& diffuse)
{
    diffuse_1a4 = diffuse;
}

// FUNCTION: SURRENDER 0x1004E6A0
srVector3T<float> srLight::getDiffuse() const
{
    return diffuse_1a4;
}

// FUNCTION: SURRENDER 0x1004E7B0
void srLight::setSpecular(const srVector3T<float>& specular)
{
    specular_1b0 = specular;
}

// FUNCTION: SURRENDER 0x1004E6E0
srVector3T<float> srLight::getSpecular() const
{
    return specular_1b0;
}

// FUNCTION: SURRENDER 0x1004E830
void srLight::setSpotDirection(const srVector3T<float>& direction)
{
    spot_direction_1bc = direction;
    float length_squared = spot_direction_1bc.x * spot_direction_1bc.x +
                           spot_direction_1bc.y * spot_direction_1bc.y +
                           spot_direction_1bc.z * spot_direction_1bc.z;
    if (length_squared != 0.0f) {
        float scale = 1.0f / sqrtf(length_squared);
        spot_direction_1bc.x *= scale;
        spot_direction_1bc.y *= scale;
        spot_direction_1bc.z *= scale;
    }
}

// FUNCTION: SURRENDER 0x1004E7D0
void srLight::setSpotAngle(float angle)
{
    if (angle <= 0.0f) {
        spot_angle_1c8 = 0.0f;
    } else if (3.141592653589793 * 0.5 <= angle) {
        spot_angle_1c8 = (float)(3.141592653589793 * 0.5);
    } else {
        spot_angle_1c8 = angle;
    }
}

// FUNCTION: SURRENDER 0x1004E8A0
void srLight::setSpotExponent(float exponent)
{
    if (exponent <= 0.0f) {
        spot_exponent_1cc = 0.0f;
    } else if (exponent >= 128.0f) {
        spot_exponent_1cc = 128.0f;
    } else {
        spot_exponent_1cc = exponent;
    }
}

// FUNCTION: SURRENDER 0x1004E720
srVector3T<float> srLight::getSpotDirection() const
{
    return spot_direction_1bc;
}

// FUNCTION: SURRENDER 0x1004E710
float srLight::getSpotAngle() const
{
    return spot_angle_1c8;
}

// FUNCTION: SURRENDER 0x1004E750
float srLight::getSpotExponent() const
{
    return spot_exponent_1cc;
}

// FUNCTION: SURRENDER 0x1004E7A0
void srLight::setIntensity(float intensity)
{
    intensity_1d0 = intensity;
}

// FUNCTION: SURRENDER 0x1004E6D0
float srLight::getIntensity() const
{
    return intensity_1d0;
}

// FUNCTION: SURRENDER 0x1004E900
void srLight::setAttenuationModel(e_attenuationModel model)
{
    attenuation_model_150 = model;
}

// FUNCTION: SURRENDER 0x1004E910
srLight::e_attenuationModel srLight::getAttenuationModel() const
{
    return attenuation_model_150;
}

// FUNCTION: SURRENDER 0x1004E920
void srLight::setAttenuation(const srVector3T<float>& attenuation)
{
    float z = attenuation.z;
    if (z <= 0.0001f) {
        z = 0.0001f;
    }
    float y = attenuation.y;
    if (y <= 0.0001f) {
        y = 0.0001f;
    }
    float x = attenuation.x;
    if (x <= 0.0001f) {
        x = 0.0001f;
    }
    opengl_attenuation_188.x = x;
    opengl_attenuation_188.y = y;
    opengl_attenuation_188.z = z;
}

// FUNCTION: SURRENDER 0x1004E9A0
srVector3T<float> srLight::getAttenuation() const
{
    return opengl_attenuation_188;
}

// FUNCTION: SURRENDER 0x1004EA60
void srLight::setNearAttenuationRange(double start, double end)
{
    near_start_158 = start;
    near_end_160 = end;
}

// FUNCTION: SURRENDER 0x1004EA00
void srLight::getNearAttenuationRange(double& start, double& end) const
{
    start = near_start_158;
    end = near_end_160;
}

// FUNCTION: SURRENDER 0x1004EA30
void srLight::setFarAttenuationRange(double start, double end)
{
    far_start_168 = start;
    far_end_170 = end;
}

// FUNCTION: SURRENDER 0x1004E9D0
void srLight::getFarAttenuationRange(double& start, double& end) const
{
    start = far_start_168;
    end = far_end_170;
}

// FUNCTION: SURRENDER 0x1004EA90
void srLight::setSafeRange(float range)
{
    safe_range_1d4 = range;
}

// FUNCTION: SURRENDER 0x1004EAA0
float srLight::getSafeRange() const
{
    return safe_range_1d4;
}

// SYNTHETIC: SURRENDER 0x1004EEF0
// srLight default constructor closure

// SYNTHETIC: SURRENDER 0X1004EF00
// srLight scalar deleting destructor

// TEMPLATE: SURRENDER 0X1004F030
// srClassSupport<srLight, srIlluminator, false, 0x1220>::clone

// TEMPLATE: SURRENDER 0X1004F050
// srClientSupport<srLight, 0x1220>::~srClientSupport

// SYNTHETIC: SURRENDER 0X1004F1E0
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X1004F1F0
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X1004F220
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X1004F230
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X1004F250
// srClientSupport<srLight, 0x1220> scalar deleting destructor

// TEMPLATE: SURRENDER 0X1004F270
// srClientSupport<srLight, 0x1220>::sGetClassNode
