/* Differential cases for the exported srColorSurface / srPalette API: raw and
   ARGB pixel access, format-to-format copies, scaling, drawing and
   whole-surface operations. Every case prints raw surface bytes and decoded
   ARGB values; nothing address-dependent. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "surrender/srColorSurface.h"
#include "surrender/srFilter.h"
#include "surrender/srPalette.h"
#include "surrender/srTextureMap.h"

void runProbe(const char* name, void (*probe)());
void gerdTextureCase(srTextureIFace* texture);

static unsigned long g_surface_rng = 1;

static unsigned long surfaceRandom()
{
    g_surface_rng ^= g_surface_rng << 13;
    g_surface_rng ^= g_surface_rng >> 17;
    g_surface_rng ^= g_surface_rng << 5;
    return g_surface_rng;
}

static void seedFromName(const char* name)
{
    unsigned long hash = 2166136261UL;
    while (*name != 0) {
        hash = (hash ^ static_cast<unsigned char>(*name++)) * 16777619UL;
    }
    g_surface_rng = hash != 0 ? hash : 1;
}

struct SurfaceType {
    srPixelConvert::e_surfaceType type;
    const char* name;
};

static const SurfaceType g_types[] = {
    {srPixelConvert::SURFACE_AP44, "AP44"},
    {srPixelConvert::SURFACE_AL44, "AL44"},
    {srPixelConvert::SURFACE_L8, "L8"},
    {srPixelConvert::SURFACE_A8, "A8"},
    {srPixelConvert::SURFACE_P8, "P8"},
    {srPixelConvert::SURFACE_AP88, "AP88"},
    {srPixelConvert::SURFACE_AL88, "AL88"},
    {srPixelConvert::SURFACE_RGB565, "RGB565"},
    {srPixelConvert::SURFACE_RGB555, "RGB555"},
    {srPixelConvert::SURFACE_ARGB1555, "ARGB1555"},
    {srPixelConvert::SURFACE_RGB444, "RGB444"},
    {srPixelConvert::SURFACE_ARGB4444, "ARGB4444"},
    {srPixelConvert::SURFACE_BGR24, "BGR24"},
    {srPixelConvert::SURFACE_BGRX32, "BGRX32"},
    {srPixelConvert::SURFACE_BGRA32, "BGRA32"},
    {srPixelConvert::SURFACE_Y4U2V2, "Y4U2V2"},
    {srPixelConvert::SURFACE_A8Y4U2V2, "A8Y4U2V2"},
    {srPixelConvert::SURFACE_RGB332, "RGB332"},
    {srPixelConvert::SURFACE_BGR565, "BGR565"},
    {srPixelConvert::SURFACE_ARGB32, "ARGB32"},
    {srPixelConvert::SURFACE_BGR555, "BGR555"},
    {srPixelConvert::SURFACE_ABGR32, "ABGR32"},
    {srPixelConvert::SURFACE_RGBA32, "RGBA32"},
    {srPixelConvert::SURFACE_RGB24, "RGB24"},
};
static const int g_type_count = sizeof(g_types) / sizeof(g_types[0]);

/* Case parameters (runProbe takes a plain function). */
static const SurfaceType* g_type_a;
static const SurfaceType* g_type_b;
static long g_size[4];
static int g_variant;
static char g_case_name[128];

static srPalette* g_palette;

static srPalette* probePalette()
{
    if (g_palette == 0) {
        srARGB colors[256];
        long index;
        for (index = 0; index < 256; ++index) {
            colors[index].blue = static_cast<unsigned char>(index * 7 + 3);
            colors[index].green = static_cast<unsigned char>(255 - index);
            colors[index].red = static_cast<unsigned char>(index * 13);
            colors[index].alpha = static_cast<unsigned char>(index ^ 0x5a);
        }
        g_palette = new srPalette(colors, 256);
    }
    return g_palette;
}

static srColorSurface* makeSurface(const SurfaceType* type, long width, long height)
{
    srColorSurface* surface = new srColorSurface(type->type, width, height);
    if (surface->isPaletted()) {
        surface->setPalette(probePalette());
    }
    return surface;
}

