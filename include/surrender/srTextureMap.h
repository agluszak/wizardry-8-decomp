#pragma once

#include "srPtr.h"
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
    /* 0x54: setSurfacePtr/getSurfacePtr; the copy-assignment emission proves
       srPtr refcounting (field-address guard + release/addref handoff). */
    srPtr<srColorSurfaceIFace> surface_54_;
    unsigned long frame_handle_58_; /* 0x58: ctor stores getNewFrameHandle() */
};

static_assert((sizeof(srTextureMap) == 0x5c), "srTextureMap_must_be_0x5c");
