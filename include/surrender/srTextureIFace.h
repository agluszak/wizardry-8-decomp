#pragma once

#include "srColorSurfaceIFace.h"
#include "srTypeRegistry.h"

class srFilter;
class srTexture;
class stSurface2D;

class SR_DLL_IMPORT srTextureIFace : public srClassSupport<srTextureIFace, srClass, true, 0x2100> {
public:
    struct Dimensions {
        unsigned long width;
        unsigned long height;
    };
    struct MultiRequest {
        long mipmap_level;
        unsigned long unknown_04;
        srColorSurfaceIFace* destinations[1];
    };
    struct PartialRequest {
        unsigned long unknown_00[3];
        long destination_x;
        long destination_y;
        long source_right;
        long source_bottom;
        srColorSurfaceIFace* destination;
    };
    /* getTextureParms copies eight bytes: packed filter/wrap/mipmap state
       from srTexture+0x18 and mipmap bias from +0x1c. */
    struct Parameters {
        unsigned long packed_state_00;
        float mipmap_bias_04;
    };
    static_assert(sizeof(Parameters) == 0x08, "srTextureIFace_Parameters_must_be_0x08");
    /* srTexture::dump: NONE / FASTEST / GOOD / BEST, else DEFAULT.
       setTextureDefaultMagFilter remaps DEFAULT (4) to GOOD. */
    enum e_filter {
        FILTER_NONE = 0,
        FILTER_FASTEST = 1,
        FILTER_GOOD = 2,
        FILTER_BEST = 3,
        FILTER_DEFAULT = 4
    };
    /* Dump: NONE / FASTEST / BEST, else DEFAULT. GOOD is not a mipmap
       enumerator. setTextureDefaultMipmap remaps DEFAULT (3) to FASTEST. */
    enum e_mipmap { MIPMAP_NONE = 0, MIPMAP_FASTEST = 1, MIPMAP_BEST = 2, MIPMAP_DEFAULT = 3 };
    enum e_hint {
        HINT_POSITIONAL_1 = 1,
        HINT_POSITIONAL_2 = 2,
        HINT_POSITIONAL_3 = 3,
        HINT_POSITIONAL_6 = 6
    };
    /* Dump prints REPEAT then CLAMP for wrap S/T. Wizardry requests 1. */
    enum e_wrap { WRAP_REPEAT = 0, WRAP_CLAMP = 1 };
    /* Dump: FASTEST / GOOD / BEST, else DEFAULT. Packed in bits 0–1 of
       srTexture+0x18. */
    enum e_correction {
        CORRECTION_FASTEST = 0,
        CORRECTION_GOOD = 1,
        CORRECTION_BEST = 2,
        CORRECTION_DEFAULT = 3
    };

    static const char* sGetClassName()
    {
        return "srTextureIFace";
    }

    /* Slot 8. Slot 6 is srClass::vInstance; slot 7 is clone. srTextureFile's
       17-slot vftable (0 through 16) is this interface exactly. */
    virtual unsigned long getTextureFrameHandle() = 0;
    virtual float getPriority() = 0;
    virtual void getDimensions(Dimensions& dimensions) = 0;
    virtual void getMipmapData(MultiRequest& request) = 0;
    virtual void getMipmapLevelPartial(PartialRequest& request) = 0;
    virtual void getTextureParms(Parameters& parameters) = 0;
    virtual const char* getTextureName();
    virtual void invalidate() = 0;

protected:
    virtual void setupDefaultValues() = 0;
};
