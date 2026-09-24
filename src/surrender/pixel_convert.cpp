#include "surrender/srPixelConvert.h"

#include <stdio.h>
#include <string.h>

#include "surrender/srARGB.h"
#include "surrender/srCore.h"
#include "surrender/srPalette.h"
#include "surrender/srVariableTimer.h"
#include "surrender/srVectorProcessor.h"

/* Conversion routines stored in the format table. The generic pair is
   selected by PixelFormat::conversion_class; the per-entry overrides cover
   formats whose converter does not fit a generic kernel. The MMX pair is
   installed by initFormats() when the CPU reports the feature bit. The YUV,
   indexed and MMX bodies are still unrecovered, so they stay
   link-unresolved like the other first-party gaps. */
void __cdecl writeRGB(const srPixelConvert::ConversionInfo& info);
void __cdecl readRGB(const srPixelConvert::ConversionInfo& info);
void __cdecl writeYUV(const srPixelConvert::ConversionInfo& info);
void __cdecl readYUV(const srPixelConvert::ConversionInfo& info);
void __cdecl writeIntensity(const srPixelConvert::ConversionInfo& info);
void __cdecl readIntensity(const srPixelConvert::ConversionInfo& info);
void __cdecl writeIndexed(const srPixelConvert::ConversionInfo& info);
void __cdecl readIndexed(const srPixelConvert::ConversionInfo& info);
void __cdecl writeRGB555(const srPixelConvert::ConversionInfo& info);
void __cdecl readRGB555(const srPixelConvert::ConversionInfo& info);
void __cdecl writeBGRX(const srPixelConvert::ConversionInfo& info);
void __cdecl readBGRX(const srPixelConvert::ConversionInfo& info);
void __cdecl writeRGB332(const srPixelConvert::ConversionInfo& info);
void __cdecl readRGB332(const srPixelConvert::ConversionInfo& info);
void __cdecl writeABGR(const srPixelConvert::ConversionInfo& info);
void __cdecl readABGR(const srPixelConvert::ConversionInfo& info);
void __cdecl writeRGB24(const srPixelConvert::ConversionInfo& info);
void __cdecl readRGB24(const srPixelConvert::ConversionInfo& info);
void __cdecl writeL8MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl readL8MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl writeRGB565MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl readRGB565MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl writeARGB1555MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl readARGB1555MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl writeARGB4444MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl readARGB4444MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl writeARGB32MMX(const srPixelConvert::ConversionInfo& info);
void __cdecl readARGB32MMX(const srPixelConvert::ConversionInfo& info);

/* Shared conversion kernels the generic dispatchers route through, by
   destination/source byte width: pack writes BGRA source pixels through
   the reduction tables, unpack expands packed source records through the
   expansion tables, packIntensity routes the luma ramps plus optional
   alpha. */
static void packIntensity16(unsigned short* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha);
static void packIntensity24(unsigned char* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha);
static void packIntensity32(unsigned long* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha);
static void pack8(unsigned char* dest, const srARGB* source,
                  const unsigned char* const* luts, const unsigned char* shifts,
                  unsigned long count, int has_alpha);
static void pack16(unsigned short* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha);
static void pack24(unsigned char* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha);
static void pack32(unsigned long* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha);
static void unpack16(unsigned long* dest, const unsigned short* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count);
static void unpack24(unsigned long* dest, const unsigned char* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count);
static void unpack32(unsigned long* dest, const unsigned long* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count);

namespace {

struct FormatEntry {
    srPixelConvert::PixelFormat format;
    srPixelConvert::ConversionFunc write;
    srPixelConvert::ConversionFunc read;
    FormatEntry* next;
};

/* Lookup tables built by Function10007850(): n-bit channel expansion
   (round(i * 255 / (2^n - 1))), 8-bit channel reduction, the ordered-dither
   bias cube, the packed-chroma decode table and the fixed-point channel
   weight ramps the conversion kernels read. */
// GLOBAL: SURRENDER 0x100A1AB0
unsigned char lutExpand1[1];
// GLOBAL: SURRENDER 0x100A1AB4
unsigned char lutExpand2[2];
// GLOBAL: SURRENDER 0x100A1AB8
unsigned char lutExpand4[4];
// GLOBAL: SURRENDER 0x100A1ABC
unsigned char lutExpand16[16];
// GLOBAL: SURRENDER 0x100A1ACC
unsigned char lutExpand32[32];
// GLOBAL: SURRENDER 0x100A1AEC
unsigned char lutExpand64[64];
// GLOBAL: SURRENDER 0x100A1B2C
unsigned char lutExpand8[8];
// GLOBAL: SURRENDER 0x100A1B34
unsigned char lutZero[256];
// GLOBAL: SURRENDER 0x100A1C34
unsigned char lutIdentity[256];
// GLOBAL: SURRENDER 0x100A1D34
unsigned char lutReduce128[256];
// GLOBAL: SURRENDER 0x100A1E34
unsigned char lutReduce64[256];
// GLOBAL: SURRENDER 0x100A1F34
unsigned char lutReduce32[256];
// GLOBAL: SURRENDER 0x100A2034
unsigned char lutReduce16[256];
// GLOBAL: SURRENDER 0x100A2134
unsigned char lutReduce8[256];
// GLOBAL: SURRENDER 0x100A2234
unsigned char lutReduce4[256];
// GLOBAL: SURRENDER 0x100A2334
unsigned char lutReduce2[256];

// GLOBAL: SURRENDER 0x100A2438
FormatEntry format_table[25];

// GLOBAL: SURRENDER 0x100A277C
unsigned char lutDecode[256][4];

// GLOBAL: SURRENDER 0x100A2B7C
FormatEntry* format_hash[32];

// GLOBAL: SURRENDER 0x100A2BFC
unsigned char lutExpand128[128];
// GLOBAL: SURRENDER 0x100A2C7C
long lutRamp18[256];
// GLOBAL: SURRENDER 0x100A307C
int lutDither[9][4][4][4];
// GLOBAL: SURRENDER 0x100A397C
long lutRamp54[256];
// GLOBAL: SURRENDER 0x100A3DA4
unsigned char lutGray[256][4];
// GLOBAL: SURRENDER 0x100A41A4
long lutRamp183[256];

// GLOBAL: SURRENDER 0x100A45A4
int formats_initialized;

/* Channel lookup-table selectors indexed by channel bit count: the write
   dispatchers reduce each 8-bit source channel, the read dispatchers
   expand each packed channel back to 8 bits. Bit count 8 selects the
   identity table. */
// GLOBAL: SURRENDER 0x1009832C
const unsigned char* const channel_expand[] = {
    lutExpand1, lutExpand2, lutExpand4,  lutExpand8,  lutExpand16,
    lutExpand32, lutExpand64, lutExpand128, lutIdentity,
};
// GLOBAL: SURRENDER 0x10098350
const unsigned char* const channel_reduce[] = {
    lutZero,   lutReduce2,  lutReduce4,  lutReduce8,  lutReduce16,
    lutReduce32, lutReduce64, lutReduce128, lutIdentity,
};

// FUNCTION: SURRENDER 0x100077A0
void initFormat(unsigned long index, unsigned char red_bits, unsigned char red_shift,
                unsigned char green_bits, unsigned char green_shift, unsigned char blue_bits,
                unsigned char blue_shift, unsigned char alpha_bits, unsigned char alpha_shift,
                long conversion_class, long bytes_per_pixel_minus_one, unsigned long flags)
{
    srPixelConvert::PixelFormat& format = format_table[index].format;
    format.red_bits = red_bits;
    format.red_shift = red_shift;
    format.green_bits = green_bits;
    format.green_shift = green_shift;
    format.blue_bits = blue_bits;
    format.blue_shift = blue_shift;
    format.alpha_bits = alpha_bits;
    format.alpha_shift = alpha_shift;
    format.conversion_class = conversion_class;
    format.bytes_per_pixel_minus_one = bytes_per_pixel_minus_one;
    format.flags = flags;
    format_table[index].write = 0;
    format_table[index].read = 0;
    format_table[index].next = 0;
}

} // namespace

