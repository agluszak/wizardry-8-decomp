#pragma once

#include "srTypeRegistry.h"

class srColorSurfaceIFace;
class stSurface2D;

class SR_DLL_IMPORT srTextureIFace : public srClass {
public:
    struct Dimensions;
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
    enum e_hint {};
    enum e_wrap {};
    enum e_correction {};

    static const char* sGetClassName();

    /* Slot 7. Slot 6 remains srClass::vInstance. */
    virtual srTextureIFace* clone() = 0;
    virtual unsigned long getTextureFrameHandle() = 0;
    virtual float getPriority() = 0;
    virtual void getDimensions(Dimensions& dimensions) = 0;
    virtual void getMipmapData(MultiRequest& request) = 0;
    virtual void getMipmapLevelPartial(PartialRequest& request) = 0;
    virtual void getTextureParms(Parameters& parameters) = 0;
    virtual const char* getTextureName();
    virtual void invalidate() = 0;
    virtual void update() = 0;
};

class SR_DLL_IMPORT srTexture : public srTextureIFace {
public:
    static const char* sGetClassName();
    srTexture& operator=(const srTexture& other);
    virtual const char* getClassName() const;
    virtual unsigned long getClassID() const;
    virtual srRegistry::ClassNode* getClassNode() const;
    virtual void dump(std::ostream& stream);
    virtual srClass* vInstance();
    virtual unsigned long getTextureFrameHandle();
    virtual float getPriority();
    virtual void getDimensions(Dimensions& dimensions);
    virtual void getMipmapData(MultiRequest& request);
    virtual void getMipmapLevelPartial(PartialRequest& request);
    virtual void getTextureParms(Parameters& parameters);
    void setMipmap(e_mipmap mipmap);
    void enableHint(e_hint hint);
    void setCorrection(e_correction correction);
    void setMagFilter(e_filter filter);
    void setMinFilter(e_filter filter);
    void setWrapS(e_wrap wrap);
    void setWrapT(e_wrap wrap);

protected:
    friend class stSurface2D;
    srTexture();
    virtual ~srTexture();
    static unsigned long getNewFrameHandle();
    void invalidateFrameHandle(unsigned long handle);
    void setupDefaultValuesFromSurface(srColorSurfaceIFace* surface);

    unsigned char unknown_04_[0x1c];
    unsigned long texture_width_;          /* 0x20 */
    unsigned long texture_height_;         /* 0x24 */
    srClass* texture_filter_;               /* 0x28 */
    unsigned long surface_format_[5];       /* 0x2c */
    unsigned char unknown_40_[0x10];
    unsigned long texture_flags_;           /* 0x50 */
};

typedef char srTexture_must_be_0x54[(sizeof(srTexture) == 0x54) ? 1 : -1];

class SR_DLL_IMPORT srTextureMap : public srTexture {
public:
    srTextureMap(srColorSurfaceIFace* surface);
    srTextureMap& operator=(const srTextureMap& other);
    virtual srClass* vInstance();
    void setSurfacePtr(srColorSurfaceIFace* surface);
    virtual void invalidate();

protected:
    unsigned char unknown_54_[0x08];
};

typedef char srTextureMap_must_be_0x5c[(sizeof(srTextureMap) == 0x5c) ? 1 : -1];
