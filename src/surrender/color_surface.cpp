#include "surrender/srColorSurface.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srPalette.h"
#include "surrender/srVectorProcessor.h"

/* srColorSurface surface-flag names; never assigned in retail, so dump reports
   numeric bit indices. */
// GLOBAL: SURRENDER 0x100A4A10
static const char* s_flag_names_100a4a10;

/* Comma-separated bit-name walker shared by the sr dumps; each TU keeps its own
   copy. */
static void dumpFlags(std::ostream& stream, unsigned long flags, const char* names)
{
    if (flags == 0) {
        stream << "[NONE]";
        return;
    }
    stream << '[';
    bool first = true;
    for (unsigned long bit = 0; bit < 0x20; ++bit) {
        if ((flags & (1 << bit)) != 0) {
            if (first) {
                first = false;
            } else {
                stream << ',';
            }
            if (names == 0 || *names == 0) {
                stream << bit;
            } else {
                while (*names != 0 && *names != ',') {
                    stream << *names++;
                }
                if (*names == ',') {
                    ++names;
                }
            }
        } else if (names != 0) {
            while (*names != 0 && *names != ',') {
                ++names;
            }
            if (*names == ',') {
                ++names;
            }
        }
    }
    stream << ']';
}

// FUNCTION: SURRENDER 0x100571F0
srColorSurfaceIFace::srColorSurfaceIFace()
{
    srZeroMemory(&width_1c, 0x28);
}

// FUNCTION: SURRENDER 0x1005A120
srColorSurfaceIFace::srColorSurfaceIFace(const srColorSurfaceIFace& other)
{
    /* Retail assigns, then re-copies the trailing field block verbatim. */
    *this = other;
    unknown_18_[1] = other.unknown_18_[1];
    memcpy(&width_1c, &other.width_1c, 0x28);
}

// FUNCTION: SURRENDER 0x1005B280
const char* srColorSurfaceIFace::sGetClassName()
{
    return "srColorSurfaceIFace";
}

// FUNCTION: SURRENDER 0x10021260
srColorSurfaceIFace::~srColorSurfaceIFace() {}

// FUNCTION: SURRENDER 0x10021310
srPalette* srColorSurfaceIFace::getPalette()
{
    return 0;
}

// FUNCTION: SURRENDER 0x10021320
void srColorSurfaceIFace::setPalette(srPalette* palette) {}

// FUNCTION: SURRENDER 0x10021330
void* srColorSurfaceIFace::getDataPtr()
{
    return 0;
}

// FUNCTION: SURRENDER 0x10021340
long srColorSurfaceIFace::getDataSize()
{
    return pitch_24 * height_20;
}

// FUNCTION: SURRENDER 0x10021350
int srColorSurfaceIFace::resize(long width, long height)
{
    return 0;
}

// FUNCTION: SURRENDER 0x10021360
int srColorSurfaceIFace::rescale(long width, long height)
{
    return 0;
}

// FUNCTION: SURRENDER 0x10021370
int srColorSurfaceIFace::changePixelFormat(const srPixelConvert::PixelFormat& format, int preserve)
{
    return 0;
}

// FUNCTION: SURRENDER 0x100572E0
long srColorSurfaceIFace::getClampedX(long x) const
{
    long width = width_1c;
    if ((clamp_modes_28 & 1) != 0) {
        if (x < 0) {
            return 0;
        }
        if (width <= x) {
            return width - 1;
        }
    } else {
        if (x < 0) {
            return width - (-1 - x) % width - 1;
        }
        if (width <= x) {
            return x % width;
        }
    }
    return x;
}

// FUNCTION: SURRENDER 0x10057330
long srColorSurfaceIFace::getClampedY(long y) const
{
    long height = height_20;
    if ((clamp_modes_28 & 2) != 0) {
        if (height <= y) {
            return height - 1;
        }
        if (y < 0) {
            return 0;
        }
    } else {
        if (y < 0) {
            return height - (-1 - y) % height - 1;
        }
        if (height <= y) {
            return y % height;
        }
    }
    return y;
}

// FUNCTION: SURRENDER 0x10057380
void srColorSurfaceIFace::clampCoordinates(long& x, long& y)
{
    x = getClampedX(x);
    y = getClampedY(y);
}

// FUNCTION: SURRENDER 0x10059860
unsigned long srColorSurfaceIFace::getAlphaBits() const
{
    return pixel_format_30.alpha_bits;
}

// FUNCTION: SURRENDER 0x10059870
double srColorSurfaceIFace::getAspectRatio() const
{
    if (height_20 == 0) {
        return 0.0;
    }
    return width_1c / (double)height_20;
}

// FUNCTION: SURRENDER 0x10059890
long srColorSurfaceIFace::getBitsPerPixel() const
{
    return (pixel_format_30.bytes_per_pixel_minus_one + 1) * 8;
}

// FUNCTION: SURRENDER 0x100598A0
long srColorSurfaceIFace::getBlueBits() const
{
    return pixel_format_30.blue_bits;
}

// FUNCTION: SURRENDER 0x100598B0
long srColorSurfaceIFace::getBytesPerPixel() const
{
    return pixel_format_30.bytes_per_pixel_minus_one + 1;
}

// FUNCTION: SURRENDER 0x100598C0
srFilter* srColorSurfaceIFace::getFilter() const
{
    return filter_2c;
}

// FUNCTION: SURRENDER 0x100598D0
long srColorSurfaceIFace::getGreenBits() const
{
    return pixel_format_30.green_bits;
}

// FUNCTION: SURRENDER 0x100598E0
int srColorSurfaceIFace::getHClampMode() const
{
    return clamp_modes_28 & 1;
}

// FUNCTION: SURRENDER 0x100599D0
long srColorSurfaceIFace::getRedBits() const
{
    return pixel_format_30.red_bits;
}

// FUNCTION: SURRENDER 0x10059A10
void srColorSurfaceIFace::getSurfaceDesc(SurfaceDesc& description) const
{
    description.width = width_1c;
    description.height = height_20;
    description.pitch = pitch_24;
    description.clamp_modes = clamp_modes_28;
    description.filter = filter_2c;
    description.pixel_format = pixel_format_30;
}

// FUNCTION: SURRENDER 0x10059A50
int srColorSurfaceIFace::getVClampMode() const
{
    return (clamp_modes_28 >> 1) & 1;
}

// FUNCTION: SURRENDER 0x10059A70
int srColorSurfaceIFace::isAlpha() const
{
    return pixel_format_30.alpha_bits != 0;
}

// FUNCTION: SURRENDER 0x10059A80
int srColorSurfaceIFace::isPaletted() const
{
    return pixel_format_30.conversion_class == 3;
}

// FUNCTION: SURRENDER 0x10059AA0
void srColorSurfaceIFace::setHClampMode(int enabled)
{
    if (enabled != 0) {
        clamp_modes_28 = clamp_modes_28 | 1;
        return;
    }
    clamp_modes_28 = clamp_modes_28 & ~1;
}

// FUNCTION: SURRENDER 0x1005A020
void srColorSurfaceIFace::setVClampMode(int enabled)
{
    if (enabled != 0) {
        clamp_modes_28 = clamp_modes_28 | 2;
        return;
    }
    clamp_modes_28 = clamp_modes_28 & ~2;
}

// FUNCTION: SURRENDER 0x1005A0D0
const srPixelConvert::PixelFormat* srColorSurfaceIFace::getPixelFormat() const
{
    return &pixel_format_30;
}

// FUNCTION: SURRENDER 0x1005A0E0
int srColorSurfaceIFace::isPixelFormatCompatible(const srColorSurfaceIFace& source) const
{
    return pixel_format_30.flags == source.pixel_format_30.flags &&
           *(const long*)&source.pixel_format_30 == *(const long*)&pixel_format_30 &&
           *((const long*)&source.pixel_format_30 + 1) == *((const long*)&pixel_format_30 + 1) &&
           pixel_format_30.bytes_per_pixel_minus_one ==
               source.pixel_format_30.bytes_per_pixel_minus_one &&
           pixel_format_30.conversion_class == source.pixel_format_30.conversion_class;
}

// FUNCTION: SURRENDER 0x1005ADD0
void srColorSurfaceIFace::setSurfaceDesc(const SurfaceDesc& description)
{
    width_1c = description.width;
    height_20 = description.height;
    pitch_24 = description.pitch;
    clamp_modes_28 = description.clamp_modes;
    filter_2c = description.filter;
    pixel_format_30 = description.pixel_format;
}

