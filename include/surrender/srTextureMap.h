#pragma once

#include "srPtr.h"
#include "srTexture.h"

class SR_DLL_IMPORT srTextureMap : public srClassSupport<srTextureMap, srTexture, 0, 0x2111> {
public:
    /* The default-constructor closure 0x100608C0 proves the surface argument
       defaults to null for paren-less new expressions. */
    srTextureMap(srColorSurfaceIFace* surface = 0);
    /* Retail's copy constructor calls operator= and leaves memberwise
       re-copies to the compiler's copy-ctor fixup emission. */
    srTextureMap(const srTextureMap& other);

    // FUNCTION: SURRENDER 0x10060770 SYMBOL
    // ?sGetClassName@srTextureMap@@SAPBDXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
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
