#pragma once

#include "srColorSurface.h"
#include "srTexture.h"

/* File-backed texture exported by SR.DLL (class id 0x2112, parent srTexture
   0x2110). Wizardry does not import this type; it owns a parallel first-party
   stTextureFile (id 0x10001) whose 17-slot vtable and method names match this
   interface. SR's ctor writes cached/filename/surface/frame-handle at the same
   offsets stTextureFile uses; Wizardry then adds has_alpha at +0x64.

   This class overrides dump, dtor, vInstance, clone, getTextureFrameHandle,
   getMipmapData, getMipmapLevelPartial, invalidate and setupDefaultValues.
   Slots 9/10/13 (getPriority, getDimensions, getTextureParms) stay on
   srTexture; slot 14 (getTextureName) stays on srTextureIFace. */
class SR_DLL_IMPORT srTextureFile : public srClassSupport<srTextureFile, srTexture, 0, 0x2112> {
public:
    srTextureFile(const char* file_name, int cached);
    srTextureFile(const srTextureFile& other);
    srTextureFile& operator=(const srTextureFile& other);

    static const char* sGetClassName();

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

    int cached_54;
    char* file_name_58;
    srColorSurface* surface_5c;
    unsigned long frame_handle_60;
};

static_assert(sizeof(srTextureFile) == 0x64, "srTextureFile_must_be_0x64");