/* Library-init table builder: channel expansion/reduction ramps, the
   ordered-dither bias cube, fixed-point channel weights and the packed
   decode/grayscale palettes. */
// FUNCTION: SURRENDER 0x10007850
void __cdecl initPixelTables(void)
{
    int i;
    int ditherKernel[4][4][4] = {
        {{0, 7, 2, 7}, {4, 5, 2, 5}, {6, 1, 7, 1}, {6, 3, 4, 3}},
        {{0, 6, 3, 7}, {4, 6, 1, 5}, {5, 1, 7, 2}, {7, 3, 4, 2}},
        {{3, 4, 3, 6}, {1, 7, 1, 6}, {5, 2, 5, 4}, {7, 2, 7, 0}},
        {{6, 1, 5, 3}, {6, 2, 6, 1}, {2, 5, 2, 7}, {2, 5, 2, 3}},
    };
    for (int level = 0; level < 9; ++level) {
        for (int a = 0; a < 4; ++a) {
            for (int b = 0; b < 4; ++b) {
                for (int c = 0; c < 4; ++c) {
                    lutDither[level][a][b][c] =
                        static_cast<long>((ditherKernel[a][b][c] - 3.5f) * (255.0f / 7.0f) /
                                              (1 << level) +
                                          0.5f);
                }
            }
        }
    }

    lutExpand1[0] = 0xff;
    for (i = 0; i < 2; ++i) {
        lutExpand2[i] = static_cast<unsigned char>(i * 255.0f + 0.5f);
    }
    for (i = 0; i < 4; ++i) {
        lutExpand4[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 3.0f) + 0.5f);
    }
    for (i = 0; i < 8; ++i) {
        lutExpand8[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 7.0f) + 0.5f);
    }
    for (i = 0; i < 16; ++i) {
        lutExpand16[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 15.0f) + 0.5f);
    }
    for (i = 0; i < 32; ++i) {
        lutExpand32[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 31.0f) + 0.5f);
    }
    for (i = 0; i < 64; ++i) {
        lutExpand64[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 63.0f) + 0.5f);
    }
    for (i = 0; i < 128; ++i) {
        lutExpand128[i] = static_cast<unsigned char>(i * 255.0f * (1.0f / 127.0f) + 0.5f);
    }

    for (i = 0; i < 256; ++i) {
        lutRamp54[i] = static_cast<long>(i * 54.4 + 0.5);
        lutRamp183[i] = static_cast<long>(i * 183.1424 + 0.5);
        lutRamp18[i] = static_cast<long>(i * 18.4576 + 0.5);
        lutGray[i][0] = static_cast<unsigned char>(i);
        lutGray[i][1] = static_cast<unsigned char>(i);
        lutGray[i][2] = static_cast<unsigned char>(i);
        lutGray[i][3] = 0;
    }

    memset(lutZero, 0, sizeof(lutZero));

    for (i = 0; i < 256; ++i) {
        lutIdentity[i] = static_cast<unsigned char>(i);
        double value = i * (1.0 / 255.0);
        lutReduce128[i] = static_cast<unsigned char>(value * 127.0 + 0.5);
        lutReduce64[i] = static_cast<unsigned char>(value * 63.0 + 0.5);
        lutReduce32[i] = static_cast<unsigned char>(value * 31.0 + 0.5);
        lutReduce16[i] = static_cast<unsigned char>(value * 15.0 + 0.5);
        lutReduce8[i] = static_cast<unsigned char>(value * 7.0 + 0.5);
        lutReduce4[i] = static_cast<unsigned char>(value * 3.0 + 0.5);
        lutReduce2[i] = static_cast<unsigned char>(value * 1.0 + 0.5);
    }

    for (i = 0; i < 256; ++i) {
        int luma = lutExpand16[(i >> 4) & 0xf];
        int chroma1 = lutExpand4[(i >> 2) & 3];
        int chroma2 = lutExpand4[i & 3];
        lutDecode[i][3] = 0xff;
        int value = static_cast<int>(luma + chroma1 * 0.956f + chroma2 * 0.620f);
        if (value < 0) {
            value = 0;
        } else if (value > 0xff) {
            value = 0xff;
        }
        lutDecode[i][2] = static_cast<unsigned char>(value);
        value = static_cast<int>(luma - chroma1 * 0.272f - chroma2 * 0.647f);
        if (value < 0) {
            value = 0;
        } else if (value > 0xff) {
            value = 0xff;
        }
        lutDecode[i][1] = static_cast<unsigned char>(value);
        value = static_cast<int>(luma - chroma1 * 1.108f + chroma2 * 1.705f);
        if (value < 0) {
            value = 0;
        } else if (value > 0xff) {
            value = 0xff;
        }
        lutDecode[i][0] = static_cast<unsigned char>(value);
    }
}

