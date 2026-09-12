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
    enum e_filter {};
    enum e_mipmap {};
    enum e_hint {
        HINT_POSITIONAL_1 = 1,
        HINT_POSITIONAL_2 = 2,
        HINT_POSITIONAL_3 = 3,
        HINT_POSITIONAL_6 = 6
    };
    /* Dump prints REPEAT then CLAMP for wrap S/T. Wizardry requests 1. */
    enum e_wrap { WRAP_REPEAT = 0, WRAP_CLAMP = 1 };
    enum e_correction {};

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
