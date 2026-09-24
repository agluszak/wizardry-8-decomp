#include "surrender/srGERD.h"

#include <string.h>

#include "surrender/srVectorProcessor.h"

/* drawSorted's {order, sort_key} pair record; retail folds this TU's
   radix-sort emission into huffman.cpp's identical sortSymbolPairs at
   0x100027F0, which is why the retail drawSorted calls that address. */
struct SortPair {
    unsigned long index_00;
    unsigned long key_04;
};

static void* copyMemory(void* destination, const void* source, long size);
static void markTransitions(unsigned long* output, const unsigned long* indices,
                            const unsigned long* table, unsigned long bit, unsigned long count,
                            unsigned long initialized);
static void fillConstant(unsigned long* destination, unsigned long value, unsigned long count);

static void sortPairs(SortPair* pairs, unsigned long count)
{
    if (count <= 1) {
        return;
    }
    SortPair* scratch = static_cast<SortPair*>(srHeap.allocate(count * 8));
    unsigned long counts[0x100];
    SortPair* src = pairs;
    SortPair* dst = scratch;
    unsigned long bulk = count & ~3;
    for (unsigned long pass = 0; pass < 4; ++pass) {
        srZeroMemory(counts, sizeof(counts));
        /* reinterpret-ok: radix pass extracts byte `pass` of each key. */
        unsigned char* keys = reinterpret_cast<unsigned char*>(&src[0].key_04) + pass;
        unsigned long index = 0;
        for (; index < bulk; index += 4) {
            ++counts[keys[index * 8]];
            ++counts[keys[index * 8 + 8]];
            ++counts[keys[index * 8 + 0x10]];
            ++counts[keys[index * 8 + 0x18]];
        }
        for (; index < count; ++index) {
            ++counts[keys[index * 8]];
        }
        unsigned long position = 0;
        for (unsigned long bucket = 0; bucket < 0x100; bucket += 4) {
            unsigned long saved = counts[bucket];
            counts[bucket] = position;
            position += saved;
            saved = counts[bucket + 1];
            counts[bucket + 1] = position;
            position += saved;
            saved = counts[bucket + 2];
            counts[bucket + 2] = position;
            position += saved;
            saved = counts[bucket + 3];
            counts[bucket + 3] = position;
            position += saved;
        }
        for (index = 0; index < bulk; index += 4) {
            dst[counts[keys[index * 8]]++] = src[index];
            dst[counts[keys[index * 8 + 8]]++] = src[index + 1];
            dst[counts[keys[index * 8 + 0x10]]++] = src[index + 2];
            dst[counts[keys[index * 8 + 0x18]]++] = src[index + 3];
        }
        for (; index < count; ++index) {
            dst[counts[keys[index * 8]]++] = src[index];
        }
        SortPair* swap = src;
        src = dst;
        dst = swap;
    }
    if (src != pairs) {
        copyMemory(pairs, src, count * 8);
    }
    srHeap.free(dst);
}

static void* copyMemory(void* destination, const void* source, long size)
{
    if (size <= 0) {
        return 0;
    }
    memcpy(destination, source, size);
    return destination;
}

/* dst[i] = src[i] + offset over `count` dwords — the index-rebase helper
   for both the triangle triples and the sort-key stream; authored unrolled
   eight wide. */
// FUNCTION: SURRENDER 0x10023CB0
static void offsetIndices(unsigned long* destination, const unsigned long* source,
                          unsigned long offset, unsigned long count)
{
    unsigned long index = 0;
    for (; index < (count & ~7UL); index += 8) {
        destination[index] = source[index] + offset;
        destination[index + 1] = source[index + 1] + offset;
        destination[index + 2] = source[index + 2] + offset;
        destination[index + 3] = source[index + 3] + offset;
        destination[index + 4] = source[index + 4] + offset;
        destination[index + 5] = source[index + 5] + offset;
        destination[index + 6] = source[index + 6] + offset;
        destination[index + 7] = source[index + 7] + offset;
    }
    for (; index < (count & ~1UL); index += 2) {
        destination[index] = source[index] + offset;
        destination[index + 1] = source[index + 1] + offset;
    }
    if (index < count) {
        destination[index] = source[index] + offset;
    }
}

/* Retail 0x10023DE0: gather `count` triangle index triples through `order`;
   a nonzero `vertex_base` additionally rebases every index (the sorted draw
   path always passes 0). */
// FUNCTION: SURRENDER 0x10023DE0
static void gatherTriangles(srVector3i* destination, const srVector3i* source,
                            const unsigned long* order, long vertex_base, unsigned long count)
{
    if (vertex_base == 0) {
        for (unsigned long index = 0; index < count; index++) {
            destination[index] = source[order[index]];
        }
    } else {
        for (unsigned long index = 0; index < count; index++) {
            const srVector3i& triangle = source[order[index]];
            destination[index].x = triangle.x + vertex_base;
            destination[index].y = triangle.y + vertex_base;
            destination[index].z = triangle.z + vertex_base;
        }
    }
}

/* Retail 0x10023D90: gather `count` triangles through `indices`, remapping
   every corner through `vertices` (the vector processor's
   _srCopyIndexedRemap), then rebase every index by `vertex_base` when it is
   nonzero. */
// FUNCTION: SURRENDER 0x10023D90
static void gatherIndexedTriangles(srVector3i* destination, const srVector3i* source,
                                   const unsigned long* indices, const unsigned long* vertices,
                                   unsigned long vertex_base, unsigned long count)
{
    srVectorProcessor::srCopyIndexedRemap(destination, source, indices, vertices, count);
    if (vertex_base != 0) {
        /* reinterpret-ok: the triples rebase as flat dwords. */
        offsetIndices(reinterpret_cast<unsigned long*>(destination),
                      reinterpret_cast<const unsigned long*>(destination), vertex_base, count * 3);
    }
}

/* Retail 0x10024000: dst[i] = src[i] + offset over `count` index triples —
   the per-record vertex rebase; authored unrolled four wide. */
