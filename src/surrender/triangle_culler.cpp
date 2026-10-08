#include "surrender/srTriangleCuller.h"

#include "surrender/srVectorProcessor.h"
#include "surrender/srRendererDefs.h"

/* Retail (0x10029740) rounds the object-space points, delta, cross and the
   transformed positive point to float, but keeps the transformed point and
   the first edge on the x87 stack through the whole cross product; the
   second edge's z is used once from the stack and once from its float spill.
   Every matrix row is summed z, y, x, then w. The in-plane offset is folded
   into point_1 without a float rounding of its own. The double locals hold
   exactly what the 53-bit x87 stack holds, and float consistency keeps VC6
   from dropping the float roundings or reassociating the sums. */
#pragma optimize("p", on)
// FUNCTION: SURRENDER 0x10029740
srVector4 srTriangleCuller::transformClipPlane(const srVector4& plane, const srMatrix4& matrix,
                                               srMatrix4::e_scaleType scale_type)
{
    srVector4 normal = plane;
    srVector3 point(-(normal.x * normal.w), -(normal.y * normal.w), -(normal.z * normal.w));
    srVector3 positive(point.x + normal.x, point.y + normal.y, point.z + normal.z);
    srVector3 point_1;
    if (normal.z != 0.0f) {
        point_1 =
            srVector3(point.x + normal.z, point.y + normal.z, point.z + (-normal.x - normal.y));
    } else {
        point_1 =
            srVector3(point.x + (-normal.y - normal.z), point.y + normal.x, point.z + normal.x);
    }
    srVector3 delta(point_1.x - point.x, point_1.y - point.y, point_1.z - point.z);
    srVector3 cross(normal.y * delta.z - normal.z * delta.y,
                    normal.z * delta.x - normal.x * delta.z,
                    normal.x * delta.y - normal.y * delta.x);
    srVector3 point_2(point.x + cross.x, point.y + cross.y, point.z + cross.z);
    const srVector4* rows = matrix.vectors;
    double transformed_x =
        ((point.z * rows[0].z + point.y * rows[0].y) + point.x * rows[0].x) + rows[0].w;
    double transformed_y =
        ((point.z * rows[1].z + point.y * rows[1].y) + point.x * rows[1].x) + rows[1].w;
    double transformed_z =
        ((point.z * rows[2].z + point.y * rows[2].y) + point.x * rows[2].x) + rows[2].w;
    srVector3 transformed_positive(
        ((positive.z * rows[0].z + positive.y * rows[0].y) + positive.x * rows[0].x) + rows[0].w,
        ((positive.z * rows[1].z + positive.y * rows[1].y) + positive.x * rows[1].x) + rows[1].w,
        ((positive.z * rows[2].z + positive.y * rows[2].y) + positive.x * rows[2].x) + rows[2].w);
    double edge_1_x =
        (((point_1.z * rows[0].z + point_1.y * rows[0].y) + point_1.x * rows[0].x) + rows[0].w) -
        transformed_x;
    double edge_1_y =
        (((point_1.z * rows[1].z + point_1.y * rows[1].y) + point_1.x * rows[1].x) + rows[1].w) -
        transformed_y;
    double edge_1_z =
        (((point_1.z * rows[2].z + point_1.y * rows[2].y) + point_1.x * rows[2].x) + rows[2].w) -
        transformed_z;
    float edge_2_x = static_cast<float>(
        (((point_2.z * rows[0].z + point_2.y * rows[0].y) + point_2.x * rows[0].x) + rows[0].w) -
        transformed_x);
    float edge_2_y = static_cast<float>(
        (((point_2.z * rows[1].z + point_2.y * rows[1].y) + point_2.x * rows[1].x) + rows[1].w) -
        transformed_y);
    double edge_2_z_wide =
        (((point_2.z * rows[2].z + point_2.y * rows[2].y) + point_2.x * rows[2].x) + rows[2].w) -
        transformed_z;
    float edge_2_z = static_cast<float>(edge_2_z_wide);
    srVector4 result;
    result.x = static_cast<float>(edge_2_z_wide * edge_1_y - edge_2_y * edge_1_z);
    result.y = static_cast<float>(edge_2_x * edge_1_z - edge_2_z * edge_1_x);
    result.z = static_cast<float>(edge_2_y * edge_1_x - edge_2_x * edge_1_y);
    result.w = static_cast<float>(
        -((result.z * transformed_z + result.y * transformed_y) + result.x * transformed_x));
    if (((result.z * transformed_positive.z + result.y * transformed_positive.y) +
         result.x * transformed_positive.x) +
            result.w <
        0.0f) {
        result.x = -result.x;
        result.y = -result.y;
        result.z = -result.z;
        result.w = -result.w;
    }
    return result;
}
#pragma optimize("", on)