namespace {

// FUNCTION: SURRENDER 0x100082C0
void initFormats()
{
    if (formats_initialized != 0) {
        return;
    }
    initFormat(0, 4, 0, 0, 0, 0, 0, 4, 4, 3, 0, 0);
    initFormat(1, 4, 0, 0, 0, 0, 0, 4, 4, 2, 0, 0);
    initFormat(2, 8, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0);
    initFormat(3, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0);
    initFormat(4, 8, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0);
    initFormat(5, 8, 0, 0, 0, 0, 0, 8, 8, 3, 1, 0);
    initFormat(6, 8, 0, 0, 0, 0, 0, 8, 8, 2, 1, 0);
    initFormat(7, 5, 0xb, 6, 5, 5, 0, 0, 0, 0, 1, 0);
    initFormat(8, 5, 10, 5, 5, 5, 0, 0, 0, 0, 1, 0);
    initFormat(9, 5, 10, 5, 5, 5, 0, 1, 0xf, 0, 1, 0);
    initFormat(10, 4, 8, 4, 4, 4, 0, 0, 0, 0, 1, 0);
    initFormat(0xb, 4, 8, 4, 4, 4, 0, 4, 0xc, 0, 1, 0);
    initFormat(0xc, 8, 0x10, 8, 8, 8, 0, 0, 0, 0, 2, 0);
    initFormat(0xd, 8, 0x10, 8, 8, 8, 0, 0, 0, 0, 3, 0);
    initFormat(0xe, 8, 0x10, 8, 8, 8, 0, 8, 0x18, 0, 3, 0);
    initFormat(0xf, 4, 4, 2, 2, 2, 0, 0, 0, 1, 0, 0);
    initFormat(0x10, 4, 4, 2, 2, 2, 0, 8, 8, 1, 1, 0);
    initFormat(0x11, 3, 5, 3, 2, 2, 0, 0, 0, 0, 0, 0);
    initFormat(0x12, 3, 5, 3, 2, 2, 0, 8, 0, 0, 1, 0);
    initFormat(0x13, 5, 0, 6, 5, 5, 0xb, 0, 0, 0, 1, 0);
    initFormat(0x14, 8, 8, 8, 0x10, 8, 0x18, 8, 0, 0, 3, 0);
    initFormat(0x15, 5, 0, 5, 5, 5, 10, 0, 0, 0, 1, 0);
    initFormat(0x16, 8, 0x18, 8, 0x10, 8, 8, 8, 0, 0, 3, 0);
    initFormat(0x17, 8, 0, 8, 8, 8, 0x10, 8, 0x18, 0, 3, 0);
    initFormat(0x18, 8, 0, 8, 8, 8, 0x10, 0, 0, 0, 2, 0);
    for (FormatEntry* entry = format_table; entry < format_table + 25; entry++) {
        switch (entry->format.conversion_class) {
        case 0:
            entry->write = writeRGB;
            entry->read = readRGB;
            break;
        case 1:
            entry->write = writeYUV;
            entry->read = readYUV;
            break;
        case 2:
            entry->write = writeIntensity;
            entry->read = readIntensity;
            break;
        case 3:
            entry->write = writeIndexed;
            entry->read = readIndexed;
        }
    }
    format_table[8].write = writeRGB555;
    format_table[8].read = readRGB555;
    format_table[0xd].write = writeBGRX;
    format_table[0xd].read = readBGRX;
    format_table[0x11].write = writeRGB332;
    format_table[0x11].read = readRGB332;
    format_table[0x17].write = writeABGR;
    format_table[0x17].read = readABGR;
    format_table[0x18].write = writeRGB24;
    format_table[0x18].read = readRGB24;
    if ((srCore.getTimer()->m_cpu_features & 0x800000) != 0) {
        format_table[2].write = writeL8MMX;
        format_table[2].read = readL8MMX;
        format_table[7].write = writeRGB565MMX;
        format_table[7].read = readRGB565MMX;
        format_table[9].write = writeARGB1555MMX;
        format_table[9].read = readARGB1555MMX;
        format_table[0xb].write = writeARGB4444MMX;
        format_table[0xb].read = readARGB4444MMX;
        format_table[0xe].write = writeARGB32MMX;
        format_table[0xe].read = readARGB32MMX;
    }
    memset(format_hash, 0, sizeof(format_hash));
    for (FormatEntry* hashed = format_table; hashed < format_table + 25; hashed++) {
        const srPixelConvert::PixelFormat& format = hashed->format;
        unsigned long hash = (format.alpha_shift + format.alpha_bits) ^
                             (format.blue_shift + format.blue_bits) * 4 ^
                             (format.red_shift + format.red_bits) ^ (format.conversion_class << 3) ^
                             format.bytes_per_pixel_minus_one;
        hash = (hash >> 5 & 0x1f) ^ (hash & 0x1f);
        hashed->next = format_hash[hash];
        format_hash[hash] = hashed;
    }
    formats_initialized = 1;
}

} // namespace

// FUNCTION: SURRENDER 0x10007E40
int srPixelConvert::PixelFormat::isValid() const
{
    if (0 <= bytes_per_pixel_minus_one && bytes_per_pixel_minus_one <= 3 && 0 <= conversion_class &&
        conversion_class < 4 && red_bits <= 8 && green_bits <= 8 && blue_bits <= 8 &&
        alpha_bits <= 8 && red_shift <= 0x1f && green_shift <= 0x1f && blue_shift <= 0x1f &&
        alpha_shift <= 0x1f) {
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10007E90
void srPixelConvert::PixelFormat::getName(char* const name)
{
    static const char channel_letters[] = "RGBAYUVAIXXAPXXA";

    if (flags != 0) {
        name[0] = static_cast<char>(flags);
        name[1] = static_cast<char>(flags >> 8);
        name[2] = static_cast<char>(flags >> 0x10);
        name[3] = static_cast<char>(flags >> 0x18);
        name[4] = '\0';
        return;
    }
    if (name == 0) {
        return;
    }
    unsigned long shifts[4] = {red_shift, green_shift, blue_shift, alpha_shift};
    unsigned char bits[4] = {red_bits, green_bits, blue_bits, alpha_bits};
    unsigned char order[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < i; j++) {
            if (shifts[j] < shifts[i]) {
                unsigned long shift = shifts[i];
                shifts[i] = shifts[j];
                shifts[j] = shift;
                unsigned char bit = bits[i];
                bits[i] = bits[j];
                bits[j] = bit;
                unsigned char channel = order[i];
                order[i] = order[j];
                order[j] = channel;
            }
        }
    }
    char text[16];
    int length = 0;
    for (int k = 0; k < 4; k++) {
        if (bits[k] != 0) {
            text[length++] = channel_letters[conversion_class * 4 + order[k]];
        }
    }
    for (int j = 0; j < 4; j++) {
        if (bits[j] != 0) {
            text[length++] = static_cast<char>(bits[j] + '0');
        }
    }
    text[length] = '\0';
    char tail[8];
    sprintf(tail, "/%d", (int)(bytes_per_pixel_minus_one * 8 + 8));
    strcat(text, tail);
    strcpy(name, text);
}

// FUNCTION: SURRENDER 0x10008020
unsigned long srPixelConvert::PixelFormat::match(const PixelFormat* formats,
                                                 unsigned long count) const
{
    if (count < 2) {
        return 0;
    }
    if (flags != 0) {
        for (unsigned long i = 0; i < count; i++) {
            if (formats[i].flags == flags) {
                return i;
            }
        }
    }
    unsigned long wanted = 0;
    if (red_bits != 0) {
        wanted |= 1;
    }
    if (green_bits != 0) {
        wanted |= 2;
    }
    if (blue_bits != 0) {
        wanted |= 4;
    }
    if (alpha_bits != 0) {
        wanted |= 8;
    }
    unsigned long best = 0;
    int best_distance = 0x7fffffff;
    int best_bytes = 5;
    for (unsigned long c = 0; c < count; c++) {
        const PixelFormat& candidate = formats[c];
        if (candidate.conversion_class != conversion_class) {
            continue;
        }
        unsigned long have = 0;
        if (candidate.red_bits != 0) {
            have |= 1;
        }
        if (candidate.green_bits != 0) {
            have |= 2;
        }
        if (candidate.blue_bits != 0) {
            have |= 4;
        }
        if (candidate.alpha_bits != 0) {
            have |= 8;
        }
        if ((have & wanted) != wanted) {
            continue;
        }
        /* Only channel shortfall costs distance: a candidate with extra bits
           in a wanted channel scores the same as an exact bit count. */
        int dr = candidate.red_bits - red_bits;
        if (dr > 0) {
            dr = 0;
        }
        int dg = candidate.green_bits - green_bits;
        if (dg > 0) {
            dg = 0;
        }
        int db = candidate.blue_bits - blue_bits;
        if (db > 0) {
            db = 0;
        }
        int da = candidate.alpha_bits - alpha_bits;
        if (da > 0) {
            da = 0;
        }
        int distance = dr * dr + dg * dg + db * db + da * da;
        if (distance < best_distance ||
            (distance == best_distance && candidate.bytes_per_pixel_minus_one < best_bytes)) {
            best = c;
            best_distance = distance;
            best_bytes = candidate.bytes_per_pixel_minus_one;
        }
    }
    if (best_distance < 0x7fffffff) {
        return best;
    }
    /* No class-compatible candidate: pick the widest pixel format. */
    unsigned long widest = 0;
    int widest_bytes = -1;
    for (unsigned long w = 0; w < count; w++) {
        if (widest_bytes < formats[w].bytes_per_pixel_minus_one) {
            widest = w;
            widest_bytes = formats[w].bytes_per_pixel_minus_one;
        }
    }
    return widest;
}

// FUNCTION: SURRENDER 0x10008240
srPixelConvert::e_surfaceType srPixelConvert::mapPixelFormat(const PixelFormat& format)
{
    initFormats();
    for (unsigned long i = 0; i < 25; i++) {
        if (format_table[i].format == format) {
            return static_cast<e_surfaceType>(i);
        }
    }
    return static_cast<e_surfaceType>(0x19);
}

// FUNCTION: SURRENDER 0x10008290
void srPixelConvert::mapPixelFormat(e_surfaceType type, PixelFormat& format)
{
    initFormats();
    if (static_cast<int>(type) < 0 || static_cast<int>(type) > 0x18) {
        type = static_cast<e_surfaceType>(0xe);
    }
    format = format_table[type].format;
}

// FUNCTION: SURRENDER 0x100087A0
void srPixelConvert::selectFuncs(const PixelFormat& format, ConversionFunc& write,
                                 ConversionFunc& read)
{
    initFormats();
    unsigned long hash = (format.alpha_shift + format.alpha_bits) ^
                         (format.blue_shift + format.blue_bits) * 4 ^
                         (format.red_shift + format.red_bits) ^ (format.conversion_class << 3) ^
                         format.bytes_per_pixel_minus_one;
    FormatEntry* entry = format_hash[(hash >> 5 & 0x1f) ^ (hash & 0x1f)];
    while (entry != 0) {
        if (entry->format == format) {
            read = entry->read;
            write = entry->write;
            return;
        }
        entry = entry->next;
    }
    switch (format.conversion_class) {
    case 0:
        write = writeRGB;
        read = readRGB;
        return;
    case 1:
        write = writeYUV;
        read = readYUV;
        return;
    case 2:
        write = writeIntensity;
        read = readIntensity;
        return;
    case 3:
        write = writeIndexed;
        read = readIndexed;
    }
}

/* PXXA (palette index + optional alpha) write dispatcher: every case
   quantizes the source color through the surface palette, shifts the index
   into the red channel position and overlays the reduced alpha. */
// FUNCTION: SURRENDER 0x10009890
void __cdecl writeIndexed(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    const unsigned char* alpha_lut = channel_reduce[format->alpha_bits];
    unsigned char alpha_shift = format->alpha_shift;
    unsigned char index_shift = format->red_shift;
    const unsigned long* source = static_cast<const unsigned long*>(info.source);
    unsigned long count = info.count;
    switch (format->bytes_per_pixel_minus_one) {
    case 0: {
        unsigned char* dest = static_cast<unsigned char*>(info.dest);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long color = source[i] & 0xffffff;
            /* reinterpret-ok: quantize reads the packed BGR bytes of the
               color; the palette index function ignores alpha. */
            unsigned char index =
                info.palette->quantize(*reinterpret_cast<const srARGB*>(&color));
            dest[i] = static_cast<unsigned char>(
                index << index_shift |
                alpha_lut[source[i] >> 24] << alpha_shift);
        }
        return;
    }
    case 1: {
        unsigned short* dest = static_cast<unsigned short*>(info.dest);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long color = source[i] & 0xffffff;
            /* reinterpret-ok: quantize reads the packed BGR bytes of the
               color; the palette index function ignores alpha. */
            unsigned char index =
                info.palette->quantize(*reinterpret_cast<const srARGB*>(&color));
            dest[i] = static_cast<unsigned short>(
                index << index_shift |
                alpha_lut[source[i] >> 24] << alpha_shift);
        }
        return;
    }
    case 2: {
        unsigned char* dest = static_cast<unsigned char*>(info.dest);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long color = source[i];
            /* reinterpret-ok: quantize reads the packed BGR bytes of the
               color; the palette index function ignores alpha. */
            unsigned char index =
                info.palette->quantize(*reinterpret_cast<const srARGB*>(&color));
            unsigned long pixel = index << index_shift |
                                  alpha_lut[source[i] >> 24] << alpha_shift;
            dest[0] = static_cast<unsigned char>(pixel);
            dest[1] = static_cast<unsigned char>(pixel >> 8);
            dest[2] = static_cast<unsigned char>(pixel >> 16);
            dest += 3;
        }
        return;
    }
    case 3: {
        unsigned long* dest = static_cast<unsigned long*>(info.dest);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long color = source[i];
            /* reinterpret-ok: quantize reads the packed BGR bytes of the
               color; the palette index function ignores alpha. */
            unsigned char index =
                info.palette->quantize(*reinterpret_cast<const srARGB*>(&color));
            dest[i] = index << index_shift |
                      alpha_lut[source[i] >> 24] << alpha_shift;
        }
    }
    }
}