static void fillRaw(srColorSurface* surface)
{
    unsigned char* data = static_cast<unsigned char*>(surface->getDataPtr());
    long row_bytes = surface->getWidth() * surface->getBytesPerPixel();
    long y;
    long x;
    for (y = 0; y < surface->getHeight(); ++y) {
        for (x = 0; x < row_bytes; ++x) {
            data[y * surface->getPitch() + x] = static_cast<unsigned char>(surfaceRandom() >> 7);
        }
    }
}

static void dumpRaw(const char* label, srColorSurface* surface)
{
    const unsigned char* data = static_cast<const unsigned char*>(surface->getDataPtr());
    long row_bytes = surface->getWidth() * surface->getBytesPerPixel();
    long y;
    long x;
    printf("%s size %ldx%ld bpp %ld\n", label, surface->getWidth(), surface->getHeight(),
           surface->getBitsPerPixel());
    for (y = 0; y < surface->getHeight(); ++y) {
        printf("%s raw %ld ", label, y);
        for (x = 0; x < row_bytes; ++x) {
            printf("%02x", data[y * surface->getPitch() + x]);
        }
        printf("\n");
    }
}

static void dumpARGB(const char* label, srColorSurface* surface)
{
    long y;
    long x;
    for (y = 0; y < surface->getHeight(); ++y) {
        printf("%s argb %ld", label, y);
        for (x = 0; x < surface->getWidth(); ++x) {
            printf(" %08lx", surface->getPixel(x, y));
        }
        printf("\n");
    }
}

static unsigned long randomARGB()
{
    return surfaceRandom();
}

/* ------------------------------------------------------------------------ */

static void readCase()
{
    srColorSurface* surface = makeSurface(g_type_a, 7, 5);
    srPixelConvert::PixelFormat format;
    unsigned long row[8];
    unsigned long column[8];
    srVector2i positions[6];
    unsigned long pixels[6];
    long index;
    surface->getPixelFormat(format);
    printf("format r%d@%d g%d@%d b%d@%d a%d@%d model %d size %d fourcc %08lx\n", format.red_bits,
           format.red_shift, format.green_bits, format.green_shift, format.blue_bits,
           format.blue_shift, format.alpha_bits, format.alpha_shift,
           static_cast<int>(format.color_model), static_cast<int>(format.pixel_size),
           format.fourcc);
    printf("bits a%ld r%ld g%ld b%ld bytes %ld alpha %d paletted %d datasize %ld\n",
           static_cast<long>(surface->getAlphaBits()), surface->getRedBits(),
           surface->getGreenBits(), surface->getBlueBits(), surface->getBytesPerPixel(),
           surface->isAlpha(), surface->isPaletted(), surface->getDataSize());
    fillRaw(surface);
    dumpRaw("s", surface);
    dumpARGB("s", surface);
    for (index = 0; index < 5; ++index) {
        printf("rawpix %ld %08lx\n", index, surface->getPixelRaw(index, index % 5));
    }
    memset(row, 0, sizeof(row));
    surface->getPixelRow(row, 2, 1, 5);
    printf("row2[1..5]");
    for (index = 0; index < 8; ++index) {
        printf(" %08lx", row[index]);
    }
    printf("\n");
    memset(column, 0, sizeof(column));
    surface->getPixelColumn(column, 3, 0, 4);
    printf("col3[0..4]");
    for (index = 0; index < 8; ++index) {
        printf(" %08lx", column[index]);
    }
    printf("\n");
    for (index = 0; index < 6; ++index) {
        positions[index].x = static_cast<long>(surfaceRandom() % 7);
        positions[index].y = static_cast<long>(surfaceRandom() % 5);
    }
    surface->getPixels(pixels, positions, 6);
    printf("getPixels");
    for (index = 0; index < 6; ++index) {
        printf(" (%ld,%ld)=%08lx", positions[index].x, positions[index].y, pixels[index]);
    }
    printf("\n");
}

