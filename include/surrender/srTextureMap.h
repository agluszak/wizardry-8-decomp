#pragma once

#include "srPtr.h"
#include "srTexture.h"

// VTABLE: SURRENDER 0x100775BC
// class srClassSupport<srTextureMap, srTexture, 0, 8465>

// VTABLE: SURRENDER 0x10077578 srTextureMap
// class srTextureMap
class SR_DLL_IMPORT SR_DLL_EXPORT srTextureMap
    : public srClassSupport<srTextureMap, srTexture, 0, 0x2111> {
public:
    srTextureMap(srColorSurfaceIFace* surface = 0);

    // FUNCTION: SURRENDER 0x10060770 SYMBOL
    // RECOMP: ?sGetClassName@srTextureMap@@SAPBDXZ
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
    virtual w8_ulong getTextureFrameHandle() override;
    virtual void getMipmapData(MultiRequest& request) override;
    void setSurfacePtr(srColorSurfaceIFace* surface);
    srColorSurfaceIFace* getSurfacePtr() const;
    virtual void invalidate() override;

protected:
    virtual void setupDefaultValues() override;
    srPtr<srColorSurfaceIFace> surface;
    w8_ulong frame_handle; /* 0x58: ctor stores getNewFrameHandle() */
};

W8_ABI_ASSERT((sizeof(srTextureMap) == 0x5c), "srTextureMap_must_be_0x5c");