// FUNCTION: SURRENDER 0x10024000
static void offsetTriangles(srVector3i* destination, const srVector3i* source, unsigned long offset,
                            unsigned long count)
{
    unsigned long index = 0;
    for (; index < (count & ~3UL); index += 4) {
        destination[index].x = source[index].x + offset;
        destination[index].y = source[index].y + offset;
        destination[index].z = source[index].z + offset;
        destination[index + 1].x = source[index + 1].x + offset;
        destination[index + 1].y = source[index + 1].y + offset;
        destination[index + 1].z = source[index + 1].z + offset;
        destination[index + 2].x = source[index + 2].x + offset;
        destination[index + 2].y = source[index + 2].y + offset;
        destination[index + 2].z = source[index + 2].z + offset;
        destination[index + 3].x = source[index + 3].x + offset;
        destination[index + 3].y = source[index + 3].y + offset;
        destination[index + 3].z = source[index + 3].z + offset;
    }
    for (; index < count; index++) {
        destination[index].x = source[index].x + offset;
        destination[index].y = source[index].y + offset;
        destination[index].z = source[index].z + offset;
    }
}

/* Retail 0x10024100: index of the first element not equal to `value`, or
   `count` when the whole range matches. drawImmediate uses it to find the
   end of each equal-texture-set run; VC6 unrolls the scan four ways. */
// FUNCTION: SURRENDER 0x10024100
static unsigned long firstMismatch(const unsigned long* values, unsigned long value,
                                   unsigned long count)
{
    unsigned long index = 0;
    while (index < count && values[index] == value) {
        index++;
    }
    return index;
}

/* Retail 0x10024170: the drawSorted analog of firstMismatch — the count of
   consecutive `order` entries whose values element equals `value`. */
// FUNCTION: SURRENDER 0x10024170
static unsigned long firstMismatchSorted(const unsigned long* values, unsigned long value,
                                         const unsigned long* order, unsigned long count)
{
    unsigned long index = 0;
    while (index < count && values[order[index]] == value) {
        index++;
    }
    return index;
}

// FUNCTION: SURRENDER 0x10024200
void srGERD::Renderer::resetStatistics()
{
    memset(statistics_28_, 0, sizeof(statistics_28_));
}

// FUNCTION: SURRENDER 0x10024260
void srGERD::Renderer::getStatistics(unsigned long* statistics)
{
    for (int i = 0; i < 7; i++) {
        statistics[i] = statistics_28_[i];
    }
}

// FUNCTION: SURRENDER 0x10024280
unsigned long srGERD::Renderer::TextureSetCache::intern(const TextureSetKey& key)
{
    srHashTable<TextureSetKey, unsigned long>* map = map_00;
    int slot = map->bucket_heads[srHashValue(key) & (map->bucket_count - 1)];
    while (slot != -1) {
        srHashEntry<TextureSetKey, unsigned long>* entry = map->entries + slot;
        if (entry->key.texture0_00 == key.texture0_00 &&
            entry->key.texture1_04 == key.texture1_04 &&
            entry->key.shader_08.value == key.shader_08.value) {
            return entry->value;
        }
        slot = entry->next_index;
    }

    unsigned long index = count_0c;
    slot = map->AllocateEntry();
    srHashEntry<TextureSetKey, unsigned long>* entry = map->entries + slot;
    entry->key = key;
    entry->value = index;
    unsigned int bucket = srHashValue(key) & (map->bucket_count - 1);
    entry->next_index = map->bucket_heads[bucket];
    map->bucket_heads[bucket] = slot;

    TextureSet& set = sets_04[index];
    set.texture0_00 = key.texture0_00;
    set.texture1_04 = key.texture1_04;
    set.shader_08 = key.shader_08;
    switch ((key.shader_08.value >> srShader::DSTBLEND_SHIFT) & 7) {
    case srShader::DSTBLEND_ONE:
        set.blend_0c = 2;
        break;
    case srShader::DSTBLEND_SRC_COLOR:
    case srShader::DSTBLEND_ONE_MINUS_SRC_COLOR:
        set.blend_0c = 3;
        break;
    case srShader::DSTBLEND_SRC_ALPHA:
    case srShader::DSTBLEND_ONE_MINUS_SRC_ALPHA:
        set.blend_0c = 1;
        break;
    default:
        set.blend_0c = 0;
        break;
    }
    count_0c += 1;
    return index;
}

// FUNCTION: SURRENDER 0x10024460
void srGERD::Renderer::IndexBatch::alloc(IndexWrite& write, unsigned long count)
{
    unsigned long needed = count + 0x40 + count_20;
    if (triangles_00.capacity <= needed) {
        triangles_00.setCapacity(triangles_00.capacity + 8 + needed);
    }
    if (texture_set_08.capacity <= needed) {
        texture_set_08.setCapacity(texture_set_08.capacity + 8 + needed);
    }
    if (sort_key_10.capacity <= needed) {
        sort_key_10.setCapacity(sort_key_10.capacity + 8 + needed);
    }
    if (aux_18.capacity <= needed) {
        aux_18.setCapacity(aux_18.capacity + 8 + needed);
    }
    write.triangles_00 = &triangles_00[count_20];
    write.texture_set_04 = &texture_set_08[count_20];
    write.sort_key_08 = &sort_key_10[count_20];
    write.aux_0c = &aux_18[count_20];
    count_20 += count;
}

// FUNCTION: SURRENDER 0x10024620
void srGERD::Renderer::IndexBatch::reset(int release)
{
    if (release != 0) {
        triangles_00.release();
        texture_set_08.release();
        sort_key_10.release();
        aux_18.release();
    }
    count_20 = 0;
}