/* PXXA read dispatcher: the packed index selects a palette entry whose
   packed BGR is kept, then the expanded alpha is overlaid in the top byte. */
// FUNCTION: SURRENDER 0x10009B00
void __cdecl readIndexed(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    unsigned char index_shift = format->red_shift;
    unsigned long index_mask = (1ul << format->red_bits) - 1;
    unsigned long alpha_mask = (1ul << format->alpha_bits) - 1;
    unsigned char alpha_shift = format->alpha_shift;
    const unsigned char* alpha_lut = channel_expand[format->alpha_bits];
    /* reinterpret-ok: palette entries are packed srARGB dwords; only the low
       24 color bits carry over, the alpha byte comes from the packed pixel. */
    const unsigned long* palette = reinterpret_cast<const unsigned long*>(
        info.palette->getPaletteDataPtr());
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    unsigned long count = info.count;
    switch (format->bytes_per_pixel_minus_one) {
    case 0: {
        const unsigned char* source = static_cast<const unsigned char*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            dest[i] = (palette[(pixel >> index_shift) & index_mask] & 0xffffff) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> alpha_shift) & alpha_mask])
                          << 24;
        }
        return;
    }
    case 1: {
        const unsigned short* source = static_cast<const unsigned short*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            dest[i] = (palette[(pixel >> index_shift) & index_mask] & 0xffffff) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> alpha_shift) & alpha_mask])
                          << 24;
        }
        return;
    }
    case 2: {
        const unsigned char* source = static_cast<const unsigned char*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[0] | source[1] << 8 | source[2] << 16;
            dest[i] = (palette[(pixel >> index_shift) & index_mask] & 0xffffff) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> alpha_shift) & alpha_mask])
                          << 24;
            source += 3;
        }
        return;
    }
    case 3: {
        const unsigned long* source = static_cast<const unsigned long*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            dest[i] = (palette[(pixel >> index_shift) & index_mask] & 0xffffff) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> alpha_shift) & alpha_mask])
                          << 24;
        }
    }
    }
}

/* IXXA (intensity + optional alpha) write dispatcher: 8-bit destinations
   compute the luma index inline, wider destinations go through the shared
   intensity kernels. */
