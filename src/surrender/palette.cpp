/* Recoverable from sr.dll */

#include "surrender/srPalette.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include "surrender/srColorSurfaceIFace.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srImporter.h"

/* Retail compares and copies palette entries as raw dwords; srARGB is a
   four-byte packed color, so the elementwise loops lower to dword moves. */

/* Squared channel distance scaled by the luma weights 299/587/114; the biased
   pointers index by (channel - reference) so negative differences work. */
// GLOBAL: SURRENDER 0x100A02A8
static unsigned long dist_green_table[0x200];
// GLOBAL: SURRENDER 0x100A0AA8
static unsigned long dist_blue_table[0x200];
// GLOBAL: SURRENDER 0x100A12A8
static unsigned long dist_red_table[0x200];
// GLOBAL: SURRENDER 0x10098320
static unsigned long* dist_red = dist_red_table + 0x100;
// GLOBAL: SURRENDER 0x10098324
static unsigned long* dist_green = dist_green_table + 0x100;
// GLOBAL: SURRENDER 0x10098328
static unsigned long* dist_blue = dist_blue_table + 0x100;
// GLOBAL: SURRENDER 0x100A1AA8
int srPalette::Quantizer::initialized = 0;

// FUNCTION: SURRENDER 0x10006940
void srPalette::Quantizer::checkLuts()
{
    if (initialized == 0) {
        for (long index = 0; index < 0x200; ++index) {
            float delta = index - 256.0f;
            long squared = (long)(delta * delta);
            dist_red_table[index] = squared * 299;
            dist_green_table[index] = squared * 587;
            dist_blue_table[index] = squared * 114;
        }
        initialized = 1;
    }
}

// FUNCTION: SURRENDER 0x100069C0
void srPalette::Quantizer::initRG(long red_lo, long red_hi, long green_lo, long green_hi)
{
    long best = 0x40000000;
    long index;
    for (index = 0; index < color_count; ++index) {
        long red_bound = red_hi;
        if ((red_lo + red_hi) / 2 <= palette[index].red) {
            red_bound = red_lo;
        }
        long green_bound = green_hi;
        if ((green_lo + green_hi) / 2 <= palette[index].green) {
            green_bound = green_lo;
        }
        long dist = dist_red[palette[index].red - red_bound] +
                    dist_green[palette[index].green - green_bound];
        if (dist < best) {
            best = dist;
        }
    }
    long count = 0;
    for (index = 0; index < color_count; ++index) {
        unsigned long red = palette[index].red;
        unsigned long green = palette[index].green;
        long dist = 0;
        long bound = red_lo;
        if ((long)red < red_lo || (bound = red_hi, red_hi < (long)red)) {
            dist = dist_red[red - bound];
        }
        bound = green_lo;
        if ((long)green < green_lo || (bound = green_hi, green_hi < (long)green)) {
            dist += dist_green[green - bound];
        }
        if (dist < best) {
            entries[count].red = red;
            entries[count].green = green;
            entries[count].blue = palette[index].blue;
            entries[count].index = index;
            ++count;
        }
    }
    entry_count = count;
}

// FUNCTION: SURRENDER 0x10006B20
unsigned char srPalette::Quantizer::findRG(long red, long green)
{
    long best = 0x40000000;
    long selected = 0;
    for (long index = 0; index < entry_count; ++index) {
        long dist = dist_red[entries[index].red - red] + dist_green[entries[index].green - green];
        if (dist < best) {
            best = dist;
            selected = index;
        }
    }
    return (unsigned char)entries[selected].index;
}

// FUNCTION: SURRENDER 0x10006BB0
void srPalette::Quantizer::createRGTable()
{
    long red_steps = 1 << (red_bits & 0x1f);
    long green_steps = 1 << (green_bits & 0x1f);
    long red_blocks = (red_steps + ((red_steps >> 0x1f) & 0xf)) >> 4;
    long green_blocks = (green_steps + ((green_steps >> 0x1f) & 0xf)) >> 4;
    long red_size = 0x10;
    if (red_blocks == 0) {
        red_blocks = 1;
        red_size = red_steps;
    }
    long green_size = 0x10;
    if (green_blocks == 0) {
        green_blocks = 1;
        green_size = green_steps;
    }
    long green_base = 0;
    for (long green_block = 0; green_block < green_blocks; ++green_block) {
        long red_base = 0;
        for (long red_block = 0; red_block < red_blocks; ++red_block) {
            initRG(red_base, red_base + red_size - 1, green_base, green_base + green_size - 1);
            for (long red = 0; red < red_size; ++red) {
                for (long green = 0; green < green_size; ++green) {
                    lut_rg[red_base + red + (green_base + green) * 0x100] =
                        findRG(red_base + red, green_base + green);
                }
            }
            red_base += red_size;
        }
        green_base += green_size;
    }
}

// FUNCTION: SURRENDER 0x10006D30
unsigned char srPalette::Quantizer::findRGB(long blue)
{
    long best = 0x40000000;
    long selected = 0;
    for (long index = 0; index < entry_count; ++index) {
        long dist = dist_blue[entries[index].blue - blue] + rg_dist[index];
        if (dist < best) {
            best = dist;
            selected = index;
        }
    }
    return (unsigned char)entries[selected].index;
}