static void writeCase()
{
    srColorSurface* surface = makeSurface(g_type_a, 7, 5);
    unsigned long row[7];
    srVector2i positions[4];
    unsigned long pixels[4];
    long x;
    long y;
    surface->fill(0);
    dumpRaw("fill0", surface);
    surface->fill(0xffffffffUL);
    dumpRaw("fill1", surface);
    surface->fill(0x80402010UL);
    dumpRaw("fillc", surface);
    for (y = 0; y < 5; ++y) {
        for (x = 0; x < 7; ++x) {
            surface->setPixel(x, y, randomARGB());
        }
    }
    dumpRaw("set", surface);
    dumpARGB("set", surface);
    for (x = 0; x < 7; ++x) {
        row[x] = randomARGB();
    }
    surface->setPixelRow(row, 1, 2, 6);
    surface->setPixelColumn(row, 0, 1, 4);
    for (x = 0; x < 4; ++x) {
        positions[x].x = static_cast<long>(surfaceRandom() % 7);
        positions[x].y = static_cast<long>(surfaceRandom() % 5);
        pixels[x] = randomARGB();
    }
    surface->setPixels(pixels, positions, 4);
    surface->setPixelRaw(6, 4, randomARGB());
    dumpRaw("rows", surface);
    dumpARGB("rows", surface);
}

static void copyCase()
{
    srColorSurface* source = makeSurface(g_type_a, 7, 5);
    srColorSurface* destination = makeSurface(g_type_b, 7, 5);
    fillRaw(source);
    destination->fill(0);
    destination->copy(*source);
    dumpRaw("d", destination);
    dumpARGB("d", destination);
}

static void changeFormatCase()
{
    srColorSurface* surface = makeSurface(g_type_a, 7, 5);
    srPixelConvert::PixelFormat format;
    int result;
    fillRaw(surface);
    srPixelConvert::mapPixelFormat(g_type_b->type, format);
    result = surface->changePixelFormat(format, 1);
    printf("changePixelFormat %d\n", result);
    dumpRaw("d", surface);
    dumpARGB("d", surface);
}

static srFilter* const g_filters[] = {0, &::srBoxFilter, &::srTriangleFilter, &::srBellFilter,
                                      &::srBSplineFilter};
static const char* const g_filter_names[] = {"default", "box", "triangle", "bell", "bspline"};

static void scaleCase()
{
    srColorSurface* source = makeSurface(g_type_a, g_size[0], g_size[1]);
    srColorSurface* destination = makeSurface(g_type_a, g_size[2], g_size[3]);
    fillRaw(source);
    destination->fill(0);
    if (g_filters[g_variant] != 0) {
        destination->setFilter(g_filters[g_variant]);
        source->setFilter(g_filters[g_variant]);
    }
    destination->copy(*source);
    dumpRaw("d", destination);
    dumpARGB("d", destination);
}

static void rescaleCase()
{
    srColorSurface* surface = makeSurface(g_type_a, g_size[0], g_size[1]);
    int result;
    fillRaw(surface);
    if (g_filters[g_variant] != 0) {
        surface->setFilter(g_filters[g_variant]);
    }
    result = surface->rescale(g_size[2], g_size[3]);
    printf("rescale %d\n", result);
    dumpRaw("d", surface);
    dumpARGB("d", surface);
    /* resize reallocates without initializing, so only the result and size are stable. */
    result = surface->resize(g_size[0], g_size[1]);
    printf("resize %d size %ldx%ld\n", result, surface->getWidth(), surface->getHeight());
}

enum SurfaceOp {
    OP_HLINE,
    OP_VLINE,
    OP_LINE,
    OP_FLIP_H,
    OP_FLIP_V,
    OP_ROTATE,
    OP_SWAP_ROWS,
    OP_FLIP_RECT,
    OP_REMAP,
    OP_COPY_CHANNEL,
    OP_FLIP_CHANNELS,
    OP_ADJUST,
    OP_SATURATION,
    OP_STATS,
    OP_BLIT,
    OP_BLIT_INFO,
    OP_COMPOSITE,
    OP_CLAMP,
    OP_NOISE,
    OP_COUNT
};

