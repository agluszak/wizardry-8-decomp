#include "surrender/srGERD.h"

#include <string.h>

#include "surrender/srVectorProcessor.h"

/* drawSorted's {order, sort_key} pair record; retail folds this TU's
   radix-sort emission into huffman.cpp's identical sortSymbolPairs at
   0x100027F0, which is why the retail drawSorted calls that address. */
struct SortPair {
    unsigned long index;
    unsigned long key;
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
        unsigned char* keys = reinterpret_cast<unsigned char*>(&src[0].key) + pass;
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
    memset(statistics, 0, sizeof(statistics));
}

// FUNCTION: SURRENDER 0x10024260
void srGERD::Renderer::getStatistics(unsigned long* statistics)
{
    for (int i = 0; i < 7; i++) {
        statistics[i] = this->statistics[i];
    }
}

// FUNCTION: SURRENDER 0x10024280
unsigned long srGERD::Renderer::TextureSetCache::intern(const TextureSetKey& key)
{
    srHashTable<TextureSetKey, unsigned long>* map = this->map;
    int slot = map->FindNextEntry(&key, -1);
    if (slot != -1) {
        return map->entries[slot].value;
    }

    unsigned long index = count;
    map->Insert(&key, &index);

    TextureSet& set = sets[index];
    set.texture0 = key.texture0;
    set.texture1 = key.texture1;
    set.shader = key.shader;
    switch ((key.shader.value >> srShader::DSTBLEND_SHIFT) & 7) {
    case srShader::DSTBLEND_ONE:
        set.blend = 2;
        break;
    case srShader::DSTBLEND_SRC_COLOR:
    case srShader::DSTBLEND_ONE_MINUS_SRC_COLOR:
        set.blend = 3;
        break;
    case srShader::DSTBLEND_SRC_ALPHA:
    case srShader::DSTBLEND_ONE_MINUS_SRC_ALPHA:
        set.blend = 1;
        break;
    default:
        set.blend = 0;
        break;
    }
    count += 1;
    return index;
}

// FUNCTION: SURRENDER 0x10024460
void srGERD::Renderer::IndexBatch::alloc(IndexWrite& write, unsigned long count)
{
    unsigned long needed = count + 0x40 + this->count;
    if (triangles.capacity <= needed) {
        triangles.setCapacity(triangles.capacity + 8 + needed);
    }
    if (texture_set.capacity <= needed) {
        texture_set.setCapacity(texture_set.capacity + 8 + needed);
    }
    if (sort_key.capacity <= needed) {
        sort_key.setCapacity(sort_key.capacity + 8 + needed);
    }
    if (aux.capacity <= needed) {
        aux.setCapacity(aux.capacity + 8 + needed);
    }
    write.triangles = &triangles[this->count];
    write.texture_set = &texture_set[this->count];
    write.sort_key = &sort_key[this->count];
    write.aux = &aux[this->count];
    this->count += count;
}

// FUNCTION: SURRENDER 0x10024620
void srGERD::Renderer::IndexBatch::reset(int release)
{
    if (release != 0) {
        triangles.release();
        texture_set.release();
        sort_key.release();
        aux.release();
    }
    count = 0;
}

/* Vertex-stream teardown expanded in Renderer::reset. The method name is
   descriptive, following the companion IndexBatch operation. */
void srGERD::Renderer::VertexArrays::reset(int release)
{
    if (release != 0) {
        diffuse.release();
        specular.release();
        positions.release();
        st[0].release();
        st[1].release();
        q[0].release();
        q[1].release();
        attributes.release();
        capacity = 0;
    }
    count = 0;
}