// FUNCTION: SURRENDER 0x10006DB0
void srPalette::Quantizer::initRGB(long red, long green, long blue_lo, long blue_hi)
{
    long best = 0x40000000;
    long index;
    for (index = 0; index < color_count; ++index) {
        if (duplicate[index] == 0) {
            long blue_bound = blue_hi;
            if ((blue_hi + blue_lo) / 2 <= palette[index].blue) {
                blue_bound = blue_lo;
            }
            long dist = dist_red[palette[index].red - red] +
                        dist_green[palette[index].green - green] +
                        dist_blue[palette[index].blue - blue_bound];
            if (dist < best) {
                best = dist;
            }
        }
    }
    long count = 0;
    for (index = 0; index < color_count; ++index) {
        if (duplicate[index] == 0) {
            long rg = dist_red[palette[index].red - red] + dist_green[palette[index].green - green];
            unsigned long blue = palette[index].blue;
            long dist = rg;
            long bound = blue_lo;
            if ((long)blue < blue_lo || (bound = blue_hi, blue_hi < (long)blue)) {
                dist = rg + dist_blue[blue - bound];
            }
            if (dist < best) {
                rg_dist[count] = rg;
                entries[count].red = palette[index].red;
                entries[count].green = palette[index].green;
                entries[count].blue = blue;
                entries[count].index = index;
                ++count;
            }
        }
    }
    entry_count = count;
}

// FUNCTION: SURRENDER 0x10006F60
void srPalette::Quantizer::partitionRGB(long blue_lo, long blue_hi)
{
    lut_row[blue_lo] = findRGB(blue_lo);
    lut_row[blue_hi] = findRGB(blue_hi);
    long next = blue_lo + 1;
    if (next < blue_hi) {
        while (lut_row[blue_lo] != lut_row[blue_hi]) {
            blue_lo = (blue_lo + blue_hi) / 2;
            partitionRGB(next, blue_lo - 1);
            blue_hi = blue_hi - 1;
            lut_row[blue_lo] = findRGB(blue_lo);
            lut_row[blue_hi] = findRGB(blue_hi);
            next = blue_lo + 1;
            if (blue_hi <= next) {
                return;
            }
        }
        memset(lut_row + blue_lo + 1, lut_row[blue_lo], blue_hi - blue_lo - 1);
    }
}

// FUNCTION: SURRENDER 0x10007080
void srPalette::Quantizer::createRGBTable()
{
    long blue_steps = 1 << (blue_bits & 0x1f);
    long blue_blocks = (blue_steps + ((blue_steps >> 0x1f) & 0x7f)) >> 7;
    long blue_size = 0x80;
    if (blue_blocks == 0) {
        blue_blocks = 1;
        blue_size = blue_steps;
    }
    unsigned char* row = lut_rgb;
    for (long index = 0; index < 0x100; ++index) {
        lut_row = row;
        unsigned char red = palette[index].red;
        unsigned char green = palette[index].green;
        for (long block = 0; block < blue_blocks; ++block) {
            long blue_lo = block * blue_size;
            initRGB(red, green, blue_lo, blue_lo + blue_size - 1);
            partitionRGB(blue_lo, blue_lo + blue_size - 1);
        }
        row += 0x100;
    }
}

// FUNCTION: SURRENDER 0x10007160
void srPalette::Quantizer::setPalette(srARGB* colors, long color_count_, unsigned char* duplicates,
                                      unsigned char red_bits_, unsigned char green_bits_,
                                      unsigned char blue_bits_)
{
    checkLuts();
    color_count = color_count_;
    blue_bits = blue_bits_;
    entry_count = 0;
    lut_row = 0;
    red_bits = red_bits_;
    green_bits = green_bits_;
    memset(palette, 0, sizeof(palette));
    memset(duplicate, 0, sizeof(duplicate));
    memset(entries, 0, sizeof(entries));
    memset(rg_dist, 0, sizeof(rg_dist));
    memset(lut_rg, 0, sizeof(lut_rg));
    memset(lut_rgb, 0, sizeof(lut_rgb));
    if (duplicates != 0) {
        memcpy(duplicate, duplicates, sizeof(duplicate));
    }
    if (0 < color_count * 4) {
        memcpy(palette, colors, color_count * 4);
    }
    long index;
    for (index = 0; index < color_count; ++index) {
        palette[index].alpha = 0xff;
    }
    for (index = 0; index < color_count; ++index) {
        for (long prior = 0; prior < index && duplicate[index] == 0; ++prior) {
            if (palette[prior].red == palette[index].red &&
                palette[prior].green == palette[index].green &&
                palette[prior].blue == palette[index].blue) {
                duplicate[index] = 1;
            }
        }
    }
    createRGTable();
    createRGBTable();
    if (duplicates != 0) {
        memcpy(duplicate, duplicates, sizeof(duplicate));
    } else {
        memset(duplicate, 0, sizeof(duplicate));
    }
}

// FUNCTION: SURRENDER 0x10007510
unsigned char srPalette::Quantizer::quantize(const srARGB& color)
{
    return lut_rgb[(lut_rg[(color.green << 8) | color.red] << 8) | color.blue];
}

// FUNCTION: SURRENDER 0x10007550
unsigned char srPalette::Quantizer::quantize(unsigned char red, unsigned char green,
                                             unsigned char blue)
{
    return lut_rgb[(lut_rg[(green << 8) | red] << 8) | blue];
}

// FUNCTION: SURRENDER 0x10007590
void srPalette::Quantizer::quantize(unsigned char* indices, const srARGB* colors, long color_count)
{
    for (long index = 0; index < color_count; ++index) {
        indices[index] = quantize(colors[index].red, colors[index].green, colors[index].blue);
    }
}

// FUNCTION: SURRENDER 0x10004B40
srPalette::Quantizer::Quantizer(srARGB* colors, long color_count, unsigned char* duplicates,
                                unsigned char red_bits, unsigned char green_bits,
                                unsigned char blue_bits)
{
    setPalette(colors, color_count, duplicates, red_bits, green_bits, blue_bits);
}

// FUNCTION: SURRENDER 0x10004B70
srPalette::Quantizer::Quantizer()
{
    color_count = 0;
}

// FUNCTION: SURRENDER 0x10004B80
srPalette::Quantizer::~Quantizer() {}

