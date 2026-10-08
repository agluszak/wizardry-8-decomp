/* Differential cases for smaller exported SurRender utilities: the triangle
   culler, the polygon triangulator, the memory pool, string tables, exponent
   tables and the palette sampler/quantizer. */

#include <math.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "surrender/srExponentTable.h"
#include "surrender/srMath.h"
#include "surrender/srMemoryPool.h"
#include "surrender/srPalette.h"
#include "surrender/srStringTable.h"
#include "surrender/srTriangleCuller.h"
#include "surrender/srTriangulator.h"

void runProbe(const char* name, void (*probe)());

static unsigned long g_misc_rng = 1;
static char g_misc_name[96];
static int g_misc_variant;

static unsigned long miscRandom()
{
    g_misc_rng ^= g_misc_rng << 13;
    g_misc_rng ^= g_misc_rng >> 17;
    g_misc_rng ^= g_misc_rng << 5;
    return g_misc_rng;
}

/* Uniform in [-range, range) with 1/4096 steps so values are exact floats. */
static float miscFloat(float range)
{
    long step = static_cast<long>(miscRandom() & 0x1fff) - 0x1000;
    return range * static_cast<float>(step) / 4096.0f;
}

static unsigned long bitsOf(float value)
{
    unsigned long bits;
    memcpy(&bits, &value, 4);
    return bits;
}

static void seedCase(const char* name, int variant)
{
    const char* cursor;
    g_misc_rng = 0x811c9dc5UL;
    for (cursor = name; *cursor != 0; ++cursor) {
        g_misc_rng = (g_misc_rng ^ static_cast<unsigned char>(*cursor)) * 0x01000193UL;
    }
    if (g_misc_rng == 0) {
        g_misc_rng = 1;
    }
    g_misc_variant = variant;
}

static void runMisc(const char* family, const char* label, int variant, void (*probe)())
{
    sprintf(g_misc_name, "%s.%s", family, label);
    seedCase(g_misc_name, variant);
    runProbe(g_misc_name, probe);
}

/* ---- srTriangleCuller ---- */

static void fillMatrix(srMatrix4T<float>& matrix, int kind)
{
    float* values = reinterpret_cast<float*>(&matrix);
    int index;
    for (index = 0; index < 16; ++index) {
        values[index] = (index % 5) == 0 ? 1.0f : 0.0f;
    }
    if (kind >= 1) {
        values[3] = miscFloat(8.0f);
        values[7] = miscFloat(8.0f);
        values[11] = miscFloat(8.0f);
    }
    if (kind >= 2) {
        for (index = 0; index < 12; ++index) {
            if ((index & 3) != 3) {
                values[index] = miscFloat(2.0f);
            }
        }
    }
    if (kind >= 3) {
        for (index = 12; index < 16; ++index) {
            values[index] = miscFloat(1.0f);
        }
    }
}

static void printVector4(const char* label, const srVector4T<float>& vector)
{
    printf("%s %08lx %08lx %08lx %08lx\n", label, bitsOf(vector.x), bitsOf(vector.y),
           bitsOf(vector.z), bitsOf(vector.w));
}

static void cullerSetClipFlags()
{
    static const unsigned long counts[] = {0, 1, 7, 256, 301};
    unsigned long count = counts[g_misc_variant % 5];
    int first = (g_misc_variant / 5) & 1;
    unsigned long shift = (g_misc_variant * 7) % 32;
    srVector3T<float>* vertices = new srVector3T<float>[count + 1];
    unsigned long* flags = new unsigned long[count + 1];
    float* distances = new float[count + 1];
    unsigned long index;
    srVector4T<float> plane;
    plane.x = miscFloat(2.0f);
    plane.y = miscFloat(2.0f);
    plane.z = miscFloat(2.0f);
    plane.w = miscFloat(4.0f);
    for (index = 0; index <= count; ++index) {
        vertices[index].x = miscFloat(4.0f);
        vertices[index].y = miscFloat(4.0f);
        vertices[index].z = miscFloat(4.0f);
        flags[index] = miscRandom();
        distances[index] = -1234.5f;
    }
    if (count > 3) {
        /* A vertex exactly on the plane and one at -0 distance. */
        vertices[2].x = 0.0f;
        vertices[2].y = 0.0f;
        vertices[2].z = 0.0f;
    }
    int any =
        srTriangleCuller::setClipFlags(flags, distances, vertices, plane, shift, count, first);
    printf("any %d shift %lu first %d\n", any, shift, first);
    for (index = 0; index <= count; ++index) {
        if (index < 40 || index + 3 > count) {
            printf("v %lu flags %08lx dist %08lx\n", index, flags[index], bitsOf(distances[index]));
        }
    }
    unsigned long sum = 0;
    for (index = 0; index <= count; ++index) {
        sum = sum * 31 + flags[index] + bitsOf(distances[index]);
    }
    printf("checksum %08lx\n", sum);
}

