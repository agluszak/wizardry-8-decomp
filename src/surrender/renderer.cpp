#include "surrender/srGERD.h"

#include <string.h>

#include "surrender/srVectorProcessor.h"

/* drawSorted's {order, sort_key} pair record. */
struct SortPair {
    w8_ulong index;
    w8_ulong key;
};

static void* copyMemory(void* destination, const void* source, w8_long size);
static void markTransitions(w8_ulong* output, const w8_ulong* indices, const w8_ulong* table,
                            w8_ulong bit, w8_ulong count, w8_ulong initialized);
static void fillConstant(w8_ulong* destination, w8_ulong value, w8_ulong count);

static void sortPairs(SortPair* pairs, w8_ulong count)
{
    if (count <= 1) {
        return;
    }
    SortPair* scratch = static_cast<SortPair*>(srHeap.allocate(count * sizeof(SortPair)));
    w8_ulong counts[0x100];
    SortPair* src = pairs;
    SortPair* dst = scratch;
    w8_ulong bulk = count & ~3;
    for (w8_ulong pass = 0; pass < 4; ++pass) {
        srZeroMemory(counts, sizeof(counts));
        /* reinterpret-ok: radix pass extracts byte `pass` of each key. */
        unsigned char* keys = reinterpret_cast<unsigned char*>(&src[0].key) + pass;
        w8_ulong index = 0;
        for (; index < bulk; index += 4) {
            ++counts[keys[index * 8]];
            ++counts[keys[index * 8 + 8]];
            ++counts[keys[index * 8 + 0x10]];
            ++counts[keys[index * 8 + 0x18]];
        }
        for (; index < count; ++index) {
            ++counts[keys[index * 8]];
        }
        w8_ulong position = 0;
        for (w8_ulong bucket = 0; bucket < 0x100; bucket += 4) {
            w8_ulong saved = counts[bucket];
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
        copyMemory(pairs, src, count * sizeof(*pairs));
    }
    srHeap.free(dst);
}

static void* copyMemory(void* destination, const void* source, w8_long size)
{
    if (size <= 0) {
        return 0;
    }
    memcpy(destination, source, size);
    return destination;
}

/* dst[i] = src[i] + offset over `count` dwords. */
// FUNCTION: SURRENDER 0x10023CB0
static void offsetIndices(w8_ulong* destination, const w8_ulong* source, w8_ulong offset,
                          w8_ulong count)
{
    w8_ulong index = 0;
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

/* Gather `count` triangle index triples through `order`; a nonzero `vertex_base` additionally
   rebases every index. */
// FUNCTION: SURRENDER 0x10023DE0
static void gatherTriangles(srVector3i* destination, const srVector3i* source,
                            const w8_ulong* order, w8_long vertex_base, w8_ulong count)
{
    if (vertex_base == 0) {
        for (w8_ulong index = 0; index < count; index++) {
            destination[index] = source[order[index]];
        }
    } else {
        for (w8_ulong index = 0; index < count; index++) {
            const srVector3i& triangle = source[order[index]];
            destination[index].x = triangle.x + vertex_base;
            destination[index].y = triangle.y + vertex_base;
            destination[index].z = triangle.z + vertex_base;
        }
    }
}

/* Gather `count` triangles through `indices`, remapping every corner through `vertices`, then
   rebase every index by a nonzero `vertex_base`. */
// FUNCTION: SURRENDER 0x10023D90
static void gatherIndexedTriangles(srVector3i* destination, const srVector3i* source,
                                   const w8_ulong* indices, const w8_ulong* vertices,
                                   w8_ulong vertex_base, w8_ulong count)
{
    srVectorProcessor::srCopyIndexedRemap(destination, source, indices, vertices, count);
    if (vertex_base != 0) {
        /* reinterpret-ok: the triples rebase as flat dwords. */
        // reinterpret-ok: the historical vector routines operate on raw attribute words
        offsetIndices(
            reinterpret_cast<w8_ulong*>(destination),
            // reinterpret-ok: the historical vector routines operate on raw attribute words
            reinterpret_cast<const w8_ulong*>(destination), vertex_base, count * 3);
    }
}

/* dst[i] = src[i] + offset over `count` index triples. */
// FUNCTION: SURRENDER 0x10024000
static void offsetTriangles(srVector3i* destination, const srVector3i* source, w8_ulong offset,
                            w8_ulong count)
{
    w8_ulong index = 0;
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

/* Index of the first element not equal to `value`, or `count` when the whole range matches. */
// FUNCTION: SURRENDER 0x10024100
static w8_ulong firstMismatch(const w8_ulong* values, w8_ulong value, w8_ulong count)
{
    w8_ulong index = 0;
    while (index < count && values[index] == value) {
        index++;
    }
    return index;
}

/* The count of consecutive `order` entries whose values element equals `value`. */
// FUNCTION: SURRENDER 0x10024170
static w8_ulong firstMismatchSorted(const w8_ulong* values, w8_ulong value, const w8_ulong* order,
                                    w8_ulong count)
{
    w8_ulong index = 0;
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
void srGERD::Renderer::getStatistics(w8_ulong* statistics)
{
    for (int i = 0; i < 7; i++) {
        statistics[i] = this->statistics[i];
    }
}

// FUNCTION: SURRENDER 0x10024280
w8_ulong srGERD::Renderer::TextureSetCache::intern(const TextureSetKey& key)
{
    srHashTable<TextureSetKey, w8_ulong>* map = this->map;
    int slot = map->FindNextEntry(&key, -1);
    if (slot != -1) {
        return map->entries[slot].value;
    }

    w8_ulong index = count;
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
void srGERD::Renderer::IndexBatch::alloc(IndexWrite& write, w8_ulong count)
{
    w8_ulong needed = count + 0x40 + this->count;
    triangles.ensureIndex(needed);
    texture_set.ensureIndex(needed);
    sort_key.ensureIndex(needed);
    aux.ensureIndex(needed);
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
void srGERD::Renderer::VertexArrays::alloc(srVertexArray& arrays, w8_ulong count)
{
    w8_ulong needed = count + this->count;
    if (capacity < needed) {
        needed += 0x40;
        diffuse.ensureIndex(needed);
        specular.ensureIndex(needed);
        positions.ensureIndex(needed);
        st[0].ensureIndex(needed);
        st[1].ensureIndex(needed);
        q[0].ensureIndex(needed);
        q[1].ensureIndex(needed);
        attributes.ensureIndex(needed);

        w8_ulong added = needed - capacity;
        /* reinterpret-ok: dword fill of the vector records. */
        // reinterpret-ok: the historical vector routines operate on raw attribute words
        fillConstant(reinterpret_cast<w8_ulong*>(&diffuse[capacity]), 0, added * 4);
        /* reinterpret-ok: as above. */
        // reinterpret-ok: the historical vector routines operate on raw attribute words
        fillConstant(reinterpret_cast<w8_ulong*>(&specular[capacity]), 0, added * 4);
        srVector4T<float> eye_default;
        eye_default.Set(0.0f, 0.0f, 0.0f, 1.0f);
        if (added != 0) {
            srVectorProcessor::copy(&positions[capacity], eye_default, added);
        }
        /* reinterpret-ok: 1.0f's bit pattern goes in through the dword
           fill. */
        // reinterpret-ok: the historical vector routines operate on raw attribute words
        srVectorProcessor::copy(reinterpret_cast<w8_ulong*>(&q[0][capacity]), 0x3f800000, added);
        /* reinterpret-ok: as above. */
        // reinterpret-ok: the historical vector routines operate on raw attribute words
        srVectorProcessor::copy(reinterpret_cast<w8_ulong*>(&q[1][capacity]), 0x3f800000, added);
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
void srGERD::Renderer::rewindVertexArray(w8_ulong count)
{
    if (first_vertex != -1) {
        vertices.count -= count;
    }
}

// FUNCTION: SURRENDER 0x10024E00
void srGERD::Renderer::allocVertexArray(srVertexArray& arrays, w8_ulong count)
{
    first_vertex = vertices.count;
    vertices.alloc(arrays, count);
}

// FUNCTION: SURRENDER 0x10024E30
void srGERD::Renderer::assignTextureSets(w8_ulong* texture_set, const w8_ulong* indices,
                                         w8_ulong count, const srTriMeshPipeline::Pass* pass)
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
        w8_ulong set = texture_sets.intern(key);
        if (count != 0) {
            srVectorProcessor::copy(texture_set, set, count);
        }
        return;
    }
    w8_ulong set = texture_sets.intern(key);
    w8_ulong done = 0;
    while (done < count) {
        w8_ulong chunk = count - done;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        const w8_ulong* chunk_indices = indices + done;
        w8_ulong* out = texture_set + done;
        w8_ulong initialized = 0;
        if ((mask & TABLE_TEXTURE0) != 0) {
            markTransitions(out, chunk_indices,
                            static_cast<const w8_ulong*>(pass->texture_tables[0]), TABLE_TEXTURE0,
                            chunk, initialized);
            initialized = 1;
        }
        if ((mask & TABLE_TEXTURE1) != 0) {
            markTransitions(out, chunk_indices,
                            static_cast<const w8_ulong*>(pass->texture_tables[1]), TABLE_TEXTURE1,
                            chunk, initialized);
            initialized += 1;
        }
        if ((mask & TABLE_SHADER) != 0) {
            markTransitions(out, chunk_indices, reinterpret_cast<const w8_ulong*>(pass->shaders),
                            TABLE_SHADER, chunk,
                            initialized); // reinterpret-ok: packed srShader words.
        }
        if (chunk != 0) {
            for (w8_ulong index = 0; index < chunk; index++) {
                w8_ulong changed = out[index];
                if (changed != 0) {
                    w8_ulong vertex = chunk_indices[index];
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

/* Depth-sort keys: each triangle's key is the negated sum of its corner view-space z's plus the
   bias, mapped to a sortable unsigned value. */
// FUNCTION: SURRENDER 0x100251E0
static void sortKeys(w8_ulong* keys, const srVector3i* triangles,
                     const srVector4T<float>* positions, float bias, w8_ulong count)
{
    if (count != 0) {
        const srVector3i* triangle = triangles;
        float scaled_bias = bias * 3.0f;
        do {
            float depth = -(positions[triangle->x].z + positions[triangle->y].z +
                            positions[triangle->z].z + scaled_bias);
            /* reinterpret-ok: the key maps the depth's IEEE sign bit onto a
               sortable unsigned order. */
            // reinterpret-ok: the historical vector routines operate on raw attribute words
            w8_ulong bits = reinterpret_cast<w8_ulong&>(depth);
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
    w8_ulong record = 0;
    while (record < input.record_count && passes[record].poly_uv == 0) {
        record++;
    }
    if (record == input.record_count) {
        /* Flat path: gather the chunk once, then replicate it per record
       with the record's vertex offset. */
        for (w8_ulong done = 0; done < input.triangle_count; done += 0x100) {
            w8_ulong chunk = input.triangle_count - done;
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
                for (w8_ulong replica = 1; replica < input.record_count; replica++) {
                    offsetIndices(write.sort_key + input.triangle_count * replica + done,
                                  write.sort_key + done, replica, chunk);
                }
            }
            for (w8_ulong replica = 1; replica < input.record_count; replica++) {
                offsetTriangles(write.triangles + input.triangle_count * replica + done,
                                write.triangles + done, replica * input.vertex_count, chunk);
            }
        }
    } else {
        /* Dedup path: any pass with a poly-UV corner table reuses the
           designated source corner's vertex and allocates fresh slots for
           the others; remap carries six dwords per triangle — the new
           vertices' batch positions, then their corner-source indices. */
        w8_ulong free_vertex = input.record_count * input.vertex_count + first_vertex;
        w8_ulong* remap = this->remap.ensure(input.triangle_count * 6);
        w8_ulong* corner_remap = remap + input.triangle_count * 3;
        w8_ulong written = 0;
        for (record = 0; record < input.record_count; record++) {
            const srTriMeshPipeline::Pass& pass = passes[record];
            w8_ulong vertex_base = record * input.vertex_count + first_vertex;
            if (pass.poly_uv != 0) {
                const srVector3i* poly_uv = pass.poly_uv;
                w8_ulong new_count = 0;
                w8_ulong next = free_vertex;
                w8_ulong base = free_vertex;
                w8_ulong* remap_out = remap;
                w8_ulong* corner_out = corner_remap;
                for (w8_ulong index = 0; index < input.triangle_count; index++) {
                    const srVector3i& triangle = input.triangles[input.indices[index]];
                    const srVector3i& uv = poly_uv[input.indices[index]];
                    srVector3i& out = write.triangles[written + index];
                    w8_ulong mapped = input.vertices[triangle.x] + vertex_base;
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
                    // reinterpret-ok: the historical vector routines operate on raw attribute words
                    srVectorProcessor::copyIndexed(
                        reinterpret_cast<w8_ulong*>(arrays.q1 + base),
                        // reinterpret-ok: the historical vector routines operate on raw attribute words
                        reinterpret_cast<const w8_ulong*>(arrays.q1), remap, new_count);
                    for (w8_ulong index = 0; index < new_count; index++) {
                        arrays.attributes[base + index] = arrays.attributes[remap[index]];
                    }
                    srVectorProcessor::copyIndexed(arrays.st0 + base, pass.texcoords, corner_remap,
                                                   new_count);
                    /* reinterpret-ok: 1.0f's bit pattern fills the q0
                       stream. */
                    // reinterpret-ok: the historical vector routines operate on raw attribute words
                    srVectorProcessor::copy(reinterpret_cast<w8_ulong*>(arrays.q0 + base),
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
    w8_ulong texture_set_offset = 0;
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
    w8_ulong zero_mask = 0;
    w8_ulong bit = 1;
    for (w8_long row = 0; row < 4; row++) {
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
    for (w8_ulong done = 0; done < input.vertex_count; done += 0x80) {
        w8_ulong chunk = input.vertex_count - done;
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
        for (w8_ulong replica = 1; replica < input.record_count; replica++) {
            srVector4T<float>* destination = write + replica * input.vertex_count;
            if (chunk != 0 && destination != write) {
                srVectorProcessor::memcopy(destination, write, chunk * 0x10);
            }
        }
        write += 0x80;
    }
}

/* Whether all clip-flag bytes share a clip plane. */
// FUNCTION: SURRENDER 0x10025C00
static int fullyClipped(const unsigned char* flags, w8_ulong count)
{
    unsigned char mask = srRendererDefs::FRUSTUM_CLIP_MASK;
    w8_ulong index;
    for (index = 0; index < (count & ~3UL); index += 4) {
        mask &= flags[index] & flags[index + 1] & flags[index + 2] & flags[index + 3];
        if (mask == 0) {
            return 0;
        }
    }
    for (; index < count; index++) {
        mask &= flags[index];
    }
    return mask != 0;
}

/* OR the per-vertex packed attribute bytes into the aggregate mask. */
// FUNCTION: SURRENDER 0x10025CB0
static w8_ulong attributeMask(const unsigned char* packed, w8_ulong count)
{
    w8_ulong mask = 0;
    if (count > 3) {
        // reinterpret-ok: the packed byte stream is folded through word loads.
        const w8_ulong* words = reinterpret_cast<const w8_ulong*>(packed);
        for (w8_ulong i = 0; i < count / 4; i++) {
            mask |= words[i];
        }
        mask = (mask & 0xff) | ((((mask & 0xff0000) | (mask >> 8)) >> 8 | (mask & 0xff00)) >> 8);
    }
    for (w8_ulong i = count & ~3UL; i < count; i++) {
        mask |= packed[i];
    }
    return mask;
}

// FUNCTION: SURRENDER 0x10025D50
void srGERD::Renderer::drawImmediate()
{
    w8_ulong count = this->indices.count;
    if (count != 0) {
        /* The [0] probes force the batch streams to their initial capacity. */
        const srVector3i* indices = &this->indices.triangles[0];
        const w8_ulong* texture_set = &this->indices.texture_set[0];
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
                               static_cast<srRendererDefs::e_indexType>(2), indices);
            return;
        }
        w8_ulong run = 0;
        while (run < count) {
            w8_ulong id = texture_set[run];
            bindTextureSet(id);
            w8_ulong length = 1 + firstMismatch(texture_set + run + 1, id, count - run - 1);
            gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, length * 3,
                               static_cast<srRendererDefs::e_indexType>(2), indices + run);
            run += length;
        }
    }
}

// FUNCTION: SURRENDER 0x10025F40
void srGERD::Renderer::drawSorted()
{
    w8_ulong count = indices.count;
    if (count != 0) {
        const srVector3i* triangles = &indices.triangles[0];
        w8_ulong* texture_set = &indices.texture_set[0];
        w8_ulong* sort_key = &indices.sort_key[0];
        indices.aux[0];
        w8_ulong* order = static_cast<w8_ulong*>(::operator new(count * sizeof(w8_ulong)));
        w8_ulong index = 0;
        for (; index < count; index++) {
            order[index] = index;
        }
        if (count > 1) {
            SortPair* pairs = static_cast<SortPair*>(srHeap.allocate(count * sizeof(SortPair)));
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
        srVector3i* batch = static_cast<srVector3i*>(srHeap.allocate(0x200 * sizeof(*batch)));
        if (srVectorProcessor::isEqual(texture_set, texture_set[order[0]], count) != 0) {
            w8_ulong offset = 0;
            do {
                w8_ulong chunk = count - offset;
                if (chunk > 0x200) {
                    chunk = 0x200;
                }
                gatherTriangles(batch, triangles, order + offset, 0, chunk);
                gerd->drawElements(srRendererDefs::PRIMITIVE_TRIANGLES, chunk * 3,
                                   static_cast<srRendererDefs::e_indexType>(2), batch);
                offset += 0x200;
            } while (offset < count);
        } else {
            w8_ulong run = 0;
            do {
                w8_ulong id = texture_set[order[run]];
                bindTextureSet(id);
                w8_ulong limit = count - run - 1;
                if (limit > 0x1ff) {
                    limit = 0x1ff;
                }
                w8_ulong length = 1 + firstMismatchSorted(texture_set, id, order + run + 1, limit);
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
void srGERD::Renderer::programVertexArrays(srVertexArray* arrays, w8_ulong count)
{
    w8_ulong attributes = attributeMask(arrays->attributes, count);
    if (texture_stages < 2) {
        /* Single-stage devices cannot take stage-1 st/q streams. */
        attributes &= ~(srVertexArray::ATTRIBUTE_ST1 | srVertexArray::ATTRIBUTE_Q1);
    }
    srFlags<srRendererDefs::e_vertexArray> mask(1 << srRendererDefs::VERTEX_ARRAY_POSITIONS);
    gerd->setClipState(srFlags<srRendererDefs::e_clip>(clip_state));
    gerd->setVertexPointer(4, srRendererDefs::TYPE_FLOAT, 0x10, arrays->eye_locations,
                           static_cast<w8_long>(count));
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
            for (w8_ulong i = 0; i < count; i++) {
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
            for (w8_ulong i = 0; i < count; i++) {
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
    w8_ulong vertex_count = vertices.count;
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

/* Copy the indices of the not-fully-clipped triangles; a triangle drops out only when all three
   corners share a clip flag. With direct_vertex_indices clear the corners index the flag array
   through the vertices remap. */
// FUNCTION: SURRENDER 0x10026A00
static w8_ulong filterTriangles(w8_ulong* destination, const unsigned char* flags,
                                const srGERD::Renderer::TriInput& input)
{
    w8_ulong kept = 0;
    if (input.direct_vertex_indices == 0) {
        for (w8_ulong index = 0; index < input.triangle_count; index++) {
            const srVector3i& triangle = input.triangles[input.indices[index]];
            unsigned char flag = flags[input.vertices[triangle.x]];
            if (flag == 0 || (flag & flags[input.vertices[triangle.z]] &
                              flags[input.vertices[triangle.y]]) == 0) {
                destination[kept] = input.indices[index];
                kept++;
            }
        }
    } else {
        for (w8_ulong index = 0; index < input.triangle_count; index++) {
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
    w8_ulong mask = attributeMask(flags, input.vertex_count);
    if (mask != 0) {
        filtered = input;
        /* const_cast-ok: the filtered list buffer was just reserved. */
        filtered.indices = dwords.ensure(input.triangle_count);
        filtered.triangle_count =
            filterTriangles(const_cast<w8_ulong*>(filtered.indices), flags, input);
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

/* Mark `bit` in output wherever the indexed table value changes between adjacent entries; when
   `initialized` is zero the output chunk is cleared first. */
// FUNCTION: SURRENDER 0x100278E0
static void markTransitions(w8_ulong* output, const w8_ulong* indices, const w8_ulong* table,
                            w8_ulong bit, w8_ulong count, w8_ulong initialized)
{
    if (initialized == 0 && count != 0) {
        srVectorProcessor::copy(output, 0, count);
    }
    w8_ulong previous = table[indices[0]];
    output[0] |= bit;
    w8_ulong index = 0;
    for (; index < (count & ~3UL); index += 4) {
        w8_ulong value = table[indices[index]];
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
        w8_ulong value = table[indices[index]];
        if (value != previous) {
            output[index] |= bit;
            previous = value;
        }
    }
}

/* Dword fill that does nothing for a zero count. */
// FUNCTION: SURRENDER 0x10027BA0
static void fillConstant(w8_ulong* destination, w8_ulong value, w8_ulong count)
{
    if (count != 0) {
        srVectorProcessor::copy(destination, value, count);
    }
}

// FUNCTION: SURRENDER 0x10027CF0
void srGERD::Renderer::VertexArrays::bind(srVertexArray& arrays, w8_ulong base)
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
void srGERD::Renderer::bindTextureSet(w8_ulong index)
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
