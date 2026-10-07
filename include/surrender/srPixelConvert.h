#pragma once

#include "srHeap.h"

class srPalette;

class srPixelConvert {
public:
    enum e_surfaceType {
        SURFACE_AP44 = 0x00,
        SURFACE_AL44 = 0x01,
        SURFACE_L8 = 0x02,
        SURFACE_A8 = 0x03,
        SURFACE_P8 = 0x04,
        SURFACE_AP88 = 0x05,
        SURFACE_AL88 = 0x06,
        SURFACE_RGB565 = 0x07,
        SURFACE_RGB555 = 0x08,
        SURFACE_ARGB1555 = 0x09,
        SURFACE_RGB444 = 0x0a,
        SURFACE_ARGB4444 = 0x0b,
        SURFACE_BGR24 = 0x0c,
        SURFACE_BGRX32 = 0x0d,
        SURFACE_BGRA32 = 0x0e,
        SURFACE_Y4U2V2 = 0x0f,
        SURFACE_A8Y4U2V2 = 0x10,
        SURFACE_RGB332 = 0x11,
        SURFACE_BGR565 = 0x13,
        SURFACE_ARGB32 = 0x14,
        SURFACE_BGR555 = 0x15,
        SURFACE_ABGR32 = 0x16,
        SURFACE_RGBA32 = 0x17,
        SURFACE_RGB24 = 0x18,
        SURFACE_INVALID = 0x19
    };

    enum e_colorModel { COLOR_RGB = 0, COLOR_YUV = 1, COLOR_INTENSITY = 2, COLOR_INDEXED = 3 };
    enum e_pixelSize { PIXEL_SIZE_8 = 0, PIXEL_SIZE_16 = 1, PIXEL_SIZE_24 = 2, PIXEL_SIZE_32 = 3 };

    struct PixelFormat {
        unsigned char red_bits;
        unsigned char red_shift;
        unsigned char green_bits;
        unsigned char green_shift;
        unsigned char blue_bits;
        unsigned char blue_shift;
        unsigned char alpha_bits;
        unsigned char alpha_shift;
        /* Generic converter family; pixel_size stores (bits per pixel / 8) - 1. */
        e_colorModel color_model;
        e_pixelSize pixel_size;
        unsigned long fourcc;

        PixelFormat() : fourcc(0) {}

#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        void getName(char* const name);
        int isValid() const;
        unsigned long match(const PixelFormat* formats, unsigned long count) const;

        int operator==(const PixelFormat& other) const
        {
            const unsigned long* a = reinterpret_cast<const unsigned long*>(
                this); // reinterpret-ok: packed pixel-format block compare
            const unsigned long* b = reinterpret_cast<const unsigned long*>(
                &other); // reinterpret-ok: packed pixel-format block compare
            return a[4] == b[4] && b[0] == a[0] && b[1] == a[1] && a[3] == b[3] && a[2] == b[2];
        }
    };

    /* Converters receive the run description by reference; palette carries
       the surface's srPalette for paletted conversion classes. */
    struct ConversionInfo {
        void* dest;
        const void* source;
        unsigned long count;
        srPalette* palette;
        const PixelFormat* format;
    };
    typedef void(__cdecl* ConversionFunc)(const ConversionInfo& info);

    static e_surfaceType mapPixelFormat(const PixelFormat& format);
    static SR_DLL_IMPORT void mapPixelFormat(e_surfaceType type, PixelFormat& format);
    static void selectFuncs(const PixelFormat& format, ConversionFunc& write, ConversionFunc& read);
};

static_assert(sizeof(srPixelConvert::PixelFormat) == 0x14,
              "srPixelConvert_PixelFormat_must_be_0x14");
static_assert(sizeof(srPixelConvert::ConversionInfo) == 0x14,
              "srPixelConvert_ConversionInfo_must_be_0x14");

/* Builds the conversion lookup tables (channel expansion/reduction ramps,
   dither cube, channel weights, decode/grayscale palettes) at library init. */
void __cdecl initPixelTables(void);
