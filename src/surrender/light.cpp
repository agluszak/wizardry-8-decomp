// Recovery of sr.dll's srLight implementation against the gog-base retail
// binary.

#include "surrender/srLight.h"

#include <math.h>
#include <ostream>

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

/* Derived-state bit meanings recovered from process/isActive: bit0 = pushed
   as an active vertex processor this frame, bit1 = spotlight cone active,
   bit2 = directional, bit3 = OpenGL attenuation active, bit4 = constant-only
   OpenGL attenuation, bit5 = far-range attenuation active. */

// FUNCTION: SURRENDER 0x1004DDA0
srLight::srLight(srNode* parent, e_preset preset)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    enable_flags = 0;
    derived_flags = 0;
    channel_mask = 0;
    setFlag(FLAG_GLOBAL);
    enable_flags = 0;
    ambient_198.SetZero();
    diffuse_1a4 = 1.0f;
    specular_1b0 = 1.0f;
    spot_direction.Set(0.0f, 0.0f, 1.0f);
    spot_exponent = 1.0f;
    intensity_1d0 = 1.0f;
    spot_angle = (float)(3.141592653589793 * 0.5);
    safe_range = 0.0f;
    attenuation_model = ATTENUATION_OPENGL;
    opengl_attenuation.x = 1.0f;
    opengl_attenuation.y = 0.0f;
    opengl_attenuation.z = 0.0f;
    near_start = 0.0;
    near_end = 0.0;
    far_start = 0.0;
    far_end = 1000.0;
    enable_flags |= 0x10;
    if (preset == PRESET_DIRECTIONAL) {
        enable_flags |= 0x12;
    } else if (preset == PRESET_SPOT) {
        enable_flags |= 0x1;
    }
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x1004DFB0
srLight& srLight::operator=(const srLight& other)
{
    if (this != &other) {
        srIlluminator::operator=(other);
        attenuation_model = other.attenuation_model;
        enable_flags = other.enable_flags;
        ambient_198 = other.ambient_198;
        diffuse_1a4 = other.diffuse_1a4;
        specular_1b0 = other.specular_1b0;
        opengl_attenuation = other.opengl_attenuation;
        spot_direction = other.spot_direction;
        spot_angle = other.spot_angle;
        spot_exponent = other.spot_exponent;
        intensity_1d0 = other.intensity_1d0;
        near_start = other.near_start;
        near_end = other.near_end;
        far_start = other.far_start;
        far_end = other.far_end;
        safe_range = other.safe_range;
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
    if (enable_flags == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_light_flag_names;
        for (unsigned long index = 0; index < 0x20; ++index) {
            if ((enable_flags & (1ul << index)) == 0) {
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
    stream << "  Spot direction: {" << spot_direction.x << ',' << spot_direction.y << ','
           << spot_direction.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Spot angle: " << spot_angle << '\n';
    stream.width(0x20);
    stream << "  Spot exponent: " << spot_exponent << '\n';
    stream.width(0x20);
    stream << "  Safe Range: " << safe_range << '\n';
    stream.width(0x20);
    stream << "  Attenuation model: ";
    if (attenuation_model == ATTENUATION_OPENGL) {
        stream << "OpenGL" << '\n';
        stream.width(0x20);
        stream << "  Attenuation fact.: {" << opengl_attenuation.x << ','
               << opengl_attenuation.y << ',' << opengl_attenuation.z << '}' << '\n';
    } else if (attenuation_model == ATTENUATION_3DSTUDIO_MAX) {
        stream << "3DStudio Max" << '\n';
        stream.width(0x20);
        stream << "  Near Range : " << near_start << " - " << near_end << '\n';
        stream.width(0x20);
        stream << "  Far Range : " << far_start << " - " << far_end << '\n';
    } else {
        stream << "No attenuation" << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004D650
void srLight::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type != PROCESS_PUSH && type != PROCESS_PUSH_GLOBAL) {
        if (type != PROCESS_POP && type != PROCESS_POP_GLOBAL) {
            return;
        }
        if ((derived_flags & 0x1) == 0) {
            return;
        }
        renderer->popVertexProcessor();
        return;
    }
    applyWorldSpaceMatrix(*renderer);
    srMatrix4T<float> model_view;
    renderer->getMatrix(model_view);
    derived_flags = 0;
    derived_flags = 1;
    channel_mask = 0;
    if (ambient_198.x != 0.0f || ambient_198.y != 0.0f || ambient_198.z != 0.0f) {
        channel_mask |= 0x200;
        scaled_ambient.x = ambient_198.x * intensity_1d0;
        scaled_ambient.y = ambient_198.y * intensity_1d0;
        scaled_ambient.z = ambient_198.z * intensity_1d0;
        scaled_ambient.w = 0.0f;
    }
    if (diffuse_1a4.x != 0.0f || diffuse_1a4.y != 0.0f || diffuse_1a4.z != 0.0f) {
        channel_mask |= 0x400;
        scaled_diffuse.x = diffuse_1a4.x * intensity_1d0;
        scaled_diffuse.y = diffuse_1a4.y * intensity_1d0;
        scaled_diffuse.z = diffuse_1a4.z * intensity_1d0;
        scaled_diffuse.w = 0.0f;
    }
    if (specular_1b0.x != 0.0f || specular_1b0.y != 0.0f || specular_1b0.z != 0.0f) {
        channel_mask |= 0x4;
        scaled_specular.x = specular_1b0.x * intensity_1d0;
        scaled_specular.y = specular_1b0.y * intensity_1d0;
        scaled_specular.z = specular_1b0.z * intensity_1d0;
        scaled_specular.w = 0.0f;
    }
    if (channel_mask == 0) {
        derived_flags &= ~0x1;
    }
    if ((enable_flags & 0x2) != 0) {
        derived_flags |= 0x4;
        srVector3T<float> direction(-model_view.vectors[0].z, -model_view.vectors[1].z,
                                    -model_view.vectors[2].z);
        float length_squared = direction.LengthSquared();
        if (length_squared != 1.0f) {
            double scale = 1.0 / sqrt(length_squared);
            direction *= scale;
        }
        eye_location = direction;
    } else {
        if ((enable_flags & 0x1) != 0 && spot_angle > 0.0f && spot_exponent != 0.0f) {
            srMatrix4T<float> inverse;
            renderer->getInverseModelViewMatrix(inverse);
            derived_flags |= 0x2;
            spot_cutoff = (float)cos(spot_angle);
            spot_direction_eye.x = inverse.vectors[0].x * spot_direction.x +
                                       inverse.vectors[1].x * spot_direction.y +
                                       inverse.vectors[2].x * spot_direction.z;
            spot_direction_eye.y = inverse.vectors[0].y * spot_direction.x +
                                       inverse.vectors[1].y * spot_direction.y +
                                       inverse.vectors[2].y * spot_direction.z;
            spot_direction_eye.z = inverse.vectors[0].z * spot_direction.x +
                                       inverse.vectors[1].z * spot_direction.y +
                                       inverse.vectors[2].z * spot_direction.z;
            float length_squared = spot_direction_eye.LengthSquared();
            if (length_squared != 0.0f) {
                double scale = 1.0 / sqrt(length_squared);
                spot_direction_eye *= scale;
            }
        }
        if (attenuation_model != ATTENUATION_NONE) {
            if (attenuation_model == ATTENUATION_OPENGL) {
                derived_flags |= 0x8;
                if (fabsf(opengl_attenuation.y) < 5.9604645e-08f &&
                    fabsf(opengl_attenuation.z) < 5.9604645e-08f) {
                    if (opengl_attenuation.x == 1.0f ||
                        fabsf(opengl_attenuation.x) < 5.9604645e-08f) {
                        derived_flags &= ~0x8;
                    }
                    derived_flags |= 0x10;
                }
            } else {
                float scale = renderer->getMaxModelViewScale();
                scaled_near_start = scale * (float)near_start;
                scaled_far_end = scale * (float)far_end;
                if (near_start == near_end) {
                    near_attenuation = 1.0f;
                } else {
                    near_attenuation = (float)(1.0 / (scale * (near_end - near_start)));
                }
                if (far_start == far_end) {
                    far_attenuation = 1.0f;
                } else {
                    far_attenuation = (float)(1.0 / (scale * (far_end - far_start)));
                }
                if ((enable_flags & 0x10) != 0) {
                    if ((enable_flags & 0x4) != 0) {
                        srVector3T<float> origin(0.0f, 0.0f, 0.0f);
                        if (renderer->testBoundingSphere(origin, safe_range +
                                                                     (float)far_end) == 0) {
                            derived_flags &= ~0x1;
                        }
                    }
                    attenuation_range = scaled_far_end;
                    derived_flags |= 0x20;
                }
            }
        }
        eye_location.x = model_view.vectors[0].w;
        eye_location.y = model_view.vectors[1].w;
        eye_location.z = model_view.vectors[2].w;
    }
    renderer->popMatrix();
    if ((derived_flags & 0x1) != 0) {
        renderer->pushVertexProcessor(*this);
    }
}

// FUNCTION: SURRENDER 0x1004CCC0
int srLight::isActive(srVertexPipe& pipe)
{
    const srVertexPipe::Input* input = pipe.input_6c;
    if ((group_mask & input->exclusion_mask) != 0) {
        return 0;
    }
    if ((derived_flags & 0x20) != 0) {
        float dx = eye_location.x - input->eye_center.x;
        float dy = eye_location.y - input->eye_center.y;
        float dz = eye_location.z - input->eye_center.z;
        float range = attenuation_range + input->eye_radius;
        if (range * range <= dx * dx + dy * dy + dz * dz) {
            return 0;
        }
    }
    if ((derived_flags & 0x2) != 0) {
        float radius = input->eye_radius;
        float dx = input->eye_center.x - eye_location.x;
        float dy = input->eye_center.y - eye_location.y;
        float dz = input->eye_center.z - eye_location.z;
        float distance_squared = dx * dx + dy * dy + dz * dz;
        if (radius * radius < distance_squared &&
            (dx * spot_direction_eye.x + dy * spot_direction_eye.y +
             dz * spot_direction_eye.z) /
                        sqrtf(distance_squared) +
                    radius / sqrtf(radius * radius + distance_squared) <
                spot_cutoff) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x1004CE00
void srLight::process(srVertexPipe& pipe)
{
    unsigned long channels = channel_mask & pipe.channel_mask;
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
    srVertexPipe::Scratch* scratch = pipe.scratch_00;
    float* attenuation = 0;

    if ((derived_flags & 0x4) == 0) {
        srVectorProcessor::copy(
            directions, pipe.eye_space_locations + pipe.batch_base + pipe.sub_batch_offset,
            count);
        srVectorProcessor::sub(directions, eye_location, directions, count);
        srVectorProcessor::dir(directions, distances, directions, count);
        if (attenuation_model == ATTENUATION_3DSTUDIO_MAX) {
            if ((enable_flags & 0x8) != 0) {
                srVectorProcessor::add(attenuation_bank, -scaled_near_start, distances, count);
                srVectorProcessor::mul(attenuation_bank, near_attenuation, attenuation_bank,
                                       count);
                srVectorProcessor::clampUnit(attenuation_bank, attenuation_bank, count);
                attenuation = attenuation_bank;
            }
            if ((enable_flags & 0x10) != 0) {
                float* far_bank = attenuation != 0 ? spot_factors : attenuation_bank;
                srVectorProcessor::sub(far_bank, scaled_far_end, distances, count);
                srVectorProcessor::mul(far_bank, far_attenuation, far_bank, count);
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
        } else if (attenuation_model == ATTENUATION_OPENGL) {
            if ((derived_flags & 0x8) != 0) {
                if ((derived_flags & 0x10) != 0) {
                    float constant = 1.0f / opengl_attenuation.x;
                    // reinterpret-ok: retail pushes the float bit pattern into
                    // the SRDWORD fill.
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(attenuation_bank),
                                            reinterpret_cast<SRDWORD&>(constant), count);
                } else {
                    srVectorProcessor::invPoly(attenuation_bank, distances, opengl_attenuation,
                                               count);
                }
                srVectorProcessor::clampUnit(attenuation_bank, attenuation_bank, count);
                attenuation = attenuation_bank;
            }
        }
        if ((derived_flags & 0x2) != 0) {
            srVector3T<float> negated(-spot_direction_eye.x, -spot_direction_eye.y,
                                      -spot_direction_eye.z);
            srVectorProcessor::dot(spot_factors, negated, directions, count);
            srVectorProcessor::add(spot_factors, -spot_cutoff, spot_factors, count);
            srVectorProcessor::mul(spot_factors, 1.0f / (1.0f - spot_cutoff), spot_factors,
                                   count);
            srVectorProcessor::clampMin(spot_factors, spot_factors, 0.0f, count);
            if (srVectorProcessor::isZero(spot_factors, count)) {
                return;
            }
            if (spot_exponent != 1.0f) {
                srVectorProcessor::srSpecularPow(spot_factors, spot_factors, spot_exponent,
                                                 count);
            }
            if (attenuation != 0) {
                srVectorProcessor::mul(attenuation, attenuation, spot_factors, count);
            } else {
                attenuation = spot_factors;
            }
        }
        if (need_normals != 0) {
            if ((scratch->flags & 0x8) == 0) {
                pipe.setupEyeSpaceNormal();
            }
            srVectorProcessor::dot(dots, scratch->normals + pipe.sub_batch_offset,
                                   directions, count);
            srVectorProcessor::clampMin(dots, dots, 0.0f, count);
        }
    } else if (need_normals != 0) {
        if ((scratch->flags & 0x8) == 0) {
            pipe.setupEyeSpaceNormal();
        }
        srVectorProcessor::dot(dots, eye_location,
                               scratch->normals + pipe.sub_batch_offset, count);
        srVectorProcessor::clampUnit(dots, dots, count);
    }

    if ((channels & 0x200) != 0) {
        srVector4T<float> ambient;
        ambient.x = scaled_ambient.x * pipe.material_info.ambient.x;
        ambient.y = scaled_ambient.y * pipe.material_info.ambient.y;
        ambient.z = scaled_ambient.z * pipe.material_info.ambient.z;
        ambient.w = scaled_ambient.w * pipe.material_info.ambient.w;
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
        diffuse.x = scaled_diffuse.x * pipe.material_info.diffuse.x;
        diffuse.y = scaled_diffuse.y * pipe.material_info.diffuse.y;
        diffuse.z = scaled_diffuse.z * pipe.material_info.diffuse.z;
        diffuse.w = scaled_diffuse.w * pipe.material_info.diffuse.w;
        srCore.getStatisticsManager()->statistics_00.diffuse_operations += count;
        if ((pipe.lazy_setup_mask & 0x2) == 0) {
            pipe.setupDiffuse();
        }
        srVector4T<float>* out =
            pipe.vertex_array->diffuse_04 + pipe.batch_base + pipe.sub_batch_offset;
        if (attenuation != 0) {
            srVectorProcessor::axpy(out, out, diffuse, attenuation, dots, count);
        } else {
            srVectorProcessor::axpy(out, out, diffuse, dots, count);
        }
    }
    if ((channels & 0x4) == 0) {
        return;
    }
    if ((derived_flags & 0x4) != 0 && count != 0) {
        if (eye_location.x == eye_location.y && eye_location.x == eye_location.z) {
            if (count * 3 != 0) {
                // reinterpret-ok: the scalar broadcast fills the v3 array as
                // flat dwords.
                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(directions),
                                        *reinterpret_cast<SRDWORD*>(&eye_location.x),
                                        count * 3);
            }
        } else {
            srVectorProcessor::copy(directions, eye_location, count);
        }
    }
    if ((scratch->flags & 0x1) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    if (count * 3 != 0) {
        // reinterpret-ok: elementwise float subtraction across the v3 array
        // (view directions subtracted from the half vectors).
        srVectorProcessor::sub(
            reinterpret_cast<float*>(directions), reinterpret_cast<const float*>(directions),
            reinterpret_cast<const float*>(scratch->dir + pipe.sub_batch_offset), count * 3);
    }
    srVectorProcessor::normalize(directions, directions, 1.0f, count);
    if ((scratch->flags & 0x8) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    srVectorProcessor::dot(distances, scratch->normals + pipe.sub_batch_offset, directions,
                           count);
    srVectorProcessor::clampMin(distances, distances, 0.0f, count);
    if (pipe.material_info.shininess > 1.0f) {
        srVectorProcessor::srSpecularPow(distances, distances, pipe.material_info.shininess,
                                         count);
    }
    srVectorProcessor::mul(distances, distances, dots, count);
    srVector4T<float> specular;
    specular.x = scaled_specular.x * pipe.material_info.specular.x;
    specular.y = scaled_specular.y * pipe.material_info.specular.y;
    specular.z = scaled_specular.z * pipe.material_info.specular.z;
    specular.w = scaled_specular.w * pipe.material_info.specular.w;
    srCore.getStatisticsManager()->statistics_00.specular_operations += count;
    if ((pipe.lazy_setup_mask & 0x4) == 0) {
        pipe.setupSpecular();
    }
    srVector4T<float>* out =
        pipe.vertex_array->specular_08 + pipe.batch_base + pipe.sub_batch_offset;
    if (attenuation != 0) {
        srVectorProcessor::axpy(out, out, specular, attenuation, distances, count);
    } else {
        srVectorProcessor::axpy(out, out, specular, distances, count);
    }
}

// FUNCTION: SURRENDER 0x1004DC80
void srLight::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE)) {
        return;
    }
    if (testFlag(FLAG_DISABLE) || fabs(intensity_1d0) <= 0.0001) {
        if (first_child_ != 0) {
            first_child_->traverse(info);
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
    if (first_child_ != 0) {
        first_child_->traverse(info);
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
    opengl_attenuation.x = 1.0f;
    opengl_attenuation.z = 0.0f;
    opengl_attenuation.y = (1.0f - attenuation) / (range * attenuation);
}

// FUNCTION: SURRENDER 0x1004E630
void srLight::enable(e_enable flag)
{
    enable_flags |= 1 << flag;
}

// FUNCTION: SURRENDER 0x1004E610
void srLight::disable(e_enable flag)
{
    enable_flags &= ~(1 << flag);
}

// FUNCTION: SURRENDER 0x1004E650
int srLight::isEnabled(e_enable flag) const
{
    return (enable_flags & (1 << flag)) != 0;
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
    spot_direction = direction;
    float length_squared = spot_direction.LengthSquared();
    if (length_squared != 0.0f) {
        double scale = 1.0 / sqrt(length_squared);
        spot_direction *= scale;
    }
}

// FUNCTION: SURRENDER 0x1004E7D0
void srLight::setSpotAngle(float angle)
{
    if (angle <= 0.0f) {
        spot_angle = 0.0f;
    } else if (3.141592653589793 * 0.5 <= angle) {
        spot_angle = (float)(3.141592653589793 * 0.5);
    } else {
        spot_angle = angle;
    }
}

// FUNCTION: SURRENDER 0x1004E8A0
void srLight::setSpotExponent(float exponent)
{
    if (exponent <= 0.0f) {
        spot_exponent = 0.0f;
    } else if (exponent >= 128.0f) {
        spot_exponent = 128.0f;
    } else {
        spot_exponent = exponent;
    }
}

// FUNCTION: SURRENDER 0x1004E720
srVector3T<float> srLight::getSpotDirection() const
{
    return spot_direction;
}

// FUNCTION: SURRENDER 0x1004E710
float srLight::getSpotAngle() const
{
    return spot_angle;
}

// FUNCTION: SURRENDER 0x1004E750
float srLight::getSpotExponent() const
{
    return spot_exponent;
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
    attenuation_model = model;
}

// FUNCTION: SURRENDER 0x1004E910
srLight::e_attenuationModel srLight::getAttenuationModel() const
{
    return attenuation_model;
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
    opengl_attenuation.x = x;
    opengl_attenuation.y = y;
    opengl_attenuation.z = z;
}

// FUNCTION: SURRENDER 0x1004E9A0
srVector3T<float> srLight::getAttenuation() const
{
    return opengl_attenuation;
}

// FUNCTION: SURRENDER 0x1004EA60
void srLight::setNearAttenuationRange(double start, double end)
{
    near_start = start;
    near_end = end;
}

// FUNCTION: SURRENDER 0x1004EA00
void srLight::getNearAttenuationRange(double& start, double& end) const
{
    start = near_start;
    end = near_end;
}

// FUNCTION: SURRENDER 0x1004EA30
void srLight::setFarAttenuationRange(double start, double end)
{
    far_start = start;
    far_end = end;
}

// FUNCTION: SURRENDER 0x1004E9D0
void srLight::getFarAttenuationRange(double& start, double& end) const
{
    start = far_start;
    end = far_end;
}

// FUNCTION: SURRENDER 0x1004EA90
void srLight::setSafeRange(float range)
{
    safe_range = range;
}

// FUNCTION: SURRENDER 0x1004EAA0
float srLight::getSafeRange() const
{
    return safe_range;
}
