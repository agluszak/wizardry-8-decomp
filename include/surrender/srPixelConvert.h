#pragma once

#include "srHeap.h"

class srPixelConvert {
public:
    enum e_surfaceType {
        SURFACE_L8 = 0x02,
        SURFACE_RGB565 = 0x07,
        SURFACE_RGB555 = 0x08,
        SURFACE_ARGB1555 = 0x09,
        SURFACE_BGR24 = 0x0c,
        SURFACE_BGRA32 = 0x0e,
        SURFACE_COPY = 0x18
    };

    struct ConversionInfo;
    typedef void(__cdecl* ConversionFunc)(const ConversionInfo& info);

    struct PixelFormat {
        unsigned char red_bits;
        unsigned char red_shift;
        unsigned char green_bits;
        unsigned char green_shift;
        unsigned char blue_bits;
        unsigned char blue_shift;
        unsigned char alpha_bits;
        unsigned char alpha_shift;
        e_surfaceType surface_type;
        long bytes_per_pixel_minus_one;
        unsigned long flags;

        void getName(char* name);
        int isValid() const;
        unsigned long match(const PixelFormat* formats, unsigned long count) const;
    };

    static e_surfaceType mapPixelFormat(const PixelFormat& format);
    /* This overload is imported by both Wiz8 and the JPEG extension. */
    static SR_DLL_IMPORT void mapPixelFormat(e_surfaceType type, PixelFormat& format);
    static void selectFuncs(const PixelFormat& format, ConversionFunc& write,
                            ConversionFunc& read);
};

static_assert(sizeof(srPixelConvert::PixelFormat) == 0x14,
              "srPixelConvert_PixelFormat_must_be_0x14");
