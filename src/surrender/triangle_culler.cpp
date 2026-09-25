#include "surrender/srTriangleCuller.h"

#include "surrender/srVectorProcessor.h"

// FUNCTION: SURRENDER 0x10029740
srVector4 srTriangleCuller::transformClipPlane(const srVector4& plane, const srMatrix4& matrix,
                                               srMatrix4::e_scaleType scale_type)
{
    srVector3 normal(plane.x, plane.y, plane.z);
    srVector3 point = normal * -plane.w;
    srVector3 positive = point + normal;
    srVector3 offset;
    if (normal.z == 0.0f) {
        offset.Set(-normal.y - normal.z, normal.x, normal.x);
    } else {
        offset.Set(normal.z, normal.z, -normal.x - normal.y);
    }
    srVector3 point_1 = point + offset;
    srVector3 point_2 = point + CrossProduct(normal, point_1 - point);
    srVector3 transformed = matrix.TransformPoint(point);
    srVector3 edge_1 = matrix.TransformPoint(point_1) - transformed;
    srVector3 edge_2 = matrix.TransformPoint(point_2) - transformed;
    srVector3 new_normal = CrossProduct(edge_1, edge_2);
    srVector4 result;
    result.Set(new_normal.x, new_normal.y, new_normal.z, -DotProduct(new_normal, transformed));
    if (DotProduct(new_normal, matrix.TransformPoint(positive)) + result.w < 0.0) {
        result *= -1.0;
    }
    return result;
}

// FUNCTION: SURRENDER 0x10029BF0
int srTriangleCuller::setClipFlags(unsigned long* clip_flags, float* distances,
                                   const srVector3* vertices, const srVector4& plane,
                                   unsigned long shift, unsigned long count, int first)
{
    unsigned long collected = 0;
    srVectorProcessor::vp->_dot(distances, plane, vertices, count);
    /* reinterpret-ok: the per-vertex outside test reads the distances' raw
       IEEE sign bits, exactly as retail's SHR 0x1f/SHL sequence. */
    const unsigned long* bits = reinterpret_cast<const unsigned long*>(distances);
    unsigned long index = 0;
    if (first == 0) {
        for (; index < (count & ~3UL); index += 4) {
            unsigned long bit_0 = (bits[index] >> 0x1f) << shift;
            unsigned long bit_1 = (bits[index + 1] >> 0x1f) << shift;
            unsigned long bit_2 = (bits[index + 2] >> 0x1f) << shift;
            unsigned long bit_3 = (bits[index + 3] >> 0x1f) << shift;
            clip_flags[index] |= bit_0;
            clip_flags[index + 1] |= bit_1;
            clip_flags[index + 2] |= bit_2;
            clip_flags[index + 3] |= bit_3;
            collected |= bit_0 | bit_1 | bit_2 | bit_3;
        }
        for (; index < count; ++index) {
            clip_flags[index] |= (bits[index] >> 0x1f) << shift;
            collected |= (bits[index] >> 0x1f) << shift;
        }
    } else {
        for (; index < (count & ~3UL); index += 4) {
            unsigned long bit_0 = (bits[index] >> 0x1f) << shift;
            unsigned long bit_1 = (bits[index + 1] >> 0x1f) << shift;
            unsigned long bit_2 = (bits[index + 2] >> 0x1f) << shift;
            unsigned long bit_3 = (bits[index + 3] >> 0x1f) << shift;
            clip_flags[index] = bit_0;
            clip_flags[index + 1] = bit_1;
            clip_flags[index + 2] = bit_2;
            clip_flags[index + 3] = bit_3;
            collected |= bit_0 | bit_1 | bit_2 | bit_3;
        }
        for (; index < count; ++index) {
            clip_flags[index] = (bits[index] >> 0x1f) << shift;
            collected |= (bits[index] >> 0x1f) << shift;
        }
    }
    return collected != 0;
}

// FUNCTION: SURRENDER 0x10029DE0
int srTriangleCuller::setClipFlagsObjectSpace(unsigned long* clip_flags, const srVector3* vertices,
                                              const srVector4* planes,
                                              const unsigned long* plane_bits,
                                              unsigned long plane_count, unsigned long vertex_count,
                                              unsigned long& shared_out)
{
    float distances[256];
    unsigned long shared = 0xffffffff;
    shared_out = 0;
    for (unsigned long base = 0; base < vertex_count; base += 0x100) {
        unsigned long limit = base + 0x100;
        if (vertex_count < limit) {
            limit = vertex_count;
        }
        for (unsigned long plane = 0; plane < plane_count; ++plane) {
            shared_out |= setClipFlags(clip_flags + base, distances, vertices + base, planes[plane],
                                       plane_bits[plane], limit - base, plane == 0);
        }
        unsigned long index = base;
        while (shared != 0 && index < limit) {
            shared &= clip_flags[index];
            ++index;
        }
    }
    return shared == 0;
}

