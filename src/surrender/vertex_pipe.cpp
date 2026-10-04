#include "surrender/srVertexPipe.h"

#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srMaterial.h"
#include "surrender/srMaterialIFace.h"
#include "surrender/srPalette.h"
#include "surrender/srShader.h"
#include "surrender/srVectorProcessor.h"

// FUNCTION: SURRENDER 0x1002AA50
srFlags<srVertexProcessor::e_channel> srVertexPipe::getShaderDisableMask(const srShader& shader)
{
    unsigned long disable = 0;
    if ((shader.value & 0x300) == 0) {
        disable = 1 << srVertexProcessor::CHANNEL_FOG;
    }
    if ((shader.value & 0xc00) == 0) {
        disable |=
            (1 << srVertexProcessor::CHANNEL_DIFFUSE) | (1 << srVertexProcessor::CHANNEL_ALPHA);
    } else if ((shader.value & srShader::MASK_ALPHATEST) == 0 &&
               ((shader.value >> srShader::DSTBLEND_SHIFT) & 7) != srShader::DSTBLEND_SRC_ALPHA &&
               ((shader.value >> srShader::DSTBLEND_SHIFT) & 7) !=
                   srShader::DSTBLEND_ONE_MINUS_SRC_ALPHA &&
               ((shader.value >> srShader::SRCBLEND_SHIFT) & 3) != srShader::SRCBLEND_SRC_ALPHA &&
               ((shader.value >> srShader::SRCBLEND_SHIFT) & 3) !=
                   srShader::SRCBLEND_ONE_MINUS_SRC_ALPHA) {
        disable |= 1 << srVertexProcessor::CHANNEL_ALPHA;
    }
    if ((shader.value & srShader::MASK_SECONDARY_GRADIENT) == 0) {
        disable |= 1 << srVertexProcessor::CHANNEL_SPECULAR;
    }
    if ((shader.value & srShader::MASK_TEXTURING) == 0) {
        return srFlags<srVertexProcessor::e_channel>(
            disable | (1 << srVertexProcessor::CHANNEL_ST0) |
            (1 << srVertexProcessor::CHANNEL_ST1) | (1 << srVertexProcessor::CHANNEL_Q0) |
            (1 << srVertexProcessor::CHANNEL_Q1));
    }
    if ((shader.value & 0xfe7f0000) == 0) {
        disable |= (1 << srVertexProcessor::CHANNEL_ST1) | (1 << srVertexProcessor::CHANNEL_Q1);
    }
    return srFlags<srVertexProcessor::e_channel>(disable);
}

// FUNCTION: SURRENDER 0x1002AAD0
srFlags<srVertexProcessor::e_channel>
srVertexPipe::getShaderDisableMask(const srShader* shaders, const unsigned long* channels,
                                   unsigned long channel_count)
{
    srShader shader;
    shader.value = shaders[channels[0]].value;
    unsigned long available = ~getShaderDisableMask(shader).value;
    unsigned long index = 0;
    if (channel_count != 0) {
        while (true) {
            index +=
                scanChangeIndexed(
                    (const unsigned long*)
                        shaders /* c-style-cast-ok: srShader is one packed dword; scanChangeIndexed walks it as a dword table */
                    ,
                    shader.value, channels + 1 + index, channel_count - index - 1) +
                1;
            if (channel_count <= index) {
                break;
            }
            shader.value = shaders[channels[index]].value;
            available |= ~getShaderDisableMask(shader).value;
            if (available == 0xffffffff) {
                return srFlags<srVertexProcessor::e_channel>(0);
            }
        }
    }
    return srFlags<srVertexProcessor::e_channel>(~available);
}

// FUNCTION: SURRENDER 0x1002AB80
unsigned long srVertexPipe::scanChangeIndexed(const unsigned long* table, unsigned long value,
                                              const unsigned long* indices,
                                              unsigned long index_count)
{
    for (unsigned long index = 0; index < index_count; ++index) {
        if (table[indices[index]] != value) {
            return index;
        }
    }
    return index_count;
}

// FUNCTION: SURRENDER 0x1002AC10
srVertexPipe::srVertexPipe()
    : processor_heap(0), processor_heap_capacity(0), channel_mask_0c(0),
      lazy_setup_mask(0), material_info()
{
    scratch_00 = static_cast<Scratch*>(::operator new(sizeof(Scratch)));
    if (scratch_00 != 0) {
        scratch_00->flags_b00 = 0;
    }
    channel_mask_0c = 0;
    lazy_setup_mask = 0;
    material_68 = 0;
    input_6c = 0;
    avt_70 = 0;
    current_record_74 = 0;
    vertex_array_78 = 0;
    eye_space_locations = 0;
    vertex_count_88 = 0;
    batch_count_8c = 0;
    active_processor_count = 0;
    active_processors = 0;
    vector_processor_98 = 0;
    batch_base = 0xffffffff;
    sub_batch_offset_84 = 0xffffffff;
}