static void makeFrustum(srVector4T<float>* planes, int count)
{
    /* Left, right, bottom, top, near, far, then extra user planes. */
    static const float base[6][4] = {
        {0.7071f, 0.0f, 0.7071f, 0.0f}, {-0.7071f, 0.0f, 0.7071f, 0.0f},
        {0.0f, 0.7071f, 0.7071f, 0.0f}, {0.0f, -0.7071f, 0.7071f, 0.0f},
        {0.0f, 0.0f, 1.0f, -1.0f},      {0.0f, 0.0f, -1.0f, 100.0f}};
    int index;
    for (index = 0; index < count; ++index) {
        if (index < 6) {
            planes[index].x = base[index][0];
            planes[index].y = base[index][1];
            planes[index].z = base[index][2];
            planes[index].w = base[index][3];
        } else {
            planes[index].x = miscFloat(1.0f);
            planes[index].y = miscFloat(1.0f);
            planes[index].z = miscFloat(1.0f);
            planes[index].w = miscFloat(4.0f);
        }
    }
}

static void cullerGetClipMask()
{
    srVector4T<float> planes[32];
    int index;
    makeFrustum(planes, 32);
    for (index = 0; index < 64; ++index) {
        srVector3T<float> center;
        float depth = -7.0f;
        float radius;
        unsigned long mask = (index & 3) == 0 ? 0 : (miscRandom() & 0xffffffc0UL);
        center.x = miscFloat(20.0f);
        center.y = miscFloat(20.0f);
        center.z = miscFloat(60.0f) + (index & 1 ? 0.0f : 50.0f);
        radius = index % 7 == 0 ? 0.0f : static_cast<float>(miscRandom() & 0xff) / 16.0f;
        if (index == 5) {
            center.z = 1.0f;
            center.x = 0.0f;
            center.y = 0.0f;
        }
        unsigned long clip = srTriangleCuller::getClipMask(center, radius, planes, mask, depth);
        printf("sphere %d mask %08lx clip %08lx depth %08lx\n", index, mask, clip, bitsOf(depth));
    }
}

static void cullerTransformPlane()
{
    int index;
    for (index = 0; index < 256; ++index) {
        srMatrix4T<float> matrix;
        srVector4T<float> plane;
        fillMatrix(matrix, index & 3);
        plane.x = miscFloat(2.0f);
        plane.y = miscFloat(2.0f);
        plane.z = miscFloat(2.0f);
        plane.w = miscFloat(4.0f);
        if (index >= 32 && index % 5 == 0) {
            plane.z = 0.0f; /* the other in-plane offset branch */
        }
        if (index >= 32 && index % 7 == 0) {
            plane.w = 0.0f;
        }
        srVector4T<float> result = srTriangleCuller::transformClipPlane(
            plane, matrix, static_cast<srMatrix4T<float>::e_scaleType>(index % 3));
        printVector4("plane", result);
    }
}

