#include "surrender/srColorSurface.h"

#include "surrender/srPalette.h"

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