// FUNCTION: SURRENDER 0x1002ACC0
srVertexPipe::~srVertexPipe()
{
    ::operator delete(scratch_00);
    if (processor_heap != 0) {
        srHeap.free(processor_heap);
    }
    processor_heap = 0;
    processor_heap_capacity = 0;
}

// FUNCTION: SURRENDER 0x1002ACF0
void srVertexPipe::processVertexBuffer()
{
    lazy_setup_mask = 0;
    unsigned long channels = ~current_record_74->channels_04 & ~material_info.flags;
    channel_mask_0c = channels;
    if ((channels & (1 << srVertexProcessor::CHANNEL_DIFFUSE)) == 0) {
        channel_mask_0c = channels & 0xfffffbff;
        channel_mask_0c = channels & 0xfffff9ff;
    }
    if ((channel_mask_0c & 0x600) == 0) {
        channel_mask_0c &= ~2u;
    }
    material_68->preProcess(*this);
    for (unsigned long index = 0; index < active_processor_count; ++index) {
        active_processors[index]->process(*this);
    }
    material_68->postProcess(*this);
    channel_mask_0c = ~current_record_74->channels_04;
    lazy_setup_mask &= channel_mask_0c;
    finishDiffuseAlpha();
    finishSpecularFog();
    unsigned long mask = channel_mask_0c;
    if ((mask & (1 << srVertexProcessor::CHANNEL_ST0)) != 0) {
        if ((lazy_setup_mask & (1 << srVertexProcessor::CHANNEL_Q0)) == 0) {
            mask &= 0xffffff7f;
        }
        srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations +=
            vertex_count_88;
        if ((lazy_setup_mask & (1 << srVertexProcessor::CHANNEL_ST0)) == 0) {
            setupST(0);
        }
        unsigned long lazy = lazy_setup_mask | (1 << srVertexProcessor::CHANNEL_ST0);
        lazy_setup_mask = lazy;
        if ((lazy & (1 << srVertexProcessor::CHANNEL_Q0)) == 0) {
            setupQ(0);
        }
        lazy_setup_mask |= 1 << srVertexProcessor::CHANNEL_Q0;
    }
    if ((channel_mask_0c & (1 << srVertexProcessor::CHANNEL_ST1)) != 0) {
        if ((lazy_setup_mask & (1 << srVertexProcessor::CHANNEL_Q1)) == 0) {
            mask &= 0xfffffeff;
        }
        srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations +=
            vertex_count_88;
        if ((lazy_setup_mask & (1 << srVertexProcessor::CHANNEL_ST1)) == 0) {
            setupST(1);
        }
        unsigned long lazy = lazy_setup_mask | (1 << srVertexProcessor::CHANNEL_ST1);
        lazy_setup_mask = lazy;
        if ((lazy & (1 << srVertexProcessor::CHANNEL_Q1)) == 0) {
            setupQ(1);
        }
        lazy_setup_mask |= 1 << srVertexProcessor::CHANNEL_Q1;
    }
    unsigned long packed = (mask >> 2) & 0xffffff7c;
    packed |= ((mask >> 1) & 2) | ((mask & ((1 << srVertexProcessor::CHANNEL_DIFFUSE) |
                                            (1 << srVertexProcessor::CHANNEL_ALPHA))) != 0
                                       ? 1
                                       : 0);
    vector_processor_98->_memcopy(vertex_array_78->packed_1c + batch_base + sub_batch_offset_84,
                                  packed, vertex_count_88);
}

// FUNCTION: SURRENDER 0x1002AE90
void srVertexPipe::setMaterial(srMaterialIFace* material)
{
    if (material == 0) {
        material = srCore.getMaterial();
    }
    if (material != material_68) {
        srCore.getStatisticsManager()->statistics_00.material_processing_stalls += 1;
        material_68 = material;
        material->getMaterialInfo(material_info);
    }
}