// FUNCTION: SURRENDER 0x10024680
void srGERD::Renderer::VertexArrays::alloc(srVertexArray& arrays, unsigned long count)
{
    unsigned long needed = count + this->count;
    if (capacity < needed) {
        needed += 0x40;
        if (diffuse.capacity <= needed) {
            diffuse.setCapacity(diffuse.capacity + 8 + needed);
        }
        if (specular.capacity <= needed) {
            specular.setCapacity(specular.capacity + 8 + needed);
        }
        if (positions.capacity <= needed) {
            positions.setCapacity(positions.capacity + 8 + needed);
        }
        if (st[0].capacity <= needed) {
            st[0].setCapacity(st[0].capacity + 8 + needed);
        }
        if (st[1].capacity <= needed) {
            st[1].setCapacity(st[1].capacity + 8 + needed);
        }
        if (q[0].capacity <= needed) {
            q[0].setCapacity(q[0].capacity + 8 + needed);
        }
        if (q[1].capacity <= needed) {
            q[1].setCapacity(q[1].capacity + 8 + needed);
        }
        if (attributes.capacity <= needed) {
            attributes.setCapacity(attributes.capacity + 8 + needed);
        }

        unsigned long added = needed - capacity;
        /* reinterpret-ok: the dword fill is the shader-agnostic byte fill the
           vector processor exposes for 0x10-stride records. */
        fillConstant(reinterpret_cast<unsigned long*>(&diffuse[capacity]), 0, added * 4);
        /* reinterpret-ok: as above. */
        fillConstant(reinterpret_cast<unsigned long*>(&specular[capacity]), 0, added * 4);
        srVector4T<float> eye_default;
        eye_default.Set(0.0f, 0.0f, 0.0f, 1.0f);
        if (added != 0) {
            srVectorProcessor::copy(&positions[capacity], eye_default, added);
        }
        /* reinterpret-ok: 1.0f's bit pattern goes in through the dword
           fill. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&q[0][capacity]), 0x3f800000,
                                added);
        /* reinterpret-ok: as above. */
        srVectorProcessor::copy(reinterpret_cast<unsigned long*>(&q[1][capacity]), 0x3f800000,
                                added);
        srVectorProcessor::copy(&st[0][capacity], srVector2T<float>(0.0f, 0.0f), added);
        srVectorProcessor::copy(&st[1][capacity], srVector2T<float>(0.0f, 0.0f), added);
        capacity = needed;
    }
    bind(arrays, this->count);
    this->count += count;
}

// FUNCTION: SURRENDER 0x10024900
srGERD::Renderer::Renderer(const Parameters& parameters)
    : gerd(parameters.gerd), sorted(parameters.sorted), batch_limit(parameters.batch_limit),
      texture_stages(parameters.texture_stages)
{
    clip_state = 0;
    first_vertex = -1;
    memset(statistics, 0, sizeof(statistics));
    texture0 = 0;
    texture1 = 0;
    shader = srShader();
}