// FUNCTION: SURRENDER 0x10024680
void srGERD::Renderer::VertexArrays::alloc(srVertexArray& arrays, unsigned long count)
{
    unsigned long needed = count + count_40;
    if (capacity_44 < needed) {
        needed += 0x40;
        if (diffuse_00.capacity <= needed) {
            diffuse_00.setCapacity(diffuse_00.capacity + 8 + needed);
        }
        if (specular_08.capacity <= needed) {
            specular_08.setCapacity(specular_08.capacity + 8 + needed);
        }
        if (positions_10.capacity <= needed) {
            positions_10.setCapacity(positions_10.capacity + 8 + needed);
        }
        if (st_18[0].capacity <= needed) {
            st_18[0].setCapacity(st_18[0].capacity + 8 + needed);
        }
        if (st_18[1].capacity <= needed) {
            st_18[1].setCapacity(st_18[1].capacity + 8 + needed);
        }
        if (q_28[0].capacity <= needed) {
            q_28[0].setCapacity(q_28[0].capacity + 8 + needed);
        }
        if (q_28[1].capacity <= needed) {
            q_28[1].setCapacity(q_28[1].capacity + 8 + needed);
        }
        if (packed_38.capacity <= needed) {
            packed_38.setCapacity(packed_38.capacity + 8 + needed);
        }

        unsigned long first = capacity_44;
        unsigned long added = needed - first;
        /* reinterpret-ok: the dword copy is the shader-agnostic byte fill the
           vector processor exposes for 0x10-stride records. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&diffuse_00[first]), 0, added * 4);
        /* reinterpret-ok: as above. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&specular_08[first]), 0,
                                added * 4);
        srVector4T<float> eye_default;
        eye_default.Set(0.0f, 0.0f, 0.0f, 1.0f);
        if (added != 0) {
            srVectorProcessor::copy(&positions_10[first], eye_default, added);
        }
        /* reinterpret-ok: 1.0f's bit pattern goes in through the dword
           fill. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&q_28[0][first]), 0x3f800000,
                                added);
        /* reinterpret-ok: as above. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&q_28[1][first]), 0x3f800000,
                                added);
        srVectorProcessor::copy(&st_18[0][first], srVector2T<float>(0.0f, 0.0f), added);
        srVectorProcessor::copy(&st_18[1][first], srVector2T<float>(0.0f, 0.0f), added);
        capacity_44 = needed;
    }
    bind(arrays, count_40);
    count_40 += count;
}

// FUNCTION: SURRENDER 0x10024900
srGERD::Renderer::Renderer(const Parameters& parameters)
    : gerd_d4_(parameters.gerd), sorted_d8_(parameters.sorted),
      batch_limit_dc_(parameters.batch_limit), texture_stages_e0_(parameters.texture_stages)
{
    clip_state_d0_ = 0;
    first_vertex_c0_ = -1;
    memset(statistics_28_, 0, sizeof(statistics_28_));
    texture0_c4_ = 0;
    texture1_c8_ = 0;
    shader_cc_ = srShader();
}

// FUNCTION: SURRENDER 0x10024DB0
int srGERD::Renderer::isBatchFull() const
{
    if (sorted_d8_ == 0 && batch_limit_dc_ < vertices_78_.count_40) {
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10024DE0
void srGERD::Renderer::rewindVertexArray(unsigned long count)
{
    if (first_vertex_c0_ != -1) {
        vertices_78_.count_40 -= count;
    }
}

// FUNCTION: SURRENDER 0x10024E00
void srGERD::Renderer::allocVertexArray(srVertexArray& arrays, unsigned long count)
{
    first_vertex_c0_ = vertices_78_.count_40;
    vertices_78_.alloc(arrays, count);
}

// FUNCTION: SURRENDER 0x10024E30
void srGERD::Renderer::assignTextureSets(unsigned long* texture_set, const unsigned long* indices,
                                         unsigned long count, const srTriMeshPipeline::Pass* pass)
{
    TextureSetKey key;
    key.texture0_00 = pass->texture_00;
    /* reinterpret-ok: the second key word rides the pass's value slot. */
    key.texture1_04 = reinterpret_cast<srTextureIFace*>(pass->pass_value_04);
    key.shader_08 = pass->flags_08;
    unsigned char mask = pass->texture_array_0c != 0;
    if (pass->texture_array_10 != 0) {
        mask |= 2;
    }
    if (pass->shader_14 != 0) {
        mask |= 4;
    }
    unsigned long set = texture_sets_44_.intern(key);
    if (mask == 0) {
        if (count != 0) {
            srVectorProcessor::copy(texture_set, set, count);
        }
        return;
    }
    unsigned long done = 0;
    while (done < count) {
        unsigned long chunk = count - done;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        const unsigned long* chunk_indices = indices + done;
        unsigned long* out = texture_set + done;
        unsigned long initialized = 0;
        if ((mask & 1) != 0) {
            /* reinterpret-ok: the texture table is a dword stream to the
               transition marker. */
            markTransitions(out, chunk_indices,
                            reinterpret_cast<const unsigned long*>(pass->texture_array_0c), 1,
                            chunk, initialized);
            initialized = 1;
        }
        if ((mask & 2) != 0) {
            markTransitions(out, chunk_indices,
                            reinterpret_cast<const unsigned long*>(pass->texture_array_10), 2,
                            chunk, initialized);
            initialized += 1;
        }
        if ((mask & 4) != 0) {
            markTransitions(out, chunk_indices,
                            reinterpret_cast<const unsigned long*>(pass->shader_14), 4, chunk,
                            initialized);
        }
        if (chunk != 0) {
            for (unsigned long index = 0; index < chunk; index++) {
                unsigned long changed = out[index];
                if (changed != 0) {
                    unsigned long vertex = chunk_indices[index];
                    if ((changed & 1) != 0) {
                        /* reinterpret-ok: the stage-0 table entries are the
                           texture interface pointers the key stores. */
                        key.texture0_00 = reinterpret_cast<srTextureIFace* const*>(
                            pass->texture_array_0c)[vertex];
                    }
                    if ((changed & 2) != 0) {
                        /* reinterpret-ok: same table form for stage 1. */
                        key.texture1_04 = reinterpret_cast<srTextureIFace* const*>(
                            pass->texture_array_10)[vertex];
                    }
                    if ((changed & 4) != 0) {
                        key.shader_08 = pass->shader_14[vertex];
                    }
                    set = texture_sets_44_.intern(key);
                }
                out[index] = set;
            }
        }
        done += 0x100;
    }
}

/* Retail 0x100251E0: depth-sort keys — each triangle's key is the negated
   sum of its corner view-space z's plus the bias, converted to a sortable
   unsigned value (sign-bit-mapped magnitude ordering). */
// FUNCTION: SURRENDER 0x100251E0
static void sortKeys(unsigned long* keys, const srVector3i* triangles,
                     const srVector4T<float>* positions, float bias, unsigned long count)
{
    if (count != 0) {
        const srVector3i* triangle = triangles;
        float scaled_bias = bias * 3.0f;
        do {
            float depth = -(positions[triangle->x].z + positions[triangle->y].z +
                            positions[triangle->z].z + scaled_bias);
            /* reinterpret-ok: the key maps the depth's IEEE sign bit onto a
               sortable unsigned order. */
            unsigned long bits = reinterpret_cast<unsigned long&>(depth);
            if ((bits & 0x80000000UL) == 0) {
                *keys = bits | 0x80000000UL;
            } else {
                *keys = 0x7fffffffUL - (bits & 0x7fffffffUL);
            }
            triangle++;
            keys++;
            count--;
        } while (count != 0);
    }
}

// FUNCTION: SURRENDER 0x10025260
void srGERD::Renderer::expandTriangles(const TriInput& input, int sorted)
{
    IndexWrite write;
    indices_54_.alloc(write, input.triangle_count_00 * input.record_count_04);
    const srTriMeshPipeline::Pass* passes =
        static_cast<const srTriMeshPipeline::Pass*>(input.passes_18);
    unsigned long record = 0;
    while (record < input.record_count_04 && passes[record].poly_uv_1c == 0) {
        record++;
    }
    if (record == input.record_count_04) {
        /* Flat path: gather the chunk once, then replicate it per record
           with the record's vertex offset. */
        for (unsigned long done = 0; done < input.triangle_count_00; done += 0x100) {
            unsigned long chunk = input.triangle_count_00 - done;
            if (chunk > 0x100) {
                chunk = 0x100;
            }
            if (input.position_is_float3_1c == 0) {
                gatherIndexedTriangles(write.triangles_00 + done, input.triangles_10,
                                       input.indices_0c + done, input.vertices_14, first_vertex_c0_,
                                       chunk);
            } else {
                gatherTriangles(write.triangles_00 + done, input.triangles_10,
                                input.indices_0c + done, first_vertex_c0_, chunk);
            }
            if (sorted != 0) {
                const srVector4T<float>* positions = &vertices_78_.positions_10[0];
                sortKeys(write.sort_key_08 + done, write.triangles_00 + done, positions,
                         reinterpret_cast<const float&>(input.value_24), chunk);
                for (unsigned long replica = 1; replica < input.record_count_04; replica++) {
                    offsetIndices(write.sort_key_08 + input.triangle_count_00 * replica + done,
                                  write.sort_key_08 + done, replica, chunk);
                }
            }
            for (unsigned long replica = 1; replica < input.record_count_04; replica++) {
                offsetTriangles(write.triangles_00 + input.triangle_count_00 * replica + done,
                                write.triangles_00 + done, replica * input.vertex_count_08, chunk);
            }
        }
    } else {
        /* Dedup path: any pass with a poly-UV corner table reuses the
           designated source corner's vertex and allocates fresh slots for
           the others; remap_10_ carries six dwords per triangle — the new
           vertices' batch positions, then their corner-source indices. */
        unsigned long free_vertex =
            input.record_count_04 * input.vertex_count_08 + first_vertex_c0_;
        unsigned long* remap = remap_10_.ensure(input.triangle_count_00 * 6);
        unsigned long* corner_remap = remap + input.triangle_count_00 * 3;
        unsigned long written = 0;
        for (record = 0; record < input.record_count_04; record++) {
            const srTriMeshPipeline::Pass& pass = passes[record];
            unsigned long vertex_base = record * input.vertex_count_08 + first_vertex_c0_;
            if (pass.poly_uv_1c != 0) {
                const srVector3i* poly_uv = pass.poly_uv_1c;
                unsigned long new_count = 0;
                unsigned long next = free_vertex;
                unsigned long base = free_vertex;
                unsigned long* remap_out = remap;
                unsigned long* corner_out = corner_remap;
                for (unsigned long index = 0; index < input.triangle_count_00; index++) {
                    const srVector3i& triangle = input.triangles_10[input.indices_0c[index]];
                    const srVector3i& uv = poly_uv[input.indices_0c[index]];
                    srVector3i& out = write.triangles_00[written + index];
                    unsigned long mapped = input.vertices_14[triangle.x] + vertex_base;
                    if (triangle.x == uv.x) {
                        out.x = mapped;
                    } else {
                        *remap_out++ = mapped;
                        *corner_out++ = uv.x;
                        out.x = next++;
                        new_count++;
                    }
                    mapped = input.vertices_14[triangle.y] + vertex_base;
                    if (triangle.y == uv.y) {
                        out.y = mapped;
                    } else {
                        *remap_out++ = mapped;
                        *corner_out++ = uv.y;
                        out.y = next++;
                        new_count++;
                    }
                    mapped = input.vertices_14[triangle.z] + vertex_base;
                    if (triangle.z == uv.z) {
                        out.z = mapped;
                    } else {
                        *remap_out++ = mapped;
                        *corner_out++ = uv.z;
                        out.z = next++;
                        new_count++;
                    }
                }
                if (new_count != 0) {
                    srVertexArray arrays;
                    vertices_78_.alloc(arrays, new_count);
                    srVector4T<float>* diffuse = &vertices_78_.diffuse_00[0];
                    srVector4T<float>* specular = &vertices_78_.specular_08[0];
                    srVector4T<float>* positions = &vertices_78_.positions_10[0];
                    srVector2T<float>* st0 = &vertices_78_.st_18[0][0];
                    srVector2T<float>* st1 = &vertices_78_.st_18[1][0];
                    float* q0 = &vertices_78_.q_28[0][0];
                    float* q1 = &vertices_78_.q_28[1][0];
                    unsigned char* packed = &vertices_78_.packed_38[0];
                    srVectorProcessor::copyIndexed(positions + base, positions, remap, new_count);
                    srVectorProcessor::copyIndexed(diffuse + base, diffuse, remap, new_count);
                    srVectorProcessor::copyIndexed(specular + base, specular, remap, new_count);
                    srVectorProcessor::copyIndexed(st1 + base, st1, remap, new_count);
                    /* reinterpret-ok: the q stream is a dword stream to the
                       indexed copy. */
                    srVectorProcessor::copyIndexed(reinterpret_cast<unsigned long*>(q1 + base),
                                                   reinterpret_cast<const unsigned long*>(q1),
                                                   remap, new_count);
                    for (unsigned long index = 0; index < new_count; index++) {
                        packed[base + index] = packed[remap[index]];
                    }
                    srVectorProcessor::copyIndexed(st0 + base, pass.st_18, corner_remap, new_count);
                    /* reinterpret-ok: 1.0f's bit pattern fills the q0
                       stream. */
                    srVectorProcessor::copy(reinterpret_cast<unsigned long*>(q0 + base), 0x3f800000,
                                            new_count);
                }
                if (sorted != 0) {
                    const srVector4T<float>* positions = &vertices_78_.positions_10[0];
                    sortKeys(write.sort_key_08 + written, write.triangles_00 + written, positions,
                             reinterpret_cast<const float&>(input.value_24),
                             input.triangle_count_00);
                }
                free_vertex = base + new_count;
            } else {
                gatherIndexedTriangles(write.triangles_00 + written, input.triangles_10,
                                       input.indices_0c, input.vertices_14, vertex_base,
                                       input.triangle_count_00);
                if (sorted != 0) {
                    const srVector4T<float>* positions = &vertices_78_.positions_10[0];
                    sortKeys(write.sort_key_08 + written, write.triangles_00 + written,
                             /* reinterpret-ok: sort bias arrives as float
                                bits in value_24. */
                             positions, reinterpret_cast<const float&>(input.value_24),
                             input.triangle_count_00);
                }
            }
            written += input.triangle_count_00;
        }
    }
    unsigned long texture_set_offset = 0;
    for (record = 0; record < input.record_count_04; record++) {
        assignTextureSets(write.texture_set_04 + texture_set_offset, input.indices_0c,
                          input.triangle_count_00, passes + record);
        texture_set_offset += input.triangle_count_00;
    }
}

// FUNCTION: SURRENDER 0x100259D0
void srGERD::Renderer::transformVertices(const TriInput& input, unsigned char* clip_flags)
{
    srVector4T<float>* write = &vertices_78_.positions_10[first_vertex_c0_];
    const float* matrix =
        /* reinterpret-ok: the projection matrix is consumed elementwise. */
        reinterpret_cast<const float*>(input.project_clip_near_20);
    unsigned long zero_mask = 0;
    unsigned long bit = 1;
    for (long row = 0; row < 4; row++) {
        if (matrix[row * 4] == 0.0f) {
            zero_mask |= bit;
        }
        if (matrix[row * 4 + 1] == 0.0f) {
            zero_mask |= bit * 2;
        }
        if (matrix[row * 4 + 2] == 0.0f) {
            zero_mask |= bit * 4;
        }
        if (matrix[row * 4 + 3] == 0.0f) {
            zero_mask |= bit * 8;
        }
        bit *= 0x10;
    }
    int mode;
    if (zero_mask == 0x7bde && matrix[0] == 1.0f && matrix[5] == 1.0f && matrix[10] == 1.0f &&
        matrix[15] == 1.0f) {
        mode = 4;
    } else if ((zero_mask & 0xb39a) == 0xb39a) {
        mode = 6;
    } else if ((zero_mask & 0x7356) == 0x7356) {
        mode = 5;
    } else if ((zero_mask & 0x7000) != 0x7000) {
        mode = 0;
    } else {
        mode = 3;
        if (matrix[15] != 1.0f) {
            mode = 0;
        }
    }
    for (unsigned long done = 0; done < input.vertex_count_08; done += 0x80) {
        unsigned long chunk = input.vertex_count_08 - done;
        if (chunk > 0x80) {
            chunk = 0x80;
        }
        if (chunk != 0 && mode != 4) {
            if (mode == 5) {
                srVectorProcessor::transformOrtho(write, write, *input.project_clip_near_20, chunk);
            } else if (mode == 6) {
                srVectorProcessor::transformPerspective(write, write, *input.project_clip_near_20,
                                                        chunk);
            } else {
                srVectorProcessor::transform(write, write, *input.project_clip_near_20, chunk);
            }
        }
        srVectorProcessor::srGetClipFlags(clip_flags + done, write, chunk);
        for (unsigned long replica = 1; replica < input.record_count_04; replica++) {
            srVector4T<float>* destination = write + replica * input.vertex_count_08;
            if (chunk != 0 && destination != write) {
                srVectorProcessor::memcopy(destination, write, chunk * 0x10);
            }
        }
        write += 0x80;
    }
}

/* Retail 0x10025C00: all clip-flag bytes share a clip plane — ANDs them in
   four-wide groups with an early out, then the scalar tail. */
// FUNCTION: SURRENDER 0x10025C00
static int fullyClipped(const unsigned char* flags, unsigned long count)
{
    unsigned char mask = 0x3f;
    unsigned long index = 0;
    if ((count & ~3UL) != 0) {
        do {
            mask &= flags[index] & flags[index + 1] & flags[index + 2] & flags[index + 3];
            if (mask == 0) {
                return 0;
            }
            index += 4;
        } while (index < (count & ~3UL));
    }
    for (; index < count; index++) {
        mask &= flags[index];
    }
    return mask != 0;
}

/* OR the per-vertex packed attribute bytes into the aggregate mask the DD
   dispatch tests; the bulk loop folds dword loads back to one byte. */
// FUNCTION: SURRENDER 0x10025CB0
static unsigned long attributeMask(const unsigned char* packed, unsigned long count)
{
    unsigned long mask = 0;
    if (count > 3) {
        // reinterpret-ok: the packed byte stream is folded through word loads.
        const unsigned long* words = reinterpret_cast<const unsigned long*>(packed);
        for (unsigned long i = 0; i < count / 4; i++) {
            mask |= words[i];
        }
        mask = (mask & 0xff) | ((((mask & 0xff0000) | (mask >> 8)) >> 8 | (mask & 0xff00)) >> 8);
    }
    for (unsigned long i = count & ~3UL; i < count; i++) {
        mask |= packed[i];
    }
    return mask;
}

// FUNCTION: SURRENDER 0x10025D50
void srGERD::Renderer::drawImmediate()
{
    unsigned long count = indices_54_.count_20;
    if (count != 0) {
        /* The [0] probes force the batch streams to their initial capacity. */
        const srVector3i* indices = &indices_54_.triangles_00[0];
        const unsigned long* texture_set = &indices_54_.texture_set_08[0];
        indices_54_.sort_key_10[0];
        indices_54_.aux_18[0];
        const TextureSet& first = texture_sets_44_.sets_04.data[texture_set[0]];
        texture0_c4_ = first.texture0_00;
        texture1_c8_ = first.texture1_04;
        shader_cc_ = first.shader_08;
        gerd_d4_->setTexture(texture0_c4_, 0);
        gerd_d4_->setTexture(texture1_c8_, 1);
        gerd_d4_->setShader(shader_cc_);
        if (srVectorProcessor::isEqual(texture_set, texture_set[0], count) != 0) {
            gerd_d4_->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, count * 3,
                                   /* The batcher's only retail index type. */
                                   static_cast<srRendererDefs::e_indexType>(2), indices);
            return;
        }
        unsigned long run = 0;
        while (run < count) {
            unsigned long id = texture_set[run];
            bindTextureSet(id);
            unsigned long length = 1 + firstMismatch(texture_set + run + 1, id, count - run - 1);
            gerd_d4_->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, length * 3,
                                   static_cast<srRendererDefs::e_indexType>(2), indices + run);
            run += length;
        }
    }
}