// FUNCTION: SURRENDER 0x10029F30
srTriangleCuller& srTriangleCuller::operator=(const srTriangleCuller& other)
{
    memcpy(this, &other, sizeof(srTriangleCuller));
    return *this;
}

// FUNCTION: SURRENDER 0x10029FC0
unsigned long srTriangleCuller::getClipMask(const srVector3& center, float radius,
                                            const srVector4* planes, unsigned long mask,
                                            float& depth)
{
    depth = 0.0f;
    unsigned long clip = 0;
    if (planes[0].z * center.z + planes[0].x * center.x <= radius) {
        clip |= 1;
    }
    if (planes[1].z * center.z + planes[1].x * center.x <= radius) {
        clip |= 2;
    }
    if (planes[2].z * center.z + planes[2].y * center.y <= radius) {
        clip |= 4;
    }
    if (planes[3].y * center.y + planes[3].z * center.z <= radius) {
        clip |= 8;
    }
    if (planes[4].z * center.z + planes[4].w <= radius) {
        clip |= 0x10;
    }
    if (planes[5].z * center.z + planes[5].w <= radius) {
        clip |= 0x20;
    }
    unsigned long remaining = mask & 0xffffffc0;
    const srVector4* plane = planes + 6;
    for (unsigned long bit = 6; remaining != 0 && bit < 0x20; ++bit, ++plane) {
        unsigned long plane_bit = 1 << bit;
        if ((remaining & plane_bit) != 0) {
            if (plane->y * center.y + plane->x * center.x + plane->z * center.z + plane->w <=
                radius) {
                clip |= plane_bit;
            }
            remaining &= ~plane_bit;
        }
    }
    if ((clip & 0x3f) == 0) {
        return clip;
    }
    depth = 1.0f;
    if ((clip & 1) != 0) {
        depth = (planes[0].z * center.z + planes[0].x * center.x + radius) / (radius + radius);
    }
    if ((clip & 2) != 0) {
        depth = ((planes[1].z * center.z + planes[1].x * center.x + radius) / (radius + radius)) *
                depth;
    }
    if ((clip & 4) != 0) {
        depth = ((planes[2].z * center.z + planes[2].y * center.y + radius) / (radius + radius)) *
                depth;
    }
    if ((clip & 8) != 0) {
        depth = ((planes[3].y * center.y + planes[3].z * center.z + radius) / (radius + radius)) *
                depth;
    }
    depth = 1.0f - depth;
    return clip;
}

// FUNCTION: SURRENDER 0x1002A1B0
unsigned long srTriangleCuller::collectNegative(unsigned long* indices, const float* distances,
                                                unsigned long first, unsigned long count)
{
    unsigned long* destination = indices;
    /* reinterpret-ok: the negative test reads the distances' raw IEEE sign
       bit, exactly as retail's SHR 0x1f store-then-advance sequence. */
    const unsigned long* bits = reinterpret_cast<const unsigned long*>(distances);
    unsigned long collected = 0;
    if (count != 0) {
        int total = count;
        int bound = total & ~3;
        int index = 0;
        for (; index < bound; index += 4) {
            *destination = first + index;
            destination += bits[index] >> 0x1f;
            *destination = first + index + 1;
            destination += bits[index + 1] >> 0x1f;
            *destination = first + index + 2;
            destination += bits[index + 2] >> 0x1f;
            *destination = first + index + 3;
            destination += bits[index + 3] >> 0x1f;
        }
        for (; index < total; ++index) {
            if ((bits[index] & 0x80000000) != 0) {
                *destination = first + index;
                ++destination;
            }
        }
        collected = (reinterpret_cast<unsigned long>(destination) -
                     reinterpret_cast<unsigned long>(indices)) >>
                    2;
    }
    return collected;
}

// FUNCTION: SURRENDER 0x1002A260
unsigned long srTriangleCuller::cullNoClip(unsigned long* indices, const srVector4* projected,
                                           const srVector4& constant, unsigned long count)
{
    return srVectorProcessor::vp->_srCullNoClip(indices, constant, projected, count);
}

