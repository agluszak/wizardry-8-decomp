#pragma once

#include "srTexture.h"

class SR_DLL_IMPORT srTextureMap : public srClassSupport<srTextureMap, srTexture, 0, 0x2111> {
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
    srColorSurfaceIFace* getSurfacePtr() const;
    virtual void invalidate() override;

protected:
    virtual void setupDefaultValues() override;
    unsigned char unknown_54_[0x08];
};

static_assert((sizeof(srTextureMap) == 0x5c), "srTextureMap_must_be_0x5c");
