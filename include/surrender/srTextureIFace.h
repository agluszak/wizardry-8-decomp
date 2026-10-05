#pragma once

#include "srColorSurfaceIFace.h"
#include "srPalette.h"
#include "srPixelConvert.h"
#include "srPtr.h"
#include "srTypeRegistry.h"

class srFilter;
class srTexture;
class stSurface2D;

class __declspec(novtable) SR_DLL_IMPORT SR_DLL_EXPORT srTextureIFace
    : public srClassSupport<srTextureIFace, srClass, true, 0x2100> {
public:
    /* srGERD::setTextureDefaultCompression remaps DEFAULT (4) to 0; the
       srTexture ctor seeds Dimensions::compression with DEFAULT. */
    enum e_compression { COMPRESSION_DEFAULT = 4 };
    /* srTexture::getDimensions copies the embedded 0x2c-byte state block at
       srTexture+0x20 wholesale: width/height, an srPtr<srPalette> assigned
       through srPtr::operator= (so the output object must be constructed), the
       surface PixelFormat, the e_hint bitmask, the compression selector and
       the raw srFilter*. The srTexture ctor seeds palette from
       srCore::getPalette() and filter from srCore::getFilter(). */
    struct Dimensions {
        unsigned long width;
        unsigned long height;
        srPtr<srPalette> palette;
        srPixelConvert::PixelFormat format;
        unsigned long hints;
        e_compression compression;
        srFilter* filter;
    };
    static_assert(sizeof(Dimensions) == 0x2c, "srTextureIFace_Dimensions_must_be_0x2c");
    struct MultiRequest {
        long mipmap_level;
        /* Last mipmap level filled; iterated level <= last_level. */
        unsigned long last_level;
        srColorSurfaceIFace* destinations[1];
    };
    /* srGERD::setTextureSubImage packs the level-0 dimensions and the target
       mipmap level ahead of the clipped destination rect. */
    struct PartialRequest {
        unsigned long width;
        unsigned long height;
        long mipmap_level;
        long destination_x;
        long destination_y;
        long source_right;
        long source_bottom;
        srColorSurfaceIFace* destination;
    };
    /* getTextureParms copies eight bytes: packed filter/wrap/mipmap state
       from srTexture+0x18 and mipmap bias from +0x1c. */
    struct Parameters {
        enum {
            CORRECTION_MASK = 0x0003u,
            MAG_FILTER_SHIFT = 4,
            MAG_FILTER_MASK = 0x0070u,
            MIN_FILTER_SHIFT = 7,
            MIN_FILTER_MASK = 0x0380u,
            MIPMAP_SHIFT = 10,
            MIPMAP_MASK = 0x0c00u,
            WRAP_S_SHIFT = 12,
            WRAP_S_MASK = 0x1000u,
            WRAP_T_SHIFT = 13,
            WRAP_T_MASK = 0x2000u
        };
        unsigned long packed_state;
        float mipmap_bias;
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
    /* enableHint ORs 1<<hint into srTexture+0x40. srTexture::dump does not
       print these. Wizardry 2D tiles pair 1/2 with shader ALPHATEST; overlay
       and poster paths also set 3; stTexture2D's ctor also sets 6. */
    enum e_hint {
        HINT_NO_ALPHA = 1,
        HINT_ONE_BIT_ALPHA = 2,
        HINT_NO_MIPMAPS = 3,
        HINT_RESIDENT = 5,
        HINT_POSITIONAL_6 = 6,
        HINT_NO_REDUCTION = 7
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

    // FUNCTION: SURRENDER 0x1005F5B0 SYMBOL
    // ?sGetClassName@srTextureIFace@@SAPBDXZ
    static const char* sGetClassName()
    {
        return "srTextureIFace";
    }

    /* No state beyond srClassSupport is modeled; lifecycle bodies are consistent
       with ordinary base-only operations. */

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