// FUNCTION: SURRENDER 0x1005B230
void srColorSurfaceIFace::copySurfaceParameters(const srColorSurfaceIFace& source)
{
    filter_2c = source.filter_2c;
    if ((source.clamp_modes_28 & 1) != 0) {
        clamp_modes_28 = clamp_modes_28 | 1;
    } else {
        clamp_modes_28 = clamp_modes_28 & ~1;
    }
    if ((source.clamp_modes_28 & 2) != 0) {
        clamp_modes_28 = clamp_modes_28 | 2;
        return;
    }
    clamp_modes_28 = clamp_modes_28 & ~2;
}

// FUNCTION: SURRENDER 0x1005B290
srColorSurfaceIFace& srColorSurfaceIFace::operator=(const srColorSurfaceIFace& other)
{
    if (&other != this) {
        srClass::operator=(other);
        width_1c = other.width_1c;
        height_20 = other.height_20;
        pitch_24 = other.pitch_24;
        clamp_modes_28 = other.clamp_modes_28;
        filter_2c = other.filter_2c;
        pixel_format_30 = other.pixel_format_30;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005B2E0
unsigned long srColorSurfaceIFace::getPixel(long x, long y)
{
    unsigned long pixel;
    getPixelRow(&pixel, y, x, x + 1);
    return pixel;
}

// FUNCTION: SURRENDER 0x1005B470
void srColorSurfaceIFace::setPixel(long x, long y, unsigned long pixel)
{
    setPixelRow(&pixel, y, x, x + 1);
}

// FUNCTION: SURRENDER 0x1005B100
unsigned long srColorSurfaceIFace::getPixelRaw(long x, long y)
{
    unsigned long pixel = 0;
    getPixelRowRaw(&pixel, y, x, x + 1);
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0:
        return pixel & 0xff;
    case 1:
        return pixel & 0xffff;
    case 2:
        return pixel & 0xffffff;
    case 3:
        return pixel;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005B1B0
void srColorSurfaceIFace::setPixelRaw(long x, long y, unsigned long pixel)
{
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0:
        pixel = (unsigned char)pixel;
        break;
        break;
    case 1:
        pixel = (unsigned short)pixel;
        break;
        break;
    case 2:
        pixel = pixel & 0xffffff;
        break;
        break;
    case 3:
        break;
        break;
    }
    setPixelRowRaw(&pixel, y, x, x + 1);
}

// FUNCTION: SURRENDER 0x1005B310
void srColorSurfaceIFace::getPixels(unsigned long* pixels, const srVector2i* positions, long count)
{
    for (; count > 0; --count, ++pixels, ++positions) {
        *pixels = getPixel(positions->x, positions->y);
    }
}

// FUNCTION: SURRENDER 0x1005B3A0
void srColorSurfaceIFace::setPixels(const unsigned long* pixels, const srVector2i* positions,
                                    long count)
{
    for (; count > 0; --count, ++pixels, ++positions) {
        setPixel(positions->x, positions->y, *pixels);
    }
}

// FUNCTION: SURRENDER 0x1005B350
void srColorSurfaceIFace::getPixelsRaw(void* pixels, const srVector2i* positions, long count)
{
    unsigned char* out = (unsigned char*)pixels;
    long bytes = pixel_format_30.bytes_per_pixel_minus_one + 1;
    for (; count > 0; --count, ++positions, out += bytes) {
        getPixelRowRaw(out, positions->y, positions->x, positions->x + 1);
    }
}

// FUNCTION: SURRENDER 0x1005B3E0
void srColorSurfaceIFace::setPixelsRaw(const void* pixels, const srVector2i* positions, long count)
{
    const unsigned char* in = (const unsigned char*)pixels;
    long bytes = pixel_format_30.bytes_per_pixel_minus_one + 1;
    for (; count > 0; --count, ++positions, in += bytes) {
        setPixelRowRaw(in, positions->y, positions->x, positions->x + 1);
    }
}

// FUNCTION: SURRENDER 0x1005B430
void srColorSurfaceIFace::getPixelColumn(unsigned long* pixels, long x, long y_start, long y_end)
{
    for (; y_start < y_end; ++y_start, ++pixels) {
        *pixels = getPixel(x, y_start);
    }
}

// FUNCTION: SURRENDER 0x1005B490
void srColorSurfaceIFace::setPixelColumn(const unsigned long* pixels, long x, long y_start,
                                         long y_end)
{
    for (; y_start < y_end; ++y_start, ++pixels) {
        setPixel(x, y_start, *pixels);
    }
}

// FUNCTION: SURRENDER 0x100573B0
void srColorSurfaceIFace::fill(unsigned long pixel)
{
    long width = width_1c;
    long height = height_20;
    unsigned long* row = new unsigned long[width];
    if (width != 0) {
        srVectorProcessor::copy(row, pixel, width);
    }
    for (long y = 0; y < height; ++y) {
        setPixelRow(row, y, 0, width);
    }
    delete[] row;
}

// FUNCTION: SURRENDER 0x10057750
void srColorSurfaceIFace::setHLine(long y, long x_start, long x_end, unsigned long pixel)
{
    if (y >= 0 && y < height_20) {
        if (x_end < x_start) {
            long swap = x_start;
            x_start = x_end;
            x_end = swap;
        }
        if (x_start < 0) {
            x_start = 0;
        }
        if (width_1c < x_end) {
            x_end = width_1c;
        }
        for (; x_start < x_end; ++x_start) {
            setPixel(x_start, y, pixel);
        }
    }
}

// FUNCTION: SURRENDER 0x100577B0
void srColorSurfaceIFace::setVLine(long x, long y_start, long y_end, unsigned long pixel)
{
    if (x >= 0 && x < width_1c) {
        if (y_end < y_start) {
            long swap = y_start;
            y_start = y_end;
            y_end = swap;
        }
        if (y_start < 0) {
            y_start = 0;
        }
        if (height_20 < y_end) {
            y_end = height_20;
        }
        for (; y_start < y_end; ++y_start) {
            setPixel(x, y_start, pixel);
        }
    }
}

// FUNCTION: SURRENDER 0x10057420
void srColorSurfaceIFace::swapPixelRows(long x0, long y0, long x1, long y1, long count)
{
    long width = width_1c;
    if (y0 >= 0 && y0 < height_20 && y1 >= 0 && y1 < height_20 && count > 0) {
        if (x0 < 0) {
            x1 = x1 - x0;
            count = count + x0;
            x0 = 0;
        }
        if (x1 < 0) {
            x0 = x0 - x1;
            count = count + x1;
            x1 = 0;
        }
        if (x0 < width && x1 < width) {
            if (width < x0 + count) {
                count = width - x0;
            }
            if (width < count + x1) {
                count = width - x1;
            }
            if (count > 0) {
                long bytes = (pixel_format_30.bytes_per_pixel_minus_one + 1) * count;
                unsigned char* buffer = (unsigned char*)srHeap.allocate(bytes * 2);
                unsigned char* second = buffer + bytes;
                getPixelRowRaw(buffer, y0, x0, x0 + count);
                getPixelRowRaw(second, y1, x1, x1 + count);
                setPixelRowRaw(buffer, y1, x1, x1 + count);
                setPixelRowRaw(second, y0, x0, x0 + count);
                srHeap.free(buffer);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10057550
void srColorSurfaceIFace::flipRectangle(const Rectangle& rectangle)
{
    long x_lo = rectangle.left;
    long x_hi = rectangle.right;
    bool flip_x = x_hi < x_lo;
    if (flip_x) {
        x_lo = rectangle.right;
        x_hi = rectangle.left;
    }
    long y_lo = rectangle.top;
    long y_hi = rectangle.bottom;
    bool flip_y = y_hi < y_lo;
    if (flip_y) {
        y_lo = rectangle.bottom;
        y_hi = rectangle.top;
    }
    if ((flip_x || flip_y) && x_lo >= 0 && y_lo >= 0 && x_hi <= width_1c && y_hi <= height_20) {
        long width = x_hi - x_lo;
        long height = y_hi - y_lo;
        if (width != 0 && height != 0) {
            unsigned long* buffer = new unsigned long[width * 2];
            long middle = y_lo + height / 2;
            long mirror = y_hi - 1;
            for (long row = y_lo; row < middle; ++row, --mirror) {
                getPixelRow(buffer, row, x_lo, width);
                getPixelRow(buffer + width, mirror, x_lo, width);
                if (flip_x) {
                    srVectorProcessor::reverse(buffer, buffer, width);
                    srVectorProcessor::reverse(buffer + width, buffer + width, width);
                }
                unsigned long* row_pixels = flip_y ? buffer + width : buffer;
                unsigned long* mirror_pixels = flip_y ? buffer : buffer + width;
                setPixelRow(row_pixels, row, x_lo, width);
                setPixelRow(mirror_pixels, mirror, x_lo, width);
            }
            if (flip_x && (height & 1) != 0) {
                getPixelRow(buffer, middle, x_lo, width);
                srVectorProcessor::reverse(buffer, buffer, width);
                setPixelRow(buffer, middle, x_lo, width);
            }
            delete[] buffer;
        }
    }
}

// FUNCTION: SURRENDER 0x10057B80
void srColorSurfaceIFace::addNoise(double amplitude, int monochrome)
{
    int magnitude = (int)fabs(amplitude * 255.0);
    if (magnitude != 0) {
        long height = height_20;
        long width = width_1c;
        unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            if (monochrome == 0) {
                for (long x = 0; x < width; ++x) {
                    unsigned char* pixel = row + x * 4 + 3;
                    for (int channel = 0; channel < 4; ++channel) {
                        int value = *pixel + (rand() % (magnitude * 2) - magnitude);
                        if (value < 0) {
                            value = 0;
                        } else if (0xff < value) {
                            value = 0xff;
                        }
                        *pixel = (unsigned char)value;
                        --pixel;
                    }
                }
            } else {
                for (long x = 0; x < width; ++x) {
                    int delta = rand() % (magnitude * 2) - magnitude;
                    unsigned char* pixel = row + x * 4 + 3;
                    for (int channel = 0; channel < 4; ++channel) {
                        int value = *pixel + delta;
                        if (value < 0) {
                            value = 0;
                        } else if (0xff < value) {
                            value = 0xff;
                        }
                        *pixel = (unsigned char)value;
                        --pixel;
                    }
                }
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        srHeap.free(row);
    }
}

// FUNCTION: SURRENDER 0x10057EB0
void srColorSurfaceIFace::adjustSaturation(double saturation)
{
    if (saturation != 1.0) {
        long height = height_20;
        long width = width_1c;
        unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            unsigned char* pixel = row + 2;
            for (long x = 0; x < width; ++x) {
                float luminance = pixel[0] * 0.003921569f * 0.2125f +
                                  pixel[-1] * 0.003921569f * 0.7154f +
                                  pixel[-2] * 0.003921569f * 0.0721f;
                float channel = pixel[1] * 0.003921569f * 255.0f;
                if (0.0f < channel) {
                    if (255.0f <= channel) {
                        channel = 255.0f;
                    }
                } else {
                    channel = 0.0f;
                }
                float value =
                    ((pixel[0] * 0.003921569f - luminance) * (float)saturation + luminance) *
                    255.0f;
                pixel[1] = (unsigned char)(int)(channel + 0.5f);
                if (0.0f < value) {
                    if (255.0f <= value) {
                        value = 255.0f;
                    }
                } else {
                    value = 0.0f;
                }
                channel = ((pixel[-1] * 0.003921569f - luminance) * (float)saturation +
                           luminance) *
                          255.0f;
                pixel[0] = (unsigned char)(int)(value + 0.5f);
                if (0.0f < channel) {
                    if (255.0f <= channel) {
                        channel = 255.0f;
                    }
                } else {
                    channel = 0.0f;
                }
                value = ((pixel[-2] * 0.003921569f - luminance) * (float)saturation +
                         luminance) *
                        255.0f;
                pixel[-1] = (unsigned char)(int)(channel + 0.5f);
                if (0.0f < value) {
                    if (255.0f <= value) {
                        value = 255.0f;
                    }
                } else {
                    value = 0.0f;
                }
                pixel[-2] = (unsigned char)(int)(value + 0.5f);
                pixel += 4;
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        srHeap.free(row);
    }
}

// FUNCTION: SURRENDER 0x10057D10
void srColorSurfaceIFace::adjust(const srVector4T<float>& scale, const srVector4T<float>& offset,
                                 const srVector4T<float>& gamma)
{
    long height = height_20;
    long width = width_1c;
    const float* scale_v = &scale.x;
    const float* offset_v = &offset.x;
    const float* gamma_v = &gamma.x;
    unsigned char lut[4][256];
    for (int channel = 0; channel < 4; ++channel) {
        for (int i = 0; i < 0x100; ++i) {
            float value = scale_v[channel] * (offset_v[channel] * (i - 128.0f) + 128.0f) +
                          gamma_v[channel] * 255.0f + 0.5f;
            if (value <= 0.0f) {
                value = 0.0f;
            } else if (value >= 255.0f) {
                value = 255.0f;
            }
            lut[channel][i] = (unsigned char)(int)value;
        }
    }
    unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
    for (long y = 0; y < height; ++y) {
        getPixelRow((unsigned long*)row, y, 0, width);
        for (long x = 0; x < width; ++x) {
            unsigned char* pixel = row + x * 4;
            pixel[2] = lut[0][pixel[2]];
            pixel[1] = lut[1][pixel[1]];
            pixel[0] = lut[2][pixel[0]];
            pixel[3] = lut[3][pixel[3]];
        }
        setPixelRow((const unsigned long*)row, y, 0, width);
    }
    srHeap.free(row);
}

// FUNCTION: SURRENDER 0x1005B040
void srColorSurfaceIFace::remapPixels(const srARGB& from, const srARGB& to)
{
    long width = width_1c;
    long height = height_20;
    unsigned long* row = (unsigned long*)srHeap.allocate(width * 4);
    for (long y = 0; y < height; ++y) {
        getPixelRow(row, y, 0, width);
        long replaced = 0;
        for (long x = 0; x < width; ++x) {
            if (row[x] == *(const unsigned long*)&to) { /* reinterpret-ok: packed ARGB dword */
                row[x] = *(const unsigned long*)&from;  /* reinterpret-ok: packed ARGB dword */
                ++replaced;
            }
        }
        if (replaced != 0) {
            setPixelRow(row, y, 0, width);
        }
    }
    srHeap.free(row);
}

// FUNCTION: SURRENDER 0x10059420
void srColorSurfaceIFace::flipColorChannels(srARGB::e_index first, srARGB::e_index second)
{
    if ((int)first >= 0 && (int)first < 4 && (int)second >= 0 && (int)second < 4 &&
        first != second) {
        long height = height_20;
        long width = width_1c;
        unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            for (long x = 0; x < width; ++x) {
                unsigned char* pixel = row + x * 4;
                unsigned char temp = pixel[3 - second];
                pixel[3 - second] = pixel[3 - first];
                pixel[3 - first] = temp;
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        srHeap.free(row);
    }
}

// FUNCTION: SURRENDER 0x10059520
void srColorSurfaceIFace::copyColorChannel(srARGB::e_index destination, srARGB::e_index source)
{
    if ((int)destination >= 0 && (int)destination < 4 && (int)source >= 0 && (int)source < 4 &&
        destination != source) {
        long height = height_20;
        long width = width_1c;
        unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            for (long x = 0; x < width; ++x) {
                unsigned char* pixel = row + x * 4;
                pixel[3 - destination] = pixel[3 - source];
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        srHeap.free(row);
    }
}

// FUNCTION: SURRENDER 0x10059240
void srColorSurfaceIFace::getChannelStatistics(srStat& statistics, srARGB::e_index channel)
{
    long height = height_20;
    long width = width_1c;
    if ((int)channel >= 0 && (int)channel < 4) {
        unsigned char* row = (unsigned char*)srHeap.allocate(width * 4);
        int* histogram = new int[0x100];
        int* bin = histogram;
        int i;
        for (i = 0x100; i != 0; --i) {
            *bin++ = 0;
        }
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            for (long x = 0; x < width; ++x) {
                ++histogram[row[x * 4 + 3 - channel]];
            }
        }
        statistics.count_00 = 0;
        statistics.mean_08 = 0.0;
        statistics.deviation_10 = 0.0;
        statistics.median_18 = 0;
        statistics.min_1c = 0x100;
        statistics.max_20 = 0;
        for (i = 0; i < 0x100; ++i) {
            if (histogram[i] != 0) {
                if (i < statistics.min_1c) {
                    statistics.min_1c = i;
                }
                if (statistics.max_20 < i) {
                    statistics.max_20 = i;
                }
            }
        }
        for (i = 0; i < 0x100; ++i) {
            statistics.mean_08 = (histogram[i] * i) + statistics.mean_08;
            statistics.count_00 = statistics.count_00 + histogram[i];
        }
        statistics.mean_08 = statistics.mean_08 / statistics.count_00;
        for (i = 0; i < 0x100; ++i) {
            double difference = i - statistics.mean_08;
            statistics.deviation_10 =
                histogram[i] * difference * difference + statistics.deviation_10;
        }
        statistics.deviation_10 = sqrt(statistics.deviation_10 / statistics.count_00);
        long running = 0;
        for (i = 0; i < 0x100; ++i) {
            running += histogram[i];
            statistics.median_18 = i;
            if (statistics.count_00 / 2 < running) {
                break;
            }
        }
        srHeap.free(row);
        delete[] histogram;
    }
}

// FUNCTION: SURRENDER 0x10059900
void srColorSurfaceIFace::copy(srColorSurfaceIFace& source)
{
    if (&source != this) {
        long width = width_1c;
        long source_width = source.width_1c;
        long height = height_20;
        long source_height = source.height_20;
        if (source_width == width && source_height == height) {
            copyNoScaling(source);
            return;
        }
        srFilter* filter = source.filter_2c;
        if (filter != &srBoxFilter && filter != 0) {
            if (filter == &srTriangleFilter) {
                if (width == source_width * 2 && height == source_height * 2) {
                    magnify(source);
                    return;
                }
                if (width * 2 == source_width && height * 2 == source_height) {
                    minify(source);
                    return;
                }
            }
            if (source_width == width) {
                scaleVertical(source);
                return;
            }
            if (source_height == height) {
                scaleHorizontal(source);
                return;
            }
            scale(source);
            return;
        }
        scaleFast(source);
    }
}

// FUNCTION: SURRENDER 0x1005AE10
void srColorSurfaceIFace::dump(std::ostream& stream)
{
    srClass::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    if (filter_2c != 0) {
        stream << "  Filter: " << filter_2c->getName() << '\n';
    } else {
        stream << "  Filter:"
               << "not defined" << '\n';
    }
    stream.width(0x20);
    stream << "  Horizontal clamp mode: " << (clamp_modes_28 & 1) << '\n';
    stream.width(0x20);
    stream << "  Vertical clamp mode: " << (clamp_modes_28 >> 1 & 1) << '\n';
    stream.width(0x20);
    stream << "  Dimensions: " << width_1c << 'x' << height_20 << 'x'
           << (pixel_format_30.bytes_per_pixel_minus_one * 8 + 8) << '\n';
    stream.width(0x20);
    stream << "  Pitch: " << pitch_24 << '\n';
    stream.width(0x20);
    stream << "  Dataptr: " << getDataPtr() << '\n';
    stream.width(0x20);
    stream << "  Data size: " << getDataSize() << " bytes" << '\n';
    stream.width(0x20);
    char format_name[12];
    pixel_format_30.getName(format_name);
    stream << "  Pixel format: " << format_name << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x1005A040
void srColorSurfaceIFace::flipVertical()
{
    Rectangle rectangle;
    rectangle.left = 0;
    rectangle.top = height_20;
    rectangle.right = width_1c;
    rectangle.bottom = 0;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005A070
void srColorSurfaceIFace::flipHorizontal()
{
    Rectangle rectangle;
    rectangle.left = width_1c;
    rectangle.top = 0;
    rectangle.right = 0;
    rectangle.bottom = height_20;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005A0A0
void srColorSurfaceIFace::rotate180()
{
    Rectangle rectangle;
    rectangle.left = width_1c;
    rectangle.top = height_20;
    rectangle.right = 0;
    rectangle.bottom = 0;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005B790
srColorSurface::srColorSurface(const srPixelConvert::PixelFormat& format, unsigned long width,
                               unsigned long height)
{
    surface_flags_50 = 0;
    init(format, width, height, (format.bytes_per_pixel_minus_one + 1) * width);
    allocData();
    srPixelConvert::selectFuncs(format, pixel_write_44, pixel_read_48);
}

// FUNCTION: SURRENDER 0x1005B8A0
srColorSurface::srColorSurface(srPixelConvert::e_surfaceType type, unsigned long width,
                               unsigned long height)
{
    srPixelConvert::PixelFormat format;
    surface_flags_50 = 0;
    srPixelConvert::mapPixelFormat(type, format);
    init(format, width, height, (format.bytes_per_pixel_minus_one + 1) * width);
    allocData();
    srPixelConvert::selectFuncs(format, pixel_write_44, pixel_read_48);
}

/* The data-taking variants adopt caller storage: flag bit 0 marks the
   non-owning path and data_size comes straight from pitch*height. */
// FUNCTION: SURRENDER 0x1005B9C0
srColorSurface::srColorSurface(srPixelConvert::e_surfaceType type, void* data, unsigned long width,
                               unsigned long height, unsigned long pitch)
{
    srPixelConvert::PixelFormat format;
    surface_flags_50 = 0;
    srPixelConvert::mapPixelFormat(type, format);
    init(format, width, height, pitch);
    surface_flags_50 |= 1;
    data_size_54 = pitch_24 * height_20;
    data_58 = data;
    srPixelConvert::selectFuncs(format, pixel_write_44, pixel_read_48);
}

// FUNCTION: SURRENDER 0x1005DE30
srColorSurface::srColorSurface(const srColorSurface& other)
{
    /* Retail assigns, then re-copies the function pointers, palette and the
       surface_flags/data_size/data tail verbatim. */
    *this = other;
    pixel_write_44 = other.pixel_write_44;
    pixel_read_48 = other.pixel_read_48;
    palette_4c = other.palette_4c;
    surface_flags_50 = other.surface_flags_50;
    data_size_54 = other.data_size_54;
    data_58 = other.data_58;
}

// FUNCTION: SURRENDER 0x1005D520
srColorSurface& srColorSurface::operator=(const srColorSurface& other)
{
    if (&other != this) {
        srColorSurfaceIFace::operator=(other);
        freeData();
        srPixelConvert::PixelFormat format = other.pixel_format_30;
        init(format, other.width_1c, other.height_20, other.pitch_24);
        palette_4c = other.palette_4c;
        surface_flags_50 = other.surface_flags_50;
        pixel_write_44 = other.pixel_write_44;
        pixel_read_48 = other.pixel_read_48;
        if ((surface_flags_50 & 1) != 0) {
            data_size_54 = other.data_size_54;
            data_58 = other.data_58;
            return *this;
        }
        allocData();
        copy(const_cast<srColorSurface&>(other));
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005BAC0
srColorSurface::srColorSurface(const srPixelConvert::PixelFormat& format, void* data,
                               unsigned long width, unsigned long height, unsigned long pitch)
{
    surface_flags_50 = 0;
    init(format, width, height, pitch);
    surface_flags_50 |= 1;
    data_size_54 = pitch_24 * height_20;
    data_58 = data;
    srPixelConvert::selectFuncs(format, pixel_write_44, pixel_read_48);
}

// FUNCTION: SURRENDER 0x1005BBE0
srColorSurface::~srColorSurface()
{
    freeData();
}

// FUNCTION: SURRENDER 0x1005B550
void* srColorSurface::getDataPtr()
{
    return data_58;
}

// FUNCTION: SURRENDER 0x1005B560
long srColorSurface::getDataSize()
{
    return data_size_54;
}

// FUNCTION: SURRENDER 0x1005B570
srPalette* srColorSurface::getPalette()
{
    return palette_4c;
}

// FUNCTION: SURRENDER 0x1005B580
void srColorSurface::setPalette(srPalette* palette)
{
    palette_4c = palette;
}

// FUNCTION: SURRENDER 0x1005B5B0
unsigned char* srColorSurface::getAddress(long x, long y)
{
    return (unsigned char*)data_58 + pitch_24 * y +
           (pixel_format_30.bytes_per_pixel_minus_one + 1) * x;
}

// FUNCTION: SURRENDER 0x1005B5D0
void srColorSurface::convertToARGB8888(unsigned long* pixels, const void* source,
                                       unsigned long count)
{
    srPixelConvert::ConversionInfo info;
    info.dest = pixels;
    info.source = source;
    info.count = count;
    info.palette = palette_4c;
    info.format = &pixel_format_30;
    pixel_read_48(info);
}

// FUNCTION: SURRENDER 0x1005B610
void srColorSurface::convertFromARGB8888(void* pixels, const unsigned long* source,
                                         unsigned long count)
{
    srPixelConvert::ConversionInfo info;
    info.dest = pixels;
    info.source = source;
    info.count = count;
    info.palette = palette_4c;
    info.format = &pixel_format_30;
    pixel_write_44(info);
}

// FUNCTION: SURRENDER 0x1005B650
void srColorSurface::allocData()
{
    data_size_54 = pitch_24 * height_20;
    if (data_size_54 != 0) {
        data_58 = srHeap.allocate(data_size_54);
    }
}

// FUNCTION: SURRENDER 0x1005B680
void srColorSurface::freeData()
{
    if (!(surface_flags_50 & 1) && data_58 != 0) {
        srHeap.free(data_58);
        data_58 = 0;
    }
}

// FUNCTION: SURRENDER 0x1005B6B0
void srColorSurface::init(const srPixelConvert::PixelFormat& format, unsigned long width,
                          unsigned long height, unsigned long pitch)
{
    SurfaceDesc desc;
    srZeroMemory(&desc, sizeof(desc));
    desc.width = width;
    desc.height = height;
    desc.pitch = pitch;
    desc.pixel_format = format;
    setSurfaceDesc(desc);
    palette_4c = srCore.getPalette();
    pixel_write_44 = 0;
    pixel_read_48 = 0;
    data_size_54 = 0;
    data_58 = 0;
    surface_flags_50 = 0;
}

// FUNCTION: SURRENDER 0x1005DD70
srPixelConvert::ConversionFunc srColorSurface::getPixelWriteFunc() const
{
    return pixel_write_44;
}

// FUNCTION: SURRENDER 0x1005DD80
srPixelConvert::ConversionFunc srColorSurface::getPixelReadFunc() const
{
    return pixel_read_48;
}

// FUNCTION: SURRENDER 0x1005DD90
void srColorSurface::setPixelWriteFunc(srPixelConvert::ConversionFunc function)
{
    if (function != 0) {
        pixel_write_44 = function;
    }
}

// FUNCTION: SURRENDER 0x1005DDA0
void srColorSurface::setPixelReadFunc(srPixelConvert::ConversionFunc function)
{
    if (function != 0) {
        pixel_read_48 = function;
    }
}

// FUNCTION: SURRENDER 0x1005DDB0
srClass* srColorSurface::vInstance()
{
    return new srColorSurface(pixel_format_30, 1, 1);
}

// FUNCTION: SURRENDER 0x1005DE20
const char* srColorSurface::sGetClassName()
{
    return "srColorSurface";
}

// FUNCTION: SURRENDER 0x1005BEC0
int srColorSurface::resize(long width, long height)
{
    if (width > 0 && height > 0) {
        if (width == width_1c && height == height_20) {
            return 1;
        }
        if (!(surface_flags_50 & 1)) {
            freeData();
            SurfaceDesc desc;
            desc.width = width;
            desc.height = height;
            desc.pitch = (pixel_format_30.bytes_per_pixel_minus_one + 1) * width;
            desc.clamp_modes = clamp_modes_28;
            desc.filter = filter_2c;
            desc.pixel_format = pixel_format_30;
            setSurfaceDesc(desc);
            allocData();
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005BD10
int srColorSurface::rescale(long width, long height)
{
    if (width > 0 && height > 0) {
        if (width == width_1c && height == height_20) {
            return 1;
        }
        if (!(surface_flags_50 & 1)) {
            srColorSurface* scaled =
                new srColorSurface(srPixelConvert::SURFACE_BGRA32, width, height);
            scaled->copySurfaceParameters(*this);
            scaled->copy(*this);
            resize(width, height);
            copy(*scaled);
            scaled->release();
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005BDE0
int srColorSurface::changePixelFormat(const srPixelConvert::PixelFormat& format, int preserve)
{
    if (surface_flags_50 & 1) {
        return 0;
    }
    if (format.flags != pixel_format_30.flags || format.red_bits != pixel_format_30.red_bits ||
        format.red_shift != pixel_format_30.red_shift ||
        format.green_bits != pixel_format_30.green_bits ||
        format.green_shift != pixel_format_30.green_shift ||
        format.blue_bits != pixel_format_30.blue_bits ||
        format.blue_shift != pixel_format_30.blue_shift ||
        format.alpha_bits != pixel_format_30.alpha_bits ||
        format.alpha_shift != pixel_format_30.alpha_shift ||
        format.conversion_class != pixel_format_30.conversion_class ||
        format.bytes_per_pixel_minus_one != pixel_format_30.bytes_per_pixel_minus_one) {
        srColorSurfaceIFace* previous = 0;
        if (preserve != 0) {
            previous = static_cast<srColorSurfaceIFace*>(clone());
        }
        srPalette* palette = getPalette();
        unsigned long flags = surface_flags_50;
        freeData();
        init(format, width_1c, height_20, (format.bytes_per_pixel_minus_one + 1) * width_1c);
        allocData();
        srPixelConvert::selectFuncs(format, pixel_write_44, pixel_read_48);
        setPalette(palette);
        surface_flags_50 = flags;
        if (previous != 0) {
            copy(*previous);
            previous->release();
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x1005C4D0
int srColorSurface::isCompatible(srColorSurfaceIFace& source)
{
    if (source.getClassID() != getClassID()) {
        return 0;
    }
    if (pixel_format_30.flags == source.pixel_format_30.flags &&
        source.pixel_format_30.red_bits == pixel_format_30.red_bits &&
        source.pixel_format_30.red_shift == pixel_format_30.red_shift &&
        source.pixel_format_30.green_bits == pixel_format_30.green_bits &&
        source.pixel_format_30.green_shift == pixel_format_30.green_shift &&
        source.pixel_format_30.blue_bits == pixel_format_30.blue_bits &&
        source.pixel_format_30.blue_shift == pixel_format_30.blue_shift &&
        source.pixel_format_30.alpha_bits == pixel_format_30.alpha_bits &&
        source.pixel_format_30.alpha_shift == pixel_format_30.alpha_shift &&
        pixel_format_30.bytes_per_pixel_minus_one ==
            source.pixel_format_30.bytes_per_pixel_minus_one &&
        pixel_format_30.conversion_class == source.pixel_format_30.conversion_class) {
        if (pixel_format_30.conversion_class == 3 && source.getPalette() != getPalette()) {
            return 0;
        }
        return source.getDataPtr() != 0;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005C460
unsigned long srColorSurface::getPixelRaw(long x, long y)
{
    unsigned char* address = getAddress(x, y);
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0:
        return *address;
    case 1:
        return *(unsigned short*)address;
    case 2:
        return address[0] | (address[1] << 8) | (address[2] << 0x10);
    case 3:
        return *(unsigned long*)address;
    default:
        return 0;
    }
}

// FUNCTION: SURRENDER 0x1005C030
void srColorSurface::setPixelRaw(long x, long y, unsigned long pixel)
{
    unsigned char* address = getAddress(x, y);
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0:
        *address = (unsigned char)pixel;
        break;
        break;
    case 1:
        *(unsigned short*)address = (unsigned short)pixel;
        break;
        break;
    case 2:
        address[0] = (unsigned char)pixel;
        address[1] = (unsigned char)(pixel >> 8);
        address[2] = (unsigned char)(pixel >> 0x10);
        break;
        break;
    case 3:
        *(unsigned long*)address = pixel;
        break;
        break;
    }
}

// FUNCTION: SURRENDER 0x1005D940
void srColorSurface::getPixelRow(unsigned long* pixels, long y, long x_start, long x_end)
{
    if (x_start < x_end) {
        convertToARGB8888(pixels, getAddress(x_start, y), x_end - x_start);
    }
}

// FUNCTION: SURRENDER 0x1005DAA0
void srColorSurface::setPixelRow(const unsigned long* pixels, long y, long x_start, long x_end)
{
    if (x_start < x_end) {
        convertFromARGB8888(getAddress(x_start, y), pixels, x_end - x_start);
    }
}

// FUNCTION: SURRENDER 0x1005BF70
void srColorSurface::getPixelRowRaw(void* pixels, long y, long x_start, long x_end)
{
    if (y >= 0 && y < height_20 && x_start >= 0 && x_start < x_end && x_end <= width_1c) {
        long count = (x_end - x_start) * (pixel_format_30.bytes_per_pixel_minus_one + 1);
        unsigned char* address = getAddress(x_start, y);
        if (count != 0 && pixels != address) {
            srVectorProcessor::memcopy(pixels, address, count);
        }
    }
}

// FUNCTION: SURRENDER 0x1005BFD0
void srColorSurface::setPixelRowRaw(const void* pixels, long y, long x_start, long x_end)
{
    if (y >= 0 && y < height_20 && x_start >= 0 && x_start < x_end && x_end <= width_1c) {
        long count = (x_end - x_start) * (pixel_format_30.bytes_per_pixel_minus_one + 1);
        unsigned char* address = getAddress(x_start, y);
        if (count != 0 && address != pixels) {
            srVectorProcessor::memcopy(address, pixels, count);
        }
    }
}

// FUNCTION: SURRENDER 0x1005D970
void srColorSurface::getPixelColumn(unsigned long* pixels, long x, long y_start, long y_end)
{
    unsigned char buffer[0x400];
    for (; y_start < y_end; y_start += 0x100) {
        unsigned long count = y_end - y_start;
        if (count > 0x100) {
            count = 0x100;
        }
        unsigned char* address = getAddress(x, y_start);
        switch (pixel_format_30.bytes_per_pixel_minus_one) {
        case 0: {
            unsigned long i = 0;
            if (count != 0) {
                do {
                    buffer[i] = *address;
                    ++i;
                    address += pitch_24;
                } while (i < count);
            }
            break;
        }
        case 1: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    *(unsigned short*)out = *(unsigned short*)address;
                    out += 2;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case 2: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    out[0] = address[0];
                    out[1] = address[1];
                    out[2] = address[2];
                    out += 3;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case 3: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    *(unsigned long*)out = *(unsigned long*)address;
                    out += 4;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        }
        convertToARGB8888(pixels, (const void*)buffer, count);
        pixels += 0x100;
    }
}

// FUNCTION: SURRENDER 0x1005DAD0
void srColorSurface::setPixelColumn(const unsigned long* pixels, long x, long y_start, long y_end)
{
    unsigned char buffer[0x400];
    for (; y_start < y_end; y_start += 0x100) {
        unsigned long count = y_end - y_start;
        if (count > 0x100) {
            count = 0x100;
        }
        convertFromARGB8888(buffer, pixels, count);
        unsigned char* address = getAddress(x, y_start);
        switch (pixel_format_30.bytes_per_pixel_minus_one) {
        case 0: {
            unsigned long i = 0;
            if (count != 0) {
                do {
                    *address = buffer[i];
                    ++i;
                    address += pitch_24;
                } while (i < count);
            }
            break;
        }
        case 1: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    *(unsigned short*)address = *(const unsigned short*)in;
                    in += 2;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case 2: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    address[0] = in[0];
                    address[1] = in[1];
                    address[2] = in[2];
                    in += 3;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case 3: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    *(unsigned long*)address = *(const unsigned long*)in;
                    in += 4;
                    address += pitch_24;
                    --i;
                } while (i != 0);
            }
            break;
        }
        }
        pixels += 0x100;
    }
}

// FUNCTION: SURRENDER 0x1005D7A0
void srColorSurface::getPixels(unsigned long* pixels, const srVector2i* positions, long count)
{
    unsigned char buffer[0x400];
    long i = 0;
    if (count > 0) {
        do {
            unsigned long chunk = count - i;
            if (chunk > 0x100) {
                chunk = 0x100;
            }
            getPixelsRaw(buffer, positions, chunk);
            convertToARGB8888(pixels, buffer, chunk);
            i += 0x100;
            positions += 0x100;
            pixels += 0x100;
        } while (i < count);
    }
}

// FUNCTION: SURRENDER 0x1005D5F0
void srColorSurface::setPixels(const unsigned long* pixels, const srVector2i* positions, long count)
{
    unsigned char buffer[0x400];
    long i = 0;
    if (count > 0) {
        do {
            unsigned long chunk = count - i;
            if (chunk > 0x100) {
                chunk = 0x100;
            }
            convertFromARGB8888(buffer, pixels, chunk);
            setPixelsRaw(buffer, positions, chunk);
            i += 0x100;
            positions += 0x100;
            pixels += 0x100;
        } while (i < count);
    }
}

// FUNCTION: SURRENDER 0x1005D830
void srColorSurface::getPixelsRaw(void* pixels, const srVector2i* positions, long count)
{
    unsigned char* out = (unsigned char*)pixels;
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0: {
        for (long i = 0; i < count; ++i, ++positions, ++out) {
            *out = *getAddress(positions->x, positions->y);
        }
        break;
    }
    case 1: {
        for (long i = 0; i < count; ++i, ++positions, out += 2) {
            *(unsigned short*)out = *(unsigned short*)getAddress(positions->x, positions->y);
        }
        break;
    }
    case 2: {
        for (long i = 0; i < count; ++i, ++positions, out += 3) {
            const unsigned char* address = getAddress(positions->x, positions->y);
            out[0] = address[0];
            out[1] = address[1];
            out[2] = address[2];
        }
        break;
    }
    case 3: {
        for (long i = 0; i < count; ++i, ++positions, out += 4) {
            *(unsigned long*)out = *(unsigned long*)getAddress(positions->x, positions->y);
        }
        break;
    }
    }
}

// FUNCTION: SURRENDER 0x1005D680
void srColorSurface::setPixelsRaw(const void* pixels, const srVector2i* positions, long count)
{
    const unsigned char* in = (const unsigned char*)pixels;
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0: {
        for (long i = 0; i < count; ++i, ++positions, ++in) {
            *getAddress(positions->x, positions->y) = *in;
        }
        break;
    }
    case 1: {
        for (long i = 0; i < count; ++i, ++positions, in += 2) {
            *(unsigned short*)getAddress(positions->x, positions->y) = *(const unsigned short*)in;
        }
        break;
    }
    case 2: {
        for (long i = 0; i < count; ++i, ++positions, in += 3) {
            unsigned char* address = getAddress(positions->x, positions->y);
            address[0] = in[0];
            address[1] = in[1];
            address[2] = in[2];
        }
        break;
    }
    case 3: {
        for (long i = 0; i < count; ++i, ++positions, in += 4) {
            *(unsigned long*)getAddress(positions->x, positions->y) = *(const unsigned long*)in;
        }
        break;
    }
    }
}

// FUNCTION: SURRENDER 0x1005E230
static void reversePixelTriplets(unsigned char* pixels, unsigned long count)
{
    unsigned char* lo = pixels;
    unsigned char* hi = pixels + (count - 1) * 3;
    for (unsigned long i = count >> 1; i != 0; --i, lo += 3, hi -= 3) {
        unsigned char t0 = lo[0];
        unsigned char t1 = lo[1];
        unsigned char t2 = lo[2];
        lo[0] = hi[0];
        lo[1] = hi[1];
        lo[2] = hi[2];
        hi[0] = t0;
        hi[1] = t1;
        hi[2] = t2;
    }
}

// FUNCTION: SURRENDER 0x1005C150
void srColorSurface::reversePixels(void* pixels, unsigned long count)
{
    unsigned char* address = (unsigned char*)pixels;
    switch (pixel_format_30.bytes_per_pixel_minus_one) {
    case 0: {
        unsigned char* lo = address;
        unsigned char* hi = address + (count - 1);
        for (unsigned long i = count >> 1; i != 0; --i, ++lo, --hi) {
            unsigned char t = *lo;
            *lo = *hi;
            *hi = t;
        }
        break;
    }
    case 1: {
        unsigned short* lo = (unsigned short*)address;
        unsigned short* hi = (unsigned short*)(address + (count - 1) * 2);
        for (unsigned long i = count >> 1; i != 0; --i, ++lo, --hi) {
            unsigned short t = *lo;
            *lo = *hi;
            *hi = t;
        }
        break;
    }
    case 2:
        reversePixelTriplets(address, count);
        break;
        break;
    case 3:
        if (count != 0) {
            srVectorProcessor::reverse((SRDWORD*)address, (const SRDWORD*)address, count);
        }
        break;
        break;
    }
}

// FUNCTION: SURRENDER 0x1005C0A0
void srColorSurface::swapPixelRows(long x0, long y0, long x1, long y1, long count)
{
    if (y0 >= 0 && y0 < height_20 && y1 >= 0 && y1 < height_20 && count > 0) {
        if (x0 < 0) {
            x1 -= x0;
            count += x0;
            x0 = 0;
        }
        if (x1 < 0) {
            x0 -= x1;
            count += x1;
            x1 = 0;
        }
        if (x0 < width_1c && x1 < width_1c) {
            if (x0 + count > width_1c) {
                count = width_1c - x0;
            }
            if (x1 + count > width_1c) {
                count = width_1c - x1;
            }
            if (count > 0) {
                srVectorProcessor::swap(getAddress(x0, y0), getAddress(x1, y1),
                                        (pixel_format_30.bytes_per_pixel_minus_one + 1) * count);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005C2E0
void srColorSurface::flipRectangle(const Rectangle& rectangle)
{
    long x_lo = rectangle.left;
    long x_hi = rectangle.right;
    bool flip_x = x_hi < x_lo;
    if (flip_x) {
        x_lo = rectangle.right;
        x_hi = rectangle.left;
    }
    long y_lo = rectangle.top;
    long y_hi = rectangle.bottom;
    bool flip_y = y_hi < y_lo;
    if (flip_y) {
        y_lo = rectangle.bottom;
        y_hi = rectangle.top;
    }
    if ((flip_x || flip_y) && x_lo >= 0 && y_lo >= 0 && x_hi <= width_1c && y_hi <= height_20) {
        unsigned long width = x_hi - x_lo;
        unsigned long height = y_hi - y_lo;
        if (width != 0 && height != 0) {
            int bpp = pixel_format_30.bytes_per_pixel_minus_one;
            long middle = y_lo + (long)height / 2;
            unsigned char* base = (unsigned char*)data_58 + (bpp + 1) * x_lo;
            unsigned long mirror = height;
            for (long row = y_lo; row < middle; ++row) {
                --mirror;
                void* row_address = (void*)(pitch_24 * row + base);
                void* mirror_address = (void*)(pitch_24 * mirror + base);
                if (flip_y) {
                    srVectorProcessor::swap(row_address, mirror_address, (bpp + 1) * width);
                }
                if (flip_x) {
                    reversePixels(row_address, width);
                    reversePixels(mirror_address, width);
                }
            }
            if (flip_x && (height & 1) != 0) {
                reversePixels((void*)(pitch_24 * middle + base), width);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005C560
void srColorSurface::setHLine(long y, long x_start, long x_end, unsigned long pixel)
{
    if (data_58 == 0) {
        srColorSurfaceIFace::setHLine(y, x_start, x_end, pixel);
        return;
    }
    if (y >= 0 && y < height_20) {
        long x_hi = x_end;
        if (x_end < x_start) {
            x_hi = x_start;
            x_start = x_end;
        }
        if (x_start < 0) {
            x_start = 0;
        }
        if (x_hi > width_1c) {
            x_hi = width_1c;
        }
        if (x_start < x_hi) {
            setPixel(x_start, y, pixel);
            unsigned long raw = getPixelRaw(x_start, y);
            unsigned char* address = getAddress(x_start, y);
            switch (pixel_format_30.bytes_per_pixel_minus_one) {
            case 0:
                if (x_hi - x_start != 0) {
                    srVectorProcessor::memcopy(address, (SRBYTE)raw, x_hi - x_start);
                }
                break;
                break;
            case 1: {
                unsigned long count = x_hi - x_start;
                unsigned long half = count >> 1;
                if (half != 0) {
                    srVectorProcessor::copy((SRDWORD*)address, (raw << 0x10) | (raw & 0xffff),
                                            half);
                }
                if ((count & 1) != 0) {
                    *(unsigned short*)(address + count * 2 - 2) = (unsigned short)raw;
                }
                break;
            }
            case 2: {
                unsigned long count = x_hi - x_start;
                for (unsigned long i = 0; i < count; ++i) {
                    *(unsigned short*)address = (unsigned short)raw;
                    address[2] = (unsigned char)(raw >> 0x10);
                    address += 3;
                }
                break;
            }
            case 3:
                if (x_hi - x_start != 0) {
                    srVectorProcessor::copy((SRDWORD*)address, raw, x_hi - x_start);
                }
                break;
                break;
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005C740
void srColorSurface::setVLine(long x, long y_start, long y_end, unsigned long pixel)
{
    if (data_58 == 0) {
        srColorSurfaceIFace::setVLine(x, y_start, y_end, pixel);
        return;
    }
    if (x >= 0 && x < width_1c) {
        long y_hi = y_end;
        if (y_end < y_start) {
            y_hi = y_start;
            y_start = y_end;
        }
        if (y_start < 0) {
            y_start = 0;
        }
        if (y_hi > height_20) {
            y_hi = height_20;
        }
        if (y_start < y_hi) {
            setPixel(x, y_start, pixel);
            unsigned long raw = getPixelRaw(x, y_start);
            unsigned char* address = getAddress(x, y_start);
            unsigned long count = y_hi - y_start;
            switch (pixel_format_30.bytes_per_pixel_minus_one) {
            case 0: {
                for (unsigned long i = 0; i < count; ++i) {
                    *address = (unsigned char)raw;
                    address += pitch_24;
                }
                break;
            }
            case 1: {
                for (unsigned long i = 0; i < count; ++i) {
                    *(unsigned short*)address = (unsigned short)raw;
                    address += pitch_24;
                }
                break;
            }
            case 2: {
                for (unsigned long i = 0; i < count; ++i) {
                    *(unsigned short*)address = (unsigned short)raw;
                    address[2] = (unsigned char)(raw >> 0x10);
                    address += pitch_24;
                }
                break;
            }
            case 3: {
                for (unsigned long i = 0; i < count; ++i) {
                    *(unsigned long*)address = raw;
                    address += pitch_24;
                }
                break;
            }
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005C900
void srColorSurface::fill(unsigned long pixel)
{
    if (getDataPtr() == 0) {
        srColorSurfaceIFace::fill(pixel);
        return;
    }
    setPixel(0, 0, pixel);
    unsigned long raw = getPixelRaw(0, 0);
    unsigned char* data = (unsigned char*)getDataPtr();
    long pitch = pitch_24;
    unsigned long width = width_1c;
    long height = height_20;
    int bpp = pixel_format_30.bytes_per_pixel_minus_one;
    if (pitch == (long)((bpp + 1) * width)) {
        unsigned long count = height * width;
        switch (bpp) {
        case 0:
            if (count != 0) {
                srVectorProcessor::memcopy(data, (SRBYTE)raw, count);
            }
            break;
            break;
        case 1: {
            unsigned long half = count >> 1;
            if (half != 0) {
                srVectorProcessor::copy((SRDWORD*)data, (raw << 0x10) | (raw & 0xffff), half);
            }
            if ((count & 1) != 0) {
                *(unsigned short*)(data + count * 2 - 2) = (unsigned short)raw;
            }
            break;
        }
        case 2: {
            for (unsigned long i = 0; i < count; ++i) {
                *(unsigned short*)data = (unsigned short)raw;
                data[2] = (unsigned char)(raw >> 0x10);
                data += 3;
            }
            break;
        }
        case 3:
            if (count != 0) {
                srVectorProcessor::copy((SRDWORD*)data, raw, count);
            }
            break;
            break;
        }
    } else {
        switch (bpp) {
        case 0: {
            for (long row = 0; row < height; ++row) {
                if (width != 0) {
                    srVectorProcessor::memcopy(data, (SRBYTE)raw, width);
                }
                data += pitch;
            }
            break;
        }
        case 1: {
            for (long row = 0; row < height; ++row) {
                unsigned long half = width >> 1;
                if (half != 0) {
                    srVectorProcessor::copy((SRDWORD*)data, (raw << 0x10) | (raw & 0xffff), half);
                }
                if ((width & 1) != 0) {
                    *(unsigned short*)(data + width * 2 - 2) = (unsigned short)raw;
                }
                data += pitch;
            }
            break;
        }
        case 2: {
            for (long row = 0; row < height; ++row) {
                unsigned char* out = data;
                for (unsigned long i = 0; i < width; ++i) {
                    *(unsigned short*)out = (unsigned short)raw;
                    out[2] = (unsigned char)(raw >> 0x10);
                    out += 3;
                }
                data += pitch;
            }
            break;
        }
        case 3: {
            for (long row = 0; row < height; ++row) {
                if (width != 0) {
                    srVectorProcessor::copy((SRDWORD*)data, raw, width);
                }
                data += pitch;
            }
            break;
        }
        }
    }
}

// FUNCTION: SURRENDER 0x1005D220
void srColorSurface::blit(long x, long y, srColorSurfaceIFace& source, long source_x, long source_y,
                          long x_end, long y_end)
{
    if (&source != this && isCompatible(source) == 0) {
        srColorSurfaceIFace::blit(x, y, source, source_x, source_y, x_end, y_end);
        return;
    }
    if (x < width_1c && y < height_20) {
        if (x < 0) {
            source_x -= x;
            x = 0;
        }
        if (y < 0) {
            source_y -= y;
            y = 0;
        }
        if (source_x < 0) {
            x -= source_x;
            source_x = 0;
        }
        if (source_y < 0) {
            y -= source_y;
            source_y = 0;
        }
        if (x_end > source.width_1c) {
            x_end = source.width_1c;
        }
        if (y_end > source.height_20) {
            y_end = source.height_20;
        }
        if (source_x < source.width_1c && source_x < x_end && source_y < source.height_20 &&
            source_y < y_end) {
            if (width_1c < (x - source_x) + x_end) {
                x_end = width_1c - x + source_x;
            }
            if (height_20 < (y - source_y) + y_end) {
                y_end = height_20 - y + source_y;
            }
            if (source_x < x_end && source_y < y_end) {
                long dest_pitch = pitch_24;
                long source_pitch = source.pitch_24;
                int bpp = pixel_format_30.bytes_per_pixel_minus_one + 1;
                unsigned long row_bytes = (x_end - source_x) * bpp;
                unsigned char* dest = (unsigned char*)getDataPtr() + dest_pitch * y + bpp * x;
                unsigned char* src =
                    (unsigned char*)source.getDataPtr() + source_pitch * source_y + bpp * source_x;
                if (&source == this && source_y <= y) {
                    if (source_y != y || source_x != x) {
                        long dest_right = (x - source_x) + x_end;
                        if (source_y == y && ((source_x <= x && x < x_end) ||
                                              (source_x <= dest_right && dest_right < x_end))) {
                            unsigned char* temp = new unsigned char[row_bytes];
                            for (long row = source_y; row < y_end; ++row) {
                                if (row_bytes != 0) {
                                    if (temp != src) {
                                        srVectorProcessor::memcopy(temp, src, row_bytes);
                                    }
                                    if (dest != temp) {
                                        srVectorProcessor::memcopy(dest, temp, row_bytes);
                                    }
                                }
                                dest += dest_pitch;
                                src += source_pitch;
                            }
                            delete[] temp;
                            return;
                        }
                        long rows = y_end - source_y;
                        src += (rows - 1) * source_pitch;
                        dest += (rows - 1) * dest_pitch;
                        do {
                            if (row_bytes != 0 && dest != src) {
                                srVectorProcessor::memcopy(dest, src, row_bytes);
                            }
                            dest -= dest_pitch;
                            src -= source_pitch;
                            --rows;
                        } while (rows != 0);
                        return;
                    }
                } else {
                    if (source_y < y_end) {
                        long rows = y_end - source_y;
                        do {
                            if (row_bytes != 0 && dest != src) {
                                srVectorProcessor::memcopy(dest, src, row_bytes);
                            }
                            dest += dest_pitch;
                            src += source_pitch;
                            --rows;
                        } while (rows != 0);
                    }
                }
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005D010
void srColorSurface::copyNoScaling(srColorSurfaceIFace& source)
{
    if (this == &source) {
        return;
    }
    if (isCompatible(source) == 0) {
        if (source.getClassID() != getClassID()) {
            srColorSurfaceIFace::copyNoScaling(source);
            return;
        }
        if (pixel_format_30.conversion_class == 0 &&
            pixel_format_30.bytes_per_pixel_minus_one == 3 && pixel_format_30.red_bits == 8 &&
            pixel_format_30.green_bits == 8 && pixel_format_30.blue_bits == 8 &&
            pixel_format_30.alpha_bits == 8 && pixel_format_30.red_shift == 0x10 &&
            pixel_format_30.green_shift == 8 && pixel_format_30.blue_shift == 0 &&
            pixel_format_30.alpha_shift == 0x18) {
            unsigned char* dest = (unsigned char*)getDataPtr();
            for (long row = 0; row < height_20; ++row) {
                source.getPixelRow((unsigned long*)dest, row, 0, width_1c);
                dest += pitch_24;
            }
            return;
        }
        const srPixelConvert::PixelFormat& source_format = source.pixel_format_30;
        if (source_format.conversion_class != 0 || source_format.bytes_per_pixel_minus_one != 3 ||
            source_format.red_bits != 8 || source_format.green_bits != 8 ||
            source_format.blue_bits != 8 || source_format.alpha_bits != 8 ||
            source_format.red_shift != 0x10 || source_format.green_shift != 8 ||
            source_format.blue_shift != 0 || source_format.alpha_shift != 0x18 ||
            source.getDataPtr() == 0) {
            srColorSurfaceIFace::copyNoScaling(source);
            return;
        }
        unsigned char* src = (unsigned char*)source.getDataPtr();
        for (long row = 0; row < height_20; ++row) {
            setPixelRow((const unsigned long*)src, row, 0, width_1c);
            src += source.pitch_24;
        }
        return;
    }
    unsigned char* dest = (unsigned char*)getDataPtr();
    unsigned char* src = (unsigned char*)source.getDataPtr();
    long dest_pitch = pitch_24;
    long source_pitch = source.pitch_24;
    if (dest_pitch == source_pitch &&
        dest_pitch == (pixel_format_30.bytes_per_pixel_minus_one + 1) * width_1c) {
        long size = getDataSize();
        if (size != 0 && dest != src) {
            srVectorProcessor::memcopy(dest, src, size);
        }
    } else {
        long row_bytes = (pixel_format_30.bytes_per_pixel_minus_one + 1) * width_1c;
        for (long row = height_20; row != 0; --row) {
            if (row_bytes != 0 && dest != src) {
                srVectorProcessor::memcopy(dest, src, row_bytes);
            }
            dest += dest_pitch;
            src += source_pitch;
        }
    }
}

// FUNCTION: SURRENDER 0x1005CC90
void srColorSurface::scaleFast(srColorSurfaceIFace& source)
{
    if (isCompatible(source) == 0) {
        srColorSurfaceIFace::scaleFast(source);
        return;
    }
    long dest_width = width_1c;
    long dest_height = height_20;
    if (source.width_1c == dest_width && source.height_20 == dest_height) {
        copyNoScaling(source);
        return;
    }
    long* columns = new long[dest_width];
    unsigned char* dest = (unsigned char*)getDataPtr();
    unsigned char* src = (unsigned char*)source.getDataPtr();
    long source_pitch = source.pitch_24;
    int bpp = pixel_format_30.bytes_per_pixel_minus_one;
    long dest_pitch = pitch_24;
    double x_ratio = source.width_1c / (double)dest_width;
    double y_ratio = source.height_20 / (double)dest_height;
    for (long i = 0; i < dest_width; ++i) {
        columns[i] = source.getClampedX((long)(i * x_ratio));
    }
    for (long row = 0; row < dest_height; ++row) {
        long source_y = source.getClampedY((long)(row * y_ratio));
        const unsigned char* source_row = src + source_y * source_pitch;
        switch (bpp) {
        case 0: {
            for (long x = 0; x < dest_width; ++x) {
                dest[x] = source_row[columns[x]];
            }
            break;
        }
        case 1: {
            for (long x = 0; x < dest_width; ++x) {
                ((unsigned short*)dest)[x] = ((const unsigned short*)source_row)[columns[x]];
            }
            break;
        }
        case 2: {
            for (long x = 0; x < dest_width; ++x) {
                const unsigned char* pixel = source_row + columns[x] * 3;
                dest[x * 3] = pixel[0];
                dest[x * 3 + 1] = pixel[1];
                dest[x * 3 + 2] = pixel[2];
            }
            break;
        }
        case 3: {
            for (long x = 0; x < dest_width; ++x) {
                ((unsigned long*)dest)[x] = ((const unsigned long*)source_row)[columns[x]];
            }
            break;
        }
        }
        dest += dest_pitch;
    }
    delete[] columns;
}

// FUNCTION: SURRENDER 0x1005DBE0
void srColorSurface::dump(std::ostream& stream)
{
    srColorSurfaceIFace::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    if (getPalette() != 0) {
        stream.width(0x20);
        stream << "  Palette: ";
        getPalette()->getUniqueName(stream);
        stream << '\n';
    }
    stream.width(0x20);
    stream << "  Flags: ";
    dumpFlags(stream, surface_flags_50, s_flag_names_100a4a10);
    stream << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}
