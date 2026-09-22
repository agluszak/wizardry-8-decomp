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
    /* The flag getters unpack the packed_state_18 fields retail writes
       through the setters below; all six are exported at 0x1005ED20..70. */
    e_correction getCorrection() const;
    e_filter getMagFilter() const;
    e_filter getMinFilter() const;
    e_mipmap getMipmap() const;
    e_wrap getWrapS() const;
    e_wrap getWrapT() const;

    /* Dump of +0x50: GENERATESURFACE_FAILURE,DIRTY_DEFAULTS. Bit indices into
       texture_flags_; ctor ORs DIRTY_DEFAULTS. */
    enum e_flag { FLAG_GENERATESURFACE_FAILURE = 0, FLAG_DIRTY_DEFAULTS = 1 };

protected:
    friend class stSurface2D;
    srTexture();
    virtual ~srTexture() override;
    static unsigned long getNewFrameHandle();
    /* Monotonic frame-handle counter at 0x100A4A1C; getNewFrameHandle
       increments then returns it. */
    static unsigned long _frameHandle;
    void invalidateFrameHandle(unsigned long handle);
    void setupDefaultValuesFromSurface(srColorSurfaceIFace* surface);

    unsigned long packed_state_18; /* 0x18: correction/mag/min/mipmap/wrap bits */
    float mipmap_bias_1c;          /* 0x1c */
    /* 0x20: width/height, palette (+0x28), pixel format (+0x2c), hint/creation
       flags (+0x40), parameter index (+0x44) and filter (+0x48, getFilter). */
    Dimensions texture_dimensions_; /* 0x20 */
    float texture_priority_4c;      /* 0x4c: getPriority; the ctor seeds 0.5f */
    unsigned long texture_flags_;   /* 0x50 */
};

static_assert((sizeof(srTexture) == 0x54), "srTexture_must_be_0x54");