static void cullerLinearAndAVT()
{
    static const unsigned long counts[] = {0, 1, 2, 5, 64, 257};
    unsigned long count = counts[g_misc_variant % 6];
    unsigned long vertex_count = count * 2 + 3;
    unsigned long* indices = new unsigned long[count + 4];
    unsigned long* avt = new unsigned long[vertex_count + 4];
    unsigned long* scratch = new unsigned long[vertex_count + 4];
    srVector3i* triangles = new srVector3i[count * 2 + 1];
    unsigned long index;
    for (index = 0; index < count + 4; ++index) {
        indices[index] = 0xdeadbeef;
    }
    srTriangleCuller::setupLinearArray(indices, count);
    for (index = 0; index < count + 4; ++index) {
        if (index < 20 || index + 4 >= count) {
            printf("linear %lu %08lx\n", index, indices[index]);
        }
    }
    for (index = 0; index < count * 2 + 1; ++index) {
        triangles[index].x = static_cast<int>(miscRandom() % vertex_count);
        triangles[index].y = static_cast<int>(miscRandom() % vertex_count);
        triangles[index].z = static_cast<int>(miscRandom() % vertex_count);
    }
    for (index = 0; index < count; ++index) {
        indices[index] = miscRandom() % (count * 2 + 1);
    }
    for (index = 0; index < vertex_count + 4; ++index) {
        avt[index] = 0xa5a5a5a5;
        scratch[index] = miscRandom();
    }
    unsigned long active =
        srTriangleCuller::buildAVT(avt, scratch, indices, triangles, count, vertex_count);
    printf("active %lu\n", active);
    for (index = 0; index < vertex_count + 4; ++index) {
        printf("avt %lu %08lx remap %08lx\n", index, avt[index],
               index < vertex_count ? scratch[index] : 0);
    }
}

static void cullerCull()
{
    /* variant: bits 0-1 cull mode, bit 2 active list, bit 3 clipping, bits 4+ size. */
    static const unsigned long sizes[] = {1, 12, 300};
    int cull_mode = g_misc_variant & 3;
    int use_active = (g_misc_variant >> 2) & 1;
    int use_clip = (g_misc_variant >> 3) & 1;
    unsigned long triangle_count = sizes[(g_misc_variant >> 4) % 3];
    unsigned long vertex_count = triangle_count + 2;
    srVector3T<float>* vertices = new srVector3T<float>[vertex_count];
    srVector4T<float>* projected = new srVector4T<float>[vertex_count];
    srVector3i* triangles = new srVector3i[triangle_count];
    unsigned long* active = new unsigned long[triangle_count];
    unsigned long* scratch = new unsigned long[triangle_count + vertex_count * 2 + 8];
    srVector4T<float> planes[32];
    srMatrix4T<float> model_view;
    srMatrix4T<float> inverse;
    unsigned long index;
    unsigned long active_count = 0;
    if (cull_mode == 3) {
        cull_mode = 2;
    }
    makeFrustum(planes, 32);
    fillMatrix(model_view, 1);
    fillMatrix(inverse, 2);
    for (index = 0; index < vertex_count; ++index) {
        vertices[index].x = miscFloat(30.0f);
        vertices[index].y = miscFloat(30.0f);
        vertices[index].z = miscFloat(60.0f) + 40.0f;
        projected[index].x = miscFloat(4.0f);
        projected[index].y = miscFloat(4.0f);
        projected[index].z = miscFloat(4.0f);
        projected[index].w = miscFloat(4.0f);
    }
    for (index = 0; index < triangle_count; ++index) {
        triangles[index].x = static_cast<int>(index);
        triangles[index].y = static_cast<int>(index + 1 + (miscRandom() & 1));
        triangles[index].z = static_cast<int>(miscRandom() % vertex_count);
        if ((miscRandom() & 3) != 0) {
            active[active_count++] = index;
        }
    }
    srTriangleCuller::Input input;
    srTriangleCuller::Output output;
    input.triangle_count = triangle_count;
    input.vertex_count = vertex_count;
    input.active_triangle_count = active_count;
    input.cull_mode = cull_mode;
    input.active_triangles = use_active ? active : 0;
    input.projected_vertices = projected;
    input.triangles = triangles;
    input.vertices = vertices;
    input.clip_planes = planes;
    input.model_view = &model_view;
    input.inverse_model_view = &inverse;
    input.scale_type = srMatrix4T<float>::SCALE_TYPE_UNIT;
    input.clip_mask = use_clip ? (0x3fUL | (g_misc_variant & 0x10 ? 0xc0UL : 0)) : 0;
    for (index = 0; index < triangle_count + vertex_count * 2 + 8; ++index) {
        scratch[index] = 0xcccccccc;
    }
    output.indices = scratch;
    output.avt = scratch + triangle_count;
    output.vertex_remap = output.avt + vertex_count;
    output.triangle_count = 0x77;
    output.vertex_count = 0x77;
    output.linear = 0x77;
    int result = srTriangleCuller::cull(output, input);
    printf("result %d triangles %lu vertices %lu linear %d\n", result, output.triangle_count,
           output.vertex_count, output.linear);
    unsigned long sum = 0;
    for (index = 0; index < triangle_count + vertex_count * 2 + 8; ++index) {
        if (index < 48) {
            printf("scratch %lu %08lx\n", index, scratch[index]);
        }
        sum = sum * 31 + scratch[index];
    }
    printf("checksum %08lx\n", sum);
}