// FUNCTION: SURRENDER 0x10009DB0
void __cdecl writeIntensity(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    const unsigned char* intensity_lut = channel_reduce[format->red_bits];
    const unsigned char* alpha_lut = channel_reduce[format->alpha_bits];
    int has_alpha = format->alpha_bits != 0;
    const srARGB* source = static_cast<const srARGB*>(info.source);
    unsigned long count = info.count;
    switch (format->bytes_per_pixel_minus_one) {
    case 0: {
        unsigned char* dest = static_cast<unsigned char*>(info.dest);
        unsigned long i = 0;
        if (has_alpha != 0) {
            for (; i + 2 <= count; i += 2) {
                unsigned long luma = (lutRamp54[source[i].red] +
                                      lutRamp183[source[i].green] +
                                      lutRamp18[source[i].blue]) >> 8;
                dest[i] = intensity_lut[luma] << format->red_shift |
                          alpha_lut[source[i].alpha] << format->alpha_shift;
                luma = (lutRamp54[source[i + 1].red] +
                        lutRamp183[source[i + 1].green] +
                        lutRamp18[source[i + 1].blue]) >> 8;
                dest[i + 1] = intensity_lut[luma] << format->red_shift |
                              alpha_lut[source[i + 1].alpha] << format->alpha_shift;
            }
            for (; i < count; i++) {
                unsigned long luma = (lutRamp54[source[i].red] +
                                      lutRamp183[source[i].green] +
                                      lutRamp18[source[i].blue]) >> 8;
                dest[i] = intensity_lut[luma] << format->red_shift |
                          alpha_lut[source[i].alpha] << format->alpha_shift;
            }
        } else {
            for (; i + 2 <= count; i += 2) {
                unsigned long luma = (lutRamp54[source[i].red] +
                                      lutRamp183[source[i].green] +
                                      lutRamp18[source[i].blue]) >> 8;
                dest[i] = intensity_lut[luma] << format->red_shift;
                luma = (lutRamp54[source[i + 1].red] +
                        lutRamp183[source[i + 1].green] +
                        lutRamp18[source[i + 1].blue]) >> 8;
                dest[i + 1] = intensity_lut[luma] << format->red_shift;
            }
            for (; i < count; i++) {
                unsigned long luma = (lutRamp54[source[i].red] +
                                      lutRamp183[source[i].green] +
                                      lutRamp18[source[i].blue]) >> 8;
                dest[i] = intensity_lut[luma] << format->red_shift;
            }
        }
        return;
    }
    case 1:
        packIntensity16(static_cast<unsigned short*>(info.dest), source, alpha_lut,
                        intensity_lut, format->alpha_shift, format->red_shift, count,
                        has_alpha);
        return;
    case 2:
        packIntensity24(static_cast<unsigned char*>(info.dest), source, alpha_lut,
                        intensity_lut, format->alpha_shift, format->red_shift, count,
                        has_alpha);
        return;
    case 3:
        packIntensity32(static_cast<unsigned long*>(info.dest), source, alpha_lut,
                        intensity_lut, format->alpha_shift, format->red_shift, count,
                        has_alpha);
    }
}

/* IXXA read dispatcher: every source width expands inline through the
   intensity and alpha luts, producing grayscale pixels via lutGray with
   the alpha channel overlaid in the top byte. */
// FUNCTION: SURRENDER 0x1000A0D0
void __cdecl readIntensity(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    unsigned long intensity_mask = (1ul << format->red_bits) - 1;
    unsigned long alpha_mask = (1ul << format->alpha_bits) - 1;
    const unsigned char* intensity_lut = channel_expand[format->red_bits];
    const unsigned char* alpha_lut = channel_expand[format->alpha_bits];
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    unsigned long count = info.count;
    switch (format->bytes_per_pixel_minus_one) {
    case 0: {
        const unsigned char* source = static_cast<const unsigned char*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            const unsigned char* gray =
                lutGray[intensity_lut[(pixel >> format->red_shift) & intensity_mask]];
            /* reinterpret-ok: the gray table entry is a packed BGRA pixel read
               as a dword so the expanded alpha can be ORed into byte 3. */
            dest[i] = *reinterpret_cast<const unsigned long*>(gray) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> format->alpha_shift) & alpha_mask])
                          << 24;
        }
        return;
    }
    case 1: {
        const unsigned short* source = static_cast<const unsigned short*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            const unsigned char* gray =
                lutGray[intensity_lut[(pixel >> format->red_shift) & intensity_mask]];
            /* reinterpret-ok: the gray table entry is a packed BGRA pixel read
               as a dword so the expanded alpha can be ORed into byte 3. */
            dest[i] = *reinterpret_cast<const unsigned long*>(gray) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> format->alpha_shift) & alpha_mask])
                          << 24;
        }
        return;
    }
    case 2: {
        const unsigned char* source = static_cast<const unsigned char*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            /* reinterpret-ok: 24-bit source records load their high two bytes as
               a word plus the low byte separately. */
            unsigned long pixel =
                *reinterpret_cast<const unsigned short*>(source + 1) * 0x100 + source[0];
            const unsigned char* gray =
                lutGray[intensity_lut[(pixel >> format->red_shift) & intensity_mask]];
            /* reinterpret-ok: the gray table entry is a packed BGRA pixel read
               as a dword so the expanded alpha can be ORed into byte 3. */
            dest[i] = *reinterpret_cast<const unsigned long*>(gray) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> format->alpha_shift) & alpha_mask])
                          << 24;
            source += 3;
        }
        return;
    }
    case 3: {
        const unsigned long* source = static_cast<const unsigned long*>(info.source);
        for (unsigned long i = 0; i < count; i++) {
            unsigned long pixel = source[i];
            const unsigned char* gray =
                lutGray[intensity_lut[(pixel >> format->red_shift) & intensity_mask]];
            /* reinterpret-ok: the gray table entry is a packed BGRA pixel read
               as a dword so the expanded alpha can be ORed into byte 3. */
            dest[i] = *reinterpret_cast<const unsigned long*>(gray) |
                      static_cast<unsigned long>(
                          alpha_lut[(pixel >> format->alpha_shift) & alpha_mask])
                          << 24;
        }
    }
    }
}

// FUNCTION: SURRENDER 0x1000A3B0
void __cdecl writeRGB(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    const unsigned char* luts[4];
    unsigned char shifts[4];
    luts[0] = channel_reduce[format->red_bits];
    luts[1] = channel_reduce[format->green_bits];
    luts[2] = channel_reduce[format->blue_bits];
    luts[3] = channel_reduce[format->alpha_bits];
    shifts[0] = format->red_shift;
    shifts[1] = format->green_shift;
    shifts[2] = format->blue_shift;
    shifts[3] = format->alpha_shift;
    int has_alpha = format->alpha_bits != 0;
    const srARGB* source = static_cast<const srARGB*>(info.source);
    switch (format->bytes_per_pixel_minus_one) {
    case 0:
        pack8(static_cast<unsigned char*>(info.dest), source, luts, shifts,
              info.count, has_alpha);
        return;
    case 1:
        pack16(static_cast<unsigned short*>(info.dest), source, luts, shifts,
               info.count, has_alpha);
        return;
    case 2:
        pack24(static_cast<unsigned char*>(info.dest), source, luts, shifts,
               info.count, has_alpha);
        return;
    case 3:
        pack32(static_cast<unsigned long*>(info.dest), source, luts, shifts,
               info.count, has_alpha);
    }
}