// FUNCTION: SURRENDER 0x1002AEC0
void srVertexPipe::process(const Input& input)
{
    input_6c = &input;
    active_processor_count = 0;
    active_processors = 0;
    vector_processor_98 = srVectorProcessor::vp;
    unsigned long processor_count = input.processor_count_50;
    if (processor_count != 0) {
        if (processor_heap_capacity < processor_count) {
            if (processor_heap != 0) {
                srHeap.free(processor_heap);
            }
            processor_heap = 0;
            processor_heap_capacity = 0;
            long capacity = 0;
            if (processor_count != 0) {
                capacity = static_cast<long>(processor_count * 1.1);
            }
            processor_heap_capacity = capacity;
            if (capacity != 0) {
                processor_heap = static_cast<srVertexProcessor**>(srHeap.allocate(capacity * 4));
            }
        }
        active_processors = processor_heap;
        for (unsigned long index = 0; index < input.processor_count_50; ++index) {
            srVertexProcessor* processor = input.processors_4c[index];
            if (processor->isActive(*this)) {
                active_processors[active_processor_count] = processor;
                active_processor_count += 1;
            }
        }
    }
    setMaterial(0);
    eye_space_locations = input.vertex_arrays_30->eye_locations;
    batch_base = 0;
    unsigned long vertex_count = input.vertex_count_04;
    while (vertex_count != 0) {
        vertex_count -= batch_base;
        batch_count_8c = vertex_count;
        if (0x40 < vertex_count) {
            batch_count_8c = 0x40;
        }
        avt_70 = input.indices_08 + batch_base;
        scratch_00->flags_b00 = 0;
        if (input_6c->position_is_float3_0c == 0) {
            vector_processor_98->_transformIndexed(eye_space_locations + batch_base,
                                                   input_6c->positions_10, avt_70,
                                                   *input_6c->model_view_28, batch_count_8c);
        } else {
            vector_processor_98->_transform(eye_space_locations + batch_base,
                                            input_6c->positions_10 + batch_base,
                                            *input_6c->model_view_28, batch_count_8c);
        }
        unsigned long record_index;
        for (record_index = 0; record_index < input.record_count_00; ++record_index) {
            const Record* record = reinterpret_cast<const Record*>(
                static_cast<const char*>(input.records_48) + record_index * 0x5c);
            current_record_74 = record;
            vertex_array_78 = input.vertex_arrays_30 + record_index;
            if ((record->flags_00 & 0x40) == 0) {
                vertex_count_88 = batch_count_8c;
                sub_batch_offset_84 = 0;
                setMaterial(record->material_08);
                processVertexBuffer();
            } else {
                srMaterialIFace* material = record->materials_28[avt_70[0]];
                setMaterial(material);
                sub_batch_offset_84 = 0;
                while (sub_batch_offset_84 < batch_count_8c) {
                    srMaterialIFace* next = record->materials_28[avt_70[sub_batch_offset_84]];
                    if (next != material) {
                        setMaterial(next);
                        material = next;
                    }
                    vertex_count_88 =
                        scanChangeIndexed(
                            (const unsigned long*)record
                                ->materials_28 /* c-style-cast-ok: retail scans the material table as a dword table through scanChangeIndexed */
                            ,
                            (unsigned long)
                                material /* c-style-cast-ok: scanChangeIndexed compares the material pointer as a dword value */
                            ,
                            avt_70 + 1 + sub_batch_offset_84,
                            batch_count_8c - sub_batch_offset_84 - 1) +
                        1;
                    processVertexBuffer();
                    sub_batch_offset_84 += vertex_count_88;
                }
            }
        }
        for (record_index = 1; record_index < input.record_count_00; ++record_index) {
            srVector4T<float>* destination =
                input.vertex_arrays_30[record_index].eye_locations + batch_base;
            srVector4T<float>* source = eye_space_locations + batch_base;
            if (((batch_count_8c * 0x10) != 0) && (destination != source)) {
                srVectorProcessor::memcopy(destination, source, batch_count_8c << 4);
            }
        }
        batch_base += 0x40;
        vertex_count = input.vertex_count_04;
        if (batch_base >= vertex_count) {
            break;
        }
    }
}