// FUNCTION: SURRENDER 0x10025F40
void srGERD::Renderer::drawSorted()
{
    unsigned long count = indices_54_.count_20;
    if (count != 0) {
        const srVector3i* triangles = &indices_54_.triangles_00[0];
        unsigned long* texture_set = &indices_54_.texture_set_08[0];
        unsigned long* sort_key = &indices_54_.sort_key_10[0];
        indices_54_.aux_18[0];
        unsigned long* order =
            static_cast<unsigned long*>(::operator new(count * sizeof(unsigned long)));
        unsigned long index = 0;
        for (; index < count; index++) {
            order[index] = index;
        }
        if (count > 1) {
            SortPair* pairs = static_cast<SortPair*>(srHeap.allocate(count * 8));
            for (index = 0; index < count; index++) {
                pairs[index].index_00 = order[index];
                pairs[index].key_04 = sort_key[index];
            }
            sortPairs(pairs, count);
            for (index = 0; index < count; index++) {
                order[index] = pairs[index].index_00;
                sort_key[index] = pairs[index].key_04;
            }
            srHeap.free(pairs);
        }
        const TextureSet& first = texture_sets_44_.sets_04.data[texture_set[order[0]]];
        texture0_c4_ = first.texture0_00;
        texture1_c8_ = first.texture1_04;
        shader_cc_ = first.shader_08;
        gerd_d4_->setTexture(texture0_c4_, 0);
        gerd_d4_->setTexture(texture1_c8_, 1);
        gerd_d4_->setShader(shader_cc_);
        /* 0x200 index triples per submission chunk (0x1800 bytes). */
        srVector3i* batch = static_cast<srVector3i*>(srHeap.allocate(0x1800));
        if (srVectorProcessor::isEqual(texture_set, texture_set[order[0]], count) != 0) {
            unsigned long offset = 0;
            do {
                unsigned long chunk = count - offset;
                if (chunk > 0x200) {
                    chunk = 0x200;
                }
                gatherTriangles(batch, triangles, order + offset, 0, chunk);
                gerd_d4_->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, chunk * 3,
                                       static_cast<srRendererDefs::e_indexType>(2), batch);
                offset += 0x200;
            } while (offset < count);
        } else {
            unsigned long run = 0;
            do {
                unsigned long id = texture_set[order[run]];
                bindTextureSet(id);
                unsigned long limit = count - run - 1;
                if (limit > 0x1ff) {
                    limit = 0x1ff;
                }
                unsigned long length =
                    1 + firstMismatchSorted(texture_set, id, order + run + 1, limit);
                gatherTriangles(batch, triangles, order + run, 0, length);
                gerd_d4_->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, length * 3,
                                       static_cast<srRendererDefs::e_indexType>(2), batch);
                run += length;
            } while (run < count);
        }
        srHeap.free(batch);
        ::operator delete(order);
    }
}