// FUNCTION: SURRENDER 0x1000A4D0
void __cdecl readRGB(const srPixelConvert::ConversionInfo& info)
{
    const srPixelConvert::PixelFormat* format = info.format;
    unsigned long masks[4];
    unsigned char shifts[4];
    const unsigned char* luts[4];
    masks[0] = (1ul << format->red_bits) - 1;
    masks[1] = (1ul << format->green_bits) - 1;
    masks[2] = (1ul << format->blue_bits) - 1;
    masks[3] = (1ul << format->alpha_bits) - 1;
    shifts[0] = format->red_shift;
    shifts[1] = format->green_shift;
    shifts[2] = format->blue_shift;
    shifts[3] = format->alpha_shift;
    luts[0] = channel_expand[format->red_bits];
    luts[1] = channel_expand[format->green_bits];
    luts[2] = channel_expand[format->blue_bits];
    luts[3] = channel_expand[format->alpha_bits];
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    switch (format->bytes_per_pixel_minus_one) {
    case 0: {
        const unsigned char* source = static_cast<const unsigned char*>(info.source);
        unsigned long i = 0;
        for (; i + 4 <= info.count; i += 4) {
            unsigned long pixel = source[i];
            dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
            pixel = source[i + 1];
            dest[i + 1] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                          luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                          luts[2][(pixel >> shifts[2]) & masks[2]] |
                          luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
            pixel = source[i + 2];
            dest[i + 2] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                          luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                          luts[2][(pixel >> shifts[2]) & masks[2]] |
                          luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
            pixel = source[i + 3];
            dest[i + 3] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                          luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                          luts[2][(pixel >> shifts[2]) & masks[2]] |
                          luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        }
        for (; i < info.count; i++) {
            unsigned long pixel = source[i];
            dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        }
        return;
    }
    case 1:
        unpack16(dest, static_cast<const unsigned short*>(info.source), luts, shifts,
                 masks, info.count);
        return;
    case 2:
        unpack24(dest, static_cast<const unsigned char*>(info.source), luts, shifts,
                 masks, info.count);
        return;
    case 3:
        unpack32(dest, static_cast<const unsigned long*>(info.source), luts, shifts,
                 masks, info.count);
    }
}

/* Per-format overrides installed by initFormats() for formats whose
   converter does not fit the generic kernels: the 8-bit indexed pair
   delegates to the vector processor copy, the 32-bit color-keyed formats
   mask through _and/_or, and the packed formats run dedicated loops. */
// FUNCTION: SURRENDER 0x1000A8C0
void __cdecl writeRGB24(const srPixelConvert::ConversionInfo& info)
{
    unsigned char* dest = static_cast<unsigned char*>(info.dest);
    const unsigned long* source = static_cast<const unsigned long*>(info.source);
    for (unsigned long i = info.count; i > 0; i--) {
        unsigned long pixel = *source++;
        dest[0] = static_cast<unsigned char>(pixel >> 16);
        dest[1] = static_cast<unsigned char>(pixel >> 8);
        dest[2] = static_cast<unsigned char>(pixel);
        dest += 3;
    }
}

// FUNCTION: SURRENDER 0x1000A900
void __cdecl readRGB24(const srPixelConvert::ConversionInfo& info)
{
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    const unsigned char* source = static_cast<const unsigned char*>(info.source);
    for (unsigned long i = info.count; i > 0; i--) {
        unsigned long pixel = source[0] | 0xffffff00;
        pixel = pixel << 8 | source[1];
        pixel = pixel << 8 | source[2];
        *dest++ = pixel;
        source += 3;
    }
}

// FUNCTION: SURRENDER 0x1000A950
void __cdecl writeRGB332(const srPixelConvert::ConversionInfo& info)
{
    if (info.count != 0 && info.dest != info.source) {
        srVectorProcessor::memcopy(info.dest, info.source, info.count * 4);
    }
}

// FUNCTION: SURRENDER 0x1000A980
void __cdecl readRGB332(const srPixelConvert::ConversionInfo& info)
{
    if (info.count != 0 && info.dest != info.source) {
        srVectorProcessor::memcopy(info.dest, info.source, info.count * 4);
    }
}

// FUNCTION: SURRENDER 0x1000A9B0
void __cdecl writeBGRX(const srPixelConvert::ConversionInfo& info)
{
    srVectorProcessor::bitwiseAnd(static_cast<SRDWORD*>(info.dest),
                                  static_cast<const SRDWORD*>(info.source), 0xffffff,
                                  info.count);
}

// FUNCTION: SURRENDER 0x1000A9E0
void __cdecl readBGRX(const srPixelConvert::ConversionInfo& info)
{
    srVectorProcessor::bitwiseOr(static_cast<SRDWORD*>(info.dest),
                                 static_cast<const SRDWORD*>(info.source), 0xff000000,
                                 info.count);
}

/* format_table[0x17] write/read: rotate each BGRA pixel one byte lane so
   red leads the record on write and BGRA is restored on read. */
// FUNCTION: SURRENDER 0x1000AA10
void __cdecl writeABGR(const srPixelConvert::ConversionInfo& info)
{
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    const unsigned long* source = static_cast<const unsigned long*>(info.source);
    unsigned long i = 0;
    for (; i + 8 <= info.count; i += 8) {
        dest[i] = source[i] << 8 | source[i] >> 24;
        dest[i + 1] = source[i + 1] << 8 | source[i + 1] >> 24;
        dest[i + 2] = source[i + 2] << 8 | source[i + 2] >> 24;
        dest[i + 3] = source[i + 3] << 8 | source[i + 3] >> 24;
        dest[i + 4] = source[i + 4] << 8 | source[i + 4] >> 24;
        dest[i + 5] = source[i + 5] << 8 | source[i + 5] >> 24;
        dest[i + 6] = source[i + 6] << 8 | source[i + 6] >> 24;
        dest[i + 7] = source[i + 7] << 8 | source[i + 7] >> 24;
    }
    for (; i < info.count; i++) {
        dest[i] = source[i] << 8 | source[i] >> 24;
    }
}

// FUNCTION: SURRENDER 0x1000AB70
void __cdecl readABGR(const srPixelConvert::ConversionInfo& info)
{
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    const unsigned long* source = static_cast<const unsigned long*>(info.source);
    unsigned long i = 0;
    for (; i + 8 <= info.count; i += 8) {
        dest[i] = source[i] << 24 | source[i] >> 8;
        dest[i + 1] = source[i + 1] << 24 | source[i + 1] >> 8;
        dest[i + 2] = source[i + 2] << 24 | source[i + 2] >> 8;
        dest[i + 3] = source[i + 3] << 24 | source[i + 3] >> 8;
        dest[i + 4] = source[i + 4] << 24 | source[i + 4] >> 8;
        dest[i + 5] = source[i + 5] << 24 | source[i + 5] >> 8;
        dest[i + 6] = source[i + 6] << 24 | source[i + 6] >> 8;
        dest[i + 7] = source[i + 7] << 24 | source[i + 7] >> 8;
    }
    for (; i < info.count; i++) {
        dest[i] = source[i] << 24 | source[i] >> 8;
    }
}

/* format_table[8] write/read: 32-bit BGRA packed to RGB555 through the
   5-bit reduction table, and expanded back with alpha forced opaque. */
// FUNCTION: SURRENDER 0x1000ACD0
void __cdecl writeRGB555(const srPixelConvert::ConversionInfo& info)
{
    unsigned short* dest = static_cast<unsigned short*>(info.dest);
    const srARGB* source = static_cast<const srARGB*>(info.source);
    unsigned long i = 0;
    for (; i + 4 <= info.count; i += 4) {
        dest[i] = lutReduce32[source[i].red] << 10 | lutReduce32[source[i].green] << 5 |
                  lutReduce32[source[i].blue];
        dest[i + 1] = lutReduce32[source[i + 1].red] << 10 |
                      lutReduce32[source[i + 1].green] << 5 |
                      lutReduce32[source[i + 1].blue];
        dest[i + 2] = lutReduce32[source[i + 2].red] << 10 |
                      lutReduce32[source[i + 2].green] << 5 |
                      lutReduce32[source[i + 2].blue];
        dest[i + 3] = lutReduce32[source[i + 3].red] << 10 |
                      lutReduce32[source[i + 3].green] << 5 |
                      lutReduce32[source[i + 3].blue];
    }
    for (; i < info.count; i++) {
        dest[i] = lutReduce32[source[i].red] << 10 | lutReduce32[source[i].green] << 5 |
                  lutReduce32[source[i].blue];
    }
}