// FUNCTION: SURRENDER 0x1002B200
void srVertexPipe::finishDiffuseAlpha()
{
    if ((channel_mask_0c & ((1 << srVertexProcessor::CHANNEL_DIFFUSE) |
                            (1 << srVertexProcessor::CHANNEL_ALPHA))) == 0) {
        return;
    }
    srVector4T<float>* diffuse = vertex_array_78->diffuse_04 + batch_base + sub_batch_offset_84;
    if (((lazy_setup_mask & 0xa) == 0) && ((current_record_74->flags_00 & 9) == 0)) {
        srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
        srVector4T<float> color;
        color.x =
            input_6c->ambient_light_38.x * material_info.ambient.x + material_info.emissive.x;
        color.y =
            input_6c->ambient_light_38.y * material_info.ambient.y + material_info.emissive.y;
        color.z =
            input_6c->ambient_light_38.z * material_info.ambient.z + material_info.emissive.z;
        color.w = material_info.diffuse.w;
        if (color.x <= 0.0f) {
            color.x = 0.0f;
        } else if (color.x >= 1.0f) {
            color.x = 1.0f;
        }
        if (color.y <= 0.0f) {
            color.y = 0.0f;
        } else if (color.y >= 1.0f) {
            color.y = 1.0f;
        }
        if (color.z <= 0.0f) {
            color.z = 0.0f;
        } else if (color.z >= 1.0f) {
            color.z = 1.0f;
        }
        if (color.w <= 0.0f) {
            color.w = 0.0f;
        } else if (color.w >= 1.0f) {
            color.w = 1.0f;
        }
        if ((current_record_74->flags_00 & 2) != 0) {
            vector_processor_98->_mulIndexed(diffuse, color, current_record_74->specular_source_14,
                                             avt_70 + sub_batch_offset_84, vertex_count_88);
            return;
        }
        if (vertex_count_88 != 0) {
            srVectorProcessor::copy(diffuse, color, vertex_count_88);
        }
        return;
    }
    if ((channel_mask_0c & (1 << srVertexProcessor::CHANNEL_DIFFUSE)) != 0) {
        srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
        if ((lazy_setup_mask & 2) == 0) {
            setupDiffuse();
        }
    }
    float* alpha;
    if ((channel_mask_0c & (1 << srVertexProcessor::CHANNEL_ALPHA)) != 0) {
        srCore.getStatisticsManager()->statistics_00.alpha_operations += vertex_count_88;
        if ((current_record_74->flags_00 & 8) != 0) {
            alpha = scratch_00->alpha_900 + sub_batch_offset_84;
            if ((lazy_setup_mask & 8) == 0) {
                if (vertex_count_88 != 0) {
                    srVectorProcessor::copyIndexed(
                        reinterpret_cast<SRDWORD*>(alpha),
                        reinterpret_cast<const SRDWORD*>(current_record_74->alpha_source_1c),
                        avt_70 + sub_batch_offset_84, vertex_count_88);
                }
                lazy_setup_mask |= 8;
            } else {
                vector_processor_98->_mulIndexed(alpha, alpha, current_record_74->alpha_source_1c,
                                                 avt_70 + sub_batch_offset_84, vertex_count_88);
            }
        }
        if ((lazy_setup_mask & 8) == 0) {
            float opacity = material_info.diffuse.w;
            if (opacity <= 0.0f) {
                opacity = 0.0f;
            } else if (opacity >= 1.0f) {
                opacity = 1.0f;
            }
            vector_processor_98->_copyW(diffuse, opacity, vertex_count_88);
        } else {
            srCore.getStatisticsManager()->statistics_00.alpha_operations += vertex_count_88;
            if ((lazy_setup_mask & 8) == 0) {
                setupAlpha();
            }
            alpha = scratch_00->alpha_900 + sub_batch_offset_84;
            if ((channel_mask_0c & 2) == 0) {
                vector_processor_98->_clampUnit(alpha, alpha, vertex_count_88);
            }
            float opacity = material_info.diffuse.w;
            if ((vertex_count_88 != 0) && (opacity != 1.0f)) {
                if (opacity == 0.0f) {
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(alpha), 0, vertex_count_88);
                } else {
                    srVectorProcessor::mul(alpha, opacity, alpha, vertex_count_88);
                }
            }
            vector_processor_98->_copyW(diffuse, alpha, vertex_count_88);
        }
    }
    if (((channel_mask_0c & 2) != 0) && (vertex_count_88 * 4 != 0)) {
        srVectorProcessor::clampUnit(reinterpret_cast<float*>(diffuse),
                                     reinterpret_cast<const float*>(diffuse), vertex_count_88 * 4);
    }
    if (((current_record_74->flags_00 & 2) != 0) && (vertex_count_88 != 0)) {
        srVectorProcessor::mulIndexed(diffuse, diffuse, current_record_74->specular_source_14,
                                      avt_70 + sub_batch_offset_84, vertex_count_88);
    }
}

// FUNCTION: SURRENDER 0x1002B5F0
void srVertexPipe::finishSpecularFog()
{
    unsigned long specular = channel_mask_0c & (1 << srVertexProcessor::CHANNEL_SPECULAR);
    if ((specular == 0) && ((channel_mask_0c & (1 << srVertexProcessor::CHANNEL_FOG)) == 0)) {
        return;
    }
    srVector4T<float>* destination =
        vertex_array_78->specular_08 + batch_base + sub_batch_offset_84;
    if ((lazy_setup_mask & 0x14) == 0) {
        srCore.getStatisticsManager()->statistics_00.specular_operations += vertex_count_88;
        unsigned long dword_count = vertex_count_88 * 4;
        if (dword_count != 0) {
            srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(destination), 0, dword_count);
        }
        return;
    }
    if (specular == 0) {
        if ((lazy_setup_mask & 0x10) == 0) {
            vector_processor_98->_copyW(destination, 0.0f, vertex_count_88);
        } else {
            srCore.getStatisticsManager()->statistics_00.fog_operations += vertex_count_88;
            if ((lazy_setup_mask & 0x10) == 0) {
                setupFog();
            }
            float* fog = scratch_00->fog_a00 + sub_batch_offset_84;
            vector_processor_98->_clampUnit(fog, fog, vertex_count_88);
            float scale = material_info.value_4c;
            if ((vertex_count_88 != 0) && (scale != 1.0f)) {
                if (scale == 0.0f) {
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(fog), 0, vertex_count_88);
                } else {
                    srVectorProcessor::mul(fog, scale, fog, vertex_count_88);
                }
            }
            vector_processor_98->_copyW(destination, fog, vertex_count_88);
        }
    } else {
        srCore.getStatisticsManager()->statistics_00.specular_operations += vertex_count_88;
        if ((lazy_setup_mask & 4) == 0) {
            setupSpecular();
        }
        if ((channel_mask_0c & (1 << srVertexProcessor::CHANNEL_FOG)) != 0) {
            srCore.getStatisticsManager()->statistics_00.fog_operations += vertex_count_88;
            if ((lazy_setup_mask & 0x10) == 0) {
                setupFog();
            }
            float* fog = scratch_00->fog_a00 + sub_batch_offset_84;
            float scale = material_info.value_4c;
            if ((vertex_count_88 != 0) && (scale != 1.0f)) {
                if (scale == 0.0f) {
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(fog), 0, vertex_count_88);
                } else {
                    srVectorProcessor::mul(fog, scale, fog, vertex_count_88);
                }
            }
            vector_processor_98->_copyW(destination, fog, vertex_count_88);
        }
        unsigned long dword_count = vertex_count_88 * 4;
        if (dword_count != 0) {
            srVectorProcessor::clampUnit(reinterpret_cast<float*>(destination),
                                         reinterpret_cast<const float*>(destination), dword_count);
        }
    }
    if (((current_record_74->flags_00 & 4) != 0) && (vertex_count_88 != 0)) {
        srVectorProcessor::mulIndexed(destination, destination,
                                      current_record_74->specular_source_18,
                                      avt_70 + sub_batch_offset_84, vertex_count_88);
    }
}