/* Program the GERD vertex-array slots from the bound batch: diffuse,
   specular (+ its w alone when only the alpha attribute is present) and a
   per-stage {st} or repacked {st,q} texcoord stream. */
// FUNCTION: SURRENDER 0x10026360
void srGERD::Renderer::programVertexArrays(srVertexArray* arrays, unsigned long count)
{
    unsigned long attributes = attributeMask(arrays->packed_1c, count);
    if (texture_stages_e0_ < 2) {
        /* Single-stage devices cannot take stage-1 st/q streams. */
        attributes &= ~0x50UL;
    }
    srFlags<srRendererDefs::e_vertexArray> mask(1 << srRendererDefs::VERTEX_ARRAY_POSITIONS);
    gerd_d4_->setClipState(srFlags<srRendererDefs::e_clip>(clip_state_d0_));
    gerd_d4_->setVertexPointer(4, srRendererDefs::TYPE_FLOAT, 0x10, arrays->eye_locations_00,
                               static_cast<long>(count));
    if ((attributes & 1) != 0) {
        gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_DIFFUSE, 4, srRendererDefs::TYPE_FLOAT,
                             0x10, arrays->diffuse_04);
        mask.set(srRendererDefs::VERTEX_ARRAY_DIFFUSE, 1);
    }
    if ((attributes & 2) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_SPECULAR, 1);
        gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_SPECULAR, ((attributes & 4) != 0) + 3,
                             srRendererDefs::TYPE_FLOAT, 0x10, arrays->specular_08);
    } else if ((attributes & 4) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_SPECULAR_ALPHA, 1);
        gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_SPECULAR_ALPHA, 1,
                             srRendererDefs::TYPE_FLOAT, 0x10, &arrays->specular_08->w);
    }
    if ((attributes & 8) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 1);
        if ((attributes & 0x20) == 0) {
            gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 2,
                                 srRendererDefs::TYPE_FLOAT, 8, arrays->st0_0c);
        } else {
            TexCoordQ* stq = stq_18_[0].ensure(count);
            for (unsigned long i = 0; i < count; i++) {
                stq[i].st_00 = arrays->st0_0c[i];
                stq[i].q_08 = arrays->q0_14[i];
            }
            gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 3,
                                 srRendererDefs::TYPE_FLOAT, 0xc, stq);
        }
    }
    if ((attributes & 0x10) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 1);
        if ((attributes & 0x40) == 0) {
            gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 2,
                                 srRendererDefs::TYPE_FLOAT, 8, arrays->st1_10);
        } else {
            TexCoordQ* stq = stq_18_[1].ensure(count);
            for (unsigned long i = 0; i < count; i++) {
                stq[i].st_00 = arrays->st1_10[i];
                stq[i].q_08 = arrays->q1_18[i];
            }
            gerd_d4_->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 3,
                                 srRendererDefs::TYPE_FLOAT, 0xc, stq);
        }
    }
    gerd_d4_->setVertexArrayMask(mask);
}