// FUNCTION: SURRENDER 0x10029BF0
int srTriangleCuller::setClipFlags(unsigned long* clip_flags, float* distances,
                                   const srVector3* vertices, const srVector4& plane,
                                   unsigned long shift, unsigned long count, int first)
{
    unsigned long collected = 0;
    srVectorProcessor::vp->_dot(distances, plane, vertices, count);
    /* reinterpret-ok: the outside test reads the distances' IEEE sign bits. */
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

// FUNCTION: SURRENDER 0x10029FC0
unsigned long srTriangleCuller::getClipMask(const srVector3& center, float radius,
                                            const srVector4* planes, unsigned long mask,
                                            float& depth)
{
    depth = 0.0f;
    unsigned long clip = 0;
    if (planes[srRendererDefs::CLIP_LEFT].z * center.z +
            planes[srRendererDefs::CLIP_LEFT].x * center.x <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_LEFT);
    }
    if (planes[srRendererDefs::CLIP_RIGHT].z * center.z +
            planes[srRendererDefs::CLIP_RIGHT].x * center.x <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_RIGHT);
    }
    if (planes[srRendererDefs::CLIP_BOTTOM].z * center.z +
            planes[srRendererDefs::CLIP_BOTTOM].y * center.y <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_BOTTOM);
    }
    if (planes[srRendererDefs::CLIP_TOP].y * center.y +
            planes[srRendererDefs::CLIP_TOP].z * center.z <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_TOP);
    }
    if (planes[srRendererDefs::CLIP_NEAR].z * center.z + planes[srRendererDefs::CLIP_NEAR].w <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_NEAR);
    }
    if (planes[srRendererDefs::CLIP_FAR].z * center.z + planes[srRendererDefs::CLIP_FAR].w <=
        radius) {
        clip |= (1UL << srRendererDefs::CLIP_FAR);
    }
    unsigned long remaining = mask & ~srRendererDefs::FRUSTUM_CLIP_MASK;
    const srVector4* plane = planes + srRendererDefs::FIRST_USER_CLIP_PLANE;
    for (unsigned long bit = srRendererDefs::FIRST_USER_CLIP_PLANE; remaining != 0 && bit < 0x20;
         ++bit, ++plane) {
        unsigned long plane_bit = 1 << bit;
        if ((remaining & plane_bit) != 0) {
            if (plane->y * center.y + plane->x * center.x + plane->z * center.z + plane->w <=
                radius) {
                clip |= plane_bit;
            }
            remaining &= ~plane_bit;
        }
    }
    if ((clip & srRendererDefs::FRUSTUM_CLIP_MASK) == 0) {
        return clip;
    }
    depth = 1.0f;
    if ((clip & (1UL << srRendererDefs::CLIP_LEFT)) != 0) {
        depth = (planes[srRendererDefs::CLIP_LEFT].z * center.z +
                 planes[srRendererDefs::CLIP_LEFT].x * center.x + radius) /
                (radius + radius);
    }
    if ((clip & (1UL << srRendererDefs::CLIP_RIGHT)) != 0) {
        depth = ((planes[srRendererDefs::CLIP_RIGHT].z * center.z +
                  planes[srRendererDefs::CLIP_RIGHT].x * center.x + radius) /
                 (radius + radius)) *
                depth;
    }
    if ((clip & (1UL << srRendererDefs::CLIP_BOTTOM)) != 0) {
        depth = ((planes[srRendererDefs::CLIP_BOTTOM].z * center.z +
                  planes[srRendererDefs::CLIP_BOTTOM].y * center.y + radius) /
                 (radius + radius)) *
                depth;
    }
    if ((clip & (1UL << srRendererDefs::CLIP_TOP)) != 0) {
        depth = ((planes[srRendererDefs::CLIP_TOP].y * center.y +
                  planes[srRendererDefs::CLIP_TOP].z * center.z + radius) /
                 (radius + radius)) *
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
    /* reinterpret-ok: the negative test reads the distances' IEEE sign bits. */
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
unsigned long srTriangleCuller::buildAVT(unsigned long* avt, unsigned long* vertex_scratch,
                                         const unsigned long* indices, const srVector3i* triangles,
                                         unsigned long triangle_count, unsigned long vertex_count)
{
    srVP* processor = srVectorProcessor::vp;
    processor->_memcopy(vertex_scratch, 0, vertex_count);
    /* reinterpret-ok: the flag scratch holds one SRBYTE per vertex here and is reused as the dword
       inverse remap below. */
    processor->_srSetIndexed(reinterpret_cast<SRBYTE*>(vertex_scratch), triangles, indices,
                             triangle_count);
    unsigned long count = processor->_srCollectNonZero(
        avt, reinterpret_cast<const SRBYTE*>(vertex_scratch), vertex_count);
    processor->_srRemapInverse(vertex_scratch, avt, count);
    return count;
}

// FUNCTION: SURRENDER 0x1002A720
int srTriangleCuller::cull(Output& output, const Input& input)
{
    if (input.triangle_count == 0) {
        return 0;
    }
    unsigned long clip_mask = input.clip_mask;
    const srMatrix4& inverse_model_view = *input.inverse_model_view;
    unsigned long* clip_flags = output.vertex_remap;
    output.linear = 1;
    output.triangle_count = 0;
    output.vertex_count = 0;
    srVector4 constant;
    if (input.cull_mode == 0) {
        constant.Set(-inverse_model_view.vectors[0].w, -inverse_model_view.vectors[1].w,
                     -inverse_model_view.vectors[2].w, -inverse_model_view.vectors[3].w);
    } else if (input.cull_mode == 1) {
        constant.Set(inverse_model_view.vectors[0].w, inverse_model_view.vectors[1].w,
                     inverse_model_view.vectors[2].w, inverse_model_view.vectors[3].w);
    } else {
        constant = 0.0f;
    }
    int clipped = clip_mask != 0;
    if (input.active_triangles == 0) {
        if (clipped != 0) {
            unsigned long plane_bits[32];
            srVector4 planes[32];
            unsigned long plane_count = 0;
            unsigned long remaining = clip_mask;
            const srVector4* plane = input.clip_planes;
            for (unsigned long bit = 0; remaining != 0; ++bit, ++plane) {
                unsigned long plane_bit = 1 << bit;
                if ((remaining & plane_bit) != 0) {
                    srVector4 source = *plane;
                    planes[plane_count] =
                        transformClipPlane(source, inverse_model_view, input.scale_type);
                    plane_bits[plane_count] = bit;
                    ++plane_count;
                    remaining &= ~plane_bit;
                }
            }
            unsigned long shared = 0;
            if (setClipFlagsObjectSpace(clip_flags, input.vertices, planes, plane_bits, plane_count,
                                        input.vertex_count, shared) == 0) {
                return 0;
            }
            clipped = shared != 0;
        }
    } else {
        clipped = 0;
    }
    if (input.active_triangles == 0) {
        if (clipped != 0) {
            if (input.cull_mode == 2) {
                output.triangle_count =
                    clip(output.indices, clip_flags, input.triangles, input.triangle_count);
            } else {
                output.triangle_count =
                    cullClip(output.indices, clip_flags, input.projected_vertices, input.triangles,
                             constant, input.triangle_count);
            }
        } else {
            if (input.cull_mode == 2) {
                setupLinearArray(output.indices, input.triangle_count);
                output.triangle_count = input.triangle_count;
            } else {
                output.triangle_count = cullNoClip(output.indices, input.projected_vertices,
                                                   constant, input.triangle_count);
            }
        }
    } else {
        unsigned long active_count = input.active_triangle_count;
        if (active_count == 0) {
            return 0;
        }
        if (input.cull_mode == 2) {
            if (active_count != 0 && output.indices != input.active_triangles) {
                srVectorProcessor::vp->_memcopy(output.indices, input.active_triangles,
                                                active_count * 4);
            }
            output.triangle_count = active_count;
        } else {
            output.triangle_count = cullNoClipAPT(output.indices, input.active_triangles,
                                                  input.projected_vertices, constant, active_count);
        }
    }
    if (output.triangle_count != 0) {
        output.vertex_count = buildAVT(output.avt, clip_flags, output.indices, input.triangles,
                                       output.triangle_count, input.vertex_count);
        return 1;
    }
    return 0;
}