// FUNCTION: SURRENDER 0x1002B860
void srVertexPipe::setupEyeSpaceNormal()
{
    const srVector3T<float>* normals = input_6c->values_14;
    Scratch* scratch = scratch_00;
    if (normals == 0) {
        srVector3T<float> constant;
        constant.x = 0.0f;
        constant.y = 0.0f;
        constant.z = -1.0f;
        vector_processor_98->_copy(scratch->normals_300, constant, batch_count_8c);
    } else if (input_6c->position_is_float3_0c == 0) {
        vector_processor_98->_transformIndexed(scratch->normals_300, normals, avt_70,
                                               *input_6c->normal_matrix_2c, batch_count_8c);
    } else {
        vector_processor_98->_transform(scratch->normals_300, normals + batch_base,
                                        *input_6c->normal_matrix_2c, batch_count_8c);
    }
    scratch->flags_b00 |= 8;
}

// FUNCTION: SURRENDER 0x1002B910
void srVertexPipe::setupEyeSpaceDirAndDist()
{
    Scratch* scratch = scratch_00;
    vector_processor_98->_dir(scratch->dir_000, scratch->dist_600,
                              eye_space_locations + batch_base, batch_count_8c);
    scratch->flags_b00 |= 1;
    scratch->flags_b00 |= 2;
}

// FUNCTION: SURRENDER 0x1002B970
void srVertexPipe::setupEyeSpaceZDist()
{
    Scratch* scratch = scratch_00;
    const srVector4T<float>* locations = eye_space_locations + batch_base;
    for (unsigned long index = 0; index < batch_count_8c; ++index) {
        scratch->z_dist[index] = locations[index].z;
    }
    scratch->flags_b00 |= 4;
}

// FUNCTION: SURRENDER 0x1002BA20
void srVertexPipe::setupAlpha()
{
    if ((lazy_setup_mask & 8) == 0) {
        srVectorProcessor::copy(
            // reinterpret-ok: the VP dword fill writes IEEE-754 1.0f into the alpha array.
            reinterpret_cast<SRDWORD*>(scratch_00->alpha_900 + sub_batch_offset_84), 0x3f800000,
            vertex_count_88);
        lazy_setup_mask |= 8;
    }
}

// FUNCTION: SURRENDER 0x1002BA70
void srVertexPipe::setupFog()
{
    if ((lazy_setup_mask & 0x10) == 0) {
        if (vertex_count_88 != 0) {
            srVectorProcessor::copy(
                // reinterpret-ok: the VP dword fill writes IEEE-754 zero into the fog array.
                reinterpret_cast<SRDWORD*>(scratch_00->fog_a00 + sub_batch_offset_84), 0,
                vertex_count_88);
        }
        lazy_setup_mask |= 0x10;
    }
}

// FUNCTION: SURRENDER 0x1002BAB0
void srVertexPipe::applyFog(const float* values)
{
    srCore.getStatisticsManager()->statistics_00.fog_operations += vertex_count_88;
    float* fog = scratch_00->fog_a00 + sub_batch_offset_84;
    if ((lazy_setup_mask & 0x10) != 0) {
        float one_minus[0x40];
        vector_processor_98->_sub(one_minus, 1.0f, const_cast<float*>(values), vertex_count_88);
        vector_processor_98->_axpy(fog, const_cast<float*>(values), fog, one_minus,
                                   vertex_count_88);
        return;
    }
    if ((vertex_count_88 != 0) && (fog != values)) {
        srVectorProcessor::memcopy(fog, values, vertex_count_88 * 4);
    }
    lazy_setup_mask |= 0x10;
}

