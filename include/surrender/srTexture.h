#pragma once

#include "srPixelConvert.h"
#include "srTextureIFace.h"

class SR_DLL_IMPORT srTexture : public srClassSupport<srTexture, srTextureIFace, false, 0x2110> {
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
    void disableHint(e_hint hint);
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

    unsigned long packed_state_18;               /* 0x18: correction/mag/min/mipmap/wrap bits */
    float mipmap_bias_1c;                        /* 0x1c */
    Dimensions texture_dimensions_;              /* 0x20 */
    srClass* texture_filter_;                    /* 0x28 */
    srPixelConvert::PixelFormat surface_format_; /* 0x2c */
    /* enableHint/disableHint operate on the first dword; stTextureAnim reads
       byte +0x42 as an alpha probe. */
    unsigned char unknown_40_[0x10];
    unsigned long texture_flags_; /* 0x50 */
};

static_assert((sizeof(srTexture) == 0x54), "srTexture_must_be_0x54");
