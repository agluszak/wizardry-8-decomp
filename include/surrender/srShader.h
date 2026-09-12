#pragma once

/* SurRender stores a shader as one 32-bit packed word. operator<<(ostream,
   srShader) at 0x10034db0 prints every field; Wizardry writes the same word
   with masks. The modeller/mesh default 0x0100241b is PASS_LEQUAL, depth and
   color write, DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_MODULATE, SRCBLEND_ONE,
   TEXTURING_DISABLE, DITHER_ENABLE. */
class srShader {
public:
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

    /* Packed-field masks. Depth/color write, texturing, alphatest and dither
       are one-bit enables. Gradient occupies bits 10–11 (MODULATE=0x400,
       ADD=0x800). Detail color/alpha 0 sit at bits 16–22; unit 1 at 25–31. */
    enum {
        PASS_MASK = 7,
        MASK_DEPTH_WRITE = 0x8,
        MASK_COLOR_WRITE = 0x10,
        DSTBLEND_SHIFT = 5,
        FOG_SHIFT = 8,
        MASK_GRADIENT_MODULATE = 0x400,
        MASK_GRADIENT_ADD = 0x800,
        MASK_SECONDARY_GRADIENT = 0x1000,
        SRCBLEND_SHIFT = 13,
        MASK_TEXTURING = 0x8000,
        DETAILCOLOR0_SHIFT = 16,
        DETAILALPHA0_SHIFT = 20,
        MASK_ALPHATEST = 0x800000,
        MASK_DITHER = 0x1000000,
        DETAILCOLOR1_SHIFT = 25,
        DETAILALPHA1_SHIFT = 29
    };

    unsigned long value;
};

static_assert(sizeof(srShader) == 0x04, "srShader_must_be_0x04");