// FUNCTION: SURRENDER 0x10004D00
srPalette::Quantizer::Quantizer(const Quantizer& other)
{
    long index;
    for (index = 0; index < 0x10000; ++index) {
        lut_rg[index] = other.lut_rg[index];
    }
    for (index = 0; index < 0x10000; ++index) {
        lut_rgb[index] = other.lut_rgb[index];
    }
    for (index = 0; index < 0x100; ++index) {
        palette[index] = other.palette[index];
    }
    for (index = 0; index < 0x100; ++index) {
        duplicate[index] = other.duplicate[index];
    }
    color_count = other.color_count;
    red_bits = other.red_bits;
    green_bits = other.green_bits;
    blue_bits = other.blue_bits;
    for (index = 0; index < 0x100; ++index) {
        entries[index] = other.entries[index];
    }
    entry_count = other.entry_count;
    lut_row = other.lut_row;
    for (index = 0; index < 0x100; ++index) {
        rg_dist[index] = other.rg_dist[index];
    }
}

// FUNCTION: SURRENDER 0x10004DD0
srPalette::Quantizer& srPalette::Quantizer::operator=(const Quantizer& other)
{
    long index;
    for (index = 0; index < 0x10000; ++index) {
        lut_rg[index] = other.lut_rg[index];
    }
    for (index = 0; index < 0x10000; ++index) {
        lut_rgb[index] = other.lut_rgb[index];
    }
    for (index = 0; index < 0x100; ++index) {
        palette[index] = other.palette[index];
    }
    for (index = 0; index < 0x100; ++index) {
        duplicate[index] = other.duplicate[index];
    }
    color_count = other.color_count;
    red_bits = other.red_bits;
    green_bits = other.green_bits;
    blue_bits = other.blue_bits;
    for (index = 0; index < 0x100; ++index) {
        entries[index] = other.entries[index];
    }
    entry_count = other.entry_count;
    lut_row = other.lut_row;
    for (index = 0; index < 0x100; ++index) {
        rg_dist[index] = other.rg_dist[index];
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10004160
int srPalette::matchPalette(const srARGB* const colors, long color_count) const
{
    if (color_count != color_count_20) {
        return 0;
    }
    for (long index = 0; index < color_count; ++index) {
        // reinterpret-ok: retail compares the packed color dwords
        if (reinterpret_cast<const unsigned long&>(colors_1c[index]) !=
            reinterpret_cast<const unsigned long&>(colors[index])) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x100041A0
srPalette* srPalette::findMatchingPalette(const srARGB* const colors, long color_count)
{
    srPalette* palette = 0;
    while (true) {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x2900);
        if (node == 0) {
            node = registry->registerClass(sGetClassName(), srClass::sGetClassNode(), 0x2900, 1);
        }
        palette = static_cast<srPalette*>(registry->findExact(node, palette));
        if (palette == 0) {
            break;
        }
        if (palette->matchPalette(colors, color_count) != 0) {
            return palette;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10004210
void srPalette::releaseQuantizer()
{
    if (quantizer_24 != 0) {
        srHeap.free(quantizer_24);
    }
    quantizer_24 = 0;
    flags_18 = flags_18 | 1;
}

// FUNCTION: SURRENDER 0x10004240
void srPalette::updateQuantizer()
{
    if (quantizer_24 != 0) {
        srHeap.free(quantizer_24);
    }
    Quantizer* quantizer = static_cast<Quantizer*>(srHeap.allocate(0x21918));
    if (quantizer != 0) {
        quantizer->setPalette(colors_1c, color_count_20, 0, '\b', '\b', '\b');
        quantizer_24 = quantizer;
    } else {
        quantizer_24 = 0;
    }
}

// FUNCTION: SURRENDER 0x10004300
void srPalette::update()
{
    updateQuantizer();
    flags_18 = flags_18 & ~1;
}

/* A null color table builds the default palette: the 6x6x6 color cube with
   its gray diagonal skipped (216 - 6 = 210 entries), then a 46-entry gray
   ramp for the rest of the 256. A non-cube count gets a linear ramp. */
// FUNCTION: SURRENDER 0x10004310
srPalette::srPalette(srARGB* colors, long color_count)
    : srClassSupport<srPalette, srClass, 1, 0x2900>()
{
    flags_18 = 0;
    colors_1c = static_cast<srARGB*>(srHeap.allocate(color_count * 4));
    color_count_20 = color_count;
    quantizer_24 = 0;
    if (colors == 0) {
        if (color_count == 0x100) {
            long index = 0;
            for (long red = 0; red < 6; ++red) {
                for (long green = 0; green < 6; ++green) {
                    for (long blue = 0; blue < 6; ++blue) {
                        if (red != green || red != blue) {
                            colors_1c[index].alpha = 0xff;
                            colors_1c[index].red = static_cast<unsigned char>(red * 0.2f * 255.0);
                            colors_1c[index].green =
                                static_cast<unsigned char>(green * 0.2f * 255.0);
                            colors_1c[index].blue = static_cast<unsigned char>(blue * 0.2f * 255.0);
                            ++index;
                        }
                    }
                }
            }
            for (long gray = 0; gray < 0x2e; ++gray) {
                double value = gray * 0.022222223f * 255.0;
                colors_1c[0xd2 + gray].alpha = 0xff;
                colors_1c[0xd2 + gray].red = static_cast<unsigned char>(value);
                colors_1c[0xd2 + gray].green = static_cast<unsigned char>(value);
                colors_1c[0xd2 + gray].blue = static_cast<unsigned char>(value);
            }
        } else {
            double step = 0.0;
            if (1 < color_count) {
                step = 1.0 / (color_count - 1);
            }
            for (long index = 0; index < color_count; ++index) {
                double value = index * step * 255.0;
                colors_1c[index].alpha = 0xff;
                colors_1c[index].red = static_cast<unsigned char>(value);
                colors_1c[index].green = static_cast<unsigned char>(value);
                colors_1c[index].blue = static_cast<unsigned char>(value);
            }
        }
    } else if (0 < color_count * 4) {
        memcpy(colors_1c, colors, color_count * 4);
    }
    flags_18 = flags_18 | 1;
}

// FUNCTION: SURRENDER 0x10004610
srPalette::~srPalette()
{
    if (colors_1c != 0) {
        srHeap.free(colors_1c);
        colors_1c = 0;
    }
    if (quantizer_24 != 0) {
        srHeap.free(quantizer_24);
        quantizer_24 = 0;
    }
}

// FUNCTION: SURRENDER 0x100046E0
srPalette& srPalette::operator=(const srPalette& other)
{
    if (this == &other) {
        return *this;
    }
    if (colors_1c != 0) {
        srHeap.free(colors_1c);
        colors_1c = 0;
    }
    if (quantizer_24 != 0) {
        srHeap.free(quantizer_24);
        quantizer_24 = 0;
    }
    srClass::operator=(other);
    flags_18 = other.flags_18;
    color_count_20 = other.color_count_20;
    colors_1c = 0;
    if (0 < color_count_20) {
        colors_1c = static_cast<srARGB*>(srHeap.allocate(color_count_20 * 4));
        for (long index = 0; index < color_count_20; ++index) {
            colors_1c[index] = other.colors_1c[index];
        }
    }
    quantizer_24 = 0;
    flags_18 = flags_18 | 1;
    return *this;
}

// FUNCTION: SURRENDER 0x100047A0
void srPalette::dump(std::ostream& stream)
{
    srClass::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Colors: " << color_count_20 << '\n';
    stream.width(0x20);
    stream << "  Dataptr: " << static_cast<void*>(colors_1c) << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x10004850
void srPalette::setColors(long destination_index, const srARGB* const colors, long color_count)
{
    if (0 <= destination_index && destination_index + color_count <= color_count_20) {
        for (long index = 0; index < color_count; ++index) {
            colors_1c[destination_index + index] = colors[index];
        }
        flags_18 = flags_18 | 1;
    }
}

// FUNCTION: SURRENDER 0x100048A0
srARGB srPalette::getColor(long index) const
{
    return colors_1c[index];
}

// FUNCTION: SURRENDER 0x100048C0
long srPalette::getPaletteSize() const
{
    return color_count_20;
}

// FUNCTION: SURRENDER 0x100048D0
void srPalette::setColor(long index, const srARGB& color)
{
    if (0 <= index && index < color_count_20) {
        colors_1c[index] = color;
        flags_18 = flags_18 | 1;
    }
}

// FUNCTION: SURRENDER 0x10004900
unsigned char srPalette::quantize(const srARGB& color)
{
    if ((flags_18 & 1) != 0) {
        update();
    }
    return quantizer_24->quantize(color);
}

// FUNCTION: SURRENDER 0x10004920
void srPalette::quantize(unsigned char* const indices, const srARGB* const colors, long color_count)
{
    if ((flags_18 & 1) != 0) {
        update();
    }
    quantizer_24->quantize(indices, colors, color_count);
}

// FUNCTION: SURRENDER 0x10004950
const srARGB* srPalette::getPaletteDataPtr()
{
    return colors_1c;
}

// FUNCTION: SURRENDER 0x10004960
const char* srPalette::sGetClassName()
{
    return "srPalette";
}

// FUNCTION: SURRENDER 0x10004970
srClass* srPalette::vInstance()
{
    return new srPalette(0, 1);
}

/* The copy constructor registers through the base like retail, delegates the
   field copy to operator=, then retail overwrites the freshly allocated
   members with the source's pointers — sharing the source's color table and
   quantizer and leaking the copies operator= just made. */
// FUNCTION: SURRENDER 0x10004EE0
srPalette::srPalette(const srPalette& other) : srClassSupport<srPalette, srClass, 1, 0x2900>()
{
    *this = other;
    flags_18 = other.flags_18;
    colors_1c = other.colors_1c;
    color_count_20 = other.color_count_20;
    quantizer_24 = other.quantizer_24;
}

/* Sampler field offsets and behavior are fixed by the retail constructor
   (0x100067d0), discard (0x100068d0) and addColor (0x10006530). */

// FUNCTION: SURRENDER 0x100067D0
srPalette::Sampler::Sampler(long sample_limit)
{
    this->sample_limit = sample_limit;
    if (sample_limit < 0) {
        this->sample_limit = 0;
    }
    setOutputPaletteSize(0x100);
    setSampleFactor(0.05);
    setSampleBits(6);
    colors = 0;
    links = 0;
    discard();
    memset(mask_flags, 0, 0x100);
    memset(mask_colors, 0, 0x400);
}

/* Retail copies the owning colors/links pointers verbatim (offsets 0x524 /
   0x528): a Sampler copy aliases the source's tables, so destruction or
   discard() of either object leaves the other's pointers dangling —
   double-free semantics proven in the original. */
// FUNCTION: SURRENDER 0x10004BA0
srPalette::Sampler::Sampler(const Sampler& other)
{
    sample_limit = other.sample_limit;
    sample_factor = other.sample_factor;
    sample_bits = other.sample_bits;
    color_count = other.color_count;
    sample_count = other.sample_count;
    capacity = other.capacity;
    output_palette_size = other.output_palette_size;
    memcpy(mask_flags, other.mask_flags, 0x100);
    memcpy(mask_colors, other.mask_colors, 0x400);
    colors = other.colors;
    links = other.links;
    memcpy(buckets, other.buckets, 0x20000);
}

// FUNCTION: SURRENDER 0x10006930
srPalette::Sampler::~Sampler()
{
    discard();
}

/* Same retail aliasing as the copy constructor: colors and links are copied
   as raw pointers, so assignment shares ownership of both tables. */
// FUNCTION: SURRENDER 0x10004C40
srPalette::Sampler& srPalette::Sampler::operator=(const Sampler& other)
{
    long index;
    sample_limit = other.sample_limit;
    sample_factor = other.sample_factor;
    sample_bits = other.sample_bits;
    color_count = other.color_count;
    sample_count = other.sample_count;
    capacity = other.capacity;
    output_palette_size = other.output_palette_size;
    for (index = 0; index < 0x100; ++index) {
        mask_flags[index] = other.mask_flags[index];
    }
    for (index = 0; index < 0x100; ++index) {
        mask_colors[index] = other.mask_colors[index];
    }
    colors = other.colors;
    links = other.links;
    for (index = 0; index < 0x8000; ++index) {
        buckets[index] = other.buckets[index];
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10004AF0
long srPalette::Sampler::getColorCount()
{
    return color_count;
}

// FUNCTION: SURRENDER 0x10004B00
long srPalette::Sampler::getOutputPaletteSize()
{
    return output_palette_size;
}

// FUNCTION: SURRENDER 0x10004B10
long srPalette::Sampler::getSampleCount()
{
    return sample_count;
}

// FUNCTION: SURRENDER 0x10004B20
double srPalette::Sampler::getSampleFactor()
{
    return sample_factor;
}

// FUNCTION: SURRENDER 0x10004B30
long srPalette::Sampler::getSampleBits()
{
    return sample_bits;
}

// FUNCTION: SURRENDER 0x10006710
void srPalette::Sampler::setOutputPaletteSize(long size)
{
    if (size < 1) {
        size = 1;
    }
    if (0x100 < size) {
        size = 0x100;
    }
    output_palette_size = size;
}

// FUNCTION: SURRENDER 0x10006740
void srPalette::Sampler::setSampleBits(long bits)
{
    if (bits < 1) {
        bits = 1;
    }
    if (8 < bits) {
        bits = 8;
    }
    sample_bits = bits;
}

// FUNCTION: SURRENDER 0x10006770
void srPalette::Sampler::setSampleFactor(double factor)
{
    if (factor <= 0.0) {
        sample_factor = 0.0;
    } else if (factor >= 1.0) {
        sample_factor = 1.0;
    } else {
        sample_factor = factor;
    }
}

// FUNCTION: SURRENDER 0x100068D0
void srPalette::Sampler::discard()
{
    for (long index = 0; index < 0x8000; ++index) {
        buckets[index] = -1;
    }
    if (colors != 0) {
        ::operator delete(colors);
    }
    if (links != 0) {
        ::operator delete(links);
    }
    color_count = 0;
    sample_count = 0;
    colors = 0;
    links = 0;
    capacity = 0;
}

// FUNCTION: SURRENDER 0x10006630
void srPalette::Sampler::reallocColors(long new_capacity)
{
    ColorEntry* new_colors =
        static_cast<ColorEntry*>(::operator new(new_capacity * sizeof(ColorEntry)));
    /* Retail tests the first allocation and stores 0 on failure — a no-op
       check identical in shape to addSurface's pixels guard. The second
       allocation is never tested. */
    if (new_colors == 0) {
        new_colors = 0;
    }
    long* new_links = static_cast<long*>(::operator new(new_capacity * sizeof(long)));
    for (long index = 0; index < new_capacity; ++index) {
        new_colors[index].color = srARGB();
        new_colors[index].count = 0;
        new_links[index] = -1;
    }
    if (colors != 0) {
        for (long index = 0; index < color_count; ++index) {
            new_colors[index] = colors[index];
            new_links[index] = links[index];
        }
        ::operator delete(colors);
        ::operator delete(links);
    }
    colors = new_colors;
    links = new_links;
    capacity = new_capacity;
}

// FUNCTION: SURRENDER 0x10006300
void srPalette::Sampler::removeMaskColor(long index)
{
    if ((0 <= index) && (index <= 0xff)) {
        mask_flags[index] = 0;
        mask_colors[index].alpha = 0;
        mask_colors[index].red = 0;
        mask_colors[index].green = 0;
        mask_colors[index].blue = 0;
    }
}

// FUNCTION: SURRENDER 0x10006320
void srPalette::Sampler::setMaskColor(long index, const srARGB& color)
{
    if ((0 <= index) && (index <= 0xff)) {
        mask_flags[index] = 1;
        mask_colors[index] = color;
    }
}

// FUNCTION: SURRENDER 0x100062A0
void srPalette::Sampler::shiftDown(srARGB& color)
{
    if (sample_bits != 8) {
        long shift = 8 - sample_bits;
        color.blue >>= shift;
        color.green >>= shift;
        color.red >>= shift;
        color.alpha >>= shift;
    }
}

// FUNCTION: SURRENDER 0x100062D0
void srPalette::Sampler::shiftUp(srARGB& color)
{
    if (sample_bits != 8) {
        long shift = 8 - sample_bits;
        color.blue <<= shift;
        color.green <<= shift;
        color.red <<= shift;
        color.alpha <<= shift;
    }
}

// FUNCTION: SURRENDER 0x10006530
void srPalette::Sampler::addColor(const srARGB& source, long weight)
{
    if (weight > 0) {
        srARGB color = source;
        sample_count += weight;
        color.alpha = 0xff;
        shiftDown(color);
        unsigned long packed =
            reinterpret_cast<unsigned long&>(color); /* reinterpret-ok: packed color dword */
        long bucket = ((packed >> 6) & 0x7c00) + ((packed >> 3) & 0x3e0) + (packed & 0x1f);
        for (long index = buckets[bucket]; index != -1; index = links[index]) {
            if (reinterpret_cast<unsigned long&>(
                    colors[index].color) == /* reinterpret-ok: packed color dword */
                packed) {
                colors[index].count += weight;
                return;
            }
        }
        if (color_count >= capacity) {
            long new_capacity = capacity * 2;
            if (new_capacity < 0x200) {
                new_capacity = 0x200;
            }
            if ((0 < sample_limit) && (sample_limit < new_capacity)) {
                new_capacity = sample_limit;
            }
            reallocColors(new_capacity);
        }
        if (color_count < capacity) {
            colors[color_count].color = color;
            colors[color_count].count = weight;
            links[color_count] = buckets[bucket];
            buckets[bucket] = color_count;
            ++color_count;
        }
    }
}

// FUNCTION: SURRENDER 0x10006500
void srPalette::Sampler::addColors(const srARGB* colors, long color_count, long weight)
{
    if ((colors != 0) && (color_count > 0)) {
        do {
            addColor(*colors, weight);
            ++colors;
            --color_count;
        } while (color_count != 0);
    }
}

// FUNCTION: SURRENDER 0x10006350
void srPalette::Sampler::addSurfaces(srColorSurfaceIFace** surfaces, long surface_count,
                                     long weight)
{
    if ((surfaces != 0) && (surface_count > 0)) {
        do {
            if (*surfaces != 0) {
                addSurface(**surfaces, weight);
            }
            ++surfaces;
            --surface_count;
        } while (surface_count != 0);
    }
}

// FUNCTION: SURRENDER 0x10006390
void srPalette::Sampler::addSurface(const char* name, long weight)
{
    if ((name != 0) && (*name != '\0')) {
        srSurfaceIOManager::ImportInfo options;
        options.unknown_00 = 0;
        options.unknown_04 = 0;
        options.option_string = 0;
        srColorSurfaceIFace* surface = srCore.getSurfaceIOManager()->importSurface(name, options);
        if (surface != 0) {
            addSurface(*surface, weight);
            surface->release();
        }
    }
}

// FUNCTION: SURRENDER 0x100063E0
void srPalette::Sampler::addSurface(srColorSurfaceIFace& surface, long weight)
{
    long width = surface.getWidth();
    long height = surface.getHeight();
    long samples = 1;
    if ((sample_factor > 0.0) &&
        ((samples = (long)(width * height * sample_factor)), samples < 1)) {
        samples = 1;
    }
    if (sample_factor == 1.0) {
        unsigned long* pixels = static_cast<unsigned long*>(
            srHeap.allocate(width * 4)); /* reinterpret-ok: raw pixel row storage */
        /* Retail shape (0x1000644F): the allocation result is tested and 0 is
           stored on failure — a no-op null check; pixels then flows into
           getPixelRow regardless. */
        if (pixels == 0) {
            pixels = 0;
        }
        for (long y = 0; y < height; ++y) {
            surface.getPixelRow(pixels, y, 0, width);
            addColors((const srARGB*)pixels, width,
                      weight); /* reinterpret-ok: packed pixel rows are srARGB */
        }
        srHeap.free(pixels);
        return;
    }
    for (; samples > 0; --samples) {
        unsigned long pixel = surface.getPixel(rand() % width, rand() % height);
        addColor(
            reinterpret_cast<srARGB&>(pixel), /* reinterpret-ok: packed pixel value as srARGB */
            weight);
    }
}

// FUNCTION: SURRENDER 0x10006010
void srPalette::Sampler::dump(std::ostream& stream)
{
    long flags = stream.flags();
    stream.flags((flags & ~0x180L) | 0x40);
    stream << "Sampling frequency:       " << sample_factor << '\n';
    stream << "Sampling bit depth:       " << sample_bits << '\n';
    stream << "Distinct colors:          " << color_count << '\n';
    stream << "Samples:                  " << sample_count << '\n';
    stream << "Output palette size:      " << output_palette_size << '\n';
    if (output_palette_size != 0) {
        stream << "Color reduction:          " << (double)color_count / output_palette_size << ":1"
               << '\n';
    }
    stream << "Sampler memory usage:     " << (capacity * 0xc + 0x20530) * 0.0009765625 << " kB"
           << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x10006190
srPalette* srPalette::Sampler::createOptimalPalette()
{
    Optimizer::PaletteInfo info;
    info.color_count = color_count;
    if ((color_count <= 0) || (info.palette_size = output_palette_size, output_palette_size <= 0)) {
        return 0;
    }
    info.colors = colors;
    info.mask_colors = 0;
    info.mask_flags = 0;
    info.mask_count = 0;
    srARGB shifted[0x100];
    long index;
    for (index = 0; index < 0x100; ++index) {
        shifted[index] = mask_colors[index];
        shiftDown(shifted[index]);
    }
    for (index = 0; index < 0x100; ++index) {
        if (mask_flags[index] != 0) {
            info.mask_colors = shifted;
            info.mask_count = output_palette_size;
            info.mask_flags = mask_flags;
            break;
        }
    }
    srPalette* palette = Optimizer::createOptimalPalette(info);
    if ((sample_bits != 8) && (output_palette_size > 0)) {
        for (index = 0; index < output_palette_size; ++index) {
            const srARGB* color = mask_colors + index;
            srARGB widened;
            if (mask_flags[index] == 0) {
                widened = palette->getColor(index);
                shiftUp(widened);
                color = &widened;
            }
            palette->setColor(index, *color);
        }
    }
    return palette;
}

// FUNCTION: SURRENDER 0x10004B90
srPalette::Optimizer& srPalette::Optimizer::operator=(const Optimizer& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10005630
void srPalette::Optimizer::setupLUT(LUT& lut, const srARGB& color)
{
    lut.color = color;
    lut.r = lut.dist_r + (0x100 - color.red);
    lut.g = lut.dist_g + (0x100 - color.green);
    lut.b = lut.dist_b + (0x100 - color.blue);
}

// FUNCTION: SURRENDER 0x10005420
void srPalette::Optimizer::findOptimalColor(Node* node, const LUT& lut)
{
    double distance = 0.0;
    if (lut.color.red < node->bound_lo.red) {
        distance = lut.r[node->bound_lo.red];
    }
    if (node->bound_hi.red < lut.color.red) {
        distance = distance + lut.r[node->bound_hi.red];
    }
    if (lut.color.green < node->bound_lo.green) {
        distance = distance + lut.g[node->bound_lo.green];
    }
    if (node->bound_hi.green < lut.color.green) {
        distance = distance + lut.g[node->bound_hi.green];
    }
    if (lut.color.blue < node->bound_lo.blue) {
        distance = distance + lut.b[node->bound_lo.blue];
    }
    if (node->bound_hi.blue < lut.color.blue) {
        distance = distance + lut.b[node->bound_hi.blue];
    }
    if (distance < node->err_min) {
        node->err_total = 0.0;
        node->err_min = 0.0;
        if (node->leaves == 0) {
            for (long index = 0; index < 8; ++index) {
                if (node->children[index] != 0) {
                    findOptimalColor(node->children[index], lut);
                }
            }
        } else {
            node->color = node->leaves[0].color;
            double best = 0.0;
            for (long index = 0; index < node->leaf_count; ++index) {
                Leaf* leaf = node->leaves + index;
                unsigned char* bytes =
                    (unsigned char*)&leaf->color; /* reinterpret-ok: packed leaf color bytes */
                float error = lut.r[bytes[2]] + lut.g[bytes[1]] + lut.b[bytes[0]];
                if (error < (float)leaf->error) {
                    leaf->error = (double)error;
                }
                if (node->err_min < leaf->error) {
                    node->err_min = leaf->error;
                }
                double contribution = leaf->weight * leaf->error;
                node->err_total = contribution + node->err_total;
                if (best < contribution) {
                    node->color = leaf->color;
                    best = contribution;
                }
            }
        }
    }
    Node* parent = node->parent;
    if (parent != 0) {
        if (parent->err_min < node->err_min) {
            parent->err_min = node->err_min;
        }
        if (parent->err_total < node->err_total) {
            parent->err_total = node->err_total;
            parent->color = node->color;
        }
    }
}

/* Hash entries link 5-5-5 quantized colors inside the two 0x8000-bucket
   tables; leaf bucket nodes then anchor the octree's deepest level. */
// FUNCTION: SURRENDER 0x10005690
srPalette* srPalette::Optimizer::createOptimalPalette(const PaletteInfo& info)
{
    struct HashEntry {
        HashEntry* next;
        unsigned long color;
        long count;
    };

    if ((info.colors == 0) || (info.color_count < 1) || (info.palette_size < 1) ||
        ((info.mask_count != 0) && ((info.mask_flags == 0) || (info.mask_colors == 0)))) {
        return 0;
    }

    HashEntry** buckets = static_cast<HashEntry**>(::operator new(0x20000));
    HashEntry* entries =
        static_cast<HashEntry*>(::operator new(info.color_count * sizeof(HashEntry)));
    HashEntry** rehash = static_cast<HashEntry**>(::operator new(0x20000));
    srARGB* palette_colors = static_cast<srARGB*>(srHeap.allocate(info.palette_size * 4));
    /* The retail epilogue frees palette_colors, two node pools and the five
       level arrays — lut is never deleted. Proven retail leak. */
    LUT* lut = static_cast<LUT*>(::operator new(sizeof(LUT)));
    memset(palette_colors, 0, info.palette_size * 4);
    memset(buckets, 0, 0x20000);
    memset(rehash, 0, 0x20000);

    long distinct = 0;
    HashEntry* entry = entries;
    long index;
    for (index = 0; index < info.color_count; ++index) {
        long weight = info.colors[index].count;
        if (weight > 0) {
            unsigned long color = reinterpret_cast<const unsigned long&>(
                                      info.colors[index].color) /* reinterpret-ok: packed dword */
                                  | 0xff000000;
            unsigned long bucket =
                ((color & 0x1f0000) >> 6) + ((color & 0x1f00) >> 3) + (color & 0x1f);
            HashEntry* link;
            for (link = buckets[bucket]; link != 0; link = link->next) {
                if (link->color == color) {
                    link->count += weight;
                    goto next_color;
                }
            }
            ++distinct;
            entry->color = color;
            entry->count = info.colors[index].count;
            entry->next = buckets[bucket];
            buckets[bucket] = entry;
            ++entry;
        }
    next_color:;
    }

    long leaf_nodes = 0;
    for (index = 0; index < 0x8000; ++index) {
        HashEntry* link = buckets[index];
        while (link != 0) {
            unsigned long color = link->color;
            HashEntry* next = link->next;
            unsigned long bucket = ((color >> 0x13 & 0x1f) * 0x20 + (color >> 0xb & 0x1f)) * 0x20 +
                                   (color >> 3 & 0x1f);
            if (rehash[bucket] == 0) {
                ++leaf_nodes;
            }
            link->next = rehash[bucket];
            rehash[bucket] = link;
            link = next;
        }
    }

    Node* leaf_pool = static_cast<Node*>(::operator new(leaf_nodes * sizeof(Node)));
    Leaf* leaves = static_cast<Leaf*>(::operator new(distinct * sizeof(Leaf)));
    Node** leaf_map = static_cast<Node**>(::operator new(0x20000));
    memset(leaf_map, 0, 0x20000);

    unsigned long dominant_color = 0;
    long dominant_weight = 0;
    long used_leaves = 0;
    long used_nodes = 0;
    for (index = 0; index < 0x8000; ++index) {
        HashEntry* link = rehash[index];
        if (link != 0) {
            Node* node = leaf_pool + used_nodes;
            ++used_nodes;
            leaf_map[index] = node;
            node->leaves = leaves + used_leaves;
            unsigned long bounds_lo = 0xffffffff;
            unsigned long bounds_hi = 0;
            long count = 0;
            do {
                Leaf* leaf = node->leaves + count;
                leaf->color = link->color;
                leaf->weight = link->count;
                leaf->error = 1e9;
                if (dominant_weight < link->count) {
                    dominant_color = link->color;
                    dominant_weight = link->count;
                }
                for (long byte = 0; byte < 4; ++byte) {
                    unsigned char value =
                        ((const unsigned char*)&link->color)[byte]; /* reinterpret-ok: packed
                                                                       color bytes */
                    if (value < ((unsigned char*)&bounds_lo)[byte]) {
                        ((unsigned char*)&bounds_lo)[byte] =
                            value; /* reinterpret-ok: byte-wise bounds */
                    }
                    if (((unsigned char*)&bounds_hi)[byte] < value) {
                        ((unsigned char*)&bounds_hi)[byte] =
                            value; /* reinterpret-ok: byte-wise bounds */
                    }
                }
                link = link->next;
                ++count;
            } while (link != 0);
            node->err_min = 1e10;
            node->err_total = 1e10;
            node->color = 0;
            node->bound_lo =
                reinterpret_cast<srARGB&>(bounds_lo); /* reinterpret-ok: packed bounds dword */
            node->bound_hi =
                reinterpret_cast<srARGB&>(bounds_hi); /* reinterpret-ok: packed bounds dword */
            node->leaf_count = count;
            used_leaves += count;
        }
    }

    ::operator delete(rehash);
    ::operator delete(entries);
    ::operator delete(buckets);

    static const unsigned long level_node_counts[] = {1, 8, 0x40, 0x200, 0x1000, 0x8000};
    Node* levels[4];
    Node* root;
    for (long level = 4; level >= 0; --level) {
        long dim = 1 << level;
        Node* nodes = static_cast<Node*>(::operator new(level_node_counts[level] * sizeof(Node)));
        memset(nodes, 0, level_node_counts[level] * sizeof(Node));
        if (level == 0) {
            root = nodes;
        } else {
            levels[level - 1] = nodes;
        }
        for (long z = 0; z < dim; ++z) {
            for (long y = 0; y < dim; ++y) {
                Node* node = nodes + (z * dim + y) * dim;
                unsigned long child_base = (z * 2 * dim + y) * dim * 4;
                for (long x = 0; x < dim; ++x, ++node, child_base += 2) {
                    node->bound_lo.alpha = 0xff;
                    node->bound_lo.red = 0xff;
                    node->bound_lo.green = 0xff;
                    node->bound_lo.blue = 0xff;
                    node->bound_hi.alpha = 0;
                    node->bound_hi.red = 0;
                    node->bound_hi.green = 0;
                    node->bound_hi.blue = 0;
                    node->err_min = 1e9;
                    node->err_total = 1e9;
                    node->leaf_count = 0;
                    for (long child = 0; child < 8; ++child) {
                        unsigned long child_index = child_base;
                        if ((child & 1) != 0) {
                            ++child_index;
                        }
                        if ((child & 2) != 0) {
                            child_index += dim * 2;
                        }
                        if ((child & 4) != 0) {
                            child_index += dim * dim * 4;
                        }
                        Node* link;
                        if (level == 4) {
                            link = leaf_map[child_index];
                        } else {
                            link = levels[level] + child_index;
                        }
                        if ((link != 0) && (link->leaf_count == 0)) {
                            link = 0;
                        }
                        node->children[child] = link;
                        if (link != 0) {
                            link->parent = node;
                            for (long byte = 0; byte < 4; ++byte) {
                                unsigned char* child_bytes =
                                    (unsigned char*)&link
                                        ->bound_lo; /* reinterpret-ok: byte bounds merge */
                                unsigned char* node_lo =
                                    (unsigned char*)&node
                                        ->bound_lo; /* reinterpret-ok: byte bounds merge */
                                unsigned char* node_hi =
                                    (unsigned char*)&node
                                        ->bound_hi; /* reinterpret-ok: byte bounds merge */
                                if (child_bytes[byte] < node_lo[byte]) {
                                    node_lo[byte] = child_bytes[byte];
                                }
                                if (child_bytes[byte + 4] > node_hi[byte]) {
                                    node_hi[byte] = child_bytes[byte + 4];
                                }
                            }
                            node->leaf_count += link->leaf_count;
                        }
                    }
                }
            }
        }
    }

    ::operator delete(leaf_map);

    for (index = 0; index < 0x200; ++index) {
        float delta = index - 256.0f;
        delta = delta * delta;
        lut->dist_r[index] = delta * 0.299f;
        lut->dist_g[index] = delta * 0.587f;
        lut->dist_b[index] = delta * 0.114f;
    }

    if (info.mask_count < 1) {
        root->color = dominant_color;
    } else {
        long limit = info.palette_size;
        if (info.mask_count < limit) {
            limit = info.mask_count;
        }
        for (index = 0; index < limit; ++index) {
            if (info.mask_flags[index] != 0) {
                palette_colors[index] = info.mask_colors[index];
                setupLUT(*lut, palette_colors[index]);
                findOptimalColor(root, *lut);
            }
        }
    }
    for (index = 0; index < info.palette_size; ++index) {
        if ((info.mask_flags == 0) || (info.mask_count <= index) || (info.mask_flags[index] == 0)) {
            palette_colors[index] =
                reinterpret_cast<srARGB&>(root->color); /* reinterpret-ok: packed color dword */
            setupLUT(*lut, palette_colors[index]);
            findOptimalColor(root, *lut);
        }
    }

    srPalette* palette =
        new (srHeap.allocate(sizeof(srPalette))) srPalette(palette_colors, info.palette_size);
    srHeap.free(palette_colors);
    ::operator delete(leaf_pool);
    ::operator delete(leaves);
    ::operator delete(root);
    for (index = 0; index < 4; ++index) {
        ::operator delete(levels[index]);
    }
    return palette;
}