/* ---- srTriangulator ---- */

static void triangulatorCase()
{
    /* variant: 0 convex, 1 star, 2 comb, 3 collinear runs; plus size. */
    static const int sizes[] = {3, 4, 5, 8, 17};
    int shape = g_misc_variant & 3;
    int count = sizes[(g_misc_variant >> 2) % 5];
    srVector2T<float>* points = new srVector2T<float>[count];
    int index;
    for (index = 0; index < count; ++index) {
        double angle = 6.283185307179586 * index / count;
        double radius = 10.0;
        if (shape == 1 && (index & 1) != 0) {
            radius = 4.0;
        }
        if (shape == 2) {
            radius = (index % 3) == 1 ? 3.0 : 10.0 + (miscRandom() & 7);
        }
        float x = static_cast<float>(radius * cos(angle));
        float y = static_cast<float>(radius * sin(angle));
        if (shape == 3 && count > 4) {
            /* A rectangle with points along its edges. */
            int side = index * 4 / count;
            float t = static_cast<float>(index * 4 - side * count) / count;
            x = side == 0 ? t * 8.0f : side == 1 ? 8.0f : side == 2 ? 8.0f - t * 8.0f : 0.0f;
            y = side == 0 ? 0.0f : side == 1 ? t * 4.0f : side == 2 ? 4.0f : 4.0f - t * 4.0f;
        }
        points[index].x = x;
        points[index].y = y;
    }
    /* The list destructor is not exported, so the triangulator is placed in
       static storage and never destroyed. */
    static double storage[8];
    srTriangulator* triangulator = new (storage) srTriangulator(points, count);
    for (index = 0; index < count - 2; ++index) {
        srVector3i triangle = triangulator->next();
        printf("triangle %d %d %d %d\n", index, triangle.x, triangle.y, triangle.z);
    }
}

/* ---- srMemoryPool ---- */

static double g_pool_memory[0x4000 / 8 + 4];