static const char* const g_op_names[] = {
    "hline",     "vline",     "line",         "flip-h",        "flip-v", "rotate180",  "swap-rows",
    "flip-rect", "remap",     "copy-channel", "flip-channels", "adjust", "saturation", "stats",
    "blit",      "blit-info", "composite",    "clamp",         "noise"};

static void opCase()
{
    srColorSurface* surface = makeSurface(g_type_a, 9, 7);
    long index;
    fillRaw(surface);
    switch (g_variant) {
    case OP_HLINE:
        surface->setHLine(1, 2, 6, 0xff336699UL);
        surface->setHLine(3, 7, 1, 0x80aabbccUL);
        surface->setHLine(5, -3, 20, 0x11223344UL);
        surface->setHLine(6, 8, 8, 0xffffffffUL);
        break;
    case OP_VLINE:
        surface->setVLine(1, 1, 5, 0xff336699UL);
        surface->setVLine(4, 6, 0, 0x80aabbccUL);
        surface->setVLine(7, -2, 30, 0x11223344UL);
        break;
    case OP_LINE:
        surface->setLine(0, 0, 8, 6, 0xff336699UL);
        surface->setLine(8, 0, 0, 6, 0x80aabbccUL);
        surface->setLine(2, 6, 3, 0, 0x11223344UL);
        surface->setLine(-5, 3, 15, 4, 0xffffffffUL);
        surface->setLine(4, 4, 4, 4, 0x01020304UL);
        break;
    case OP_FLIP_H:
        surface->flipHorizontal();
        break;
    case OP_FLIP_V:
        surface->flipVertical();
        break;
    case OP_ROTATE:
        surface->rotate180();
        break;
    case OP_SWAP_ROWS:
        surface->swapPixelRows(1, 0, 2, 5, 6);
        surface->swapPixelRows(0, 3, 0, 4, 9);
        break;
    case OP_FLIP_RECT: {
        srColorSurfaceIFace::Rectangle rectangle = {1, 1, 7, 6};
        surface->flipRectangle(rectangle);
        break;
    }
    case OP_REMAP: {
        srARGB from;
        srARGB to;
        unsigned long pixel = surface->getPixel(2, 2);
        memcpy(&from, &pixel, 4);
        to.alpha = 0x12;
        to.red = 0x34;
        to.green = 0x56;
        to.blue = 0x78;
        surface->setPixel(5, 5, pixel);
        surface->remapPixels(from, to);
        break;
    }
    case OP_COPY_CHANNEL:
        surface->copyColorChannel(srARGB::INDEX_ALPHA, srARGB::INDEX_GREEN);
        surface->copyColorChannel(srARGB::INDEX_BLUE, srARGB::INDEX_RED);
        break;
    case OP_FLIP_CHANNELS:
        surface->flipColorChannels(srARGB::INDEX_RED, srARGB::INDEX_BLUE);
        surface->flipColorChannels(srARGB::INDEX_ALPHA, srARGB::INDEX_GREEN);
        break;
    case OP_ADJUST: {
        srVector4T<float> scale;
        srVector4T<float> offset;
        srVector4T<float> gamma;
        scale.x = 0.5f;
        scale.y = 1.25f;
        scale.z = 1.0f;
        scale.w = 0.75f;
        offset.x = 0.1f;
        offset.y = -0.05f;
        offset.z = 0.0f;
        offset.w = 0.2f;
        gamma.x = 1.0f;
        gamma.y = 2.2f;
        gamma.z = 0.45f;
        gamma.w = 1.0f;
        surface->adjust(scale, offset, gamma);
        break;
    }
    case OP_SATURATION:
        surface->adjustSaturation(0.35);
        dumpRaw("sat035", surface);
        surface->adjustSaturation(1.8);
        break;
    case OP_STATS:
        for (index = 0; index < 4; ++index) {
            srStat statistics;
            memset(&statistics, 0, sizeof(statistics));
            surface->getChannelStatistics(statistics, static_cast<srARGB::e_index>(index));
            printf("stats %ld count %ld mean %.17g dev %.17g median %ld min %ld max %ld\n", index,
                   statistics.count, statistics.mean, statistics.deviation, statistics.median,
                   statistics.min, statistics.max);
        }
        break;
    case OP_BLIT: {
        srColorSurface* source = makeSurface(g_type_b, 5, 4);
        fillRaw(source);
        surface->blit(2, 1, *source, 0, 0, 5, 4);
        surface->blit(-2, 4, *source, 1, 1, 4, 3);
        surface->blit(7, -1, *source, 0, 0, 5, 4);
        surface->blit(0, 0, *source, 3, 2, 10, 10);
        break;
    }
    case OP_BLIT_INFO: {
        srColorSurface* source = makeSurface(g_type_b, 5, 4);
        srColorSurfaceIFace::BlitInfo info;
        fillRaw(source);
        info.destination.left = 1;
        info.destination.top = 1;
        info.destination.right = 8;
        info.destination.bottom = 6;
        info.source.left = 0;
        info.source.top = 0;
        info.source.right = 5;
        info.source.bottom = 4;
        static_cast<srColorSurfaceIFace*>(surface)->blit(info, *source);
        break;
    }
    case OP_COMPOSITE: {
        srColorSurface* source = makeSurface(g_type_b, 5, 4);
        fillRaw(source);
        surface->composite(1, 1, *source, 0, 0, 5, 4, 0.25);
        surface->composite(4, 3, *source, 1, 0, 4, 4, 1.0);
        surface->composite(-1, -1, *source, 0, 0, 3, 3, 0.5);
        break;
    }
    case OP_CLAMP: {
        long coordinates[6] = {-3, 0, 4, 8, 9, 100};
        for (index = 0; index < 6; ++index) {
            printf("clamp %ld x %ld y %ld\n", coordinates[index],
                   surface->getClampedX(coordinates[index]),
                   surface->getClampedY(coordinates[index]));
        }
        surface->setHClampMode(1);
        surface->setVClampMode(0);
        printf("modes h %d v %d\n", surface->getHClampMode(), surface->getVClampMode());
        for (index = 0; index < 6; ++index) {
            long x = coordinates[index];
            long y = coordinates[5 - index];
            surface->clampCoordinates(x, y);
            printf("clampCoordinates %ld %ld -> %ld %ld\n", coordinates[index],
                   coordinates[5 - index], x, y);
        }
        break;
    }
    case OP_NOISE:
        srand(12345);
        surface->addNoise(0.25, 0);
        dumpRaw("noise", surface);
        srand(12345);
        surface->addNoise(0.5, 1);
        break;
    }
    dumpRaw("d", surface);
    dumpARGB("d", surface);
}

