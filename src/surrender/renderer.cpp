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

// FUNCTION: SURRENDER 0x100027F0 FOLDED
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

// FUNCTION: SURRENDER 0x10002A90 FOLDED
static void* copyMemory(void* destination, const void* source, long size)
{
    if (size <= 0) {
        return 0;
    }
    memcpy(destination, source, size);
    return destination;
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

// FUNCTION: SURRENDER 0x10024E00
void srGERD::Renderer::allocVertexArray(srVertexArray& arrays, unsigned long count)
{
    first_vertex_c0_ = vertices_78_.count_40;
    vertices_78_.alloc(arrays, count);
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
