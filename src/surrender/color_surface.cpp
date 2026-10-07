#include "surrender/srColorSurface.h"

#include <math.h>
#include <ostream>
#include <stdlib.h>
#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srFilter.h"
#include "surrender/srHeap.h"
#include "surrender/srPalette.h"
#include "surrender/srVectorProcessor.h"

/* srColorSurface surface-flag names; never assigned, so dump reports numeric bit indices. */
// GLOBAL: SURRENDER 0x100A4A10
static const char* s_flag_names2;

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
    srZeroMemory(&width, 0x28);
}

// FUNCTION: SURRENDER 0x1005A120
srColorSurfaceIFace::srColorSurfaceIFace(const srColorSurfaceIFace& other)
{
    *this = other;
    unknown_18_[1] = other.unknown_18_[1];
    memcpy(&width, &other.width, 0x28);
}

// FUNCTION: SURRENDER 0x1005B280
const char* srColorSurfaceIFace::sGetClassName()
{
    return "srColorSurfaceIFace";
}

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
    return pitch * height;
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
    long width = this->width;
    if ((clamp_modes & CLAMP_HORIZONTAL) != 0) {
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
    long height = this->height;
    if ((clamp_modes & CLAMP_VERTICAL) != 0) {
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
    return pixel_format.alpha_bits;
}

// FUNCTION: SURRENDER 0x10059870
double srColorSurfaceIFace::getAspectRatio() const
{
    if (height == 0) {
        return 0.0;
    }
    return width / (double)height;
}

// FUNCTION: SURRENDER 0x10059890
long srColorSurfaceIFace::getBitsPerPixel() const
{
    return (pixel_format.pixel_size + 1) * 8;
}

// FUNCTION: SURRENDER 0x100598A0
long srColorSurfaceIFace::getBlueBits() const
{
    return pixel_format.blue_bits;
}

// FUNCTION: SURRENDER 0x100598B0
long srColorSurfaceIFace::getBytesPerPixel() const
{
    return pixel_format.pixel_size + 1;
}

// FUNCTION: SURRENDER 0x100598C0
srFilter* srColorSurfaceIFace::getFilter() const
{
    return filter;
}

// FUNCTION: SURRENDER 0x100598D0
long srColorSurfaceIFace::getGreenBits() const
{
    return pixel_format.green_bits;
}

// FUNCTION: SURRENDER 0x100598E0
int srColorSurfaceIFace::getHClampMode() const
{
    return clamp_modes & CLAMP_HORIZONTAL;
}

// FUNCTION: SURRENDER 0x100599D0
long srColorSurfaceIFace::getRedBits() const
{
    return pixel_format.red_bits;
}

// FUNCTION: SURRENDER 0x10059A50
int srColorSurfaceIFace::getVClampMode() const
{
    return (clamp_modes >> 1) & 1;
}

// FUNCTION: SURRENDER 0x10059A70
int srColorSurfaceIFace::isAlpha() const
{
    return pixel_format.alpha_bits != 0;
}

// FUNCTION: SURRENDER 0x10059A80
int srColorSurfaceIFace::isPaletted() const
{
    return pixel_format.color_model == srPixelConvert::COLOR_INDEXED;
}

// FUNCTION: SURRENDER 0x10059AA0
void srColorSurfaceIFace::setHClampMode(int enabled)
{
    if (enabled != 0) {
        clamp_modes = clamp_modes | CLAMP_HORIZONTAL;
        return;
    }
    clamp_modes = clamp_modes & ~CLAMP_HORIZONTAL;
}

// FUNCTION: SURRENDER 0x1005A020
void srColorSurfaceIFace::setVClampMode(int enabled)
{
    if (enabled != 0) {
        clamp_modes = clamp_modes | CLAMP_VERTICAL;
        return;
    }
    clamp_modes = clamp_modes & ~CLAMP_VERTICAL;
}

// FUNCTION: SURRENDER 0x1005A0D0
const srPixelConvert::PixelFormat* srColorSurfaceIFace::getPixelFormat() const
{
    return &pixel_format;
}

// FUNCTION: SURRENDER 0x1005A0E0
int srColorSurfaceIFace::isPixelFormatCompatible(const srColorSurfaceIFace& source) const
{
    return pixel_format == source.pixel_format;
}

// FUNCTION: SURRENDER 0x1005ADD0
void srColorSurfaceIFace::setSurfaceDesc(const SurfaceDesc& description)
{
    width = description.width;
    height = description.height;
    pitch = description.pitch;
    clamp_modes = description.clamp_modes;
    filter = description.filter;
    pixel_format = description.pixel_format;
}

// FUNCTION: SURRENDER 0x1005B230
void srColorSurfaceIFace::copySurfaceParameters(const srColorSurfaceIFace& source)
{
    filter = source.filter;
    if ((source.clamp_modes & CLAMP_HORIZONTAL) != 0) {
        clamp_modes = clamp_modes | CLAMP_HORIZONTAL;
    } else {
        clamp_modes = clamp_modes & ~CLAMP_HORIZONTAL;
    }
    if ((source.clamp_modes & CLAMP_VERTICAL) != 0) {
        clamp_modes = clamp_modes | CLAMP_VERTICAL;
        return;
    }
    clamp_modes = clamp_modes & ~CLAMP_VERTICAL;
}

// FUNCTION: SURRENDER 0x10057810
void srColorSurfaceIFace::setLine(long x0, long y0, long x1, long y1, unsigned long pixel)
{
    long dx = x1 - x0;
    long dy = y1 - y0;
    if (dx == 0) {
        setVLine(x0, y0, y1, pixel);
        return;
    }
    if (dy == 0) {
        setHLine(y0, x0, x1, pixel);
        return;
    }
    long width = this->width;
    long height = this->height;
    if ((dy < 0 ? -dy : dy) < (dx < 0 ? -dx : dx)) {
        long last_x = x1;
        long last_y = y1;
        if (dx < 0) {
            last_x = x0;
            last_y = y0;
            y0 = y1;
            x0 = x1;
        }
        long high_y = last_y;
        long low_y = y0;
        if (last_y <= y0) {
            high_y = y0;
            low_y = last_y;
        }
        height = height - 1;
        if ((-1 < last_x) && (x0 < width) && (-1 < high_y) && (low_y <= height)) {
            float gradient = (float)dy / dx;
            long step = (long)(gradient * 65536.0);
            if (x0 < 0) {
                y0 = (long)(y0 - x0 * gradient);
                x0 = 0;
            }
            if (y0 < last_y) {
                if (y0 < 0) {
                    x0 = x0 - (long)(y0 / gradient);
                    y0 = 0;
                }
                if (height <= last_y) {
                    last_x = (long)(last_x - (last_y - (float)height) / gradient);
                }
            } else {
                if (last_y < 0) {
                    last_x = last_x - (long)(last_y / gradient);
                }
                if (height <= y0) {
                    x0 = (long)(x0 - (y0 - (float)height) / gradient);
                    y0 = height;
                }
            }
            if (width < last_x) {
                last_x = width;
            }
            long y_fixed = y0 << 0x10;
            for (; x0 < last_x; x0 = x0 + 1) {
                setPixel(x0, (y_fixed + (y_fixed >> 0x1f & 0xffff)) >> 0x10, pixel);
                y_fixed = y_fixed + step;
            }
        }
    } else {
        long last_y = x1;
        long last_x = y1;
        if (dy < 0) {
            last_y = x0;
            last_x = y0;
            y0 = y1;
            x0 = x1;
        }
        long high_x = last_y;
        long low_x = x0;
        if (last_y <= x0) {
            high_x = x0;
            low_x = last_y;
        }
        width = width - 1;
        if (((-1 < low_x) && (high_x <= width) && (-1 < last_x)) && (y0 < height)) {
            float gradient = dx / (float)dy;
            long step = (long)(gradient * 65536.0);
            if (y0 < 0) {
                x0 = (long)(x0 - y0 * gradient);
                y0 = 0;
            }
            if (x0 < last_y) {
                if (x0 < 0) {
                    y0 = y0 - (long)(x0 / gradient);
                    x0 = 0;
                }
                if (width <= last_y) {
                    last_x = (long)(last_x - (last_y - (float)width) / gradient);
                }
            } else {
                if (last_y < 0) {
                    last_x = last_x - (long)(last_y / gradient);
                }
                if (width <= x0) {
                    y0 = (long)(y0 - (x0 - (float)width) / gradient);
                    x0 = width;
                }
            }
            if (height < last_x) {
                last_x = height;
            }
            long x_fixed = x0 << 0x10;
            for (; y0 < last_x; y0 = y0 + 1) {
                setPixel((x_fixed + (x_fixed >> 0x1f & 0xffff)) >> 0x10, y0, pixel);
                x_fixed = x_fixed + step;
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005B290
srColorSurfaceIFace& srColorSurfaceIFace::operator=(const srColorSurfaceIFace& other)
{
    if (&other != this) {
        srClass::operator=(other);
        width = other.width;
        height = other.height;
        pitch = other.pitch;
        clamp_modes = other.clamp_modes;
        filter = other.filter;
        pixel_format = other.pixel_format;
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
    unsigned long pixel;
    getPixelRowRaw(&pixel, y, x, x + 1);
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8:
        return pixel & 0xff;
    case srPixelConvert::PIXEL_SIZE_16:
        return pixel & 0xffff;
    case srPixelConvert::PIXEL_SIZE_24:
        return pixel & 0xffffff;
    case srPixelConvert::PIXEL_SIZE_32:
        return pixel;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005B1B0
void srColorSurfaceIFace::setPixelRaw(long x, long y, unsigned long pixel)
{
    /* reinterpret-ok: the retail setter writes only the pixel word's low bytes
       for each raw format before passing that same word to setPixelRowRaw. */
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&pixel);
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8:
        bytes[0] = static_cast<unsigned char>(pixel);
        break;
    case srPixelConvert::PIXEL_SIZE_16:
        /* reinterpret-ok: a two-byte write into the raw pixel word. */
        *reinterpret_cast<unsigned short*>(bytes) = static_cast<unsigned short>(pixel);
        break;
    case srPixelConvert::PIXEL_SIZE_24:
        bytes[0] = static_cast<unsigned char>(pixel);
        bytes[1] = static_cast<unsigned char>(pixel >> 8);
        bytes[2] = static_cast<unsigned char>(pixel >> 16);
        break;
    case srPixelConvert::PIXEL_SIZE_32:
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
    long bytes = pixel_format.pixel_size + 1;
    for (; count > 0; --count, ++positions, out += bytes) {
        getPixelRowRaw(out, positions->y, positions->x, positions->x + 1);
    }
}

// FUNCTION: SURRENDER 0x1005B3E0
void srColorSurfaceIFace::setPixelsRaw(const void* pixels, const srVector2i* positions, long count)
{
    const unsigned char* in = (const unsigned char*)pixels;
    long bytes = pixel_format.pixel_size + 1;
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
    long width = this->width;
    long height = this->height;
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
    if (y >= 0 && y < height) {
        if (x_end < x_start) {
            long swap = x_start;
            x_start = x_end;
            x_end = swap;
        }
        if (x_start < 0) {
            x_start = 0;
        }
        if (width < x_end) {
            x_end = width;
        }
        for (; x_start < x_end; ++x_start) {
            setPixel(x_start, y, pixel);
        }
    }
}

// FUNCTION: SURRENDER 0x100577B0
void srColorSurfaceIFace::setVLine(long x, long y_start, long y_end, unsigned long pixel)
{
    if (x >= 0 && x < width) {
        if (y_end < y_start) {
            long swap = y_start;
            y_start = y_end;
            y_end = swap;
        }
        if (y_start < 0) {
            y_start = 0;
        }
        if (height < y_end) {
            y_end = height;
        }
        for (; y_start < y_end; ++y_start) {
            setPixel(x, y_start, pixel);
        }
    }
}

// FUNCTION: SURRENDER 0x10057420
void srColorSurfaceIFace::swapPixelRows(long x0, long y0, long x1, long y1, long count)
{
    long width = this->width;
    if (y0 >= 0 && y0 < height && y1 >= 0 && y1 < height && count > 0) {
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
                long bytes = (pixel_format.pixel_size + 1) * count;
                unsigned char* buffer = static_cast<unsigned char*>(::operator new(bytes * 2));
                unsigned char* second = buffer + bytes;
                getPixelRowRaw(buffer, y0, x0, x0 + count);
                getPixelRowRaw(second, y1, x1, x1 + count);
                setPixelRowRaw(buffer, y1, x1, x1 + count);
                setPixelRowRaw(second, y0, x0, x0 + count);
                ::operator delete(buffer);
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
    if ((flip_x || flip_y) && x_lo >= 0 && y_lo >= 0 && x_hi <= this->width &&
        y_hi <= this->height) {
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
        long height = this->height;
        long width = this->width;
        srARGB* row_colors = new srARGB[width];
        unsigned char* row = (unsigned char*)row_colors;
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
        delete[] row_colors;
    }
}

// FUNCTION: SURRENDER 0x10057EB0
void srColorSurfaceIFace::adjustSaturation(double saturation)
{
    if (saturation != 1.0) {
        long height = this->height;
        long width = this->width;
        srARGB* row_colors = new srARGB[width];
        unsigned char* row = (unsigned char*)row_colors;
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            unsigned char* pixel = row + 2;
            for (long x = 0; x < width; ++x) {
                float red = pixel[0] * 0.003921569f;
                float green = pixel[-1] * 0.003921569f;
                float blue = pixel[-2] * 0.003921569f;
                float alpha = pixel[1] * 0.003921569f;
                float luminance = red * 0.2125f + green * 0.7154f + blue * 0.0721f;
                double channel = alpha * 255.0;
                if (0.0 < channel) {
                    if (255.0 <= channel) {
                        channel = 255.0;
                    }
                } else {
                    channel = 0.0;
                }
                double value = ((red - luminance) * saturation + luminance) * 255.0;
                pixel[1] = (unsigned char)srFloatToInt(channel);
                if (0.0 < value) {
                    if (255.0 <= value) {
                        value = 255.0;
                    }
                } else {
                    value = 0.0;
                }
                channel = ((green - luminance) * saturation + luminance) * 255.0;
                pixel[0] = (unsigned char)srFloatToInt(value);
                if (0.0 < channel) {
                    if (255.0 <= channel) {
                        channel = 255.0;
                    }
                } else {
                    channel = 0.0;
                }
                value = ((blue - luminance) * saturation + luminance) * 255.0;
                pixel[-1] = (unsigned char)srFloatToInt(channel);
                if (0.0 < value) {
                    if (255.0 <= value) {
                        value = 255.0;
                    }
                } else {
                    value = 0.0;
                }
                pixel[-2] = (unsigned char)srFloatToInt(value);
                pixel += 4;
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        delete[] row_colors;
    }
}

// FUNCTION: SURRENDER 0x10057D10
void srColorSurfaceIFace::adjust(const srVector4T<float>& scale, const srVector4T<float>& offset,
                                 const srVector4T<float>& gamma)
{
    long height = this->height;
    long width = this->width;
    const float* scale_v = &scale.x;
    const float* offset_v = &offset.x;
    const float* gamma_v = &gamma.x;
    unsigned char lut[4][256];
    for (int channel = 0; channel < 4; ++channel) {
        for (int i = 0; i < 0x100; ++i) {
            double value = scale_v[channel] * (offset_v[channel] * (i - 128.0) + 128.0) +
                           gamma_v[channel] * 255.0 + 0.5;
            if (value <= 0.0) {
                value = 0.0;
            } else if (value >= 255.0) {
                value = 255.0;
            }
            lut[channel][i] = (unsigned char)(int)value;
        }
    }
    srARGB* row_colors = new srARGB[width];
    unsigned char* row = (unsigned char*)row_colors;
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
    delete[] row_colors;
}

// FUNCTION: SURRENDER 0x1005B040
void srColorSurfaceIFace::remapPixels(const srARGB& from, const srARGB& to)
{
    long width = this->width;
    long height = this->height;
    srARGB* row_colors = new srARGB[width];
    unsigned long* row = (unsigned long*)row_colors;
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
    delete[] row_colors;
}

// FUNCTION: SURRENDER 0x10059690
void srColorSurfaceIFace::scaleFast(srColorSurfaceIFace& source)
{
    long height = this->height;
    long source_width = source.width;
    long source_height = source.height;
    long width = this->width;
    if ((source_width == width) && (source_height == height)) {
        copyNoScaling(source);
        return;
    }
    long* column_map = new long[width];
    srARGB* source_row_colors = new srARGB[source_width];
    unsigned long* source_row = (unsigned long*)source_row_colors;
    srARGB* row_colors = new srARGB[width];
    unsigned long* row = (unsigned long*)row_colors;
    for (long x = 0; x < width; x++) {
        column_map[x] = source.getClampedX((long)((float)x * source_width / width));
    }
    long last_y = -1;
    for (long y = 0; y < height; y++) {
        long source_y = source.getClampedY((long)((float)y * source_height / height));
        if ((source_y != last_y) && (source.getPixelRow(source_row, source_y, 0, source_width),
                                     last_y = source_y, 0 < width)) {
            for (long x = 0; x < width; x++) {
                row[x] = source_row[column_map[x]];
            }
        }
        setPixelRow(row, y, 0, width);
    }
    delete[] column_map;
    delete[] source_row_colors;
    delete[] row_colors;
}

// FUNCTION: SURRENDER 0x10059420
void srColorSurfaceIFace::flipColorChannels(srARGB::e_index first, srARGB::e_index second)
{
    if ((int)first >= 0 && (int)first < 4 && (int)second >= 0 && (int)second < 4 &&
        first != second) {
        long height = this->height;
        long width = this->width;
        srARGB* row_colors = new srARGB[width];
        unsigned char* row = (unsigned char*)row_colors;
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
        delete[] row_colors;
    }
}

// FUNCTION: SURRENDER 0x10059520
void srColorSurfaceIFace::copyColorChannel(srARGB::e_index destination, srARGB::e_index source)
{
    if ((int)destination >= 0 && (int)destination < 4 && (int)source >= 0 && (int)source < 4 &&
        destination != source) {
        long height = this->height;
        long width = this->width;
        srARGB* row_colors = new srARGB[width];
        unsigned char* row = (unsigned char*)row_colors;
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            for (long x = 0; x < width; ++x) {
                unsigned char* pixel = row + x * 4;
                pixel[3 - destination] = pixel[3 - source];
            }
            setPixelRow((const unsigned long*)row, y, 0, width);
        }
        delete[] row_colors;
    }
}

// FUNCTION: SURRENDER 0x10059600
void srColorSurfaceIFace::copyNoScaling(srColorSurfaceIFace& source)
{
    if (this != &source) {
        long height = source.height;
        long width = source.width;
        srARGB* row_colors = new srARGB[width];
        unsigned long* row = (unsigned long*)row_colors;
        for (long y = 0; y < height; y++) {
            source.getPixelRow(row, y, 0, width);
            setPixelRow(row, y, 0, width);
        }
        delete[] row_colors;
    }
}

// FUNCTION: SURRENDER 0x10059240
void srColorSurfaceIFace::getChannelStatistics(srStat& statistics, srARGB::e_index channel)
{
    long height = this->height;
    long width = this->width;
    if ((int)channel >= 0 && (int)channel < 4) {
        srARGB* row_colors = new srARGB[width];
        unsigned char* row = (unsigned char*)row_colors;
        int* histogram = new int[0x100];
        int i;
        for (i = 0; i < 0x100; ++i) {
            histogram[i] = 0;
        }
        for (long y = 0; y < height; ++y) {
            getPixelRow((unsigned long*)row, y, 0, width);
            for (long x = 0; x < width; ++x) {
                ++histogram[row[x * 4 + 3 - channel]];
            }
        }
        statistics.count = 0;
        statistics.mean = 0.0;
        statistics.deviation = 0.0;
        statistics.median = 0;
        statistics.min = 0x100;
        statistics.max = 0;
        for (i = 0; i < 0x100; ++i) {
            if (histogram[i] != 0) {
                if (i < statistics.min) {
                    statistics.min = i;
                }
                if (statistics.max < i) {
                    statistics.max = i;
                }
            }
        }
        for (i = 0; i < 0x100; ++i) {
            statistics.mean = (histogram[i] * i) + statistics.mean;
            statistics.count = statistics.count + histogram[i];
        }
        statistics.mean = statistics.mean / statistics.count;
        for (i = 0; i < 0x100; ++i) {
            double difference = i - statistics.mean;
            statistics.deviation = histogram[i] * difference * difference + statistics.deviation;
        }
        statistics.deviation = sqrt(statistics.deviation / statistics.count);
        long running = 0;
        for (i = 0; i < 0x100; ++i) {
            running += histogram[i];
            statistics.median = i;
            if (statistics.count / 2 < running) {
                break;
            }
        }
        delete[] row_colors;
        delete[] histogram;
    }
}

// FUNCTION: SURRENDER 0x10059900
void srColorSurfaceIFace::copy(srColorSurfaceIFace& source)
{
    if (&source != this) {
        long width = this->width;
        long source_width = source.width;
        long height = this->height;
        long source_height = source.height;
        if (source_width == width && source_height == height) {
            copyNoScaling(source);
            return;
        }
        srFilter* filter = source.filter;
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

// FUNCTION: SURRENDER 0x1005A7A0
void srColorSurfaceIFace::scale(srColorSurfaceIFace& source)
{
    unsigned long source_height = source.height;
    unsigned long width = this->width;
    srColorSurface* scaled =
        new srColorSurface(srPixelConvert::SURFACE_BGRA32, width, source_height);
    srColorSurfaceIFace* scaled_iface = scaled;
    scaled_iface->copySurfaceParameters(source);
    scaled_iface->scaleHorizontal(source);
    scaleVertical(*scaled);
    scaled->release();
}

// FUNCTION: SURRENDER 0x1005AE10
void srColorSurfaceIFace::dump(std::ostream& stream)
{
    srClass::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    if (filter != 0) {
        stream << "  Filter: " << filter->getName() << '\n';
    } else {
        stream << "  Filter:"
               << "not defined" << '\n';
    }
    stream.width(0x20);
    stream << "  Horizontal clamp mode: " << (clamp_modes & CLAMP_HORIZONTAL) << '\n';
    stream.width(0x20);
    stream << "  Vertical clamp mode: " << (clamp_modes >> 1 & 1) << '\n';
    stream.width(0x20);
    stream << "  Dimensions: " << width << 'x' << height << 'x' << (pixel_format.pixel_size * 8 + 8)
           << '\n';
    stream.width(0x20);
    stream << "  Pitch: " << pitch << '\n';
    stream.width(0x20);
    stream << "  Dataptr: " << getDataPtr() << '\n';
    stream.width(0x20);
    stream << "  Data size: " << getDataSize() << " bytes" << '\n';
    stream.width(0x20);
    char format_name[12];
    pixel_format.getName(format_name);
    stream << "  Pixel format: " << format_name << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x1005A040
void srColorSurfaceIFace::flipVertical()
{
    Rectangle rectangle;
    rectangle.left = 0;
    rectangle.top = height;
    rectangle.right = width;
    rectangle.bottom = 0;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005A070
void srColorSurfaceIFace::flipHorizontal()
{
    Rectangle rectangle;
    rectangle.left = width;
    rectangle.top = 0;
    rectangle.right = 0;
    rectangle.bottom = height;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005A0A0
void srColorSurfaceIFace::rotate180()
{
    Rectangle rectangle;
    rectangle.left = width;
    rectangle.top = height;
    rectangle.right = 0;
    rectangle.bottom = 0;
    flipRectangle(rectangle);
}

// FUNCTION: SURRENDER 0x1005B790
srColorSurface::srColorSurface(const srPixelConvert::PixelFormat& format, unsigned long arg_width,
                               unsigned long arg_height)
{
    surface_flags = 0;
    init(format, arg_width, arg_height, (format.pixel_size + 1) * arg_width);
    allocData();
    srPixelConvert::selectFuncs(format, pixel_write, pixel_read);
}

// FUNCTION: SURRENDER 0x1005B8A0
srColorSurface::srColorSurface(srPixelConvert::e_surfaceType type, unsigned long arg_width,
                               unsigned long arg_height)
{
    srPixelConvert::PixelFormat format;
    surface_flags = 0;
    srPixelConvert::mapPixelFormat(type, format);
    init(format, arg_width, arg_height, (format.pixel_size + 1) * arg_width);
    allocData();
    srPixelConvert::selectFuncs(format, pixel_write, pixel_read);
}

/* The data-taking variants borrow caller storage: flag bit 0 marks the
   non-owning path and data_size comes straight from pitch*height. */
// FUNCTION: SURRENDER 0x1005B9C0
srColorSurface::srColorSurface(srPixelConvert::e_surfaceType type, void* data,
                               unsigned long arg_width, unsigned long arg_height,
                               unsigned long arg_pitch)
{
    srPixelConvert::PixelFormat format;
    surface_flags = 0;
    srPixelConvert::mapPixelFormat(type, format);
    init(format, arg_width, arg_height, arg_pitch);
    surface_flags |= BORROWED_DATA;
    data_size = pitch * height;
    this->data = data;
    srPixelConvert::selectFuncs(format, pixel_write, pixel_read);
}

// FUNCTION: SURRENDER 0x1005D520
srColorSurface& srColorSurface::operator=(const srColorSurface& other)
{
    if (&other != this) {
        srColorSurfaceIFace::operator=(other);
        freeData();
        srPixelConvert::PixelFormat format = other.pixel_format;
        init(format, other.width, other.height, other.pitch);
        palette = other.palette;
        surface_flags = other.surface_flags;
        pixel_write = other.pixel_write;
        pixel_read = other.pixel_read;
        if ((surface_flags & BORROWED_DATA) != 0) {
            data_size = other.data_size;
            data = other.data;
            return *this;
        }
        allocData();
        copy(const_cast<srColorSurface&>(other));
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005BAC0
srColorSurface::srColorSurface(const srPixelConvert::PixelFormat& format, void* data,
                               unsigned long arg_width, unsigned long arg_height,
                               unsigned long arg_pitch)
{
    surface_flags = 0;
    init(format, arg_width, arg_height, arg_pitch);
    surface_flags |= BORROWED_DATA;
    data_size = pitch * height;
    this->data = data;
    srPixelConvert::selectFuncs(format, pixel_write, pixel_read);
}

// FUNCTION: SURRENDER 0x1005BBE0
srColorSurface::~srColorSurface()
{
    freeData();
}

// FUNCTION: SURRENDER 0x1005B550
void* srColorSurface::getDataPtr()
{
    return data;
}

// FUNCTION: SURRENDER 0x1005B560
long srColorSurface::getDataSize()
{
    return data_size;
}

// FUNCTION: SURRENDER 0x1005B570
srPalette* srColorSurface::getPalette()
{
    return palette;
}

// FUNCTION: SURRENDER 0x1005B580
void srColorSurface::setPalette(srPalette* palette)
{
    this->palette = palette;
}

// FUNCTION: SURRENDER 0x1005B5B0
unsigned char* srColorSurface::getAddress(long x, long y)
{
    return static_cast<unsigned char*>(data) + pitch * y + (pixel_format.pixel_size + 1) * x;
}

// FUNCTION: SURRENDER 0x1005B5D0
void srColorSurface::convertToARGB8888(unsigned long* pixels, const void* source,
                                       unsigned long count)
{
    srPixelConvert::ConversionInfo info;
    info.dest = pixels;
    info.source = source;
    info.count = count;
    info.palette = palette;
    info.format = &pixel_format;
    pixel_read(info);
}

// FUNCTION: SURRENDER 0x1005B610
void srColorSurface::convertFromARGB8888(void* pixels, const unsigned long* source,
                                         unsigned long count)
{
    srPixelConvert::ConversionInfo info;
    info.dest = pixels;
    info.source = source;
    info.count = count;
    info.palette = palette;
    info.format = &pixel_format;
    pixel_write(info);
}

// FUNCTION: SURRENDER 0x1005B650
void srColorSurface::allocData()
{
    data_size = pitch * height;
    if (data_size != 0) {
        data = srHeap.allocate(data_size);
    }
}

// FUNCTION: SURRENDER 0x1005B680
void srColorSurface::freeData()
{
    if (!(surface_flags & BORROWED_DATA) && data != 0) {
        srHeap.free(data);
        data = 0;
    }
}

// FUNCTION: SURRENDER 0x1005B6B0
void srColorSurface::init(const srPixelConvert::PixelFormat& format, unsigned long arg_width,
                          unsigned long arg_height, unsigned long arg_pitch)
{
    SurfaceDesc desc;
    srZeroMemory(&desc, sizeof(desc));
    desc.width = arg_width;
    desc.height = arg_height;
    desc.pitch = arg_pitch;
    desc.pixel_format = format;
    setSurfaceDesc(desc);
    palette = srCore.getPalette();
    pixel_write = 0;
    pixel_read = 0;
    data_size = 0;
    data = 0;
    surface_flags = 0;
}

// FUNCTION: SURRENDER 0x1005DD70
srPixelConvert::ConversionFunc srColorSurface::getPixelWriteFunc() const
{
    return pixel_write;
}

// FUNCTION: SURRENDER 0x1005DD80
srPixelConvert::ConversionFunc srColorSurface::getPixelReadFunc() const
{
    return pixel_read;
}

// FUNCTION: SURRENDER 0x1005DD90
void srColorSurface::setPixelWriteFunc(srPixelConvert::ConversionFunc function)
{
    if (function != 0) {
        pixel_write = function;
    }
}

// FUNCTION: SURRENDER 0x1005DDA0
void srColorSurface::setPixelReadFunc(srPixelConvert::ConversionFunc function)
{
    if (function != 0) {
        pixel_read = function;
    }
}

// FUNCTION: SURRENDER 0x1005DDB0
srClass* srColorSurface::vInstance()
{
    return new srColorSurface(pixel_format, 1, 1);
}

// FUNCTION: SURRENDER 0x1005DE20
const char* srColorSurface::sGetClassName()
{
    return "srColorSurface";
}

// FUNCTION: SURRENDER 0x1005BEC0
int srColorSurface::resize(long arg_width, long arg_height)
{
    if (arg_width > 0 && arg_height > 0) {
        if (arg_width == width && arg_height == height) {
            return 1;
        }
        if (!(surface_flags & BORROWED_DATA)) {
            freeData();
            SurfaceDesc desc;
            desc.width = arg_width;
            desc.height = arg_height;
            desc.pitch = (pixel_format.pixel_size + 1) * arg_width;
            desc.clamp_modes = clamp_modes;
            desc.filter = filter;
            desc.pixel_format = pixel_format;
            setSurfaceDesc(desc);
            allocData();
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1005BD10
int srColorSurface::rescale(long arg_width, long arg_height)
{
    if (arg_width > 0 && arg_height > 0) {
        if (arg_width == width && arg_height == height) {
            return 1;
        }
        if (!(surface_flags & BORROWED_DATA)) {
            srColorSurface* scaled =
                new srColorSurface(srPixelConvert::SURFACE_BGRA32, arg_width, arg_height);
            scaled->copySurfaceParameters(*this);
            scaled->copy(*this);
            resize(arg_width, arg_height);
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
    if (surface_flags & BORROWED_DATA) {
        return 0;
    }
    if (!(format == pixel_format)) {
        srColorSurfaceIFace* previous = 0;
        if (preserve != 0) {
            previous = static_cast<srColorSurfaceIFace*>(vClone());
        }
        srPalette* palette = getPalette();
        unsigned long flags = surface_flags;
        freeData();
        init(format, width, height, (format.pixel_size + 1) * width);
        allocData();
        srPixelConvert::selectFuncs(format, pixel_write, pixel_read);
        setPalette(palette);
        surface_flags = flags;
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
    if (pixel_format == source.pixel_format) {
        if (pixel_format.color_model == srPixelConvert::COLOR_INDEXED &&
            source.getPalette() != getPalette()) {
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
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8:
        return *address;
    case srPixelConvert::PIXEL_SIZE_16:
        return *(unsigned short*)address;
    case srPixelConvert::PIXEL_SIZE_24:
        return address[0] | (address[1] << 8) | (address[2] << 0x10);
    case srPixelConvert::PIXEL_SIZE_32:
        return *(unsigned long*)address;
    default:
        return 0;
    }
}

// FUNCTION: SURRENDER 0x1005C030
void srColorSurface::setPixelRaw(long x, long y, unsigned long pixel)
{
    unsigned char* address = getAddress(x, y);
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8:
        *address = (unsigned char)pixel;
        break;
        break;
    case srPixelConvert::PIXEL_SIZE_16:
        *(unsigned short*)address = (unsigned short)pixel;
        break;
        break;
    case srPixelConvert::PIXEL_SIZE_24:
        address[0] = (unsigned char)pixel;
        address[1] = (unsigned char)(pixel >> 8);
        address[2] = (unsigned char)(pixel >> 0x10);
        break;
        break;
    case srPixelConvert::PIXEL_SIZE_32:
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
    if (y >= 0 && y < height && x_start >= 0 && x_start < x_end && x_end <= width) {
        long count = (x_end - x_start) * (pixel_format.pixel_size + 1);
        unsigned char* address = getAddress(x_start, y);
        if (count != 0 && pixels != address) {
            srVectorProcessor::memcopy(pixels, address, count);
        }
    }
}

// FUNCTION: SURRENDER 0x1005BFD0
void srColorSurface::setPixelRowRaw(const void* pixels, long y, long x_start, long x_end)
{
    if (y >= 0 && y < height && x_start >= 0 && x_start < x_end && x_end <= width) {
        long count = (x_end - x_start) * (pixel_format.pixel_size + 1);
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
        switch (pixel_format.pixel_size) {
        case srPixelConvert::PIXEL_SIZE_8: {
            for (unsigned long i = 0; i < count; ++i) {
                buffer[i] = *address;
                address += pitch;
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_16: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    *(unsigned short*)out = *(unsigned short*)address;
                    out += 2;
                    address += pitch;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_24: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    out[0] = address[0];
                    out[1] = address[1];
                    out[2] = address[2];
                    out += 3;
                    address += pitch;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_32: {
            if (count != 0) {
                unsigned char* out = buffer;
                unsigned long i = count;
                do {
                    *(unsigned long*)out = *(unsigned long*)address;
                    out += 4;
                    address += pitch;
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
        switch (pixel_format.pixel_size) {
        case srPixelConvert::PIXEL_SIZE_8: {
            for (unsigned long i = 0; i < count; ++i) {
                *address = buffer[i];
                address += pitch;
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_16: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    *(unsigned short*)address = *(const unsigned short*)in;
                    in += 2;
                    address += pitch;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_24: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    address[0] = in[0];
                    address[1] = in[1];
                    address[2] = in[2];
                    in += 3;
                    address += pitch;
                    --i;
                } while (i != 0);
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_32: {
            if (count != 0) {
                const unsigned char* in = buffer;
                unsigned long i = count;
                do {
                    *(unsigned long*)address = *(const unsigned long*)in;
                    in += 4;
                    address += pitch;
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
    for (long i = 0; i < count; i += 0x100) {
        unsigned long chunk = count - i;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        getPixelsRaw(buffer, positions, chunk);
        convertToARGB8888(pixels, buffer, chunk);
        positions += 0x100;
        pixels += 0x100;
    }
}

// FUNCTION: SURRENDER 0x1005D5F0
void srColorSurface::setPixels(const unsigned long* pixels, const srVector2i* positions, long count)
{
    unsigned char buffer[0x400];
    for (long i = 0; i < count; i += 0x100) {
        unsigned long chunk = count - i;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        convertFromARGB8888(buffer, pixels, chunk);
        setPixelsRaw(buffer, positions, chunk);
        positions += 0x100;
        pixels += 0x100;
    }
}

// FUNCTION: SURRENDER 0x1005D830
void srColorSurface::getPixelsRaw(void* pixels, const srVector2i* positions, long count)
{
    unsigned char* out = (unsigned char*)pixels;
    unsigned long pixel_count = static_cast<unsigned long>(count);
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, ++out) {
            *out = *getAddress(positions->x, positions->y);
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_16: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, out += 2) {
            *(unsigned short*)out = *(unsigned short*)getAddress(positions->x, positions->y);
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_24: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, out += 3) {
            const unsigned char* address = getAddress(positions->x, positions->y);
            out[0] = address[0];
            out[1] = address[1];
            out[2] = address[2];
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_32: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, out += 4) {
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
    unsigned long pixel_count = static_cast<unsigned long>(count);
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, ++in) {
            *getAddress(positions->x, positions->y) = *in;
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_16: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, in += 2) {
            *(unsigned short*)getAddress(positions->x, positions->y) = *(const unsigned short*)in;
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_24: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, in += 3) {
            unsigned char* address = getAddress(positions->x, positions->y);
            address[0] = in[0];
            address[1] = in[1];
            address[2] = in[2];
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_32: {
        for (unsigned long i = 0; i < pixel_count; ++i, ++positions, in += 4) {
            *(unsigned long*)getAddress(positions->x, positions->y) = *(const unsigned long*)in;
        }
        break;
    }
    }
}

// FUNCTION: SURRENDER 0x1005E230
static void reversePixelTriplets(unsigned char* pixels, unsigned long count)
{
    unsigned long half = count >> 1;
    unsigned long i = 0;
    for (; i < (half & ~3UL); i += 4) {
        /* reinterpret-ok: each 24-bit pixel record swaps its low word plus
           high byte separately. */
        unsigned short w = *reinterpret_cast<unsigned short*>(pixels + i * 3);
        unsigned char b = pixels[i * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + i * 3) =
            *reinterpret_cast<unsigned short*>(pixels + (count - 1 - i) * 3);
        pixels[i * 3 + 2] = pixels[(count - 1 - i) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (count - 1 - i) * 3) = w;
        pixels[(count - 1 - i) * 3 + 2] = b;
        w = *reinterpret_cast<unsigned short*>(pixels + (i + 1) * 3);
        b = pixels[(i + 1) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (i + 1) * 3) =
            *reinterpret_cast<unsigned short*>(pixels + (count - 2 - i) * 3);
        pixels[(i + 1) * 3 + 2] = pixels[(count - 2 - i) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (count - 2 - i) * 3) = w;
        pixels[(count - 2 - i) * 3 + 2] = b;
        w = *reinterpret_cast<unsigned short*>(pixels + (i + 2) * 3);
        b = pixels[(i + 2) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (i + 2) * 3) =
            *reinterpret_cast<unsigned short*>(pixels + (count - 3 - i) * 3);
        pixels[(i + 2) * 3 + 2] = pixels[(count - 3 - i) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (count - 3 - i) * 3) = w;
        pixels[(count - 3 - i) * 3 + 2] = b;
        w = *reinterpret_cast<unsigned short*>(pixels + (i + 3) * 3);
        b = pixels[(i + 3) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (i + 3) * 3) =
            *reinterpret_cast<unsigned short*>(pixels + (count - 4 - i) * 3);
        pixels[(i + 3) * 3 + 2] = pixels[(count - 4 - i) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (count - 4 - i) * 3) = w;
        pixels[(count - 4 - i) * 3 + 2] = b;
    }
    for (; i < half; ++i) {
        unsigned short w = *reinterpret_cast<unsigned short*>(pixels + i * 3);
        unsigned char b = pixels[i * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + i * 3) =
            *reinterpret_cast<unsigned short*>(pixels + (count - 1 - i) * 3);
        pixels[i * 3 + 2] = pixels[(count - 1 - i) * 3 + 2];
        *reinterpret_cast<unsigned short*>(pixels + (count - 1 - i) * 3) = w;
        pixels[(count - 1 - i) * 3 + 2] = b;
    }
}

// FUNCTION: SURRENDER 0x1005C150
void srColorSurface::reversePixels(void* pixels, unsigned long count)
{
    unsigned char* address = (unsigned char*)pixels;
    switch (pixel_format.pixel_size) {
    case srPixelConvert::PIXEL_SIZE_8: {
        unsigned long half = count >> 1;
        unsigned long i = 0;
        for (; i < (half & ~3UL); i += 4) {
            unsigned char t = address[i];
            address[i] = address[count - 1 - i];
            address[count - 1 - i] = t;
            t = address[i + 1];
            address[i + 1] = address[count - 2 - i];
            address[count - 2 - i] = t;
            t = address[i + 2];
            address[i + 2] = address[count - 3 - i];
            address[count - 3 - i] = t;
            t = address[i + 3];
            address[i + 3] = address[count - 4 - i];
            address[count - 4 - i] = t;
        }
        for (; i < half; ++i) {
            unsigned char t = address[i];
            address[i] = address[count - 1 - i];
            address[count - 1 - i] = t;
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_16: {
        unsigned long half = count >> 1;
        unsigned short* row = (unsigned short*)address;
        unsigned long i = 0;
        for (; i < (half & ~3UL); i += 4) {
            unsigned short t = row[i];
            row[i] = row[count - 1 - i];
            row[count - 1 - i] = t;
            t = row[i + 1];
            row[i + 1] = row[count - 2 - i];
            row[count - 2 - i] = t;
            t = row[i + 2];
            row[i + 2] = row[count - 3 - i];
            row[count - 3 - i] = t;
            t = row[i + 3];
            row[i + 3] = row[count - 4 - i];
            row[count - 4 - i] = t;
        }
        for (; i < half; ++i) {
            unsigned short t = row[i];
            row[i] = row[count - 1 - i];
            row[count - 1 - i] = t;
        }
        break;
    }
    case srPixelConvert::PIXEL_SIZE_24:
        reversePixelTriplets(address, count);
        break;
    case srPixelConvert::PIXEL_SIZE_32:
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
    if (y0 >= 0 && y0 < height && y1 >= 0 && y1 < height && count > 0) {
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
        if (x0 < width && x1 < width) {
            if (x0 + count > width) {
                count = width - x0;
            }
            if (x1 + count > width) {
                count = width - x1;
            }
            if (count > 0) {
                srVectorProcessor::swap(getAddress(x0, y0), getAddress(x1, y1),
                                        (pixel_format.pixel_size + 1) * count);
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
    if ((flip_x || flip_y) && x_lo >= 0 && y_lo >= 0 && x_hi <= this->width &&
        y_hi <= this->height) {
        unsigned long width = x_hi - x_lo;
        unsigned long height = y_hi - y_lo;
        if (width != 0 && height != 0) {
            int bpp = pixel_format.pixel_size;
            long middle = y_lo + (long)height / 2;
            unsigned char* base = (unsigned char*)data + (bpp + 1) * x_lo;
            unsigned long mirror = height;
            for (long row = y_lo; row < middle; ++row) {
                --mirror;
                void* row_address = (void*)(pitch * row + base);
                void* mirror_address = (void*)(pitch * mirror + base);
                if (flip_y) {
                    srVectorProcessor::swap(row_address, mirror_address, (bpp + 1) * width);
                }
                if (flip_x) {
                    reversePixels(row_address, width);
                    reversePixels(mirror_address, width);
                }
            }
            if (flip_x && (height & 1) != 0) {
                reversePixels((void*)(pitch * middle + base), width);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005C560
void srColorSurface::setHLine(long y, long x_start, long x_end, unsigned long pixel)
{
    if (data == 0) {
        srColorSurfaceIFace::setHLine(y, x_start, x_end, pixel);
        return;
    }
    if (y >= 0 && y < height) {
        long x_hi = x_end;
        if (x_end < x_start) {
            x_hi = x_start;
            x_start = x_end;
        }
        if (x_start < 0) {
            x_start = 0;
        }
        if (x_hi > width) {
            x_hi = width;
        }
        if (x_start < x_hi) {
            setPixel(x_start, y, pixel);
            unsigned long raw = getPixelRaw(x_start, y);
            unsigned char* address = getAddress(x_start, y);
            switch (pixel_format.pixel_size) {
            case srPixelConvert::PIXEL_SIZE_8:
                if (x_hi - x_start != 0) {
                    srVectorProcessor::memcopy(address, (SRBYTE)raw, x_hi - x_start);
                }
                break;
                break;
            case srPixelConvert::PIXEL_SIZE_16: {
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
            case srPixelConvert::PIXEL_SIZE_24: {
                unsigned long count = x_hi - x_start;
                unsigned long i = 0;
                unsigned long bulk = count & ~3UL;
                for (; i < bulk; i += 4) {
                    unsigned char* out = address + i * 3;
                    *(unsigned short*)out = (unsigned short)raw;
                    out[2] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 3) = (unsigned short)raw;
                    out[5] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 6) = (unsigned short)raw;
                    out[8] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 9) = (unsigned short)raw;
                    out[11] = (unsigned char)(raw >> 0x10);
                }
                if (i < count) {
                    unsigned char* out = address + i * 3;
                    *(unsigned short*)out = (unsigned short)raw;
                    out[2] = (unsigned char)(raw >> 0x10);
                    unsigned char* dst = out + 3;
                    for (unsigned long n = (count - i) * 3 - 3; n != 0; --n) {
                        *dst++ = *out++;
                    }
                }
                break;
            }
            case srPixelConvert::PIXEL_SIZE_32:
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
    if (data == 0) {
        srColorSurfaceIFace::setVLine(x, y_start, y_end, pixel);
        return;
    }
    if (x >= 0 && x < width) {
        long y_hi = y_end;
        if (y_end < y_start) {
            y_hi = y_start;
            y_start = y_end;
        }
        if (y_start < 0) {
            y_start = 0;
        }
        if (y_hi > height) {
            y_hi = height;
        }
        if (y_start < y_hi) {
            setPixel(x, y_start, pixel);
            unsigned long raw = getPixelRaw(x, y_start);
            unsigned char* address = getAddress(x, y_start);
            unsigned long count = y_hi - y_start;
            switch (pixel_format.pixel_size) {
            case srPixelConvert::PIXEL_SIZE_8: {
                unsigned long i = 0;
                unsigned long bulk = count & ~1UL;
                for (; i < bulk; i += 2) {
                    *address = (unsigned char)raw;
                    address[pitch] = (unsigned char)raw;
                    address += pitch * 2;
                }
                if (i < count) {
                    *address = (unsigned char)raw;
                }
                break;
            }
            case srPixelConvert::PIXEL_SIZE_16: {
                unsigned long i = 0;
                unsigned long bulk = count & ~1UL;
                for (; i < bulk; i += 2) {
                    *(unsigned short*)address = (unsigned short)raw;
                    *(unsigned short*)(address + pitch) = (unsigned short)raw;
                    address += pitch * 2;
                }
                if (i < count) {
                    *(unsigned short*)address = (unsigned short)raw;
                }
                break;
            }
            case srPixelConvert::PIXEL_SIZE_24: {
                unsigned long i = 0;
                unsigned long bulk = count & ~1UL;
                for (; i < bulk; i += 2) {
                    *(unsigned short*)address = (unsigned short)raw;
                    address[2] = (unsigned char)(raw >> 0x10);
                    address += pitch;
                    *(unsigned short*)address = (unsigned short)raw;
                    address[2] = (unsigned char)(raw >> 0x10);
                    address += pitch;
                }
                if (i < count) {
                    *(unsigned short*)address = (unsigned short)raw;
                    address[2] = (unsigned char)(raw >> 0x10);
                }
                break;
            }
            case srPixelConvert::PIXEL_SIZE_32: {
                unsigned long i = 0;
                unsigned long bulk = count & ~1UL;
                for (; i < bulk; i += 2) {
                    *(unsigned long*)address = raw;
                    *(unsigned long*)(address + pitch) = raw;
                    address += pitch * 2;
                }
                if (i < count) {
                    *(unsigned long*)address = raw;
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
    long pitch = this->pitch;
    unsigned long width = this->width;
    long height = this->height;
    unsigned long rows = static_cast<unsigned long>(height);
    int bpp = pixel_format.pixel_size;
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
            unsigned long i = 0;
            unsigned long bulk = count & ~3UL;
            for (; i < bulk; i += 4) {
                unsigned char* out = data + i * 3;
                *(unsigned short*)out = (unsigned short)raw;
                out[2] = (unsigned char)(raw >> 0x10);
                *(unsigned short*)(out + 3) = (unsigned short)raw;
                out[5] = (unsigned char)(raw >> 0x10);
                *(unsigned short*)(out + 6) = (unsigned short)raw;
                out[8] = (unsigned char)(raw >> 0x10);
                *(unsigned short*)(out + 9) = (unsigned short)raw;
                out[11] = (unsigned char)(raw >> 0x10);
            }
            if (i < count) {
                unsigned char* out = data + i * 3;
                *(unsigned short*)out = (unsigned short)raw;
                out[2] = (unsigned char)(raw >> 0x10);
                unsigned char* dst = out + 3;
                for (unsigned long n = (count - i) * 3 - 3; n != 0; --n) {
                    *dst++ = *out++;
                }
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
            for (unsigned long row = 0; row < rows; ++row) {
                if (width != 0) {
                    srVectorProcessor::memcopy(data, (SRBYTE)raw, width);
                }
                data += pitch;
            }
            break;
        }
        case 1: {
            for (unsigned long row = 0; row < rows; ++row) {
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
            for (unsigned long row = 0; row < rows; ++row) {
                unsigned long i = 0;
                unsigned long bulk = width & ~3UL;
                for (; i < bulk; i += 4) {
                    unsigned char* out = data + i * 3;
                    *(unsigned short*)out = (unsigned short)raw;
                    out[2] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 3) = (unsigned short)raw;
                    out[5] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 6) = (unsigned short)raw;
                    out[8] = (unsigned char)(raw >> 0x10);
                    *(unsigned short*)(out + 9) = (unsigned short)raw;
                    out[11] = (unsigned char)(raw >> 0x10);
                }
                if (i < width) {
                    unsigned char* out = data + i * 3;
                    *(unsigned short*)out = (unsigned short)raw;
                    out[2] = (unsigned char)(raw >> 0x10);
                    unsigned char* dst = out + 3;
                    for (unsigned long n = (width - i) * 3 - 3; n != 0; --n) {
                        *dst++ = *out++;
                    }
                }
                data += pitch;
            }
            break;
        }
        case 3: {
            for (unsigned long row = 0; row < rows; ++row) {
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
    if (x < width && y < height) {
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
        if (x_end > source.width) {
            x_end = source.width;
        }
        if (y_end > source.height) {
            y_end = source.height;
        }
        if (source_x < source.width && source_x < x_end && source_y < source.height &&
            source_y < y_end) {
            if (width < (x - source_x) + x_end) {
                x_end = width - x + source_x;
            }
            if (height < (y - source_y) + y_end) {
                y_end = height - y + source_y;
            }
            if (source_x < x_end && source_y < y_end) {
                long dest_pitch = pitch;
                long source_pitch = source.pitch;
                int bpp = pixel_format.pixel_size + 1;
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
        if (pixel_format.color_model == srPixelConvert::COLOR_RGB &&
            pixel_format.pixel_size == srPixelConvert::PIXEL_SIZE_32 &&
            pixel_format.red_bits == 8 && pixel_format.green_bits == 8 &&
            pixel_format.blue_bits == 8 && pixel_format.alpha_bits == 8 &&
            pixel_format.red_shift == 0x10 && pixel_format.green_shift == 8 &&
            pixel_format.blue_shift == 0 && pixel_format.alpha_shift == 0x18) {
            unsigned char* dest = (unsigned char*)getDataPtr();
            for (long row = 0; row < height; ++row) {
                source.getPixelRow((unsigned long*)dest, row, 0, width);
                dest += pitch;
            }
            return;
        }
        const srPixelConvert::PixelFormat& source_format = source.pixel_format;
        if (source_format.color_model != 0 || source_format.pixel_size != 3 ||
            source_format.red_bits != 8 || source_format.green_bits != 8 ||
            source_format.blue_bits != 8 || source_format.alpha_bits != 8 ||
            source_format.red_shift != 0x10 || source_format.green_shift != 8 ||
            source_format.blue_shift != 0 || source_format.alpha_shift != 0x18 ||
            source.getDataPtr() == 0) {
            srColorSurfaceIFace::copyNoScaling(source);
            return;
        }
        unsigned char* src = (unsigned char*)source.getDataPtr();
        for (long row = 0; row < height; ++row) {
            setPixelRow((const unsigned long*)src, row, 0, width);
            src += source.pitch;
        }
        return;
    }
    unsigned char* dest = (unsigned char*)getDataPtr();
    unsigned char* src = (unsigned char*)source.getDataPtr();
    long dest_pitch = pitch;
    long source_pitch = source.pitch;
    if (dest_pitch == source_pitch && dest_pitch == (pixel_format.pixel_size + 1) * width) {
        long size = getDataSize();
        if (size != 0 && dest != src) {
            srVectorProcessor::memcopy(dest, src, size);
        }
    } else {
        long row_bytes = (pixel_format.pixel_size + 1) * width;
        for (long row = height; row != 0; --row) {
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
    long dest_width = width;
    long dest_height = height;
    if (source.width == dest_width && source.height == dest_height) {
        copyNoScaling(source);
        return;
    }
    long* columns = new long[dest_width];
    unsigned char* dest = (unsigned char*)getDataPtr();
    unsigned char* src = (unsigned char*)source.getDataPtr();
    long source_pitch = source.pitch;
    srPixelConvert::e_pixelSize bpp = pixel_format.pixel_size;
    long dest_pitch = pitch;
    double x_ratio = source.width / (double)dest_width;
    double y_ratio = source.height / (double)dest_height;
    for (long i = 0; i < dest_width; ++i) {
        columns[i] = source.getClampedX((long)(i * x_ratio));
    }
    for (long row = 0; row < dest_height; ++row) {
        long source_y = source.getClampedY((long)(row * y_ratio));
        const unsigned char* source_row = src + source_y * source_pitch;
        switch (bpp) {
        case srPixelConvert::PIXEL_SIZE_8: {
            long x = 0;
            for (; x < (dest_width & ~3L); x += 4) {
                dest[x] = source_row[columns[x]];
                dest[x + 1] = source_row[columns[x + 1]];
                dest[x + 2] = source_row[columns[x + 2]];
                dest[x + 3] = source_row[columns[x + 3]];
            }
            for (; x < dest_width; ++x) {
                dest[x] = source_row[columns[x]];
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_16: {
            unsigned short* out = (unsigned short*)dest;
            const unsigned short* in = (const unsigned short*)source_row;
            long x = 0;
            for (; x < (dest_width & ~3L); x += 4) {
                out[x] = in[columns[x]];
                out[x + 1] = in[columns[x + 1]];
                out[x + 2] = in[columns[x + 2]];
                out[x + 3] = in[columns[x + 3]];
            }
            for (; x < dest_width; ++x) {
                out[x] = in[columns[x]];
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_24: {
            long x = 0;
            for (; x < (dest_width & ~3L); x += 4) {
                const unsigned char* pixel = source_row + columns[x] * 3;
                /* reinterpret-ok: the 24-bit pixel record copies its low word
                   plus high byte separately. */
                *reinterpret_cast<unsigned short*>(dest + x * 3) =
                    *reinterpret_cast<const unsigned short*>(pixel);
                dest[x * 3 + 2] = pixel[2];
                pixel = source_row + columns[x + 1] * 3;
                *reinterpret_cast<unsigned short*>(dest + x * 3 + 3) =
                    *reinterpret_cast<const unsigned short*>(pixel);
                dest[x * 3 + 5] = pixel[2];
                pixel = source_row + columns[x + 2] * 3;
                *reinterpret_cast<unsigned short*>(dest + x * 3 + 6) =
                    *reinterpret_cast<const unsigned short*>(pixel);
                dest[x * 3 + 8] = pixel[2];
                pixel = source_row + columns[x + 3] * 3;
                *reinterpret_cast<unsigned short*>(dest + x * 3 + 9) =
                    *reinterpret_cast<const unsigned short*>(pixel);
                dest[x * 3 + 11] = pixel[2];
            }
            for (; x < dest_width; ++x) {
                const unsigned char* pixel = source_row + columns[x] * 3;
                *reinterpret_cast<unsigned short*>(dest + x * 3) =
                    *reinterpret_cast<const unsigned short*>(pixel);
                dest[x * 3 + 2] = pixel[2];
            }
            break;
        }
        case srPixelConvert::PIXEL_SIZE_32:
            srVectorProcessor::copyIndexed((SRDWORD*)dest, (const SRDWORD*)source_row,
                                           (const SRDWORD*)columns, dest_width);
            break;
        }
        dest += dest_pitch;
    }
    delete[] columns;
}

/* The horizontal and vertical filters use the same two records: a source index plus a float weight,
   and a count plus a pointer to those records. The names are descriptive. A zero total weight is
   not guarded. */
struct SampleWeight {
    long index;
    float weight;
};

struct SampleContributions {
    long count;
    SampleWeight* samples;

    void append(long index, double weight)
    {
        SampleWeight& sample = samples[count++];
        sample.index = index;
        sample.weight = static_cast<float>(weight);
    }

    void normalize(double total)
    {
        float scale = static_cast<float>(1.0 / total);
        for (long index = 0; index < count; ++index) {
            samples[index].weight = scale * samples[index].weight;
        }
    }
};

static_assert(sizeof(SampleWeight) == 8, "SampleWeight_must_be_8");
static_assert(sizeof(SampleContributions) == 8, "SampleContributions_must_be_8");

// FUNCTION: SURRENDER 0x10059AC0
void srColorSurfaceIFace::scaleHorizontal(srColorSurfaceIFace& source)
{
    long height = source.height;
    long width = this->width;
    long source_width = source.width;
    if (width != source_width) {
        double support = source.filter->getSupport();
        double scale = (double)width / source_width;
        SampleContributions* counts = new SampleContributions[width];
        srARGB* source_row_colors = new srARGB[source_width];
        unsigned long* source_row = (unsigned long*)source_row_colors;
        srARGB* row_colors = new srARGB[width];
        unsigned long* row = (unsigned long*)row_colors;
        srVector4T<float>* channel_vectors = new srVector4T<float>[source_width];
        SampleWeight* storage;
        if (1.0 <= scale) {
            long entries = 1 - (long)(support * -2.0);
            storage = new SampleWeight[entries * width];
            for (long x = 0; x < width; x++) {
                SampleContributions* entry = counts + x;
                entry->count = 0;
                entry->samples = storage + x * entries;
                double center = x / scale - 0.5;
                double total = 0.0;
                long first = (long)ceil(center - support);
                long last = (long)floor(center + support);
                for (; first <= last; first++) {
                    double weight = source.filter->getWeight(center - first);
                    if (0.0 < weight) {
                        long index = source.getClampedX(first);
                        entry->append(index, weight);
                        total = weight + total;
                    }
                }
                entry->normalize(total);
            }
        } else {
            double scaled_support = support / scale;
            double inverse = 1.0 / scale;
            long entries = 1 - (long)(scaled_support * -2.0);
            storage = new SampleWeight[entries * width];
            for (long x = 0; x < width; x++) {
                SampleContributions* entry = counts + x;
                entry->count = 0;
                entry->samples = storage + x * entries;
                double center = x / scale + 0.5;
                double total = 0.0;
                long first = (long)ceil(center - scaled_support);
                long last = (long)floor(center + scaled_support);
                for (; first <= last; first++) {
                    double weight = source.filter->getWeight((center - first) / inverse) / inverse;
                    if (0.0 < weight) {
                        long index = source.getClampedX(first);
                        entry->append(index, weight);
                        total = weight + total;
                    }
                }
                entry->normalize(total);
            }
        }
        for (long y = 0; y < height; y++) {
            source.getPixelRow(source_row, y, 0, source_width);
            long x;
            for (x = 0; x < source_width; x++) {
                channel_vectors[x].x = static_cast<float>(source_row_colors[x].alpha);
                channel_vectors[x].y = static_cast<float>(source_row_colors[x].red);
                channel_vectors[x].z = static_cast<float>(source_row_colors[x].green);
                channel_vectors[x].w = static_cast<float>(source_row_colors[x].blue);
            }
            for (x = 0; x < width; x++) {
                SampleContributions* entry = counts + x;
                long count = entry->count;
                SampleWeight* slot = entry->samples;
                float a = 0.0f;
                float r = 0.0f;
                float g = 0.0f;
                float b = 0.0f;
                for (; 0 < count; count--) {
                    float weight = slot->weight;
                    const srVector4T<float>& source_pixel = channel_vectors[slot->index];
                    ++slot;
                    a = source_pixel.x * weight + a;
                    r = source_pixel.y * weight + r;
                    g = source_pixel.z * weight + g;
                    b = source_pixel.w * weight + b;
                }
                row_colors[x].alpha = static_cast<unsigned char>(srFloatToInt(a));
                row_colors[x].red = static_cast<unsigned char>(srFloatToInt(r));
                row_colors[x].green = static_cast<unsigned char>(srFloatToInt(g));
                row_colors[x].blue = static_cast<unsigned char>(srFloatToInt(b));
            }
            setPixelRow(row, y, 0, width);
        }
        delete[] channel_vectors;
        delete[] source_row_colors;
        delete[] row_colors;
        delete[] counts;
        delete[] storage;
        return;
    }
    copyNoScaling(source);
}

// FUNCTION: SURRENDER 0x1005A250
void srColorSurfaceIFace::scaleVertical(srColorSurfaceIFace& source)
{
    long width = this->width;
    long height = this->height;
    long source_height = source.height;
    if (height != source_height) {
        double support = source.filter->getSupport();
        double scale = height / (double)source_height;
        SampleContributions* counts = new SampleContributions[height];
        srARGB* source_column_colors = new srARGB[source_height];
        unsigned long* source_column = (unsigned long*)source_column_colors;
        srARGB* column_colors = new srARGB[height];
        unsigned long* column = (unsigned long*)column_colors;
        srVector4T<float>* channel_vectors = new srVector4T<float>[source_height];
        SampleWeight* storage;
        if (1.0 <= scale) {
            long entries = 1 - (long)(support * -2.0);
            storage = new SampleWeight[entries * height];
            for (long y = 0; y < height; y++) {
                SampleContributions* entry = counts + y;
                entry->count = 0;
                entry->samples = storage + y * entries;
                double center = y / scale - 0.5;
                double total = 0.0;
                long first = (long)ceil(center - support);
                long last = (long)floor(center + support);
                for (; first <= last; first++) {
                    double weight = source.filter->getWeight(center - first);
                    if (0.0 < weight) {
                        long index = source.getClampedY(first);
                        entry->append(index, weight);
                        total = weight + total;
                    }
                }
                entry->normalize(total);
            }
        } else {
            double scaled_support = support / scale;
            double inverse = 1.0 / scale;
            long entries = 1 - (long)(scaled_support * -2.0);
            storage = new SampleWeight[entries * height];
            for (long y = 0; y < height; y++) {
                SampleContributions* entry = counts + y;
                entry->count = 0;
                entry->samples = storage + y * entries;
                double center = y / scale + 0.5;
                double total = 0.0;
                long first = (long)ceil(center - scaled_support);
                long last = (long)floor(center + scaled_support);
                for (; first <= last; first++) {
                    double weight = source.filter->getWeight((center - first) / inverse) / inverse;
                    if (0.0 < weight) {
                        long index = source.getClampedY(first);
                        entry->append(index, weight);
                        total = weight + total;
                    }
                }
                entry->normalize(total);
            }
        }
        for (long x = 0; x < width; x++) {
            source.getPixelColumn(source_column, x, 0, source_height);
            long y;
            for (y = 0; y < source_height; y++) {
                channel_vectors[y].x = static_cast<float>(source_column_colors[y].alpha);
                channel_vectors[y].y = static_cast<float>(source_column_colors[y].red);
                channel_vectors[y].z = static_cast<float>(source_column_colors[y].green);
                channel_vectors[y].w = static_cast<float>(source_column_colors[y].blue);
            }
            for (y = 0; y < height; y++) {
                SampleContributions* entry = counts + y;
                long count = entry->count;
                SampleWeight* slot = entry->samples;
                float a = 0.0f;
                float r = 0.0f;
                float g = 0.0f;
                float b = 0.0f;
                for (; 0 < count; count--) {
                    float weight = slot->weight;
                    const srVector4T<float>& source_pixel = channel_vectors[slot->index];
                    ++slot;
                    a = source_pixel.x * weight + a;
                    r = source_pixel.y * weight + r;
                    g = source_pixel.z * weight + g;
                    b = source_pixel.w * weight + b;
                }
                column_colors[y].alpha = static_cast<unsigned char>(srFloatToInt(a));
                column_colors[y].red = static_cast<unsigned char>(srFloatToInt(r));
                column_colors[y].green = static_cast<unsigned char>(srFloatToInt(g));
                column_colors[y].blue = static_cast<unsigned char>(srFloatToInt(b));
            }
            setPixelColumn(column, x, 0, height);
        }
        delete[] channel_vectors;
        delete[] source_column_colors;
        delete[] column_colors;
        delete[] counts;
        delete[] storage;
        return;
    }
    copyNoScaling(source);
}

// FUNCTION: SURRENDER 0x10058150
void srColorSurfaceIFace::blit(long x, long y, srColorSurfaceIFace& source, long source_x,
                               long source_y, long source_right, long source_bottom)
{
    long width = this->width;
    long source_width = source.width;
    long source_height = source.height;
    long height = this->height;
    if ((x < width) && (y < height)) {
        if (x < 0) {
            source_x = source_x - x;
            x = 0;
        }
        if (y < 0) {
            source_y = source_y - y;
            y = 0;
        }
        if (source_x < 0) {
            x = x - source_x;
            source_x = 0;
        }
        if (source_y < 0) {
            y = y - source_y;
            source_y = 0;
        }
        long right = source_right;
        if (source_width < source_right) {
            right = source_width;
        }
        long bottom = source_bottom;
        if (source_height < source_bottom) {
            bottom = source_height;
        }
        if (((source_x < source_width) && (source_x < right)) &&
            ((source_y < source_height) && (source_y < bottom))) {
            long dest_span = x - source_x;
            if (width < dest_span + right) {
                right = (width - x) + source_x;
            }
            if (height < (y - source_y) + bottom) {
                bottom = (height - y) + source_y;
            }
            if ((source_x < right) && (source_y < bottom)) {
                if (((&source == this) && (x < right) && (y < bottom)) &&
                    (source_x < dest_span + right) &&
                    ((source_y < (y - source_y) + bottom) && (source_y <= y))) {
                    long span = right - source_x;
                    srARGB* temp_colors = new srARGB[(bottom - source_y) * span];
                    unsigned long* temp = (unsigned long*)temp_colors;
                    if (source_y < bottom) {
                        long row;
                        for (row = source_y; row < bottom; row++) {
                            source.getPixelRow(temp + (row - source_y) * span, row, source_x,
                                               right);
                        }
                        for (row = 0; row < bottom - source_y; row++) {
                            setPixelRow(temp + row * span, y, x, dest_span + right);
                            y = y + 1;
                        }
                    }
                    delete[] temp_colors;
                    return;
                }
                srARGB* temp_colors = new srARGB[right - source_x];
                unsigned long* temp = (unsigned long*)temp_colors;
                for (; source_y < bottom; source_y++) {
                    source.getPixelRow(temp, source_y, source_x, right);
                    setPixelRow(temp, y, x, dest_span + right);
                    y = y + 1;
                }
                delete[] temp_colors;
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10058450
void srColorSurfaceIFace::blit(const BlitInfo& info, srColorSurfaceIFace& source)
{
    if (info.destination.left == info.destination.right) {
        return;
    }
    if (info.destination.top == info.destination.bottom) {
        return;
    }
    if (info.source.left == info.source.right) {
        return;
    }
    if (info.source.top == info.source.bottom) {
        return;
    }
    int flip_h = info.destination.right < info.destination.left;
    int flip_v = info.destination.bottom < info.destination.top;
    if (info.source.right < info.source.left) {
        flip_h = flip_h == 0;
    }
    if (info.source.bottom < info.source.top) {
        flip_v = flip_v == 0;
    }
    Rectangle destination = info.destination;
    long source_left = info.source.left;
    long source_top = info.source.top;
    long source_right = info.source.right;
    long source_bottom = info.source.bottom;
    if (source_right < source_left) {
        long swap = source_left;
        source_left = source_right;
        source_right = swap;
    }
    if (source_bottom < source_top) {
        long swap = source_top;
        source_top = source_bottom;
        source_bottom = swap;
    }
    if (destination.right < destination.left) {
        long swap = destination.left;
        destination.left = destination.right;
        destination.right = swap;
    }
    if (destination.bottom < destination.top) {
        long swap = destination.top;
        destination.top = destination.bottom;
        destination.bottom = swap;
    }
    if (width <= destination.left) {
        return;
    }
    if (height <= destination.top) {
        return;
    }
    if (destination.right < 1) {
        return;
    }
    if (destination.bottom < 1) {
        return;
    }
    if (source_left < 0) {
        return;
    }
    if (source.width < source_right) {
        return;
    }
    if (source_top < 0) {
        return;
    }
    if (source.height < source_bottom) {
        return;
    }
    int full_destination = 0;
    if ((destination.left != 0) || (destination.top != 0) || (destination.right != width)) {
        full_destination = 0;
    } else {
        full_destination = destination.bottom == height;
    }
    int full_source = 0;
    if ((source_left == 0) && (source_top == 0) && (source_right == source.width) &&
        (source_bottom == source.height)) {
        full_source = 1;
    }
    int clipped = 0;
    if ((destination.left < 0) || (width < destination.right) || (destination.top < 0) ||
        (height < destination.bottom)) {
        clipped = 1;
    }
    if ((full_destination != 0) && (full_source != 0)) {
        copy(source);
        Rectangle rectangle;
        if (flip_h == 0) {
            if (flip_v == 0) {
                return;
            }
            rectangle.left = 0;
            rectangle.top = height;
            rectangle.right = width;
            rectangle.bottom = 0;
            flipRectangle(rectangle);
            return;
        }
        if (flip_v != 0) {
            rectangle.left = width;
            rectangle.top = height;
            rectangle.right = 0;
            rectangle.bottom = 0;
            flipRectangle(rectangle);
            return;
        }
        rectangle.left = width;
        rectangle.top = 0;
        rectangle.right = 0;
        rectangle.bottom = height;
        flipRectangle(rectangle);
        return;
    }
    long destination_width = destination.right - destination.left;
    long source_width = source_right - source_left;
    srColorSurface* temporary = 0;
    if (destination_width == source_width) {
        if ((flip_h == 0) && (flip_v == 0)) {
            blit(destination.left, destination.top, source, source_left, source_top, source_right,
                 source_bottom);
            return;
        }
        if (clipped == 0) {
            blit(destination.left, destination.top, source, source_left, source_top, source_right,
                 source_bottom);
            Rectangle rectangle;
            rectangle.left = destination.left;
            rectangle.top = destination.top;
            rectangle.right = destination.right;
            rectangle.bottom = destination.bottom;
            if (flip_h != 0) {
                long swap = rectangle.left;
                rectangle.left = rectangle.right;
                rectangle.right = swap;
            }
            if (flip_v != 0) {
                long swap = rectangle.top;
                rectangle.top = rectangle.bottom;
                rectangle.bottom = swap;
            }
            flipRectangle(rectangle);
            return;
        }
    }
    srColorSurfaceIFace* scaled = 0;
    if ((full_source == 0) || (flip_h != 0) || (flip_v != 0)) {
        srPixelConvert::PixelFormat format;
        source.getPixelFormat(format);
        scaled = new srColorSurface(format, source_width, source_bottom - source_top);
        srColorSurfaceIFace* scaled_iface = scaled;
        scaled_iface->copySurfaceParameters(source);
        scaled_iface->blit(0, 0, source, source_left, source_top, source_right, source_bottom);
        Rectangle rectangle;
        if (flip_h == 0) {
            if (flip_v != 0) {
                rectangle.left = 0;
                rectangle.top = scaled->height;
                rectangle.right = scaled->width;
                rectangle.bottom = 0;
                scaled->flipRectangle(rectangle);
            }
        } else {
            if (flip_v == 0) {
                rectangle.left = scaled->width;
                rectangle.top = 0;
                rectangle.right = 0;
                rectangle.bottom = scaled->height;
            } else {
                rectangle.left = scaled->width;
                rectangle.top = scaled->height;
                rectangle.right = 0;
                rectangle.bottom = 0;
            }
            scaled->flipRectangle(rectangle);
        }
    } else {
        scaled = &source;
    }
    if (full_destination == 0) {
        srPixelConvert::PixelFormat format = pixel_format;
        temporary =
            new srColorSurface(format, destination_width, destination.bottom - destination.top);
        srColorSurfaceIFace* temporary_iface = temporary;
        temporary_iface->copySurfaceParameters(*this);
        temporary->copy(*scaled);
        blit(destination.left, destination.top, *temporary, 0, 0, destination.right,
             destination.bottom);
        temporary->release();
    } else {
        copy(*scaled);
    }
    if (scaled != &source) {
        ((srColorSurface*)scaled)->release();
    }
}

// FUNCTION: SURRENDER 0x100589D0
void srColorSurfaceIFace::composite(long x, long y, srColorSurfaceIFace& source, long source_x,
                                    long source_y, long source_right, long source_bottom,
                                    double alpha)
{
    long width = this->width;
    long height = this->height;
    long source_height = source.height;
    if (0.0 < alpha) {
        if (1.0 <= alpha) {
            alpha = 1.0;
        }
        if ((source.pixel_format.alpha_bits == 0) && (alpha == 1.0)) {
            blit(x, y, source, source_x, source_y, source_right, source_bottom);
            return;
        }
        if ((x < width) && (y < height)) {
            if (x < 0) {
                source_x = source_x - x;
                x = 0;
            }
            if (y < 0) {
                source_y = source_y - y;
                y = 0;
            }
            if (source_x < 0) {
                x = x - source_x;
                source_x = 0;
            }
            if (source_y < 0) {
                y = y - source_y;
                source_y = 0;
            }
            if (source.width < source_right) {
                source_right = source.width;
            }
            if (source_height < source_bottom) {
                source_bottom = source_height;
            }
            if (((source_x < source.width) && (source_x < source_right)) &&
                ((source_y < source_height) && (source_y < source_bottom))) {
                long dest_span = x - source_x;
                if (width < dest_span + source_right) {
                    source_right = (width - x) + source_x;
                }
                if (height < source_bottom + (y - source_y)) {
                    source_bottom = (height - y) + source_y;
                }
                if ((source_x < source_right) && (source_y < source_bottom)) {
                    if ((((&source == this) && (x < source_right) && (y < source_bottom)) &&
                         (source_x < dest_span + source_right)) &&
                        ((source_y < source_bottom + (y - source_y)) && (source_y <= y))) {
                        long rows = source_bottom - source_y;
                        long span = source_right - source_x;
                        srARGB* temp_colors = new srARGB[rows * span];
                        unsigned long* temp = (unsigned long*)temp_colors;
                        srARGB* row_colors = new srARGB[span];
                        unsigned long* row = (unsigned long*)row_colors;
                        long r;
                        for (r = source_y; r < source_bottom; r++) {
                            source.getPixelRow(temp + (r - source_y) * span, r, source_x,
                                               source_right);
                        }
                        for (r = source_bottom - source_y; r != 0; r--) {
                            getPixelRow(row, y, x, dest_span + source_right);
                            for (long i = 0; i < span; i++) {
                                unsigned char* source_pixel =
                                    (unsigned char*)&temp[(source_bottom - source_y - r) * span +
                                                          i];
                                unsigned char* dest_pixel = (unsigned char*)&row[i];
                                double blend = alpha;
                                if ((source_pixel[3] != 0) && (0.0 < alpha)) {
                                    if (1.0 < alpha) {
                                        blend = 1.0;
                                    }
                                    blend = source_pixel[3] * 0.00392156862745098 * blend;
                                    double inverse = 1.0 - blend;
                                    dest_pixel[2] = (unsigned char)srFloatToInt(
                                        dest_pixel[2] * inverse + source_pixel[2] * blend);
                                    dest_pixel[1] = (unsigned char)srFloatToInt(
                                        dest_pixel[1] * inverse + source_pixel[1] * blend);
                                    dest_pixel[0] = (unsigned char)srFloatToInt(
                                        dest_pixel[0] * inverse + source_pixel[0] * blend);
                                    dest_pixel[3] = (unsigned char)srFloatToInt(
                                        blend * 255.0 + dest_pixel[3] * inverse);
                                }
                            }
                            setPixelRow(row, y, x, dest_span + source_right);
                            y = y + 1;
                        }
                        delete[] temp_colors;
                        delete[] row_colors;
                        return;
                    }
                    long span = source_right - source_x;
                    srARGB* source_row_colors = new srARGB[span];
                    unsigned long* source_row = (unsigned long*)source_row_colors;
                    srARGB* row_colors = new srARGB[span];
                    unsigned long* row = (unsigned long*)row_colors;
                    if (alpha == 1.0) {
                        for (; source_y < source_bottom; source_y++) {
                            getPixelRow(row, y, x, dest_span + source_right);
                            source.getPixelRow(source_row, source_y, source_x, source_right);
                            for (long i = 0; i < span; i++) {
                                unsigned char* source_pixel = (unsigned char*)&source_row[i];
                                unsigned char* dest_pixel = (unsigned char*)&row[i];
                                unsigned char alpha_byte = source_pixel[3];
                                if (alpha_byte != 0) {
                                    if (alpha_byte == 0xff) {
                                        row[i] = source_row[i];
                                    } else {
                                        double blend = alpha_byte * 0.00392156862745098;
                                        double inverse = 1.0 - blend;
                                        dest_pixel[2] = (unsigned char)srFloatToInt(
                                            dest_pixel[2] * inverse + source_pixel[2] * blend);
                                        dest_pixel[1] = (unsigned char)srFloatToInt(
                                            dest_pixel[1] * inverse + source_pixel[1] * blend);
                                        dest_pixel[0] = (unsigned char)srFloatToInt(
                                            dest_pixel[0] * inverse + source_pixel[0] * blend);
                                        dest_pixel[3] = (unsigned char)srFloatToInt(
                                            blend * 255.0 + dest_pixel[3] * inverse);
                                    }
                                }
                            }
                            setPixelRow(row, y, x, dest_span + source_right);
                            y = y + 1;
                        }
                    } else {
                        for (; source_y < source_bottom; source_y++) {
                            getPixelRow(row, y, x, dest_span + source_right);
                            source.getPixelRow(source_row, source_y, source_x, source_right);
                            for (long i = 0; i < span; i++) {
                                unsigned char* source_pixel = (unsigned char*)&source_row[i];
                                unsigned char* dest_pixel = (unsigned char*)&row[i];
                                double blend = alpha;
                                if ((source_pixel[3] != 0) && (0.0 < alpha)) {
                                    if (1.0 < alpha) {
                                        blend = 1.0;
                                    }
                                    blend = source_pixel[3] * 0.00392156862745098 * blend;
                                    double inverse = 1.0 - blend;
                                    dest_pixel[2] = (unsigned char)srFloatToInt(
                                        dest_pixel[2] * inverse + source_pixel[2] * blend);
                                    dest_pixel[1] = (unsigned char)srFloatToInt(
                                        dest_pixel[1] * inverse + source_pixel[1] * blend);
                                    dest_pixel[0] = (unsigned char)srFloatToInt(
                                        dest_pixel[0] * inverse + source_pixel[0] * blend);
                                    dest_pixel[3] = (unsigned char)srFloatToInt(
                                        blend * 255.0 + dest_pixel[3] * inverse);
                                }
                            }
                            setPixelRow(row, y, x, dest_span + source_right);
                            y = y + 1;
                        }
                    }
                    delete[] source_row_colors;
                    delete[] row_colors;
                }
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1005A840
static void __cdecl minifyRow_MMX(unsigned long* destination, const unsigned long* first,
                                  const unsigned long* second, unsigned long count)
{
    __asm {
        mov ecx, count
        test ecx, ecx
        jz minifyRow_MMX_done
        mov edi, destination
        mov eax, first
        mov ebx, second
        pcmpeqw mm6, mm6
        psrlw mm6, 0xe
        pxor mm7, mm7
        test edi, 0x4
        jz minifyRow_MMX_pairs
    minifyRow_MMX_single:
        movq mm0, qword ptr [eax]
        movq mm1, qword ptr [ebx]
        movq mm4, mm0
        movq mm5, mm1
        punpcklbw mm0, mm7
        punpcklbw mm1, mm7
        punpckhbw mm4, mm7
        punpckhbw mm5, mm7
        paddw mm0, mm4
        paddw mm1, mm5
        paddw mm0, mm1
        paddw mm0, mm6
        psrlw mm0, 0x2
        packuswb mm0, mm0
        movd dword ptr [edi], mm0
        add eax, 0x8
        add ebx, 0x8
        add edi, 0x4
        dec ecx
        jz minifyRow_MMX_done
    minifyRow_MMX_pairs:
        push ecx
        and ecx, 0xfffffffe
        jz minifyRow_MMX_tail
        lea eax, [eax + ecx*0x8]
        lea ebx, [ebx + ecx*0x8]
        lea edi, [edi + ecx*0x4]
        neg ecx
    minifyRow_MMX_pair_loop:
        movq mm0, qword ptr [eax + ecx*0x8]
        movq mm1, qword ptr [ebx + ecx*0x8]
        movq mm2, qword ptr [eax + ecx*0x8 + 0x8]
        movq mm3, qword ptr [ebx + ecx*0x8 + 0x8]
        movq mm4, mm0
        movq mm5, mm1
        punpcklbw mm0, mm7
        punpcklbw mm1, mm7
        punpckhbw mm4, mm7
        punpckhbw mm5, mm7
        paddw mm0, mm4
        paddw mm1, mm5
        movq mm4, mm2
        movq mm5, mm3
        punpcklbw mm2, mm7
        punpcklbw mm3, mm7
        punpckhbw mm4, mm7
        punpckhbw mm5, mm7
        paddw mm2, mm4
        paddw mm3, mm5
        paddw mm0, mm1
        paddw mm2, mm3
        paddw mm0, mm6
        paddw mm2, mm6
        psrlw mm0, 0x2
        psrlw mm2, 0x2
        packuswb mm0, mm2
        movq qword ptr [edi + ecx*0x4], mm0
        add ecx, 0x2
        js minifyRow_MMX_pair_loop
    minifyRow_MMX_tail:
        pop ecx
        and ecx, 0x1
        jnz minifyRow_MMX_single
    minifyRow_MMX_done:
        emms
    }
}

// FUNCTION: SURRENDER 0x1005A930
void srColorSurfaceIFace::minify(srColorSurfaceIFace& source)
{
    long height = this->height;
    unsigned long width = this->width;
    long source_width = source.width;
    if (((width == (unsigned long)(source_width / 2)) && (height == source.height / 2)) &&
        (this != &source)) {
        srARGB* buffer_colors = new srARGB[width + source_width * 2];
        unsigned long* buffer = (unsigned long*)buffer_colors;
        unsigned long* second = buffer + source_width;
        unsigned long* row = buffer + source_width * 2;
        if ((srCore.getTimer()->m_cpu_features & (1UL << srTimer::CPU_FEATURE_MMX)) == 0) {
            for (long y = 0; y < height; y++) {
                source.getPixelRow(buffer, y * 2, 0, source_width);
                source.getPixelRow(second, y * 2 + 1, 0, source_width);
                for (unsigned long x = 0; x < width; x++) {
                    unsigned char* top = (unsigned char*)&buffer[x * 2];
                    unsigned char* bottom = (unsigned char*)&second[x * 2];
                    unsigned char* pixel = (unsigned char*)&row[x];
                    pixel[0] = (unsigned char)((top[0] + top[4] + bottom[0] + bottom[4] + 3) >> 2);
                    pixel[1] = (unsigned char)((top[1] + top[5] + bottom[1] + bottom[5] + 3) >> 2);
                    pixel[2] = (unsigned char)((top[2] + top[6] + bottom[2] + bottom[6] + 3) >> 2);
                    pixel[3] = (unsigned char)((top[3] + top[7] + bottom[3] + bottom[7] + 3) >> 2);
                }
                setPixelRow(row, y, 0, width);
            }
        } else {
            for (long y = 0; y < height; y++) {
                source.getPixelRow(buffer, y * 2, 0, source_width);
                source.getPixelRow(second, y * 2 + 1, 0, source_width);
                minifyRow_MMX(row, buffer, second, width);
                setPixelRow(row, y, 0, width);
            }
        }
        delete[] buffer_colors;
    }
}

// FUNCTION: SURRENDER 0x1005ABB0
void srColorSurfaceIFace::magnify(srColorSurfaceIFace& source)
{
    long source_height = source.height;
    long width = this->width;
    long source_width = source.width;
    if (width == source_width * 2 && height == source_height * 2 && this != &source) {
        srARGB* buffer_colors = new srARGB[source_width + width * 2];
        unsigned long* buffer = (unsigned long*)buffer_colors;
        unsigned long* even = buffer + source_width;
        unsigned long* odd = even + width;
        source.getPixelRow(buffer, 0, 0, source_width);
        long x;
        for (x = 0; x < source_width - 1; x++) {
            even[x * 2] = buffer[x];
            even[x * 2 + 1] = (buffer[x + 1] >> 1 & 0x7f7f7f7f) + (buffer[x] >> 1 & 0x7f7f7f7f);
        }
        even[(source_width - 1) * 2] = buffer[source_width - 1];
        even[(source_width - 1) * 2 + 1] = buffer[source_width - 1];
        setPixelRow(even, 0, 0, width);
        for (long y = 1; y < source_height; y++) {
            source.getPixelRow(buffer, y, 0, source_width);
            for (x = 0; x < source_width - 1; x++) {
                odd[x * 2] = buffer[x];
                odd[x * 2 + 1] = (buffer[x + 1] >> 1 & 0x7f7f7f7f) + (buffer[x] >> 1 & 0x7f7f7f7f);
            }
            odd[(source_width - 1) * 2] = buffer[source_width - 1];
            odd[(source_width - 1) * 2 + 1] = buffer[source_width - 1];
            for (x = 0; x < width; x++) {
                even[x] = (even[x] >> 1 & 0x7f7f7f7f) + (odd[x] >> 1 & 0x7f7f7f7f);
            }
            setPixelRow(even, y * 2 - 1, 0, width);
            setPixelRow(odd, y * 2, 0, width);
            unsigned long* swap = even;
            even = odd;
            odd = swap;
        }
        setPixelRow(even, height - 1, 0, width);
        delete[] buffer_colors;
    }
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
    dumpFlags(stream, surface_flags, s_flag_names2);
    stream << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}