static void paletteCase()
{
    srPalette* palette = probePalette();
    srARGB colors[16];
    unsigned char indices[16];
    long index;
    printf("size %ld\n", palette->getPaletteSize());
    for (index = 0; index < 16; ++index) {
        unsigned long value = surfaceRandom();
        memcpy(&colors[index], &value, 4);
    }
    colors[3] = palette->getColor(77);
    palette->quantize(indices, colors, 16);
    for (index = 0; index < 16; ++index) {
        printf("quantize %08lx -> %u\n", *reinterpret_cast<unsigned long*>(&colors[index]),
               indices[index]);
    }
    printf("single %u\n", palette->quantize(colors[5]));
    printf("match %d\n", palette->matchPalette(palette->getPaletteDataPtr(), 256));
    printf("match-other %d\n", palette->matchPalette(colors, 16));
}

/* ------------------------------------------------------------------------ */
/* srTextureMap mip chains. Source and destinations live in runner buffers
   with padded pitches, guarded on both sides, so the cases also see writes
   into row padding or past the surface. */

static const unsigned char GUARD = 0xa7;

struct PitchedSurface {
    srColorSurface* surface;
    unsigned char* buffer;
    unsigned long bytes;
    long pitch;
    long row_bytes;
};

static PitchedSurface makePitched(const SurfaceType* type, long width, long height, long padding)
{
    PitchedSurface result;
    srPixelConvert::PixelFormat format;
    srPixelConvert::mapPixelFormat(type->type, format);
    long bytes_per_pixel = static_cast<long>(format.pixel_size) + 1;
    result.row_bytes = width * bytes_per_pixel;
    result.pitch = result.row_bytes + padding;
    result.bytes = static_cast<unsigned long>(result.pitch * height + 32);
    result.buffer = static_cast<unsigned char*>(malloc(result.bytes));
    memset(result.buffer, GUARD, result.bytes);
    result.surface =
        new srColorSurface(type->type, result.buffer + 16, width, height, result.pitch);
    if (result.surface->isPaletted()) {
        result.surface->setPalette(probePalette());
    }
    return result;
}

