#include "surrender/srLight.h"

#include <string.h>

#include "surrender/srGERD.h"
#include "surrender/srVP.h"
#include "surrender/srVertexPipe.h"
#include "surrender/srVectorProcessor.h"

/* Per-class flag-name table: like srCamera's, retail leaves this global
   zero-initialized, so dump prints numeric bit indices. Retail references
   absolute 0x100A49D0, a slot inside the .bss extent already claimed by
   timer.cpp's storage_class; no GLOBAL marker, since the shared address
   cannot be claimed twice. */
static const char* flag_names;

// FUNCTION: SURRENDER 0x1004CCC0
int srLight::isActive(srVertexPipe& pipe)
{
    const srVertexPipe::Input* input = pipe.input_6c;
    if ((group_mask_13c & input->exclusion_mask_34) != 0) {
        return 0;
    }
    if ((activity_21c & 0x20) != 0) {
        float dx = eye_location_140.x - input->eye_center_18.x;
        float dy = eye_location_140.y - input->eye_center_18.y;
        float dz = eye_location_140.z - input->eye_center_18.z;
        float range = far_end_current_218 + input->eye_radius_24;
        if (range * range <= dx * dx + dy * dy + dz * dz) {
            return 0;
        }
    }
    if ((activity_21c & 2) != 0) {
        float radius = input->eye_radius_24;
        float dx = input->eye_center_18.x - eye_location_140.x;
        float dy = input->eye_center_18.y - eye_location_140.y;
        float dz = input->eye_center_18.z - eye_location_140.z;
        float dist2 = dx * dx + dy * dy + dz * dz;
        if ((radius * radius < dist2) &&
            ((dx * eye_spot_dir_208.x + dy * eye_spot_dir_208.y + dz * eye_spot_dir_208.z) /
                     sqrt(dist2) +
                 radius / sqrt(radius * radius + dist2) <
             spot_cos_214)) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x1004CE00
void srLight::process(srVertexPipe& pipe)
{
    srVP* vp = srVectorProcessor::vp;
    unsigned long mask = channel_bits_220 & pipe.channel_mask_0c;
    if (mask == 0) {
        return;
    }
    unsigned long diffuse_pending = mask & 0x400;
    int specular_pending = 0;
    if (diffuse_pending != 0 || (mask & 4) != 0) {
        specular_pending = 1;
    }
    float spot_term[0x100];
    float attenuation_term[0x100];
    float spec_term[0x100];
    float distances[0x100];
    srVector3T<float> directions[0x100];
    unsigned long count = pipe.vertex_count_88;
    const float* factors = 0;
    if ((activity_21c & 4) == 0) {
        vp->_copy(directions,
                  pipe.eye_space_locations_7c + pipe.batch_base_80 + pipe.sub_batch_offset_84,
                  count);
        vp->_sub(directions, eye_location_140, (const float*)directions, count);
        vp->_dir(directions, distances, directions, count);
        if (attenuation_model_150 == ATTENUATION_OPENGL) {
            if ((enable_flags_194 & 8) != 0) {
                if ((enable_flags_194 & 0x10) != 0) {
                    float inverse = 1.0f / opengl_attenuation_188.x;
                    // reinterpret-ok: retail pushes the computed float's bit pattern into
                    // the SRDWORD fill overload of _copy.
                    vp->_copy(reinterpret_cast<SRDWORD*>(attenuation_term),
                              *reinterpret_cast<SRDWORD*>(&inverse), count);
                } else {
                    vp->_invPoly(attenuation_term, distances, opengl_attenuation_188, count);
                }
                vp->_clampUnit(attenuation_term, attenuation_term, count);
                factors = attenuation_term;
            }
        } else if (attenuation_model_150 == ATTENUATION_3DSTUDIO_MAX) {
            if ((enable_flags_194 & 8) != 0) {
                vp->_add(attenuation_term, -near_start_scaled_178, distances, count);
                vp->_mul(attenuation_term, near_inverse_180, attenuation_term, count);
                vp->_clampUnit(attenuation_term, attenuation_term, count);
                factors = attenuation_term;
            }
            if ((enable_flags_194 & 0x10) != 0) {
                float* far_term = (factors != 0) ? spot_term : attenuation_term;
                vp->_sub(far_term, far_end_scaled_17c, distances, count);
                vp->_mul(far_term, far_inverse_184, far_term, count);
                vp->_clampUnit(far_term, far_term, count);
                if (far_term == spot_term) {
                    vp->_mul(attenuation_term, attenuation_term, spot_term, count);
                } else {
                    factors = far_term;
                }
            }
            if (factors != 0 && vp->_isZero(factors, count)) {
                return;
            }
        }
    } else if (specular_pending != 0) {
        // raw-offset-ok: VC6 keeps the scratch setup flags at scratch_00 + 0xb00,
        // outside any modeled field.
        if ((*(unsigned long*)((char*)pipe.scratch_00 + 0xb00) & 8) == 0) {
            pipe.setupEyeSpaceNormal();
        }
        vp->_dot(spec_term, eye_location_140,
                 reinterpret_cast<const srVector3T<float>*>(
                     static_cast<char*>(pipe.scratch_00) + (pipe.sub_batch_offset_84 + 0x40) * 0xc),
                 count);
        vp->_clampUnit(spec_term, spec_term, count);
    }
    if ((activity_21c & 2) != 0) {
        srVector3T<float> neg_dir;
        neg_dir.x = -eye_spot_dir_208.x;
        neg_dir.y = -eye_spot_dir_208.y;
        neg_dir.z = -eye_spot_dir_208.z;
        vp->_dot(spot_term, neg_dir, directions, count);
        vp->_add(spot_term, -spot_cos_214, spot_term, count);
        vp->_mul(spot_term, 1.0f / (1.0f - spot_cos_214), spot_term, count);
        vp->_clampMin(spot_term, spot_term, 0.0f, count);
        if (vp->_isZero(spot_term, count)) {
            return;
        }
        if (spot_exponent_1cc != 1.0f) {
            vp->_srSpecularPow(spot_term, spot_term, spot_exponent_1cc, count);
        }
        if (factors != 0) {
            vp->_mul((float*)factors, factors, spot_term, count);
        } else {
            factors = spot_term;
        }
    }
    if (specular_pending != 0 && (activity_21c & 4) == 0) {
        if ((*(unsigned long*)((char*)pipe.scratch_00 + 0xb00) & 8) == 0) {
            pipe.setupEyeSpaceNormal();
        }
        vp->_dot(spec_term,
                 reinterpret_cast<const srVector3T<float>*>(
                     static_cast<char*>(pipe.scratch_00) + (pipe.sub_batch_offset_84 + 0x40) * 0xc),
                 directions, count);
        vp->_clampMin(spec_term, spec_term, 0.0f, count);
    }
    if ((mask & 0x200) != 0) {
        srVector4T<float> ambient;
        ambient.x = eye_ambient_1d8.x * pipe.material_info_14.ambient.x;
        ambient.y = eye_ambient_1d8.y * pipe.material_info_14.ambient.y;
        ambient.z = eye_ambient_1d8.z * pipe.material_info_14.ambient.z;
        ambient.w = eye_ambient_1d8.w * pipe.material_info_14.ambient.w;
        if (factors != 0) {
            pipe.applyDiffuseLight(factors, ambient);
        } else {
            pipe.applyDiffuseLight(ambient);
        }
    }
    if (specular_pending == 0) {
        return;
    }
    const float* spec_weights = spec_term;
    if (vp->_isZero(spec_weights, count)) {
        return;
    }
    if (diffuse_pending != 0) {
        srVector4T<float> diffuse;
        diffuse.x = eye_diffuse_1e8.x * pipe.material_info_14.diffuse.x;
        diffuse.y = eye_diffuse_1e8.y * pipe.material_info_14.diffuse.y;
        diffuse.z = eye_diffuse_1e8.z * pipe.material_info_14.diffuse.z;
        diffuse.w = eye_diffuse_1e8.w * pipe.material_info_14.diffuse.w;
        srCore.getStatisticsManager()->statistics_00.diffuse_operations_24 += pipe.vertex_count_88;
        if ((pipe.lazy_setup_mask_10 & 2) == 0) {
            pipe.setupDiffuse();
        }
        srVector4T<float>* output =
            pipe.vertex_array_78->diffuse_04 + pipe.batch_base_80 + pipe.sub_batch_offset_84;
        if (factors != 0) {
            if (count != 0) {
                vp->_axpy(output, output, diffuse, spec_weights, factors, count);
            }
        } else if (count != 0) {
            vp->_axpy(output, output, diffuse, spec_weights, count);
        }
    }
    if ((mask & 4) == 0) {
        return;
    }
    if ((activity_21c & 4) != 0 && count != 0) {
        if (eye_location_140.x == eye_location_140.y && eye_location_140.x == eye_location_140.z) {
            // reinterpret-ok: retail fills count*3 SRDWORDs with the shared
            // component bit pattern through the SRDWORD _copy overload.
            vp->_copy(reinterpret_cast<SRDWORD*>(directions),
                      *reinterpret_cast<const SRDWORD*>(&eye_location_140.x), count * 3);
        } else {
            vp->_copy(directions, eye_location_140, count);
        }
    }
    if ((*(unsigned long*)((char*)pipe.scratch_00 + 0xb00) & 1) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    if (count * 3 != 0) {
        // raw-offset-ok: eye-space directions live in scratch_00 at
        // sub_batch_offset_84 vec3s — no modeled field covers them.
        vp->_sub(
            (float*)directions,
            reinterpret_cast<const float*>(reinterpret_cast<srVector3T<float>*>(pipe.scratch_00) +
                                           pipe.sub_batch_offset_84),
            (const float*)directions, count * 3);
    }
    vp->_normalize(directions, directions, 1.0f, count);
    if ((*(unsigned long*)((char*)pipe.scratch_00 + 0xb00) & 8) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    vp->_dot(spec_term,
             reinterpret_cast<const srVector3T<float>*>(static_cast<char*>(pipe.scratch_00) +
                                                        (pipe.sub_batch_offset_84 + 0x40) * 0xc),
             directions, count);
    vp->_clampMin(spec_term, spec_term, 0.0f, count);
    float shininess = pipe.material_info_14.shininess;
    if (shininess != 1.0f) {
        vp->_srSpecularPow(spec_term, spec_term, shininess, count);
    }
    vp->_mul(spec_term, spec_term, spec_weights, count);
    srVector4T<float> specular;
    specular.x = eye_specular_1f8.x * pipe.material_info_14.specular.x;
    specular.y = eye_specular_1f8.y * pipe.material_info_14.specular.y;
    specular.z = eye_specular_1f8.z * pipe.material_info_14.specular.z;
    specular.w = eye_specular_1f8.w * pipe.material_info_14.specular.w;
    srCore.getStatisticsManager()->statistics_00.specular_operations_28 += pipe.vertex_count_88;
    if ((pipe.lazy_setup_mask_10 & 4) == 0) {
        pipe.setupSpecular();
    }
    srVector4T<float>* output =
        pipe.vertex_array_78->specular_08 + pipe.batch_base_80 + pipe.sub_batch_offset_84;
    if (count == 0) {
        return;
    }
    if (factors != 0) {
        vp->_axpy(output, output, specular, spec_term, factors, count);
    } else {
        vp->_axpy(output, output, specular, spec_term, count);
    }
}

// FUNCTION: SURRENDER 0x1004D650
void srLight::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type != static_cast<e_processType>(1) && type != static_cast<e_processType>(3)) {
        if (type != static_cast<e_processType>(2) && type != static_cast<e_processType>(4)) {
            return;
        }
        if ((activity_21c & 1) == 0) {
            return;
        }
        renderer->popVertexProcessor();
        return;
    }
    applyWorldSpaceMatrix(*renderer);
    srMatrix4T<float> model_view;
    renderer->getMatrix(srGERD::MATRIX_MODELVIEW, model_view);
    activity_21c = 0;
    activity_21c = 1;
    channel_bits_220 = 0;
    if (ambient_198.x != 0.0f || ambient_198.y != 0.0f || ambient_198.z != 0.0f) {
        channel_bits_220 |= 0x200;
        eye_ambient_1d8.x = ambient_198.x * intensity_1d0;
        eye_ambient_1d8.y = ambient_198.y * intensity_1d0;
        eye_ambient_1d8.z = intensity_1d0 * ambient_198.z;
        eye_ambient_1d8.w = 0.0f;
    }
    if (diffuse_1a4.x != 0.0f || diffuse_1a4.y != 0.0f || diffuse_1a4.z != 0.0f) {
        channel_bits_220 |= 0x400;
        eye_diffuse_1e8.x = diffuse_1a4.x * intensity_1d0;
        eye_diffuse_1e8.y = diffuse_1a4.y * intensity_1d0;
        eye_diffuse_1e8.z = intensity_1d0 * diffuse_1a4.z;
        eye_diffuse_1e8.w = 0.0f;
    }
    if (specular_1b0.x != 0.0f || specular_1b0.y != 0.0f || specular_1b0.z != 0.0f) {
        channel_bits_220 |= 4;
        eye_specular_1f8.x = specular_1b0.x * intensity_1d0;
        eye_specular_1f8.y = specular_1b0.y * intensity_1d0;
        eye_specular_1f8.z = intensity_1d0 * specular_1b0.z;
        eye_specular_1f8.w = 0.0f;
    }
    if (channel_bits_220 == 0) {
        activity_21c &= ~1;
    }
    if ((enable_flags_194 & 2) != 0) {
        activity_21c |= 4;
        eye_location_140.x = -model_view.vectors[2].x;
        eye_location_140.y = -model_view.vectors[2].y;
        eye_location_140.z = -model_view.vectors[2].z;
        float len2 = eye_location_140.x * eye_location_140.x +
                     eye_location_140.y * eye_location_140.y +
                     eye_location_140.z * eye_location_140.z;
        if (len2 != 1.0f) {
            float inverse = 1.0f / sqrt(len2);
            eye_location_140.x = inverse * eye_location_140.x;
            eye_location_140.y = inverse * eye_location_140.y;
            eye_location_140.z = inverse * eye_location_140.z;
        }
    } else {
        if ((enable_flags_194 & 1) != 0 && 0.0f < spot_angle_1c8 && spot_exponent_1cc != 0.0f) {
            srMatrix4T<float> inverse_mv;
            renderer->getInverseModelViewMatrix(inverse_mv);
            activity_21c |= 2;
            spot_cos_214 = cos(spot_angle_1c8);
            float dx = spot_direction_1bc.x;
            float dy = spot_direction_1bc.y;
            float dz = spot_direction_1bc.z;
            eye_spot_dir_208.x = inverse_mv.vectors[0].x * dx + inverse_mv.vectors[1].x * dy +
                                 inverse_mv.vectors[2].x * dz;
            eye_spot_dir_208.y = inverse_mv.vectors[0].y * dx + inverse_mv.vectors[1].y * dy +
                                 inverse_mv.vectors[2].y * dz;
            eye_spot_dir_208.z = inverse_mv.vectors[0].z * dx + inverse_mv.vectors[1].z * dy +
                                 inverse_mv.vectors[2].z * dz;
            float len2 = eye_spot_dir_208.z * eye_spot_dir_208.z +
                         eye_spot_dir_208.y * eye_spot_dir_208.y +
                         eye_spot_dir_208.x * eye_spot_dir_208.x;
            if (len2 != 0.0f) {
                float inverse = 1.0f / sqrt(len2);
                eye_spot_dir_208.x = eye_spot_dir_208.x * inverse;
                eye_spot_dir_208.y = eye_spot_dir_208.y * inverse;
                eye_spot_dir_208.z = eye_spot_dir_208.z * inverse;
            }
        }
        if (attenuation_model_150 != ATTENUATION_NONE) {
            if (attenuation_model_150 == ATTENUATION_OPENGL) {
                activity_21c |= 8;
                if (fabs(opengl_attenuation_188.y) < 5.9604645e-08 &&
                    fabs(opengl_attenuation_188.z) < 5.9604645e-08) {
                    if (*(int*)&opengl_attenuation_188.x == 0x3f800000 ||
                        fabs(opengl_attenuation_188.x) < 5.9604645e-08) {
                        activity_21c &= ~8;
                    }
                    activity_21c |= 0x10;
                }
            } else {
                float scale = renderer->getMaxModelViewScale();
                near_start_scaled_178 = scale * (float)near_start_158;
                far_end_scaled_17c = scale * (float)far_end_170;
                if (near_start_158 == near_end_160) {
                    near_inverse_180 = 1.0f;
                } else {
                    near_inverse_180 = (float)(1.0 / (scale * (near_end_160 - near_start_158)));
                }
                if (far_start_168 == far_end_170) {
                    far_inverse_184 = 1.0f;
                } else {
                    far_inverse_184 = (float)(1.0 / (scale * (far_end_170 - far_start_168)));
                }
                if ((enable_flags_194 & 0x10) != 0) {
                    if ((enable_flags_194 & 4) != 0) {
                        srVector3T<float> center;
                        center.x = 0.0f;
                        center.y = 0.0f;
                        center.z = 0.0f;
                        if (renderer->testBoundingSphere(center,
                                                         safe_range_1d4 + (float)far_end_170) ==
                            static_cast<srGERD::e_visibility>(0)) {
                            activity_21c &= ~1;
                        }
                    }
                    far_end_current_218 = far_end_scaled_17c;
                    activity_21c |= 0x20;
                }
            }
        }
        eye_location_140.x = model_view.vectors[3].x;
        eye_location_140.y = model_view.vectors[3].y;
        eye_location_140.z = model_view.vectors[3].z;
    }
    renderer->popMatrix();
    if ((activity_21c & 1) != 0) {
        renderer->pushVertexProcessor(*this);
    }
}

// FUNCTION: SURRENDER 0x1004DC80
void srLight::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE) == 0) {
        if (testFlag(FLAG_DISABLE) != 0 || fabs(intensity_1d0) <= 0.0001) {
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
        } else {
            if (testFlag(FLAG_GLOBAL) == 0) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 1;
                info.entry_count++;
            } else {
                if (info.nodes.capacity <= info.node_count) {
                    info.nodes.setCapacity(info.nodes.capacity + 8 + info.node_count);
                }
                info.nodes.data[info.node_count] = this;
                info.node_count++;
            }
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
            if (testFlag(FLAG_GLOBAL) == 0) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 2;
                info.entry_count++;
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1004DDA0
srLight::srLight(srNode* parent, e_preset preset)
    : srClassSupport<srLight, srIlluminator, false, 0x1220>(static_cast<srNode*>(0))
{
    setFlag(FLAG_GLOBAL);
    activity_21c = 0;
    channel_bits_220 = 0;
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

// FUNCTION: SURRENDER 0x1004E0C0
void srLight::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Control flags: ";
    if (enable_flags_194 == 0) {
        stream << "_NONE_";
    } else {
        stream << '[';
        bool first = true;
        const char* names = flag_names;
        for (unsigned long bit = 0; bit < 0x20; bit++) {
            if ((enable_flags_194 & (1 << bit)) == 0) {
                if (names != 0) {
                    while (*names != 0 && *names != ',') {
                        names++;
                    }
                }
            } else {
                if (!first) {
                    stream << ',';
                }
                first = false;
                if (names == 0 || *names == 0) {
                    stream << bit;
                } else {
                    while (*names != ',' && *names != 0) {
                        stream << *names;
                        names++;
                    }
                }
            }
            if (names != 0 && *names == ',') {
                names++;
            }
        }
        stream << ']';
    }
    stream << '\n';
    stream.width(0x20);
    stream << "  Intensity: " << intensity_1d0 << '\n';
    stream.width(0x20);
    stream << "  Ambient coeff: {" << ambient_198.x << ',' << ambient_198.y << ',' << ambient_198.z
           << '}' << '\n';
    stream.width(0x20);
    stream << "  Diffuse coeff: {" << diffuse_1a4.x << ',' << diffuse_1a4.y << ',' << diffuse_1a4.z
           << '}' << '\n';
    stream.width(0x20);
    stream << "  Specular coeff: {" << specular_1b0.x << ',' << specular_1b0.y << ','
           << specular_1b0.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Spot direction {" << spot_direction_1bc.x << ',' << spot_direction_1bc.y << ','
           << spot_direction_1bc.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Spot angle " << spot_angle_1c8 << '\n';
    stream.width(0x20);
    stream << "  Spot exponent " << spot_exponent_1cc << '\n';
    stream.width(0x20);
    stream << "  Safe Range: " << safe_range_1d4 << '\n';
    stream.width(0x20);
    stream << "  Attenuation model: ";
    if (attenuation_model_150 == ATTENUATION_OPENGL) {
        stream << "OpenGL" << '\n';
        stream.width(0x20);
        stream << "  Attenuation fact. {" << opengl_attenuation_188.x << ','
               << opengl_attenuation_188.y << ',' << opengl_attenuation_188.z << '}' << '\n';
    } else if (attenuation_model_150 == ATTENUATION_3DSTUDIO_MAX) {
        stream << "3DStudio Max" << '\n';
        stream.width(0x20);
        stream << "  Near Range:  " << near_start_158 << ".." << near_end_160 << '\n';
        stream.width(0x20);
        stream << "  Far Range:   " << far_start_168 << ".." << far_end_170 << '\n';
    } else {
        stream << "No attenuation" << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004E610
void srLight::disable(e_enable option)
{
    enable_flags_194 &= ~(1 << (option & 0x1f));
}

// FUNCTION: SURRENDER 0x1004E630
void srLight::enable(e_enable option)
{
    enable_flags_194 |= 1 << (option & 0x1f);
}

// FUNCTION: SURRENDER 0x1004E650
int srLight::isEnabled(e_enable option) const
{
    return (enable_flags_194 & 1 << (option & 0x1f)) != 0;
}

// FUNCTION: SURRENDER 0x1004E670
srVector3T<float> srLight::getAmbient() const
{
    return ambient_198;
}

// FUNCTION: SURRENDER 0x1004E6A0
srVector3T<float> srLight::getDiffuse() const
{
    return diffuse_1a4;
}

// FUNCTION: SURRENDER 0x1004E6D0
float srLight::getIntensity() const
{
    return intensity_1d0;
}

// FUNCTION: SURRENDER 0x1004E6E0
srVector3T<float> srLight::getSpecular() const
{
    return specular_1b0;
}

// FUNCTION: SURRENDER 0x1004E710
float srLight::getSpotAngle() const
{
    return spot_angle_1c8;
}

// FUNCTION: SURRENDER 0x1004E720
srVector3T<float> srLight::getSpotDirection() const
{
    return spot_direction_1bc;
}

// FUNCTION: SURRENDER 0x1004E750
float srLight::getSpotExponent() const
{
    return spot_exponent_1cc;
}

// FUNCTION: SURRENDER 0x1004E760
void srLight::setAmbient(const srVector3T<float>& ambient)
{
    ambient_198 = ambient;
}

// FUNCTION: SURRENDER 0x1004E780
void srLight::setDiffuse(const srVector3T<float>& diffuse)
{
    diffuse_1a4 = diffuse;
}

// FUNCTION: SURRENDER 0x1004E7A0
void srLight::setIntensity(float intensity)
{
    intensity_1d0 = intensity;
}

// FUNCTION: SURRENDER 0x1004E7B0
void srLight::setSpecular(const srVector3T<float>& specular)
{
    specular_1b0 = specular;
}

// FUNCTION: SURRENDER 0x1004E7D0
void srLight::setSpotAngle(float angle)
{
    if (angle < 0.0f) {
        angle = 0.0f;
    }
    if (3.141592653589793 * 0.5 < angle) {
        angle = 3.141592653589793 * 0.5;
    }
    spot_angle_1c8 = angle;
}

// FUNCTION: SURRENDER 0x1004E830
void srLight::setSpotDirection(const srVector3T<float>& direction)
{
    spot_direction_1bc = direction;
    float len2 = spot_direction_1bc.x * spot_direction_1bc.x +
                 spot_direction_1bc.y * spot_direction_1bc.y +
                 spot_direction_1bc.z * spot_direction_1bc.z;
    if (len2 != 0.0f) {
        float inverse = 1.0f / sqrt(len2);
        spot_direction_1bc.x = spot_direction_1bc.x * inverse;
        spot_direction_1bc.y = spot_direction_1bc.y * inverse;
        spot_direction_1bc.z = spot_direction_1bc.z * inverse;
    }
}

// FUNCTION: SURRENDER 0x1004E8A0
void srLight::setSpotExponent(float exponent)
{
    if (exponent < 0.0f) {
        exponent = 0.0f;
    }
    if (128.0f < exponent) {
        exponent = 128.0f;
    }
    spot_exponent_1cc = exponent;
}

// FUNCTION: SURRENDER 0x1004E8F0
const char* srLight::sGetClassName()
{
    return "srLight";
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

// FUNCTION: SURRENDER 0x1004E9D0
void srLight::getFarAttenuationRange(double& start, double& end) const
{
    start = far_start_168;
    end = far_end_170;
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

// FUNCTION: SURRENDER 0x1004EA60
void srLight::setNearAttenuationRange(double start, double end)
{
    near_start_158 = start;
    near_end_160 = end;
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
    near_start_scaled_178 = other.near_start_scaled_178;
    far_end_scaled_17c = other.far_end_scaled_17c;
    near_inverse_180 = other.near_inverse_180;
    far_inverse_184 = other.far_inverse_184;
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
    eye_ambient_1d8 = other.eye_ambient_1d8;
    eye_diffuse_1e8 = other.eye_diffuse_1e8;
    eye_specular_1f8 = other.eye_specular_1f8;
    eye_spot_dir_208 = other.eye_spot_dir_208;
    spot_cos_214 = other.spot_cos_214;
    far_end_current_218 = other.far_end_current_218;
    activity_21c = other.activity_21c;
    channel_bits_220 = other.channel_bits_220;
}

// SYNTHETIC: SURRENDER 0x1004EEF0
// srLight default_constructor_closure