/* Retail copies the two owned allocations at +0x00/+0x04 verbatim. Assigning
   two live pipes aliases their storage and leaves the old destination storage
   behind; the function has no retail ownership repair. */

// FUNCTION: SURRENDER 0x1002BCC0
void srVertexPipe::applyDiffuseLight(const float* values, const srVector4T<float>& light)
{
    srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
    if ((lazy_setup_mask & 2) == 0) {
        setupDiffuse();
    }
    srVector4T<float>* diffuse = vertex_array_78->diffuse_04 + batch_base + sub_batch_offset_84;
    if (vertex_count_88 != 0) {
        srVectorProcessor::axpy(diffuse, diffuse, light, values, vertex_count_88);
    }
}

// FUNCTION: SURRENDER 0x1002BD30
void srVertexPipe::copySpecularToDiffuse()
{
    srCore.getStatisticsManager()->statistics_00.specular_operations += vertex_count_88;
    if ((lazy_setup_mask & 4) == 0) {
        setupSpecular();
    }
    unsigned long offset = batch_base + sub_batch_offset_84;
    srVector4T<float>* specular = vertex_array_78->specular_08 + offset;
    srVector4T<float>* diffuse = vertex_array_78->diffuse_04 + offset;
    if (((vertex_count_88 * 4) != 0) && (diffuse != specular)) {
        srVectorProcessor::memcopy(diffuse, specular, vertex_count_88 << 4);
        lazy_setup_mask |= 2;
        return;
    }
    lazy_setup_mask |= 2;
}

// FUNCTION: SURRENDER 0x1002BDB0
void srVertexPipe::copyDiffuseToSpecular()
{
    srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
    if ((lazy_setup_mask & 2) == 0) {
        setupDiffuse();
    }
    unsigned long offset = batch_base + sub_batch_offset_84;
    srVector4T<float>* diffuse = vertex_array_78->diffuse_04 + offset;
    srVector4T<float>* specular = vertex_array_78->specular_08 + offset;
    if (((vertex_count_88 * 4) != 0) && (specular != diffuse)) {
        srVectorProcessor::memcopy(specular, diffuse, vertex_count_88 << 4);
        lazy_setup_mask |= 4;
        return;
    }
    lazy_setup_mask |= 4;
}

// FUNCTION: SURRENDER 0x1002BE30
void srVertexPipe::swapDiffuseAndSpecular()
{
    srCore.getStatisticsManager()->statistics_00.specular_operations += vertex_count_88;
    if ((lazy_setup_mask & 4) == 0) {
        setupSpecular();
    }
    srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
    if ((lazy_setup_mask & 2) == 0) {
        setupDiffuse();
    }
    unsigned long offset = batch_base + sub_batch_offset_84;
    srVectorProcessor::swap(vertex_array_78->diffuse_04 + offset,
                            vertex_array_78->specular_08 + offset, vertex_count_88 << 4);
    lazy_setup_mask |= 4;
    lazy_setup_mask |= 6;
}

// FUNCTION: SURRENDER 0x1002BEC0
void srVertexPipe::applyDiffuseLight(const srVector4T<float>& light)
{
    srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
    if ((lazy_setup_mask & 2) == 0) {
        setupDiffuse();
    }
    srVector4T<float>* diffuse = vertex_array_78->diffuse_04 + batch_base + sub_batch_offset_84;
    vector_processor_98->_add(diffuse, light, diffuse, vertex_count_88);
}

/* The indexed diffuse-source helper setupDiffuse reaches through the record's
   colors/kind pair: kind 0 selects ARGB, 1 vector3 and 2 vector4 copyIndexed. */
// FUNCTION: SURRENDER 0x1002BF20
void srVertexPipe::Record::ColorSource::copyDiffuseColors(srVector4T<float>* destination,
                                                          const unsigned long* indices,
                                                          unsigned long count) const
{
    if (colors_00 == 0) {
        if (count * 4 != 0) {
            srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(destination), 0, count * 4);
        }
        return;
    }
    if (kind_04 == 0) {
        srVectorProcessor::copyIndexed(destination, static_cast<const srARGB*>(colors_00), indices,
                                       count);
        return;
    }
    if (kind_04 == 1) {
        srVectorProcessor::copyIndexed(destination, static_cast<const srVector3*>(colors_00),
                                       indices, count);
        return;
    }
    if (kind_04 == 2) {
        srVectorProcessor::copyIndexed(destination, static_cast<const srVector4*>(colors_00),
                                       indices, count);
    }
}

