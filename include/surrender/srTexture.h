#pragma once

#include "srPixelConvert.h"
#include "srTextureIFace.h"

class __declspec(novtable) SR_DLL_IMPORT SR_DLL_EXPORT srTexture
    : public srClassSupport<srTexture, srTextureIFace, false, 0x2110> {
public:
    static const char* sGetClassName();
    /* The copy body is consistent with ordinary member copy-construction;
       in particular Dimensions' srPtr palette is addref'd as a fresh member. */

    srTexture& operator=(const srTexture& other);
    virtual void dump(std::ostream& stream) override;
    virtual w8_ulong getTextureFrameHandle() override;
    virtual float getPriority() override;
    virtual void getDimensions(Dimensions& dimensions) override;
    void setDimensions(const Dimensions& dimensions);
    virtual void getMipmapData(MultiRequest& request) override;
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void getTextureParms(Parameters& parameters) override;
    srFilter* getFilter() const;
    void setFilter(srFilter* filter);
    void setMipmap(e_mipmap mipmap);
    void setMipmapBias(float bias);
    float getMipmapBias() const;
    void setPriority(float priority);
    void setDimensionsDirty();
    e_compression getCompression() const;
    void setCompression(e_compression compression);
    void enableHint(e_hint hint);
    void disableHint(e_hint hint);
    int isHintEnabled(e_hint hint) const;
    void setCorrection(e_correction correction);
    void setMagFilter(e_filter filter);
    void setMinFilter(e_filter filter);
    void setWrapS(e_wrap wrap);
    void setWrapT(e_wrap wrap);
    e_correction getCorrection() const;
    e_filter getMagFilter() const;
    e_filter getMinFilter() const;
    e_mipmap getMipmap() const;
    e_wrap getWrapS() const;
    e_wrap getWrapT() const;

    /* Bit indices into texture_flags_. */
    enum e_flag { FLAG_GENERATESURFACE_FAILURE = 0, FLAG_DIRTY_DEFAULTS = 1 };

protected:
    friend class stSurface2D;
    srTexture();
    virtual ~srTexture() override;
    static w8_ulong getNewFrameHandle();
    /* Allocates count consecutive frame handles and returns the first. */
    static w8_ulong getNewFrameHandles(w8_ulong count);
    static w8_ulong _frameHandle;
    void invalidateFrameHandle(w8_ulong handle);
    void setupDefaultValuesFromSurface(srColorSurfaceIFace* surface);

    w8_ulong packed_state;          /* 0x18: correction/mag/min/mipmap/wrap bits */
    float mipmap_bias;              /* 0x1c */
    Dimensions texture_dimensions_; /* 0x20 */
    float texture_priority;         /* 0x4c: getPriority; the ctor seeds 0.5f */
    w8_ulong texture_flags_;        /* 0x50 */
};

W8_ABI_ASSERT((sizeof(srTexture) == 0x54), "srTexture_must_be_0x54");
