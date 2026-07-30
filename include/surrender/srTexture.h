#pragma once

#include "srColorSurface.h"
#include "srTypeRegistry.h"

class srFilter;
class srTexture;
class stSurface2D;

class SR_DLL_IMPORT srTextureIFace
    : public srClassSupport<srTextureIFace, srClass, true, 0x2100> {
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
    struct Parameters;
    enum e_filter {};
    enum e_mipmap {};
    enum e_hint {
        HINT_POSITIONAL_1 = 1,
        HINT_POSITIONAL_2 = 2
    };
    enum e_wrap {
        WRAP_POSITIONAL_1 = 1
    };
    enum e_correction {};

    static const char* sGetClassName()
    {
        return "srTextureIFace";
    }

    /* Slot 7. Slot 6 remains srClass::vInstance. */
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

class SR_DLL_IMPORT srTexture
    : public srClassSupport<srTexture, srTextureIFace, false, 0x2110> {
public:
    static const char* sGetClassName();
    srTexture& operator=(const srTexture& other);
    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual unsigned long getTextureFrameHandle() override;
    virtual float getPriority() override;
    virtual void getDimensions(Dimensions& dimensions) override;
    virtual void getMipmapData(MultiRequest& request) override;
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void getTextureParms(Parameters& parameters) override;
    srFilter* getFilter() const;
    void setMipmap(e_mipmap mipmap);
    void setMipmapBias(float bias);
    void enableHint(e_hint hint);
    void setCorrection(e_correction correction);
    void setMagFilter(e_filter filter);
    void setMinFilter(e_filter filter);
    void setWrapS(e_wrap wrap);
    void setWrapT(e_wrap wrap);

protected:
    friend class stSurface2D;
    srTexture();
    virtual ~srTexture() override;
    static unsigned long getNewFrameHandle();
    void invalidateFrameHandle(unsigned long handle);
    void setupDefaultValuesFromSurface(srColorSurfaceIFace* surface);

    unsigned char unknown_18_[0x08];
    Dimensions texture_dimensions_;        /* 0x20 */
    srClass* texture_filter_;               /* 0x28 */
    srPixelConvert::PixelFormat surface_format_; /* 0x2c */
    unsigned char unknown_40_[0x10];
    unsigned long texture_flags_;           /* 0x50 */
};

static_assert((sizeof(srTexture) == 0x54), "srTexture_must_be_0x54");

class SR_DLL_IMPORT srTextureMap
    : public srClassSupport<srTextureMap, srTexture, 0, 0x2111> {
public:
    srTextureMap(srColorSurfaceIFace* surface);

    static const char* sGetClassName()
    {
        return "srTextureMap";
    }
    srTextureMap& operator=(const srTextureMap& other);
    virtual void dump(std::ostream& stream) override;

protected:
    virtual ~srTextureMap() override;

public:
    virtual srClass* vInstance() override;
    virtual unsigned long getTextureFrameHandle() override;
    virtual void getMipmapData(MultiRequest& request) override;
    void setSurfacePtr(srColorSurfaceIFace* surface);
    virtual void invalidate() override;

protected:
    virtual void setupDefaultValues() override;
    unsigned char unknown_54_[0x08];
};

static_assert((sizeof(srTextureMap) == 0x5c), "srTextureMap_must_be_0x5c");
