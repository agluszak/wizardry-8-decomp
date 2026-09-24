#include "surrender/srPixelConvert.h"

#include <stdio.h>
#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srVariableTimer.h"

/* Conversion routines stored in the format table. The generic pair is
   selected by PixelFormat::conversion_class; the per-entry overrides cover
   formats whose converter does not fit a generic kernel. The MMX pair is
   installed by initFormats() when the CPU reports the feature bit. Their
   bodies are still unrecovered, so they stay link-unresolved like the other
   first-party gaps. */
void __cdecl Function1000A3B0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A4D0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function100088D0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function10008F70(const srPixelConvert::ConversionInfo& info);
void __cdecl Function10009DB0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A0D0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function10009890(const srPixelConvert::ConversionInfo& info);
void __cdecl Function10009B00(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000ACD0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000AE80(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A9B0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A9E0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A950(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A980(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000AA10(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000AB70(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A8C0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000A900(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B570(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B250(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B6A0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B440(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B8A0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B330(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B7C0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B150(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B9D0(const srPixelConvert::ConversionInfo& info);
void __cdecl Function1000B050(const srPixelConvert::ConversionInfo& info);

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
            entry->write = Function1000A3B0;
            entry->read = Function1000A4D0;
            break;
        case 1:
            entry->write = Function100088D0;
            entry->read = Function10008F70;
            break;
        case 2:
            entry->write = Function10009DB0;
            entry->read = Function1000A0D0;
            break;
        case 3:
            entry->write = Function10009890;
            entry->read = Function10009B00;
        }
    }
    format_table[8].write = Function1000ACD0;
    format_table[8].read = Function1000AE80;
    format_table[0xd].write = Function1000A9B0;
    format_table[0xd].read = Function1000A9E0;
    format_table[0x11].write = Function1000A950;
    format_table[0x11].read = Function1000A980;
    format_table[0x17].write = Function1000AA10;
    format_table[0x17].read = Function1000AB70;
    format_table[0x18].write = Function1000A8C0;
    format_table[0x18].read = Function1000A900;
    if ((srCore.getTimer()->m_cpu_features & 0x800000) != 0) {
        format_table[2].write = Function1000B570;
        format_table[2].read = Function1000B250;
        format_table[7].write = Function1000B6A0;
        format_table[7].read = Function1000B440;
        format_table[9].write = Function1000B8A0;
        format_table[9].read = Function1000B330;
        format_table[0xb].write = Function1000B7C0;
        format_table[0xb].read = Function1000B150;
        format_table[0xe].write = Function1000B9D0;
        format_table[0xe].read = Function1000B050;
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
void srPixelConvert::PixelFormat::getName(char* name)
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
        write = Function1000A3B0;
        read = Function1000A4D0;
        return;
    case 1:
        write = Function100088D0;
        read = Function10008F70;
        return;
    case 2:
        write = Function10009DB0;
        read = Function1000A0D0;
        return;
    case 3:
        write = Function10009890;
        read = Function10009B00;
    }
}