// FUNCTION: SURRENDER 0x1002BFB0
void srVertexPipe::setupDiffuse()
{
    if ((lazy_setup_mask & 2) == 0) {
        srVector4T<float> color;
        color.x =
            input_6c->ambient_light_38.x * material_info.ambient.x + material_info.emissive.x;
        color.y =
            input_6c->ambient_light_38.y * material_info.ambient.y + material_info.emissive.y;
        color.z =
            input_6c->ambient_light_38.z * material_info.ambient.z + material_info.emissive.z;
        color.w =
            input_6c->ambient_light_38.w * material_info.ambient.w + material_info.emissive.w;
        srVector4T<float>* diffuse =
            vertex_array_78->diffuse_04 + batch_base + sub_batch_offset_84;
        if ((current_record_74->flags_00 & 1) == 0) {
            if (vertex_count_88 != 0) {
                srVectorProcessor::copy(diffuse, color, vertex_count_88);
            }
        } else {
            current_record_74->color_source.copyDiffuseColors(
                diffuse, avt_70 + sub_batch_offset_84, vertex_count_88);
            if ((((color.x != 0.0f) || (color.y != 0.0f)) ||
                 ((color.z != 0.0f) || (color.w != 0.0f))) &&
                (vertex_count_88 != 0)) {
                srVectorProcessor::add(diffuse, color, diffuse, vertex_count_88);
            }
        }
        lazy_setup_mask |= 2;
    }
}

/* Retail uses the whole-batch count and destination with a source offset by
   sub_batch_offset_84. Later sub-batches can therefore overrun the source
   scratch array; preserve that observed range contract. */
// FUNCTION: SURRENDER 0x1002C0D0
void srVertexPipe::setupDepthCue()
{
    float* depth_cue = scratch_00->depth_cue_800;
    float minimum = input_6c->environment_minimum_54;
    float maximum = input_6c->environment_maximum_58;
    float near_value = 1.0f - input_6c->environment_scale_5c;
    float far_value = 1.0f - input_6c->environment_inverse_scale_60;
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 2) == 0) {
        setupEyeSpaceDirAndDist();
    }
    unsigned long count = batch_count_8c;
    float* dist = scratch->dist_600 + sub_batch_offset_84;
    if (minimum == maximum) {
        if (near_value == far_value) {
            srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(depth_cue),
                                    *reinterpret_cast<SRDWORD*>(&near_value), count);
        } else if (count != 0) {
            float* source = dist;
            float* out = depth_cue;
            do {
                float value = far_value;
                if (*source < minimum) {
                    value = near_value;
                }
                *out = value;
                ++out;
                ++source;
                --count;
            } while (count != 0);
        }
    } else {
        if (count != 0) {
            if (maximum == 0.0f) {
                srVectorProcessor::neg(depth_cue, dist, count);
            } else {
                srVectorProcessor::sub(depth_cue, maximum, dist, count);
            }
        }
        float scale = 1.0f / (maximum - minimum);
        if (count != 0) {
            if (scale != 1.0f) {
                if (scale == 0.0f) {
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(depth_cue), 0, count);
                } else {
                    srVectorProcessor::mul(depth_cue, scale, depth_cue, count);
                }
            }
            srVectorProcessor::clampUnit(depth_cue, depth_cue, count);
        }
        float range = near_value - far_value;
        if (((range != 1.0f) && (count != 0)) && (range != 1.0f)) {
            if (range == 0.0f) {
                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(depth_cue), 0, count);
            } else {
                srVectorProcessor::mul(depth_cue, range, depth_cue, count);
            }
        }
        if (((near_value != 1.0f) && (count != 0)) && (1.0f - near_value != 0.0f)) {
            srVectorProcessor::add(depth_cue, 1.0f - near_value, depth_cue, count);
        }
    }
    scratch->flags_b00 |= 0x10;
}

// FUNCTION: SURRENDER 0x1002C300
unsigned long srVertexPipe::getExclusionMask() const
{
    return input_6c->exclusion_mask_34;
}