static void memoryPoolCase()
{
    /* variant: bits 0-1 alignment, bit 2 best fit, bit 3 locks, bit 4 mask area. */
    static const long alignments[] = {1, 4, 32, 256};
    long alignment = alignments[g_misc_variant & 3];
    char* base = reinterpret_cast<char*>(g_pool_memory) + 3;
    srMemoryPool* pool = new srMemoryPool(base, 0x4000 - 3, alignment);
    void* live[48];
    int index;
    memset(live, 0, sizeof(live));
    if ((g_misc_variant & 4) != 0) {
        pool->setPolicy(srMemoryPool::FIT_BEST);
    }
    printf("init size %ld align %ld policy %d avail %ld free %ld used %ld\n", pool->getSize(),
           pool->getAlignment(), static_cast<int>(pool->getPolicy()), pool->memAvail(),
           pool->memFreeTotal(), pool->memUsed());
    if ((g_misc_variant & 16) != 0) {
        int masked = pool->maskArea(base + 0x1000, 0x123);
        printf("mask %d avail %ld free %ld used %ld\n", masked, pool->memAvail(),
               pool->memFreeTotal(), pool->memUsed());
    }
    for (index = 0; index < 400; ++index) {
        unsigned long choice = miscRandom();
        int slot = static_cast<int>((choice >> 8) % 48);
        if (live[slot] == 0 || (choice & 3) == 0) {
            long size = static_cast<long>((choice >> 16) % ((choice & 0x10) ? 1500 : 80)) - 2;
            void* old = live[slot];
            live[slot] = pool->allocate(size);
            printf("%d alloc %ld -> %ld", index, size,
                   live[slot] ? static_cast<long>(static_cast<char*>(live[slot]) - base) : -1L);
            if (live[slot] != 0) {
                printf(" size %ld", pool->getSize(live[slot]));
            } else {
                live[slot] = old;
            }
        } else if ((choice & 3) == 1 && (g_misc_variant & 8) != 0) {
            if (pool->getLockStatus(live[slot])) {
                pool->unlock(live[slot]);
                printf("%d unlock %ld", index,
                       static_cast<long>(static_cast<char*>(live[slot]) - base));
            } else {
                pool->lock(live[slot]);
                printf("%d lock %ld", index,
                       static_cast<long>(static_cast<char*>(live[slot]) - base));
            }
        } else {
            int locked = pool->getLockStatus(live[slot]);
            printf("%d free %ld locked %d", index,
                   static_cast<long>(static_cast<char*>(live[slot]) - base), locked);
            pool->free(live[slot]);
            if (!locked) {
                live[slot] = 0;
            }
        }
        printf(" avail %ld free %ld used %ld\n", pool->memAvail(), pool->memFreeTotal(),
               pool->memUsed());
    }
    for (index = 0; index < 48; ++index) {
        if (live[index] != 0) {
            pool->unlock(live[index]);
            pool->free(live[index]);
        }
    }
    printf("final avail %ld free %ld used %ld\n", pool->memAvail(), pool->memFreeTotal(),
           pool->memUsed());
    void* whole = pool->allocate(pool->memAvail());
    printf("whole %ld\n", whole ? static_cast<long>(static_cast<char*>(whole) - base) : -1L);
}

/* ---- srStringTable ---- */

static void printTable(const char* label, srStringTable& table)
{
    long index;
    printf("%s count %ld\n", label, table.getCount());
    for (index = 0; index < table.getCount(); ++index) {
        const char* text = table.getString(index);
        printf("%s %ld [%s] [%s]\n", label, index, text ? text : "(null)",
               table[static_cast<int>(index)] ? table[static_cast<int>(index)] : "(null)");
    }
}

static void stringTableCase()
{
    static const char* const inputs[] = {
        "alpha;beta;gamma",
        ";;leading;;trailing;;",
        "c:\\games\\wiz8;d:\\data\\;e:/mixed/slash/",
        "",
        "one",
        "a b\tc;d,e",
        "path\\;other\\\\;x",
    };
    static const char* const separators[] = {";", "; \t,", ",", ""};
    srStringTable table;
    int index;
    table.addString("first");
    table.addString("");
    for (index = 0; index < 7; ++index) {
        table.addSeparatedStrings(inputs[index], separators[(index + g_misc_variant) % 4],
                                  (index + g_misc_variant) & 1);
    }
    printTable("table", table);
    for (index = 0; index < 70; ++index) {
        char text[32];
        sprintf(text, "grow%03d", index);
        table.addString(text);
    }
    printf("grown %ld [%s]\n", table.getCount(), table.getString(table.getCount() - 1));
    srStringTable copy;
    copy.addString("discarded");
    copy = table;
    printf("copy %ld [%s] [%s]\n", copy.getCount(), copy.getString(0),
           copy.getString(copy.getCount() - 1));
    table.reset();
    printTable("reset", table);
    table.addString("after-reset");
    printTable("again", table);
    printf("copy-still %ld [%s]\n", copy.getCount(), copy.getString(3));
}

/* ---- srExponentTable ---- */