static void dumpPitched(const char* label, const PitchedSurface& pitched)
{
    unsigned long index;
    unsigned long touched = 0;
    long height = pitched.surface->getHeight();
    dumpRaw(label, pitched.surface);
    for (index = 0; index < pitched.bytes; ++index) {
        long offset = static_cast<long>(index) - 16;
        int inside = offset >= 0 && offset < pitched.pitch * height &&
                     (offset % pitched.pitch) < pitched.row_bytes;
        if (!inside && pitched.buffer[index] != GUARD) {
            if (touched < 8) {
                printf("%s guard %ld %02x\n", label, offset, pitched.buffer[index]);
            }
            ++touched;
        }
    }
    printf("%s pitch %ld guards-touched %lu\n", label, pitched.pitch, touched);
}

static void textureMipCase()
{
    long width = g_size[0];
    long height = g_size[1];
    long start = g_size[2];
    PitchedSurface source = makePitched(g_type_a, width, height, 3);
    unsigned char* data = static_cast<unsigned char*>(source.surface->getDataPtr());
    long y;
    long x;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < source.row_bytes; ++x) {
            data[y * source.pitch + x] = static_cast<unsigned char>(surfaceRandom() >> 7);
        }
    }
    if (g_variant != 0) {
        source.surface->setFilter(g_filters[g_variant]);
    }
    srTextureMap* texture = new srTextureMap(source.surface);
    srTextureIFace::Dimensions dimensions;
    texture->getDimensions(dimensions);
    const srPixelConvert::PixelFormat& format = dimensions.format;
    printf("dimensions %lux%lu format %d/%d %d/%d %d/%d %d/%d model %d size %d palette %d\n",
           dimensions.width, dimensions.height, format.red_bits, format.red_shift,
           format.green_bits, format.green_shift, format.blue_bits, format.blue_shift,
           format.alpha_bits, format.alpha_shift, static_cast<int>(format.color_model),
           static_cast<int>(format.pixel_size), dimensions.palette != 0 ? 1 : 0);
    srTextureIFace::Parameters parameters;
    texture->getTextureParms(parameters);
    float bias = parameters.mipmap_bias;
    float priority = texture->getPriority();
    printf("parameters %08lx bias %08lx priority %08lx\n", parameters.packed_state,
           *reinterpret_cast<unsigned long*>(&bias), *reinterpret_cast<unsigned long*>(&priority));
    srTextureIFace::MultiRequest request;
    PitchedSurface levels[12];
    long level_count = 0;
    long level;
    memset(&request, 0, sizeof(request));
    for (level = 0; level < 12; ++level) {
        long level_width = width >> level;
        long level_height = height >> level;
        levels[level] = makePitched(g_type_b, level_width ? level_width : 1,
                                    level_height ? level_height : 1, level & 1 ? 5 : 2);
        request.destinations[level] = levels[level].surface;
        ++level_count;
        if (level_width <= 1 && level_height <= 1) {
            break;
        }
    }
    request.mipmap_level = start;
    request.last_level = static_cast<unsigned long>(level_count - 1);
    texture->getMipmapData(request);
    for (level = start; level < level_count; ++level) {
        char label[16];
        sprintf(label, "L%ld", level);
        dumpPitched(label, levels[level]);
    }
    dumpPitched("source", source);
}