// FUNCTION: SURRENDER 0x1002C350
int srVertexPipe::testEyeSpaceBounds(const srVector3T<float>& center, float radius) const
{
    float dx = center.x - input_6c->eye_center_18.x;
    float dy = center.y - input_6c->eye_center_18.y;
    float dz = center.z - input_6c->eye_center_18.z;
    float sum = radius + input_6c->eye_radius_24;
    if (dx * dx + dy * dy + dz * dz < sum * sum) {
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1002C3B0
void srVertexPipe::setupSpecular()
{
    if ((lazy_setup_mask & 4) == 0) {
        unsigned long dword_count = vertex_count_88 * 4;
        if (dword_count != 0) {
            srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(vertex_array_78->specular_08 +
                                                               batch_base + sub_batch_offset_84),
                                    0, dword_count);
        }
        lazy_setup_mask |= 4;
    }
}

// FUNCTION: SURRENDER 0x1002C400
void srVertexPipe::setupST(unsigned long index)
{
    if (((current_record_74->flags_00 & (1 << (index + 4))) != 0)) {
        const srVector2T<float>* source = current_record_74->st_source[index];
        if (source != 0) {
            vector_processor_98->_copyIndexed((&vertex_array_78->st0_0c)[index] + batch_base +
                                                  sub_batch_offset_84,
                                              reinterpret_cast<const srVector2*>(source),
                                              avt_70 + sub_batch_offset_84, vertex_count_88);
        }
    }
    lazy_setup_mask |= 1 << (index + 5);
}

// FUNCTION: SURRENDER 0x1002C480
void srVertexPipe::getEyeSpaceBoundingSphere(srVector3T<float>& center, float& radius) const
{
    center = input_6c->eye_center_18;
    radius = input_6c->eye_radius_24;
}

// FUNCTION: SURRENDER 0x1002C4B0
unsigned long srVertexPipe::getVertexCount() const
{
    return vertex_count_88;
}

// FUNCTION: SURRENDER 0x1002C4C0
const srVertexProcessor::MaterialInfo& srVertexPipe::getMaterialInfo() const
{
    return material_info;
}

// FUNCTION: SURRENDER 0x1002C4D0
srFlags<srVertexProcessor::e_channel> srVertexPipe::getChannelMask() const
{
    return srFlags<srVertexProcessor::e_channel>(channel_mask_0c);
}

// FUNCTION: SURRENDER 0x1002C4E0
int srVertexPipe::isChannelAvailable(srVertexProcessor::e_channel channel) const
{
    return ((1u << channel) & channel_mask_0c) != 0;
}

// FUNCTION: SURRENDER 0x1002C500
void srVertexPipe::setupQ(unsigned long index)
{
    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>((&vertex_array_78->q0_14)[index] +
                                                       batch_base + sub_batch_offset_84),
                            0x3f800000, vertex_count_88);
    lazy_setup_mask |= 1 << (index + 7);
}

// FUNCTION: SURRENDER 0x1002C560
const unsigned long* srVertexPipe::getAVT() const
{
    return avt_70 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C570
const srVector4T<float>* srVertexPipe::getEyeSpaceLocation()
{
    return eye_space_locations + batch_base + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C590
const float* srVertexPipe::getDepthCue()
{
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 0x10) == 0) {
        setupDepthCue();
    }
    return scratch->depth_cue_800 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C5C0
const srVector3T<float>* srVertexPipe::getEyeSpaceNormal()
{
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 8) == 0) {
        setupEyeSpaceNormal();
    }
    return scratch->normals_300 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C5F0
const srVector3T<float>* srVertexPipe::getEyeSpaceDir()
{
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 1) == 0) {
        setupEyeSpaceDirAndDist();
    }
    return scratch->dir_000 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C620
const float* srVertexPipe::getEyeSpaceDist()
{
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 2) == 0) {
        setupEyeSpaceDirAndDist();
    }
    return scratch->dist_600 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C650
const float* srVertexPipe::getEyeSpaceZDist()
{
    Scratch* scratch = scratch_00;
    if ((scratch->flags_b00 & 4) == 0) {
        setupEyeSpaceZDist();
    }
    return scratch->z_dist + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C680
float* srVertexPipe::getFog()
{
    srCore.getStatisticsManager()->statistics_00.fog_operations += vertex_count_88;
    Scratch* scratch = scratch_00;
    if ((lazy_setup_mask & 0x10) == 0) {
        setupFog();
    }
    return scratch->fog_a00 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C6C0
float* srVertexPipe::getAlpha()
{
    srCore.getStatisticsManager()->statistics_00.alpha_operations += vertex_count_88;
    Scratch* scratch = scratch_00;
    if ((lazy_setup_mask & 8) == 0) {
        setupAlpha();
    }
    return scratch->alpha_900 + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C700
srVector4T<float>* srVertexPipe::getDiffuse()
{
    srCore.getStatisticsManager()->statistics_00.diffuse_operations += vertex_count_88;
    if ((lazy_setup_mask & 2) == 0) {
        setupDiffuse();
    }
    return vertex_array_78->diffuse_04 + batch_base + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C740
srVector4T<float>* srVertexPipe::getSpecular()
{
    srCore.getStatisticsManager()->statistics_00.specular_operations += vertex_count_88;
    if ((lazy_setup_mask & 4) == 0) {
        setupSpecular();
    }
    return vertex_array_78->specular_08 + batch_base + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C780
srVector2T<float>* srVertexPipe::getST(unsigned long index, int create)
{
    srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations +=
        vertex_count_88;
    if ((create == 0) && ((lazy_setup_mask & (1 << (index + 5))) == 0)) {
        setupST(index);
    }
    lazy_setup_mask |= 1 << (index + 5);
    return (&vertex_array_78->st0_0c)[index] + batch_base + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C7F0
float* srVertexPipe::getQ(unsigned long index, int create)
{
    if ((create == 0) && ((lazy_setup_mask & (1 << (index + 7))) == 0)) {
        setupQ(index);
    }
    lazy_setup_mask |= 1 << (index + 7);
    return (&vertex_array_78->q0_14)[index] + batch_base + sub_batch_offset_84;
}

// FUNCTION: SURRENDER 0x1002C850
void* srVertexPipe::getUserArray(unsigned long index)
{
    return current_record_74->user_2c[index];
}