// FUNCTION: SURRENDER 0x100266E0
void srGERD::Renderer::submit()
{
    unsigned long vertex_count = vertices_78_.count_40;
    statistics_28_[4] += 1;
    statistics_28_[5] += vertex_count;
    statistics_28_[6] += indices_54_.count_20;
    if (gerd_d4_ != 0 && vertex_count != 0 && indices_54_.count_20 != 0) {
        e_matrixMode saved_mode = gerd_d4_->getMatrixMode();
        gerd_d4_->matrixMode(MATRIX_PROJECTION);
        gerd_d4_->pushMatrix();
        gerd_d4_->loadIdentity();
        srVertexArray arrays;
        vertices_78_.bind(arrays, 0);
        programVertexArrays(&arrays, vertex_count);
        e_cullMode saved_cull = gerd_d4_->getCullMode();
        gerd_d4_->setCullMode(CULL_FRONT);
        if (sorted_d8_ == 0) {
            drawImmediate();
        } else {
            drawSorted();
        }
        gerd_d4_->popMatrix();
        gerd_d4_->matrixMode(saved_mode);
        gerd_d4_->setCullMode(saved_cull);
    }
    gerd_d4_->getDD()->fence();
    indices_54_.reset(0);
    vertices_78_.count_40 = 0;
    clip_state_d0_ = 0;
}