// FUNCTION: SURRENDER 0x1000AE80
void __cdecl readRGB555(const srPixelConvert::ConversionInfo& info)
{
    unsigned long* dest = static_cast<unsigned long*>(info.dest);
    const unsigned short* source = static_cast<const unsigned short*>(info.source);
    unsigned long i = 0;
    for (; i + 4 <= info.count; i += 4) {
        unsigned long pixel = source[i];
        dest[i] = 0xff000000 | lutExpand32[pixel >> 10] << 16 |
                  lutExpand32[pixel >> 5 & 0x1f] << 8 | lutExpand32[pixel & 0x1f];
        pixel = source[i + 1];
        dest[i + 1] = 0xff000000 | lutExpand32[pixel >> 10] << 16 |
                      lutExpand32[pixel >> 5 & 0x1f] << 8 | lutExpand32[pixel & 0x1f];
        pixel = source[i + 2];
        dest[i + 2] = 0xff000000 | lutExpand32[pixel >> 10] << 16 |
                      lutExpand32[pixel >> 5 & 0x1f] << 8 | lutExpand32[pixel & 0x1f];
        pixel = source[i + 3];
        dest[i + 3] = 0xff000000 | lutExpand32[pixel >> 10] << 16 |
                      lutExpand32[pixel >> 5 & 0x1f] << 8 | lutExpand32[pixel & 0x1f];
    }
    for (; i < info.count; i++) {
        unsigned long pixel = source[i];
        dest[i] = 0xff000000 | lutExpand32[pixel >> 10] << 16 |
                  lutExpand32[pixel >> 5 & 0x1f] << 8 | lutExpand32[pixel & 0x1f];
    }
}

/* Intensity write kernels: 32-bit BGRA source to 16/24/32-bit IXXA
   records. The luma index sums the fixed-point channel weight ramps. */
// FUNCTION: SURRENDER 0x1000BAD0
static void packIntensity16(unsigned short* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 2 <= count; i += 2) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                      << intensity_shift;
            dest[i + 1] = intensity_lut[(lutRamp54[source[i + 1].red] +
                                         lutRamp183[source[i + 1].green] +
                                         lutRamp18[source[i + 1].blue]) >> 8]
                          << intensity_shift;
        }
        for (; i < count; i++) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                      << intensity_shift;
        }
    } else {
        for (; i + 2 <= count; i += 2) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                          << intensity_shift |
                      alpha_lut[source[i].alpha] << alpha_shift;
            dest[i + 1] = intensity_lut[(lutRamp54[source[i + 1].red] +
                                         lutRamp183[source[i + 1].green] +
                                         lutRamp18[source[i + 1].blue]) >> 8]
                              << intensity_shift |
                          alpha_lut[source[i + 1].alpha] << alpha_shift;
        }
        for (; i < count; i++) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                          << intensity_shift |
                      alpha_lut[source[i].alpha] << alpha_shift;
        }
    }
}

// FUNCTION: SURRENDER 0x1000BD60
static void packIntensity24(unsigned char* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 2 <= count; i += 2) {
            unsigned long value = intensity_lut[(lutRamp54[source[i].red] +
                                                 lutRamp183[source[i].green] +
                                                 lutRamp18[source[i].blue]) >> 8]
                                  << intensity_shift;
            /* reinterpret-ok: the 24-bit pixel record stores its low word plus
               high byte separately. */
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            value = intensity_lut[(lutRamp54[source[i + 1].red] +
                                   lutRamp183[source[i + 1].green] +
                                   lutRamp18[source[i + 1].blue]) >> 8]
                    << intensity_shift;
            *reinterpret_cast<unsigned short*>(dest + 3) =
                static_cast<unsigned short>(value);
            dest[5] = static_cast<unsigned char>(value >> 16);
            dest += 6;
        }
        for (; i < count; i++) {
            unsigned long value = intensity_lut[(lutRamp54[source[i].red] +
                                                 lutRamp183[source[i].green] +
                                                 lutRamp18[source[i].blue]) >> 8]
                                  << intensity_shift;
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            dest += 3;
        }
    } else {
        for (; i + 2 <= count; i += 2) {
            unsigned long value =
                intensity_lut[(lutRamp54[source[i].red] +
                               lutRamp183[source[i].green] +
                               lutRamp18[source[i].blue]) >> 8]
                    << intensity_shift |
                alpha_lut[source[i].alpha] << alpha_shift;
            /* reinterpret-ok: the 24-bit pixel record stores its low word plus
               high byte separately. */
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            value = intensity_lut[(lutRamp54[source[i + 1].red] +
                                   lutRamp183[source[i + 1].green] +
                                   lutRamp18[source[i + 1].blue]) >> 8]
                        << intensity_shift |
                    alpha_lut[source[i + 1].alpha] << alpha_shift;
            *reinterpret_cast<unsigned short*>(dest + 3) =
                static_cast<unsigned short>(value);
            dest[5] = static_cast<unsigned char>(value >> 16);
            dest += 6;
        }
        for (; i < count; i++) {
            unsigned long value =
                intensity_lut[(lutRamp54[source[i].red] +
                               lutRamp183[source[i].green] +
                               lutRamp18[source[i].blue]) >> 8]
                    << intensity_shift |
                alpha_lut[source[i].alpha] << alpha_shift;
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            dest += 3;
        }
    }
}

// FUNCTION: SURRENDER 0x1000C0A0
static void packIntensity32(unsigned long* dest, const srARGB* source,
                            const unsigned char* alpha_lut,
                            const unsigned char* intensity_lut,
                            unsigned char alpha_shift, unsigned char intensity_shift,
                            unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 2 <= count; i += 2) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                      << intensity_shift;
            dest[i + 1] = intensity_lut[(lutRamp54[source[i + 1].red] +
                                         lutRamp183[source[i + 1].green] +
                                         lutRamp18[source[i + 1].blue]) >> 8]
                          << intensity_shift;
        }
        for (; i < count; i++) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                      << intensity_shift;
        }
    } else {
        for (; i + 2 <= count; i += 2) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                          << intensity_shift |
                      alpha_lut[source[i].alpha] << alpha_shift;
            dest[i + 1] = intensity_lut[(lutRamp54[source[i + 1].red] +
                                         lutRamp183[source[i + 1].green] +
                                         lutRamp18[source[i + 1].blue]) >> 8]
                              << intensity_shift |
                          alpha_lut[source[i + 1].alpha] << alpha_shift;
        }
        for (; i < count; i++) {
            dest[i] = intensity_lut[(lutRamp54[source[i].red] +
                                     lutRamp183[source[i].green] +
                                     lutRamp18[source[i].blue]) >> 8]
                          << intensity_shift |
                      alpha_lut[source[i].alpha] << alpha_shift;
        }
    }
}

/* Generic write kernels: 32-bit BGRA source packed through the channel
   reduction luts into 8/16/24/32-bit records. */
// FUNCTION: SURRENDER 0x1000C350
static void pack8(unsigned char* dest, const srARGB* source,
                  const unsigned char* const* luts, const unsigned char* shifts,
                  unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
        }
    } else {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[3][source[i + 1].alpha] << shifts[3] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[3][source[i + 2].alpha] << shifts[3] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[3][source[i + 3].alpha] << shifts[3] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
        }
    }
}