static void textureGerdCase()
{
    srColorSurface* source = makeSurface(g_type_a, g_size[0], g_size[1]);
    fillRaw(source);
    srTextureMap* texture = new srTextureMap(source);
    if (g_variant & 1) {
        texture->enableHint(srTextureIFace::HINT_NO_MIPMAPS);
    }
    if (g_variant & 2) {
        texture->setMipmap(srTextureIFace::MIPMAP_BEST);
    }
    gerdTextureCase(texture);
}

static void clippedBlitCase()
{
    static const long rects[][8] = {
        {0, 0, 7, 5, 0, 0, 7, 5},   {-3, -2, 4, 3, 0, 0, 7, 5},     {5, 3, 12, 9, 0, 0, 7, 5},
        {2, 1, 6, 4, -4, -4, 3, 3}, {0, 0, 7, 5, 4, 2, 11, 7},      {3, 2, 3, 4, 0, 0, 7, 5},
        {6, 4, 2, 1, 0, 0, 7, 5},   {-10, -10, -1, -1, 0, 0, 3, 3}, {1, 1, 6, 4, 2, 2, 4, 3},
        {0, 0, 14, 10, 0, 0, 7, 5},
    };
    PitchedSurface destination = makePitched(g_type_b, 7, 5, 3);
    PitchedSurface source = makePitched(g_type_a, 7, 5, 1);
    unsigned char* data = static_cast<unsigned char*>(source.surface->getDataPtr());
    long y;
    long x;
    for (y = 0; y < 5; ++y) {
        for (x = 0; x < source.row_bytes; ++x) {
            data[y * source.pitch + x] = static_cast<unsigned char>(surfaceRandom() >> 7);
        }
    }
    destination.surface->fill(0x11223344);
    const long* rect = rects[g_variant];
    srColorSurfaceIFace::BlitInfo info;
    info.destination.left = rect[0];
    info.destination.top = rect[1];
    info.destination.right = rect[2];
    info.destination.bottom = rect[3];
    info.source.left = rect[4];
    info.source.top = rect[5];
    info.source.right = rect[6];
    info.source.bottom = rect[7];
    static_cast<srColorSurfaceIFace*>(destination.surface)->blit(info, *source.surface);
    dumpPitched("d", destination);
    dumpARGB("d", destination.surface);
}

/* ------------------------------------------------------------------------ */

static void submitSurface(void (*function)())
{
    seedFromName(g_case_name);
    runProbe(g_case_name, function);
}