static void exponentTableCase()
{
    /* Exponent 0 is left unfilled: the constructor zeroes exponent_ and setExponent skips equal values. */
    static const float exponents[] = {0.5f, 0.25f, 1.0f, 2.0f, 7.5f, 32.0f, 128.0f, 1000.0f};
    float exponent = exponents[g_misc_variant % 8];
    srExponentTable table(exponent);
    int index;
    printf("exponent %08lx\n", bitsOf(table.getExponent()));
    for (index = 0; index <= 1024; index += (index < 40 || index > 1000) ? 1 : 13) {
        float x = static_cast<float>(index) / 1024.0f;
        printf("x %08lx -> %08lx\n", bitsOf(x), bitsOf(table.getValue(x)));
    }
    /* getValue does not clamp; inputs outside [0, 1] read past the table. */
    float odd[] = {0.5f, 0.999f, 1.0f, 0.75f, 0.0004f, 0.0f, 0.0009765625f, 0.333333f};
    for (index = 0; index < 8; ++index) {
        printf("odd %08lx -> %08lx\n", bitsOf(odd[index]), bitsOf(table.getValue(odd[index])));
    }
}

static void cachedExponentCase()
{
    srCachedExponentTable* tables[20];
    int index;
    for (index = 0; index < 20; ++index) {
        float exponent = static_cast<float>((index * 7) % 12) + 0.5f;
        tables[index] = srCachedExponentTable::get(exponent);
        printf("get %d exp %08lx same-as-prev %d v %08lx\n", index,
               bitsOf(tables[index]->getExponent()),
               index > 0 && tables[index] == tables[index - 1],
               bitsOf(tables[index]->getValue(0.75f)));
    }
    for (index = 0; index < 20; ++index) {
        int match = -1;
        int other;
        for (other = 0; other < index; ++other) {
            if (tables[other] == tables[index]) {
                match = other;
                break;
            }
        }
        printf("identity %d first %d\n", index, match);
    }
    for (index = 0; index < 20; index += 2) {
        tables[index]->release();
    }
    srCachedExponentTable::freeUnused();
    srCachedExponentTable* again = srCachedExponentTable::get(3.5f);
    printf("again exp %08lx v %08lx\n", bitsOf(again->getExponent()),
           bitsOf(again->getValue(0.5f)));
    again->release();
    for (index = 1; index < 20; index += 2) {
        tables[index]->release();
    }
    srCachedExponentTable::freeAll();
    printf("freed\n");
}

/* ---- srPalette sampler / quantizer ---- */

static void makeColor(srARGB& color, unsigned long value)
{
    memcpy(&color, &value, 4);
}

static void quantizerCase()
{
    /* variant: palette size and channel bits. */
    static const long sizes[] = {1, 2, 16, 255, 256};
    static const unsigned char bits[][3] = {{8, 8, 8}, {5, 6, 5}, {5, 5, 5}, {3, 3, 2}};
    long color_count = sizes[g_misc_variant % 5];
    const unsigned char* channel_bits = bits[(g_misc_variant / 5) % 4];
    srARGB palette[256];
    unsigned char duplicates[256];
    long index;
    for (index = 0; index < 256; ++index) {
        unsigned long value = miscRandom() | 0xff000000UL;
        if (index > 4 && (miscRandom() & 7) == 0) {
            value = *reinterpret_cast<unsigned long*>(&palette[index - 3]);
        }
        makeColor(palette[index], value);
        duplicates[index] = static_cast<unsigned char>(index & 1);
    }
    srPalette::Quantizer* quantizer =
        new srPalette::Quantizer(palette, color_count, (g_misc_variant & 1) ? duplicates : 0,
                                 channel_bits[0], channel_bits[1], channel_bits[2]);
    unsigned long sum = 0;
    for (index = 0; index < 600; ++index) {
        unsigned long value = miscRandom();
        if (index < 64 && index < color_count) {
            value = *reinterpret_cast<unsigned long*>(&palette[index]);
        }
        unsigned char red = static_cast<unsigned char>(value >> 16);
        unsigned char green = static_cast<unsigned char>(value >> 8);
        unsigned char blue = static_cast<unsigned char>(value);
        unsigned char result = quantizer->quantize(red, green, blue);
        if (index < 96) {
            printf("q %06lx -> %u\n", value & 0xffffff, result);
        }
        sum = sum * 31 + result;
    }
    srARGB colors[64];
    unsigned char indices[64];
    for (index = 0; index < 64; ++index) {
        makeColor(colors[index], miscRandom());
    }
    quantizer->quantize(indices, colors, 64);
    for (index = 0; index < 64; ++index) {
        sum = sum * 31 + indices[index];
    }
    printf("bulk %u %u single %u\n", indices[0], indices[63], quantizer->quantize(colors[7]));
    printf("checksum %08lx\n", sum);
}