// FUNCTION: SURRENDER 0x10024DB0
int srGERD::Renderer::isBatchFull() const
{
    if (sorted == 0 && batch_limit < vertices.count) {
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10024DE0
void srGERD::Renderer::rewindVertexArray(unsigned long count)
{
    if (first_vertex != -1) {
        vertices.count -= count;
    }
}

// FUNCTION: SURRENDER 0x10024E00
void srGERD::Renderer::allocVertexArray(srVertexArray& arrays, unsigned long count)
{
    first_vertex = vertices.count;
    vertices.alloc(arrays, count);
}

// FUNCTION: SURRENDER 0x10024E30
void srGERD::Renderer::assignTextureSets(unsigned long* texture_set, const unsigned long* indices,
                                         unsigned long count, const srTriMeshPipeline::Pass* pass)
{
    enum { TABLE_TEXTURE0 = 1u, TABLE_TEXTURE1 = 2u, TABLE_SHADER = 4u };
    TextureSetKey key;
    key.texture0 = pass->textures[0];
    key.texture1 = pass->textures[1];
    key.shader = pass->shader;
    unsigned char mask = pass->texture_tables[0] != 0 ? TABLE_TEXTURE0 : 0;
    if (pass->texture_tables[1] != 0) {
        mask |= TABLE_TEXTURE1;
    }
    if (pass->shaders != 0) {
        mask |= TABLE_SHADER;
    }
    if (mask == 0) {
        unsigned long set = texture_sets.intern(key);
        if (count != 0) {
            srVectorProcessor::copy(texture_set, set, count);
        }
        return;
    }
    unsigned long set = texture_sets.intern(key);
    unsigned long done = 0;
    while (done < count) {
        unsigned long chunk = count - done;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        const unsigned long* chunk_indices = indices + done;
        unsigned long* out = texture_set + done;
        unsigned long initialized = 0;
        if ((mask & TABLE_TEXTURE0) != 0) {
            markTransitions(out, chunk_indices,
                            static_cast<const unsigned long*>(pass->texture_tables[0]),
                            TABLE_TEXTURE0, chunk, initialized);
            initialized = 1;
        }
        if ((mask & TABLE_TEXTURE1) != 0) {
            markTransitions(out, chunk_indices,
                            static_cast<const unsigned long*>(pass->texture_tables[1]),
                            TABLE_TEXTURE1, chunk, initialized);
            initialized += 1;
        }
        if ((mask & TABLE_SHADER) != 0) {
            markTransitions(out, chunk_indices,
                            reinterpret_cast<const unsigned long*>(pass->shaders), TABLE_SHADER,
                            chunk,
                            initialized); // reinterpret-ok: packed srShader words at 0x100278E0.
        }
        if (chunk != 0) {
            for (unsigned long index = 0; index < chunk; index++) {
                unsigned long changed = out[index];
                if (changed != 0) {
                    unsigned long vertex = chunk_indices[index];
                    if ((changed & TABLE_TEXTURE0) != 0) {
                        /* reinterpret-ok: the stage-0 table entries are the
                           texture interface pointers the key stores. */
                        key.texture0 = reinterpret_cast<srTextureIFace* const*>(
                            pass->texture_tables[0])[vertex];
                    }
                    if ((changed & TABLE_TEXTURE1) != 0) {
                        /* reinterpret-ok: same table form for stage 1. */
                        key.texture1 = reinterpret_cast<srTextureIFace* const*>(
                            pass->texture_tables[1])[vertex];
                    }
                    if ((changed & TABLE_SHADER) != 0) {
                        key.shader = pass->shaders[vertex];
                    }
                    set = texture_sets.intern(key);
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
    indices.alloc(write, input.triangle_count * input.record_count);
    const srTriMeshPipeline::Pass* passes = input.passes;
    unsigned long record = 0;
    while (record < input.record_count && passes[record].poly_uv == 0) {
        record++;
    }
    if (record == input.record_count) {
        /* Flat path: gather the chunk once, then replicate it per record
       with the record's vertex offset. */
        for (unsigned long done = 0; done < input.triangle_count; done += 0x100) {
            unsigned long chunk = input.triangle_count - done;
            if (chunk > 0x100) {
                chunk = 0x100;
            }
            if (input.direct_vertex_indices == 0) {
                gatherIndexedTriangles(write.triangles + done, input.triangles,
                                       input.indices + done, input.vertices, first_vertex, chunk);
            } else {
                gatherTriangles(write.triangles + done, input.triangles, input.indices + done,
                                first_vertex, chunk);
            }
            if (sorted != 0) {
                const srVector4T<float>* positions = &vertices.positions[0];
                sortKeys(write.sort_key + done, write.triangles + done, positions, input.sort_bias,
                         chunk);
                for (unsigned long replica = 1; replica < input.record_count; replica++) {
                    offsetIndices(write.sort_key + input.triangle_count * replica + done,
                                  write.sort_key + done, replica, chunk);
                }
            }
            for (unsigned long replica = 1; replica < input.record_count; replica++) {
                offsetTriangles(write.triangles + input.triangle_count * replica + done,
                                write.triangles + done, replica * input.vertex_count, chunk);
            }
        }
    } else {
        /* Dedup path: any pass with a poly-UV corner table reuses the
           designated source corner's vertex and allocates fresh slots for
           the others; remap carries six dwords per triangle — the new
           vertices' batch positions, then their corner-source indices. */
        unsigned long free_vertex = input.record_count * input.vertex_count + first_vertex;
        unsigned long* remap = this->remap.ensure(input.triangle_count * 6);
        unsigned long* corner_remap = remap + input.triangle_count * 3;
        unsigned long written = 0;
        for (record = 0; record < input.record_count; record++) {
            const srTriMeshPipeline::Pass& pass = passes[record];
            unsigned long vertex_base = record * input.vertex_count + first_vertex;
            if (pass.poly_uv != 0) {
                const srVector3i* poly_uv = pass.poly_uv;
                unsigned long new_count = 0;
                unsigned long next = free_vertex;
                unsigned long base = free_vertex;
                unsigned long* remap_out = remap;
                unsigned long* corner_out = corner_remap;
                for (unsigned long index = 0; index < input.triangle_count; index++) {
                    const srVector3i& triangle = input.triangles[input.indices[index]];
                    const srVector3i& uv = poly_uv[input.indices[index]];
                    srVector3i& out = write.triangles[written + index];
                    unsigned long mapped = input.vertices[triangle.x] + vertex_base;
                    if (triangle.x == uv.x) {
                        out.x = mapped;
                    } else {
                        *remap_out++ = mapped;
                        *corner_out++ = uv.x;
                        out.x = next++;
                        new_count++;
                    }
                    mapped = input.vertices[triangle.y] + vertex_base;
                    if (triangle.y == uv.y) {
                        out.y = mapped;
                    } else {
                        *remap_out++ = mapped;
                        *corner_out++ = uv.y;
                        out.y = next++;
                        new_count++;
                    }
                    mapped = input.vertices[triangle.z] + vertex_base;
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
                    vertices.alloc(arrays, new_count);
                    arrays.diffuse = &vertices.diffuse[0];
                    arrays.specular = &vertices.specular[0];
                    arrays.eye_locations = &vertices.positions[0];
                    arrays.st0 = &vertices.st[0][0];
                    arrays.st1 = &vertices.st[1][0];
                    arrays.q0 = &vertices.q[0][0];
                    arrays.q1 = &vertices.q[1][0];
                    arrays.attributes = &vertices.attributes[0];
                    srVectorProcessor::copyIndexed(arrays.eye_locations + base,
                                                   arrays.eye_locations, remap, new_count);
                    srVectorProcessor::copyIndexed(arrays.diffuse + base, arrays.diffuse, remap,
                                                   new_count);
                    srVectorProcessor::copyIndexed(arrays.specular + base, arrays.specular, remap,
                                                   new_count);
                    srVectorProcessor::copyIndexed(arrays.st1 + base, arrays.st1, remap, new_count);
                    /* reinterpret-ok: the q stream is a dword stream to the
                       indexed copy. */
                    srVectorProcessor::copyIndexed(
                        reinterpret_cast<unsigned long*>(arrays.q1 + base),
                        reinterpret_cast<const unsigned long*>(arrays.q1), remap, new_count);
                    for (unsigned long index = 0; index < new_count; index++) {
                        arrays.attributes[base + index] = arrays.attributes[remap[index]];
                    }
                    srVectorProcessor::copyIndexed(arrays.st0 + base, pass.texcoords, corner_remap,
                                                   new_count);
                    /* reinterpret-ok: 1.0f's bit pattern fills the q0
                       stream. */
                    srVectorProcessor::copy(reinterpret_cast<unsigned long*>(arrays.q0 + base),
                                            0x3f800000, new_count);
                }
                if (sorted != 0) {
                    const srVector4T<float>* positions = &vertices.positions[0];
                    sortKeys(write.sort_key + written, write.triangles + written, positions,
                             input.sort_bias, input.triangle_count);
                }
                free_vertex = base + new_count;
            } else {
                gatherIndexedTriangles(write.triangles + written, input.triangles, input.indices,
                                       input.vertices, vertex_base, input.triangle_count);
                if (sorted != 0) {
                    const srVector4T<float>* positions = &vertices.positions[0];
                    sortKeys(write.sort_key + written, write.triangles + written, positions,
                             input.sort_bias, input.triangle_count);
                }
            }
            written += input.triangle_count;
        }
    }
    unsigned long texture_set_offset = 0;
    for (record = 0; record < input.record_count; record++) {
        assignTextureSets(write.texture_set + texture_set_offset, input.indices,
                          input.triangle_count, passes + record);
        texture_set_offset += input.triangle_count;
    }
}

// FUNCTION: SURRENDER 0x100259D0
void srGERD::Renderer::transformVertices(const TriInput& input, unsigned char* clip_flags)
{
    srVector4T<float>* write = &vertices.positions[first_vertex];
    const srMatrix4T<float>& matrix = *input.project_clip_near;
    unsigned long zero_mask = 0;
    unsigned long bit = 1;
    for (long row = 0; row < 4; row++) {
        if (matrix.vectors[row].x == 0.0f) {
            zero_mask |= bit;
        }
        if (matrix.vectors[row].y == 0.0f) {
            zero_mask |= bit * 2;
        }
        if (matrix.vectors[row].z == 0.0f) {
            zero_mask |= bit * 4;
        }
        if (matrix.vectors[row].w == 0.0f) {
            zero_mask |= bit * 8;
        }
        bit *= 0x10;
    }
    srMatrix4T<float>::e_type mode;
    if (zero_mask == 0x7bde && matrix.vectors[0].x == 1.0f && matrix.vectors[1].y == 1.0f &&
        matrix.vectors[2].z == 1.0f && matrix.vectors[3].w == 1.0f) {
        mode = srMatrix4T<float>::TYPE_IDENTITY;
    } else if ((zero_mask & 0xb39a) == 0xb39a) {
        mode = srMatrix4T<float>::TYPE_PERSPECTIVE;
    } else if ((zero_mask & 0x7356) == 0x7356) {
        mode = srMatrix4T<float>::TYPE_ORTHOGRAPHIC;
    } else if ((zero_mask & 0x7000) != 0x7000) {
        mode = srMatrix4T<float>::TYPE_GENERAL;
    } else {
        mode = srMatrix4T<float>::TYPE_AFFINE;
        if (matrix.vectors[3].w != 1.0f) {
            mode = srMatrix4T<float>::TYPE_GENERAL;
        }
    }
    for (unsigned long done = 0; done < input.vertex_count; done += 0x80) {
        unsigned long chunk = input.vertex_count - done;
        if (chunk > 0x80) {
            chunk = 0x80;
        }
        if (chunk != 0 && mode != srMatrix4T<float>::TYPE_IDENTITY) {
            if (mode == srMatrix4T<float>::TYPE_ORTHOGRAPHIC) {
                srVectorProcessor::transformOrtho(write, write, *input.project_clip_near, chunk);
            } else if (mode == srMatrix4T<float>::TYPE_PERSPECTIVE) {
                srVectorProcessor::transformPerspective(write, write, *input.project_clip_near,
                                                        chunk);
            } else {
                srVectorProcessor::transform(write, write, *input.project_clip_near, chunk);
            }
        }
        srVectorProcessor::srGetClipFlags(clip_flags + done, write, chunk);
        for (unsigned long replica = 1; replica < input.record_count; replica++) {
            srVector4T<float>* destination = write + replica * input.vertex_count;
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
    unsigned char mask = srRendererDefs::FRUSTUM_CLIP_MASK;
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
    unsigned long count = this->indices.count;
    if (count != 0) {
        /* The [0] probes force the batch streams to their initial capacity. */
        const srVector3i* indices = &this->indices.triangles[0];
        const unsigned long* texture_set = &this->indices.texture_set[0];
        this->indices.sort_key[0];
        this->indices.aux[0];
        const TextureSet& first = texture_sets.sets.data[texture_set[0]];
        texture0 = first.texture0;
        texture1 = first.texture1;
        shader = first.shader;
        gerd->setTexture(texture0, 0);
        gerd->setTexture(texture1, 1);
        gerd->setShader(shader);
        if (srVectorProcessor::isEqual(texture_set, texture_set[0], count) != 0) {
            gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, count * 3,
                               /* The batcher's only retail index type. */
                               static_cast<srRendererDefs::e_indexType>(2), indices);
            return;
        }
        unsigned long run = 0;
        while (run < count) {
            unsigned long id = texture_set[run];
            bindTextureSet(id);
            unsigned long length = 1 + firstMismatch(texture_set + run + 1, id, count - run - 1);
            gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, length * 3,
                               static_cast<srRendererDefs::e_indexType>(2), indices + run);
            run += length;
        }
    }
}

// FUNCTION: SURRENDER 0x10025F40
void srGERD::Renderer::drawSorted()
{
    unsigned long count = indices.count;
    if (count != 0) {
        const srVector3i* triangles = &indices.triangles[0];
        unsigned long* texture_set = &indices.texture_set[0];
        unsigned long* sort_key = &indices.sort_key[0];
        indices.aux[0];
        unsigned long* order =
            static_cast<unsigned long*>(::operator new(count * sizeof(unsigned long)));
        unsigned long index = 0;
        for (; index < count; index++) {
            order[index] = index;
        }
        if (count > 1) {
            SortPair* pairs = static_cast<SortPair*>(srHeap.allocate(count * 8));
            for (index = 0; index < count; index++) {
                pairs[index].index = order[index];
                pairs[index].key = sort_key[index];
            }
            sortPairs(pairs, count);
            for (index = 0; index < count; index++) {
                order[index] = pairs[index].index;
                sort_key[index] = pairs[index].key;
            }
            srHeap.free(pairs);
        }
        const TextureSet& first = texture_sets.sets.data[texture_set[order[0]]];
        texture0 = first.texture0;
        texture1 = first.texture1;
        shader = first.shader;
        gerd->setTexture(texture0, 0);
        gerd->setTexture(texture1, 1);
        gerd->setShader(shader);
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
                gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, chunk * 3,
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
                gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, length * 3,
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
    unsigned long attributes = attributeMask(arrays->attributes, count);
    if (texture_stages < 2) {
        /* Single-stage devices cannot take stage-1 st/q streams. */
        attributes &= ~(srVertexArray::ATTRIBUTE_ST1 | srVertexArray::ATTRIBUTE_Q1);
    }
    srFlags<srRendererDefs::e_vertexArray> mask(1 << srRendererDefs::VERTEX_ARRAY_POSITIONS);
    gerd->setClipState(srFlags<srRendererDefs::e_clip>(clip_state));
    gerd->setVertexPointer(4, srRendererDefs::TYPE_FLOAT, 0x10, arrays->eye_locations,
                           static_cast<long>(count));
    if ((attributes & srVertexArray::ATTRIBUTE_DIFFUSE) != 0) {
        gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_DIFFUSE, 4, srRendererDefs::TYPE_FLOAT, 0x10,
                         arrays->diffuse);
        mask.set(srRendererDefs::VERTEX_ARRAY_DIFFUSE, 1);
    }
    if ((attributes & srVertexArray::ATTRIBUTE_SPECULAR) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_SPECULAR, 1);
        gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_SPECULAR,
                         ((attributes & srVertexArray::ATTRIBUTE_SPECULAR_ALPHA) != 0) + 3,
                         srRendererDefs::TYPE_FLOAT, 0x10, arrays->specular);
    } else if ((attributes & srVertexArray::ATTRIBUTE_SPECULAR_ALPHA) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_SPECULAR_ALPHA, 1);
        gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_SPECULAR_ALPHA, 1, srRendererDefs::TYPE_FLOAT,
                         0x10, &arrays->specular->w);
    }
    if ((attributes & srVertexArray::ATTRIBUTE_ST0) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 1);
        if ((attributes & srVertexArray::ATTRIBUTE_Q0) == 0) {
            gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 2, srRendererDefs::TYPE_FLOAT,
                             8, arrays->st0);
        } else {
            TexCoordQ* stq = this->stq[0].ensure(count);
            for (unsigned long i = 0; i < count; i++) {
                stq[i].st = arrays->st0[i];
                stq[i].q = arrays->q0[i];
            }
            gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD0, 3, srRendererDefs::TYPE_FLOAT,
                             0xc, stq);
        }
    }
    if ((attributes & srVertexArray::ATTRIBUTE_ST1) != 0) {
        mask.set(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 1);
        if ((attributes & srVertexArray::ATTRIBUTE_Q1) == 0) {
            gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 2, srRendererDefs::TYPE_FLOAT,
                             8, arrays->st1);
        } else {
            TexCoordQ* stq = this->stq[1].ensure(count);
            for (unsigned long i = 0; i < count; i++) {
                stq[i].st = arrays->st1[i];
                stq[i].q = arrays->q1[i];
            }
            gerd->setDataPtr(srRendererDefs::VERTEX_ARRAY_TEXCOORD1, 3, srRendererDefs::TYPE_FLOAT,
                             0xc, stq);
        }
    }
    gerd->setVertexArrayMask(mask);
}

