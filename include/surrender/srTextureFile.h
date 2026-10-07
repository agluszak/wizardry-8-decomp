#pragma once

#include "srColorSurface.h"
#include "srTexture.h"

// VTABLE: SURRENDER 0x1007752C
// class srClassSupport<srTextureFile, srTexture, 0, 8466>

/* File-backed texture. Wizardry does not use it; it owns a parallel stTextureFile with the same
   interface. */
// VTABLE: SURRENDER 0x100774E8 srTextureFile
class SR_DLL_EXPORT srTextureFile : public srClassSupport<srTextureFile, srTexture, 0, 0x2112> {
public:
    srTextureFile(const char* file_name = 0, int cached = 0);

    srTextureFile& operator=(const srTextureFile& other);

    // FUNCTION: SURRENDER 0x1005FF10
    static const char* sGetClassName()
    {
        return "srTextureFile";
    }

    const char* getFileName() const;
    void setFileName(const char* file_name);
    void setCached(int cached);
    int isSurfaceLoaded() const;
    void loadSurface();
    void releaseSurface();

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual unsigned long getTextureFrameHandle() override;
    virtual void getMipmapData(MultiRequest& request) override;
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void invalidate() override;

protected:
    virtual ~srTextureFile() override;
    virtual void setupDefaultValues() override;

    int cached;
    char* file_name;
    srColorSurfaceIFace* surface;
    unsigned long frame_handle;
};

static_assert(sizeof(srTextureFile) == 0x64, "srTextureFile_must_be_0x64");