static void samplerCase()
{
    /* variant: color source shape and output palette size. */
    static const long palette_sizes[] = {2, 16, 64, 256};
    long palette_size = palette_sizes[g_misc_variant % 4];
    int shape = (g_misc_variant / 4) % 4;
    srPalette::Sampler* sampler = new srPalette::Sampler(0);
    long index;
    printf("defaults bits %ld factor %08lx%08lx size %ld\n", sampler->getSampleBits(),
           static_cast<unsigned long>(0), bitsOf(static_cast<float>(sampler->getSampleFactor())),
           sampler->getOutputPaletteSize());
    sampler->setOutputPaletteSize(palette_size);
    if (shape == 3) {
        sampler->setSampleBits(4);
    }
    if ((g_misc_variant & 1) != 0) {
        srARGB mask;
        makeColor(mask, 0xffff00ffUL);
        sampler->setMaskColor(0, mask);
        makeColor(mask, 0xff000000UL);
        sampler->setMaskColor(255, mask);
    }
    for (index = 0; index < 3000; ++index) {
        unsigned long value;
        srARGB color;
        switch (shape) {
        case 0:
            value = miscRandom() | 0xff000000UL;
            break;
        case 1: {
            /* A few clusters. */
            unsigned long centre = (index % 5) * 0x00332211UL;
            value = 0xff000000UL | (centre + (miscRandom() & 0x070707UL));
            break;
        }
        case 2: {
            unsigned long level = static_cast<unsigned long>(index % 256);
            value = 0xff000000UL | (level << 16) | (level << 8) | level;
            break;
        }
        default:
            value = 0xff000000UL | (miscRandom() & 0xf0f0f0UL);
            break;
        }
        makeColor(color, value);
        sampler->addColor(color, 1 + static_cast<long>(index % 3));
    }
    printf("colors %ld samples %ld bits %ld\n", sampler->getColorCount(), sampler->getSampleCount(),
           sampler->getSampleBits());
    srPalette* palette = sampler->createOptimalPalette();
    if (palette == 0) {
        printf("palette null\n");
        return;
    }
    printf("palette size %ld\n", palette->getPaletteSize());
    for (index = 0; index < palette->getPaletteSize(); ++index) {
        srARGB color = palette->getColor(index);
        printf("p %ld %08lx\n", index, *reinterpret_cast<unsigned long*>(&color));
    }
}

void miscCases()
{
    int variant;
    char label[32];
    for (variant = 0; variant < 10; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("culler.setClipFlags", label, variant, cullerSetClipFlags);
    }
    runMisc("culler.getClipMask", "spheres", 0, cullerGetClipMask);
    runMisc("culler.transformClipPlane", "matrices", 0, cullerTransformPlane);
    for (variant = 0; variant < 6; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("culler.buildAVT", label, variant, cullerLinearAndAVT);
    }
    for (variant = 0; variant < 48; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("culler.cull", label, variant, cullerCull);
    }
    for (variant = 0; variant < 20; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("triangulator", label, variant, triangulatorCase);
    }
    for (variant = 0; variant < 32; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("memorypool", label, variant, memoryPoolCase);
    }
    for (variant = 0; variant < 4; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("stringtable", label, variant, stringTableCase);
    }
    for (variant = 0; variant < 8; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("exponent.table", label, variant, exponentTableCase);
    }
    runMisc("exponent.cached", "sequence", 0, cachedExponentCase);
    for (variant = 0; variant < 20; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("palette.quantizer", label, variant, quantizerCase);
    }
    for (variant = 0; variant < 16; ++variant) {
        sprintf(label, "v%d", variant);
        runMisc("palette.sampler", label, variant, samplerCase);
    }
}