// FUNCTION: SURRENDER 0x100266E0
void srGERD::Renderer::submit()
{
    unsigned long vertex_count = vertices.count;
    statistics[4] += 1;
    statistics[5] += vertex_count;
    statistics[6] += indices.count;
    if (gerd != 0 && vertex_count != 0 && indices.count != 0) {
        e_matrixMode saved_mode = gerd->getMatrixMode();
        gerd->matrixMode(MATRIX_PROJECTION);
        gerd->pushMatrix();
        gerd->loadIdentity();
        srVertexArray arrays;
        vertices.bind(arrays, 0);
        programVertexArrays(&arrays, vertex_count);
        e_cullMode saved_cull = gerd->getCullMode();
        gerd->setCullMode(CULL_NONE);
        if (sorted == 0) {
            drawImmediate();
        } else {
            drawSorted();
        }
        gerd->popMatrix();
        gerd->matrixMode(saved_mode);
        gerd->setCullMode(saved_cull);
    }
    gerd->getDD()->fence();
    indices.reset(0);
    vertices.count = 0;
    clip_state = 0;
}

// FUNCTION: SURRENDER 0x100268A0
void srGERD::Renderer::reset(int release_buffers)
{
    indices.reset(release_buffers);
    vertices.reset(release_buffers);
    texture_sets.clear();
    if (release_buffers != 0) {
        bytes.release();
        dwords.release();
        remap.release();
        stq[0].release();
        stq[1].release();
    }
}