// FUNCTION: SURRENDER 0x100268A0
void srGERD::Renderer::reset(int release_buffers)
{
    indices_54_.reset(release_buffers);
    if (release_buffers != 0) {
        vertices_78_.diffuse_00.release();
        vertices_78_.specular_08.release();
        vertices_78_.positions_10.release();
        vertices_78_.st_18[0].release();
        vertices_78_.st_18[1].release();
        vertices_78_.q_28[0].release();
        vertices_78_.q_28[1].release();
        vertices_78_.packed_38.release();
        vertices_78_.capacity_44 = 0;
    }
    vertices_78_.count_40 = 0;
    texture_sets_44_.clear();
    if (release_buffers != 0) {
        bytes_00_.release();
        dwords_08_.release();
        remap_10_.release();
        stq_18_[0].release();
        stq_18_[1].release();
    }
}

/* Retail 0x10026A00: copy the indices of the not-fully-clipped triangles —
   a triangle drops out only when all three corners share a clip flag. With
   position_is_float3_1c clear the corners index the flag array through the
   vertices remap. */
// FUNCTION: SURRENDER 0x10026A00
static unsigned long filterTriangles(unsigned long* destination, const unsigned char* flags,
                                     const srGERD::Renderer::TriInput& input)
{
    unsigned long kept = 0;
    if (input.position_is_float3_1c == 0) {
        for (unsigned long index = 0; index < input.triangle_count_00; index++) {
            const srVector3i& triangle = input.triangles_10[input.indices_0c[index]];
            unsigned char flag = flags[input.vertices_14[triangle.x]];
            if (flag == 0 || (flag & flags[input.vertices_14[triangle.z]] &
                              flags[input.vertices_14[triangle.y]]) == 0) {
                destination[kept] = input.indices_0c[index];
                kept++;
            }
        }
    } else {
        for (unsigned long index = 0; index < input.triangle_count_00; index++) {
            const srVector3i& triangle = input.triangles_10[input.indices_0c[index]];
            unsigned char flag = flags[triangle.x];
            if (flag == 0 || (flag & flags[triangle.z] & flags[triangle.y]) == 0) {
                destination[kept] = input.indices_0c[index];
                kept++;
            }
        }
    }
    return kept;
}