void surfaceCases()
{
    static const SurfaceType* scale_types[] = {&g_types[19], &g_types[7], &g_types[2],
                                               &g_types[11], &g_types[4], &g_types[12]};
    static const long sizes[][4] = {{8, 8, 16, 16}, {16, 16, 8, 8}, {7, 5, 13, 3}, {8, 8, 8, 4},
                                    {5, 7, 5, 7},   {3, 9, 12, 2},  {16, 4, 5, 9}, {1, 1, 4, 4},
                                    {4, 4, 1, 1},   {12, 12, 7, 11}};
    static const int op_types[] = {19, 7, 9, 2, 6, 12, 11, 4, 14, 13};
    int a;
    int b;
    int index;
    int filter;
    for (a = 0; a < g_type_count; ++a) {
        g_type_a = &g_types[a];
        sprintf(g_case_name, "surface.read.%s", g_types[a].name);
        submitSurface(readCase);
        sprintf(g_case_name, "surface.write.%s", g_types[a].name);
        submitSurface(writeCase);
    }
    for (a = 0; a < g_type_count; ++a) {
        for (b = 0; b < g_type_count; ++b) {
            g_type_a = &g_types[a];
            g_type_b = &g_types[b];
            sprintf(g_case_name, "surface.copy.%s.%s", g_types[a].name, g_types[b].name);
            submitSurface(copyCase);
        }
    }
    for (a = 0; a < g_type_count; ++a) {
        for (b = 0; b < g_type_count; ++b) {
            g_type_a = &g_types[a];
            g_type_b = &g_types[b];
            sprintf(g_case_name, "surface.format.%s.%s", g_types[a].name, g_types[b].name);
            submitSurface(changeFormatCase);
        }
    }
    for (a = 0; a < 6; ++a) {
        for (index = 0; index < 10; ++index) {
            for (filter = 0; filter < 5; ++filter) {
                g_type_a = scale_types[a];
                memcpy(g_size, sizes[index], sizeof(g_size));
                g_variant = filter;
                sprintf(g_case_name, "surface.scale.%s.%ldx%ld-%ldx%ld.%s", g_type_a->name,
                        g_size[0], g_size[1], g_size[2], g_size[3], g_filter_names[filter]);
                submitSurface(scaleCase);
                sprintf(g_case_name, "surface.rescale.%s.%ldx%ld-%ldx%ld.%s", g_type_a->name,
                        g_size[0], g_size[1], g_size[2], g_size[3], g_filter_names[filter]);
                submitSurface(rescaleCase);
            }
        }
    }
    for (a = 0; a < 10; ++a) {
        for (index = 0; index < OP_COUNT; ++index) {
            g_type_a = &g_types[op_types[a]];
            g_type_b = &g_types[op_types[(a + 3) % 10]];
            g_variant = index;
            sprintf(g_case_name, "surface.op.%s.%s", g_type_a->name, g_op_names[index]);
            submitSurface(opCase);
        }
    }
    sprintf(g_case_name, "palette.quantize");
    submitSurface(paletteCase);
    static const int mip_types[] = {19, 7, 2, 11, 4, 12, 9, 3, 14, 6};
    static const long mip_sizes[][3] = {{16, 16, 0}, {7, 5, 0}, {1, 9, 0}, {13, 1, 0},
                                        {3, 3, 0},   {8, 8, 1}, {32, 4, 2}};
    for (a = 0; a < 10; ++a) {
        for (b = 0; b < 10; ++b) {
            for (index = 0; index < 7; ++index) {
                if (index >= 2 && (a + b + index) % 3 != 0) {
                    continue; /* thin out the size matrix */
                }
                g_type_a = &g_types[mip_types[a]];
                g_type_b = &g_types[mip_types[b]];
                g_size[0] = mip_sizes[index][0];
                g_size[1] = mip_sizes[index][1];
                g_size[2] = mip_sizes[index][2];
                g_variant = (a * 7 + b * 3 + index) % 5;
                sprintf(g_case_name, "texture.mips.%s.%s.%ldx%ld.from%ld.%s", g_type_a->name,
                        g_type_b->name, g_size[0], g_size[1], g_size[2], g_filter_names[g_variant]);
                submitSurface(textureMipCase);
            }
        }
    }
    static const long gerd_sizes[][2] = {{16, 16}, {7, 5}, {1, 1}, {64, 2}, {3, 17}};
    for (a = 0; a < 10; ++a) {
        for (index = 0; index < 5; ++index) {
            for (filter = 0; filter < 4; ++filter) {
                g_type_a = &g_types[mip_types[a]];
                g_size[0] = gerd_sizes[index][0];
                g_size[1] = gerd_sizes[index][1];
                g_variant = filter;
                sprintf(g_case_name, "texture.gerd.%s.%ldx%ld.v%d", g_type_a->name, g_size[0],
                        g_size[1], filter);
                submitSurface(textureGerdCase);
            }
        }
    }
    for (a = 0; a < 10; ++a) {
        for (index = 0; index < 10; ++index) {
            g_type_a = &g_types[mip_types[a]];
            g_type_b = &g_types[mip_types[(a + index) % 10]];
            g_variant = index;
            sprintf(g_case_name, "surface.blitclip.%s.%s.r%d", g_type_a->name, g_type_b->name,
                    index);
            submitSurface(clippedBlitCase);
        }
    }
}