/* Retail 0x10026A00: copy the indices of the not-fully-clipped triangles —
   a triangle drops out only when all three corners share a clip flag. With
   direct_vertex_indices clear the corners index the flag array through the
   vertices remap. */
// FUNCTION: SURRENDER 0x10026A00
static unsigned long filterTriangles(unsigned long* destination, const unsigned char* flags,
                                     const srGERD::Renderer::TriInput& input)
{
    unsigned long kept = 0;
    if (input.direct_vertex_indices == 0) {
        for (unsigned long index = 0; index < input.triangle_count; index++) {
            const srVector3i& triangle = input.triangles[input.indices[index]];
            unsigned char flag = flags[input.vertices[triangle.x]];
            if (flag == 0 || (flag & flags[input.vertices[triangle.z]] &
                              flags[input.vertices[triangle.y]]) == 0) {
                destination[kept] = input.indices[index];
                kept++;
            }
        }
    } else {
        for (unsigned long index = 0; index < input.triangle_count; index++) {
            const srVector3i& triangle = input.triangles[input.indices[index]];
            unsigned char flag = flags[triangle.x];
            if (flag == 0 || (flag & flags[triangle.z] & flags[triangle.y]) == 0) {
                destination[kept] = input.indices[index];
                kept++;
            }
        }
    }
    return kept;
}