// FUNCTION: SURRENDER 0x1002A290
unsigned long srTriangleCuller::cullNoClipAPT(unsigned long* indices, const unsigned long* active,
                                              const srVector4* projected, const srVector4& constant,
                                              unsigned long count)
{
    float distances[256];
    unsigned long collected = 0;
    for (unsigned long base = 0; base < count; base += 0x100) {
        unsigned long chunk = count - base;
        if (0x100 < chunk) {
            chunk = 0x100;
        }
        srVectorProcessor::vp->_dotIndexed(distances, constant, projected, active + base, chunk);
        unsigned long found = collectNegative(indices + collected, distances, base, chunk);
        if (found != 0) {
            unsigned long* destination = indices + collected;
            unsigned long index = 0;
            for (; index < (found & ~3UL); index += 4) {
                destination[index] = active[destination[index]];
                destination[index + 1] = active[destination[index + 1]];
                destination[index + 2] = active[destination[index + 2]];
                destination[index + 3] = active[destination[index + 3]];
            }
            for (; index < found; ++index) {
                destination[index] = active[destination[index]];
            }
        }
        collected += found;
    }
    return collected;
}

// FUNCTION: SURRENDER 0x1002A3E0
unsigned long srTriangleCuller::collectCF(unsigned long* indices, const unsigned long* clip_flags,
                                          const srVector3i* triangles, unsigned long count)
{
    if (((unsigned long)indices & 3) == 0) {
        unsigned long collected = 0;
        for (unsigned long index = 0; index < count; ++index) {
            unsigned long triangle = indices[index];
            indices[collected] = triangle;
            collected += (clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
                          clip_flags[triangles[triangle].z]) == 0;
        }
        return collected;
    }
    unsigned long collected = 0;
    unsigned long index = 0;
    for (; index < (count & ~3UL); index += 4) {
        unsigned long triangle = indices[index];
        if ((clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
             clip_flags[triangles[triangle].z]) == 0) {
            indices[collected] = triangle;
            ++collected;
        }
        triangle = indices[index + 1];
        if ((clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
             clip_flags[triangles[triangle].z]) == 0) {
            indices[collected] = triangle;
            ++collected;
        }
        triangle = indices[index + 2];
        if ((clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
             clip_flags[triangles[triangle].z]) == 0) {
            indices[collected] = triangle;
            ++collected;
        }
        triangle = indices[index + 3];
        if ((clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
             clip_flags[triangles[triangle].z]) == 0) {
            indices[collected] = triangle;
            ++collected;
        }
    }
    for (; index < count; ++index) {
        unsigned long triangle = indices[index];
        if ((clip_flags[triangles[triangle].x] & clip_flags[triangles[triangle].y] &
             clip_flags[triangles[triangle].z]) == 0) {
            indices[collected] = triangle;
            ++collected;
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x1002A5C0
unsigned long srTriangleCuller::cullClip(unsigned long* indices, const unsigned long* clip_flags,
                                         const srVector4* projected, const srVector3i* triangles,
                                         const srVector4& constant, unsigned long count)
{
    unsigned long culled =
        srVectorProcessor::vp->_srCullNoClip(indices, constant, projected, count);
    return collectCF(indices, clip_flags, triangles, culled);
}

// FUNCTION: SURRENDER 0x1002A600
unsigned long srTriangleCuller::clip(unsigned long* indices, const unsigned long* clip_flags,
                                     const srVector3i* triangles, unsigned long count)
{
    unsigned long collected = 0;
    for (unsigned long index = 0; index < count; ++index) {
        if ((clip_flags[triangles[index].x] & clip_flags[triangles[index].z] &
             clip_flags[triangles[index].y]) == 0) {
            indices[collected] = index;
            ++collected;
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x1002A650
void srTriangleCuller::setupLinearArray(unsigned long* indices, unsigned long count)
{
    unsigned long index = 0;
    for (; index < (count & ~7UL); index += 8) {
        indices[index] = index;
        indices[index + 1] = index + 1;
        indices[index + 2] = index + 2;
        indices[index + 3] = index + 3;
        indices[index + 4] = index + 4;
        indices[index + 5] = index + 5;
        indices[index + 6] = index + 6;
        indices[index + 7] = index + 7;
    }
    for (; index < count; ++index) {
        indices[index] = index;
    }
}

// FUNCTION: SURRENDER 0x1002A6C0
unsigned long srTriangleCuller::buildAVT(unsigned long* avt, unsigned long* clip_flags,
                                         const unsigned long* indices, const srVector3i* triangles,
                                         unsigned long triangle_count, unsigned long vertex_count)
{
    srVP* processor = srVectorProcessor::vp;
    processor->_memcopy(clip_flags, 0, vertex_count);
    /* reinterpret-ok: the caller-provided flag scratch holds one SRBYTE per
       vertex here and is reused as the dword inverse remap below. */
    processor->_srSetIndexed(reinterpret_cast<SRBYTE*>(clip_flags), triangles, indices,
                             triangle_count);
    unsigned long count = processor->_srCollectNonZero(
        avt, reinterpret_cast<const SRBYTE*>(clip_flags), vertex_count);
    processor->_srRemapInverse(clip_flags, avt, count);
    return count;
}

// FUNCTION: SURRENDER 0x1002A720
int srTriangleCuller::cull(Output& output, const Input& input)
{
    if (input.triangle_count_00 == 0) {
        return 0;
    }
    unsigned long clip_mask = input.clip_mask_30;
    const srMatrix4& inverse_model_view = *input.inverse_model_view_28;
    unsigned long* clip_flags = output.clip_flags_08;
    output.linear_14 = 1;
    output.triangle_count_0c = 0;
    output.vertex_count_10 = 0;
    srVector4 constant;
    if (input.cull_mode_0c == 0) {
        constant.Set(-inverse_model_view.vectors[0].w, -inverse_model_view.vectors[1].w,
                     -inverse_model_view.vectors[2].w, -inverse_model_view.vectors[3].w);
    } else if (input.cull_mode_0c == 1) {
        constant.Set(inverse_model_view.vectors[0].w, inverse_model_view.vectors[1].w,
                     inverse_model_view.vectors[2].w, inverse_model_view.vectors[3].w);
    } else {
        constant = 0.0f;
    }
    int clipped = clip_mask != 0;
    if (input.active_triangles_10 == 0) {
        if (clipped != 0) {
            unsigned long plane_bits[32];
            srVector4 planes[32];
            unsigned long plane_count = 0;
            unsigned long remaining = clip_mask;
            const srVector4* plane = input.clip_planes_20;
            for (unsigned long bit = 0; remaining != 0; ++bit, ++plane) {
                unsigned long plane_bit = 1 << bit;
                if ((remaining & plane_bit) != 0) {
                    srVector4 source = *plane;
                    planes[plane_count] =
                        transformClipPlane(source, inverse_model_view, input.scale_type_2c);
                    plane_bits[plane_count] = bit;
                    ++plane_count;
                    remaining &= ~plane_bit;
                }
            }
            unsigned long shared = 0;
            if (setClipFlagsObjectSpace(clip_flags, input.vertices_1c, planes, plane_bits,
                                        plane_count, input.vertex_count_04, shared) == 0) {
                return 0;
            }
            clipped = shared != 0;
        }
    } else {
        clipped = 0;
    }
    if (input.active_triangles_10 == 0) {
        if (clipped != 0) {
            if (input.cull_mode_0c == 2) {
                output.triangle_count_0c = clip(output.indices_00, clip_flags, input.triangles_18,
                                                input.triangle_count_00);
            } else {
                output.triangle_count_0c =
                    cullClip(output.indices_00, clip_flags, input.projected_vertices_14,
                             input.triangles_18, constant, input.triangle_count_00);
            }
        } else {
            if (input.cull_mode_0c == 2) {
                setupLinearArray(output.indices_00, input.triangle_count_00);
                output.triangle_count_0c = input.triangle_count_00;
            } else {
                output.triangle_count_0c =
                    cullNoClip(output.indices_00, input.projected_vertices_14, constant,
                               input.triangle_count_00);
            }
        }
    } else {
        unsigned long active_count = input.active_triangle_count_08;
        if (active_count == 0) {
            return 0;
        }
        if (input.cull_mode_0c == 2) {
            if (active_count != 0 && output.indices_00 != input.active_triangles_10) {
                srVectorProcessor::vp->_memcopy(output.indices_00, input.active_triangles_10,
                                                active_count * 4);
            }
            output.triangle_count_0c = active_count;
        } else {
            output.triangle_count_0c =
                cullNoClipAPT(output.indices_00, input.active_triangles_10,
                              input.projected_vertices_14, constant, active_count);
        }
    }
    if (output.triangle_count_0c != 0) {
        output.vertex_count_10 =
            buildAVT(output.avt_04, clip_flags, output.indices_00, input.triangles_18,
                     output.triangle_count_0c, input.vertex_count_04);
        return 1;
    }
    return 0;
}

// SYNTHETIC: SURRENDER 0X10029F50
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10029F60
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X10029F90
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X10029FA0
// std::_Winit global atexit registrar
