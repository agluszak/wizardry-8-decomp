#pragma once

#include <iosfwd>

#include "srHeap.h"

class srShader;

SR_DLL_IMPORT std::ostream& operator<<(std::ostream& stream, const srShader& shader);

/* A shader is one 32-bit packed word. The modeler/mesh default 0x0100241b is PASS_LEQUAL, depth and
   color write, DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_MODULATE, SRCBLEND_ONE, TEXTURING_DISABLE,
   DITHER_ENABLE. */
class srShader {
public:
    // FUNCTION: SURRENDER 0x100199F0
    // FUNCTION: WIZ8 0x00424A40
    // NAME: srShader::srShader
    // RECOMP: ??0srShader@@QAE@XZ
    srShader() : value(0x0100241b) {}

    // FUNCTION: SURRENDER 0x1003B930
    // FUNCTION: WIZ8 0x0041CF80
    srShader(const srShader& other) : value(other.value) {}

    enum e_pass {
        PASS_NEVER = 0,
        PASS_LESS = 1,
        PASS_EQUAL = 2,
        PASS_LEQUAL = 3,
        PASS_GREATER = 4,
        PASS_NOTEQUAL = 5,
        PASS_GEQUAL = 6,
        PASS_ALWAYS = 7
    };
    enum e_dstBlend {
        DSTBLEND_ZERO = 0,
        DSTBLEND_ONE = 1,
        DSTBLEND_SRC_COLOR = 2,
        DSTBLEND_ONE_MINUS_SRC_COLOR = 3,
        DSTBLEND_SRC_ALPHA = 4,
        DSTBLEND_ONE_MINUS_SRC_ALPHA = 5
    };
    enum e_fog { FOG_DISABLE = 0, FOG_ENABLE = 1, FOG_SCALE_FRAGMENT = 2, FOG_WHITE = 3 };
    enum e_gradient { GRADIENT_DISABLE = 0, GRADIENT_MODULATE = 1, GRADIENT_ADD = 2 };
    enum e_srcBlend {
        SRCBLEND_ZERO = 0,
        SRCBLEND_ONE = 1,
        SRCBLEND_SRC_ALPHA = 2,
        SRCBLEND_ONE_MINUS_SRC_ALPHA = 3
    };
    enum e_detailColor {
        DETAILCOLOR_DISABLE = 0,
        DETAILCOLOR_DETAIL = 1,
        DETAILCOLOR_SCALE = 2,
        DETAILCOLOR_INVSCALE = 3,
        DETAILCOLOR_ADD = 4,
        DETAILCOLOR_SUB = 5,
        DETAILCOLOR_SUBR = 6,
        DETAILCOLOR_BLEND = 7,
        DETAILCOLOR_DETAILBLEND = 8
    };
    enum e_detailAlpha {
        DETAILALPHA_DISABLE = 0,
        DETAILALPHA_DETAIL = 1,
        DETAILALPHA_SCALE = 2,
        DETAILALPHA_INVSCALE = 3
    };
    enum e_depthWrite { DEPTH_WRITE_DISABLE = 0, DEPTH_WRITE_ENABLE = 1 };
    enum e_colorWrite { COLOR_WRITE_DISABLE = 0, COLOR_WRITE_ENABLE = 1 };
    enum e_alphaTest { ALPHATEST_DISABLE = 0, ALPHATEST_ENABLE = 1 };
    enum e_dither { DITHER_DISABLE = 0, DITHER_ENABLE = 1 };
    enum e_secondaryGradient { SECONDARY_GRADIENT_DISABLE = 0, SECONDARY_GRADIENT_ENABLE = 1 };
    enum e_texturing { TEXTURING_DISABLE = 0, TEXTURING_ENABLE = 1 };

    /* Packed-field masks. */
    enum {
        PASS_MASK = 7,
        DEPTH_WRITE_SHIFT = 3,
        MASK_DEPTH_WRITE = 0x8,
        COLOR_WRITE_SHIFT = 4,
        MASK_COLOR_WRITE = 0x10,
        DSTBLEND_SHIFT = 5,
        FOG_SHIFT = 8,
        MASK_FOG = 0x300,
        GRADIENT_SHIFT = 10,
        MASK_GRADIENT = 0xc00,
        MASK_GRADIENT_MODULATE = 0x400,
        MASK_GRADIENT_ADD = 0x800,
        SECONDARY_GRADIENT_SHIFT = 12,
        MASK_SECONDARY_GRADIENT = 0x1000,
        SRCBLEND_SHIFT = 13,
        TEXTURING_SHIFT = 15,
        MASK_TEXTURING = 0x8000,
        DETAILCOLOR0_SHIFT = 16,
        MASK_DETAILCOLOR0 = 0x000f0000,
        DETAILALPHA0_SHIFT = 20,
        MASK_DETAILALPHA0 = 0x00700000,
        ALPHATEST_SHIFT = 23,
        MASK_ALPHATEST = 0x800000,
        DITHER_SHIFT = 24,
        MASK_DITHER = 0x1000000,
        DETAILCOLOR1_SHIFT = 25,
        MASK_DETAILCOLOR1 = 0x1e000000,
        DETAILALPHA1_SHIFT = 29,
        MASK_DETAILALPHA1 = 0xe0000000
    };

    /* Packed-field range test. */
    int isValid() const
    {
        return !((value & PASS_MASK) > PASS_ALWAYS || (value >> DEPTH_WRITE_SHIFT & 0x1) > 1 ||
                 (value >> COLOR_WRITE_SHIFT & 0x1) > 1 ||
                 (value >> DSTBLEND_SHIFT & 0x7) > DSTBLEND_ONE_MINUS_SRC_ALPHA ||
                 (value >> FOG_SHIFT & 0x3) > FOG_WHITE ||
                 (value >> GRADIENT_SHIFT & 0x3) > GRADIENT_ADD ||
                 (value >> SECONDARY_GRADIENT_SHIFT & 0x1) > 1 ||
                 (value >> SRCBLEND_SHIFT & 0x3) > SRCBLEND_ONE_MINUS_SRC_ALPHA ||
                 (value >> TEXTURING_SHIFT & 0x1) > 1 ||
                 (value >> DETAILCOLOR0_SHIFT & 0xf) > DETAILCOLOR_DETAILBLEND ||
                 (value >> DETAILALPHA0_SHIFT & 0x7) > DETAILALPHA_INVSCALE ||
                 (value >> ALPHATEST_SHIFT & 0x1) > 1 || (value >> DITHER_SHIFT & 0x1) > 1 ||
                 (value >> DETAILCOLOR1_SHIFT & 0xf) > DETAILCOLOR_DETAILBLEND ||
                 (value >> DETAILALPHA1_SHIFT & 0x7) > DETAILALPHA_INVSCALE);
    }

    unsigned long value;
};

static_assert(sizeof(srShader) == 0x04, "srShader_must_be_0x04");