// FUNCTION: SURRENDER 0x10026B30
void srGERD::Renderer::render(const TriInput& input)
{
    if (first_vertex == -1) {
        return;
    }
    if (input.triangle_count == 0) {
        vertices.count += first_vertex - vertices.count;
        first_vertex = -1;
        return;
    }
    statistics[0] += 1;
    statistics[1] += input.triangle_count * input.record_count;
    statistics[3] += input.vertex_count * input.record_count;
    unsigned char* flags = bytes.ensure(input.vertex_count);
    transformVertices(input, flags);
    if (fullyClipped(flags, input.vertex_count) != 0) {
        vertices.count += first_vertex - vertices.count;
        first_vertex = -1;
        return;
    }
    const TriInput* batch = &input;
    TriInput filtered;
    unsigned long mask = attributeMask(flags, input.vertex_count);
    if (mask != 0) {
        filtered = input;
        /* const_cast-ok: the filtered list buffer was just reserved. */
        filtered.indices = dwords.ensure(input.triangle_count);
        filtered.triangle_count =
            filterTriangles(const_cast<unsigned long*>(filtered.indices), flags, input);
        if (filtered.triangle_count != 0) {
            clip_state |= mask;
            batch = &filtered;
        } else {
            vertices.count += first_vertex - vertices.count;
            first_vertex = -1;
            return;
        }
    }
    if (gerd->pick.pick_depth != 0) {
        PickInput pick;
        pick.indices = batch->indices;
        pick.triangles = batch->triangles;
        pick.triangle_count = batch->triangle_count;
        pick.vertices = batch->vertices;
        pick.positions = &vertices.positions[first_vertex];
        pick.vertex_count = batch->vertex_count;
        gerd->performPickTest(pick);
    }
    expandTriangles(*batch, sorted != 0);
    first_vertex = -1;
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
        srVectorProcessor::copy(output, 0, count);
    }
    unsigned long previous = table[indices[0]];
    output[0] |= bit;
    unsigned long index = 0;
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
    arrays.diffuse = &diffuse[base];
    arrays.specular = &specular[base];
    arrays.eye_locations = &positions[base];
    arrays.st0 = &st[0][base];
    arrays.st1 = &st[1][base];
    arrays.q0 = &q[0][base];
    arrays.q1 = &q[1][base];
    arrays.attributes = &attributes[base];
}

// FUNCTION: SURRENDER 0x10027ED0
void srGERD::Renderer::bindTextureSet(unsigned long index)
{
    const TextureSet& set = texture_sets.sets.data[index];
    if (set.texture0 != texture0) {
        texture0 = set.texture0;
        gerd->setTexture(texture0, 0);
    }
    if (set.texture1 != texture1) {
        texture1 = set.texture1;
        gerd->setTexture(texture1, 1);
    }
    if (set.shader.value != shader.value) {
        shader = set.shader;
        gerd->setShader(shader);
    }
}

/* Compiler-generated memberwise teardown; the Renderer releases its members
   individually, the nested records theirs. */