// FUNCTION: SURRENDER 0x1000C800
static void pack16(unsigned short* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
        }
    } else {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[3][source[i + 1].alpha] << shifts[3] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[3][source[i + 2].alpha] << shifts[3] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[3][source[i + 3].alpha] << shifts[3] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
        }
    }
}

// FUNCTION: SURRENDER 0x1000CC90
static void pack24(unsigned char* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 4 <= count; i += 4) {
            unsigned long value = luts[0][source[i].red] << shifts[0] |
                                  luts[1][source[i].green] << shifts[1] |
                                  luts[2][source[i].blue] << shifts[2];
            /* reinterpret-ok: the 24-bit pixel record stores its low word plus
               high byte separately. */
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 1].red] << shifts[0] |
                    luts[1][source[i + 1].green] << shifts[1] |
                    luts[2][source[i + 1].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 3) =
                static_cast<unsigned short>(value);
            dest[5] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 2].red] << shifts[0] |
                    luts[1][source[i + 2].green] << shifts[1] |
                    luts[2][source[i + 2].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 6) =
                static_cast<unsigned short>(value);
            dest[8] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 3].red] << shifts[0] |
                    luts[1][source[i + 3].green] << shifts[1] |
                    luts[2][source[i + 3].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 9) =
                static_cast<unsigned short>(value);
            dest[11] = static_cast<unsigned char>(value >> 16);
            dest += 12;
        }
        for (; i < count; i++) {
            unsigned long value = luts[0][source[i].red] << shifts[0] |
                                  luts[1][source[i].green] << shifts[1] |
                                  luts[2][source[i].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            dest += 3;
        }
    } else {
        for (; i + 4 <= count; i += 4) {
            unsigned long value = luts[0][source[i].red] << shifts[0] |
                                  luts[1][source[i].green] << shifts[1] |
                                  luts[3][source[i].alpha] << shifts[3] |
                                  luts[2][source[i].blue] << shifts[2];
            /* reinterpret-ok: the 24-bit pixel record stores its low word plus
               high byte separately. */
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 1].red] << shifts[0] |
                    luts[1][source[i + 1].green] << shifts[1] |
                    luts[3][source[i + 1].alpha] << shifts[3] |
                    luts[2][source[i + 1].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 3) =
                static_cast<unsigned short>(value);
            dest[5] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 2].red] << shifts[0] |
                    luts[1][source[i + 2].green] << shifts[1] |
                    luts[3][source[i + 2].alpha] << shifts[3] |
                    luts[2][source[i + 2].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 6) =
                static_cast<unsigned short>(value);
            dest[8] = static_cast<unsigned char>(value >> 16);
            value = luts[0][source[i + 3].red] << shifts[0] |
                    luts[1][source[i + 3].green] << shifts[1] |
                    luts[3][source[i + 3].alpha] << shifts[3] |
                    luts[2][source[i + 3].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest + 9) =
                static_cast<unsigned short>(value);
            dest[11] = static_cast<unsigned char>(value >> 16);
            dest += 12;
        }
        for (; i < count; i++) {
            unsigned long value = luts[0][source[i].red] << shifts[0] |
                                  luts[1][source[i].green] << shifts[1] |
                                  luts[3][source[i].alpha] << shifts[3] |
                                  luts[2][source[i].blue] << shifts[2];
            *reinterpret_cast<unsigned short*>(dest) =
                static_cast<unsigned short>(value);
            dest[2] = static_cast<unsigned char>(value >> 16);
            dest += 3;
        }
    }
}

// FUNCTION: SURRENDER 0x1000D280
static void pack32(unsigned long* dest, const srARGB* source,
                   const unsigned char* const* luts, const unsigned char* shifts,
                   unsigned long count, int has_alpha)
{
    unsigned long i = 0;
    if (has_alpha == 0) {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[2][source[i].blue] << shifts[2];
        }
    } else {
        for (; i + 4 <= count; i += 4) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
            dest[i + 1] = luts[0][source[i + 1].red] << shifts[0] |
                          luts[1][source[i + 1].green] << shifts[1] |
                          luts[3][source[i + 1].alpha] << shifts[3] |
                          luts[2][source[i + 1].blue] << shifts[2];
            dest[i + 2] = luts[0][source[i + 2].red] << shifts[0] |
                          luts[1][source[i + 2].green] << shifts[1] |
                          luts[3][source[i + 2].alpha] << shifts[3] |
                          luts[2][source[i + 2].blue] << shifts[2];
            dest[i + 3] = luts[0][source[i + 3].red] << shifts[0] |
                          luts[1][source[i + 3].green] << shifts[1] |
                          luts[3][source[i + 3].alpha] << shifts[3] |
                          luts[2][source[i + 3].blue] << shifts[2];
        }
        for (; i < count; i++) {
            dest[i] = luts[0][source[i].red] << shifts[0] |
                      luts[1][source[i].green] << shifts[1] |
                      luts[3][source[i].alpha] << shifts[3] |
                      luts[2][source[i].blue] << shifts[2];
        }
    }
}

/* Generic read kernels: packed 16/24/32-bit source records expanded to
   32-bit BGRA through the channel expansion luts, shifts and masks. */
// FUNCTION: SURRENDER 0x1000D740
static void unpack16(unsigned long* dest, const unsigned short* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count)
{
    unsigned long i = 0;
    for (; i + 4 <= count; i += 4) {
        unsigned long pixel = source[i];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 1];
        dest[i + 1] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 2];
        dest[i + 2] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 3];
        dest[i + 3] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
    }
    for (; i < count; i++) {
        unsigned long pixel = source[i];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
    }
}

// FUNCTION: SURRENDER 0x1000DA20
static void unpack24(unsigned long* dest, const unsigned char* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count)
{
    unsigned long i = 0;
    for (; i + 4 <= count; i += 4) {
        /* reinterpret-ok: 24-bit source records load their high two bytes as a
           word plus the low byte separately. */
        unsigned long pixel =
            *reinterpret_cast<const unsigned short*>(source + 1) * 0x100 + source[0];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = *reinterpret_cast<const unsigned short*>(source + 4) * 0x100 +
                source[3];
        dest[i + 1] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = *reinterpret_cast<const unsigned short*>(source + 7) * 0x100 +
                source[6];
        dest[i + 2] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = *reinterpret_cast<const unsigned short*>(source + 10) * 0x100 +
                source[9];
        dest[i + 3] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        source += 12;
    }
    for (; i < count; i++) {
        unsigned long pixel =
            *reinterpret_cast<const unsigned short*>(source + 1) * 0x100 + source[0];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        source += 3;
    }
}

// FUNCTION: SURRENDER 0x1000DD60
static void unpack32(unsigned long* dest, const unsigned long* source,
                     const unsigned char* const* luts, const unsigned char* shifts,
                     const unsigned long* masks, unsigned long count)
{
    unsigned long i = 0;
    for (; i + 4 <= count; i += 4) {
        unsigned long pixel = source[i];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 1];
        dest[i + 1] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 2];
        dest[i + 2] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
        pixel = source[i + 3];
        dest[i + 3] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                      luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                      luts[2][(pixel >> shifts[2]) & masks[2]] |
                      luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
    }
    for (; i < count; i++) {
        unsigned long pixel = source[i];
        dest[i] = luts[0][(pixel >> shifts[0]) & masks[0]] << 16 |
                  luts[1][(pixel >> shifts[1]) & masks[1]] << 8 |
                  luts[2][(pixel >> shifts[2]) & masks[2]] |
                  luts[3][(pixel >> shifts[3]) & masks[3]] << 24;
    }
}