// FUNCTION: SURRENDER 0x10026B30
void srGERD::Renderer::render(const TriInput& input)
{
    if (first_vertex_c0_ == -1) {
        return;
    }
    if (input.triangle_count_00 == 0) {
        vertices_78_.count_40 += first_vertex_c0_ - vertices_78_.count_40;
        first_vertex_c0_ = -1;
        return;
    }
    statistics_28_[0] += 1;
    statistics_28_[1] += input.triangle_count_00 * input.record_count_04;
    statistics_28_[3] += input.vertex_count_08 * input.record_count_04;
    unsigned char* flags = bytes_00_.ensure(input.vertex_count_08);
    transformVertices(input, flags);
    if (fullyClipped(flags, input.vertex_count_08) != 0) {
        vertices_78_.count_40 += first_vertex_c0_ - vertices_78_.count_40;
        first_vertex_c0_ = -1;
        return;
    }
    const TriInput* batch = &input;
    TriInput filtered;
    unsigned long mask = attributeMask(flags, input.vertex_count_08);
    if (mask != 0) {
        filtered = input;
        /* const_cast-ok: the filtered list buffer was just reserved. */
        filtered.indices_0c = dwords_08_.ensure(input.triangle_count_00);
        filtered.triangle_count_00 =
            filterTriangles(const_cast<unsigned long*>(filtered.indices_0c), flags, input);
        if (filtered.triangle_count_00 != 0) {
            clip_state_d0_ |= mask;
            batch = &filtered;
        } else {
            vertices_78_.count_40 += first_vertex_c0_ - vertices_78_.count_40;
            first_vertex_c0_ = -1;
            return;
        }
    }
    if (gerd_d4_->pick_176c_.pick_depth_280_ != 0) {
        PickInput pick;
        pick.indices_00 = batch->indices_0c;
        pick.triangles_04 = batch->triangles_10;
        pick.triangle_count_08 = batch->triangle_count_00;
        pick.vertices_0c = batch->vertices_14;
        pick.positions_10 = &vertices_78_.positions_10[first_vertex_c0_];
        pick.vertex_count_14 = batch->vertex_count_08;
        gerd_d4_->performPickTest(pick);
    }
    expandTriangles(*batch, sorted_d8_ != 0);
    first_vertex_c0_ = -1;
}

/* Retail 0x100278E0: mark `bit` in output wherever the indexed table value
   changes between adjacent entries; when `initialized` is zero the output
   chunk is cleared first. */
// FUNCTION: SURRENDER 0x100278E0
static void markTransitions(unsigned long* output, const unsigned long* indices,
                            const unsigned long* table, unsigned long bit, unsigned long count,
                            unsigned long initialized)
{
    if (initialized == 0 && count != 0) {
        fillConstant(output, 0, count);
    }
    unsigned long previous = table[indices[0]];
    output[0] |= bit;
    unsigned long index = 1;
    for (; index < (count & ~3UL); index += 4) {
        unsigned long value = table[indices[index]];
        if (value != previous) {
            output[index] |= bit;
            previous = value;
        }
        value = table[indices[index + 1]];
        if (value != previous) {
            output[index + 1] |= bit;
            previous = value;
        }
        value = table[indices[index + 2]];
        if (value != previous) {
            output[index + 2] |= bit;
            previous = value;
        }
        value = table[indices[index + 3]];
        if (value != previous) {
            output[index + 3] |= bit;
            previous = value;
        }
    }
    for (; index < count; index++) {
        unsigned long value = table[indices[index]];
        if (value != previous) {
            output[index] |= bit;
            previous = value;
        }
    }
}

/* Retail 0x10027BA0: the guarded dword fill — the vector processor's _copy
   emits nothing for a zero count. */
// FUNCTION: SURRENDER 0x10027BA0
static void fillConstant(unsigned long* destination, unsigned long value, unsigned long count)
{
    if (count != 0) {
        srVectorProcessor::copy(destination, value, count);
    }
}

// FUNCTION: SURRENDER 0x10027CF0
void srGERD::Renderer::VertexArrays::bind(srVertexArray& arrays, unsigned long base)
{
    arrays.diffuse_04 = &diffuse_00[base];
    arrays.specular_08 = &specular_08[base];
    arrays.eye_locations_00 = &positions_10[base];
    arrays.st0_0c = &st_18[0][base];
    arrays.st1_10 = &st_18[1][base];
    arrays.q0_14 = &q_28[0][base];
    arrays.q1_18 = &q_28[1][base];
    arrays.packed_1c = &packed_38[base];
}

// FUNCTION: SURRENDER 0x10027ED0
void srGERD::Renderer::bindTextureSet(unsigned long index)
{
    const TextureSet& set = texture_sets_44_.sets_04.data[index];
    if (set.texture0_00 != texture0_c4_) {
        texture0_c4_ = set.texture0_00;
        gerd_d4_->setTexture(texture0_c4_, 0);
    }
    if (set.texture1_04 != texture1_c8_) {
        texture1_c8_ = set.texture1_04;
        gerd_d4_->setTexture(texture1_c8_, 1);
    }
    if (set.shader_08.value != shader_cc_.value) {
        shader_cc_ = set.shader_08;
        gerd_d4_->setShader(shader_cc_);
    }
}

/* Compiler-generated memberwise teardown; the Renderer releases its members
   individually, the nested records theirs. */
// SYNTHETIC: SURRENDER 0x10024AE0
// srGERD::Renderer::IndexBatch::~IndexBatch

// SYNTHETIC: SURRENDER 0x10024B30
// srGERD::Renderer::VertexArrays::~VertexArrays

// SYNTHETIC: SURRENDER 0x10024BF0
// srGERD::Renderer::~Renderer
