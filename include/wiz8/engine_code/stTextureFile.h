#pragma once

#include "surrender/srColorSurface.h"
#include "surrender/srTexture.h"

class stTextureFile;

/* Wizardry's virtual-file-backed texture. SR.DLL exports a parallel
   srTextureFile (id 0x2112) whose 17-slot vtable is:

   0-2 class identity, 3 dump, 4 verify, 5 dtor, 6 vInstance, 7 clone,
   8 getTextureFrameHandle, 9 getPriority, 10 getDimensions, 11 getMipmapData,
   12 getMipmapLevelPartial, 13 getTextureParms, 14 getTextureName,
   15 invalidate, 16 setupDefaultValues.

   stTextureFile overrides the same slots SR overrides (3, 5-8, 11-12, 15-16).
   Slots 9/10/13 are inherited from srTexture; slot 14 from srTextureIFace.
   Do not invent stTextureFile overrides for those four. Fields at
   +0x54..+0x60 match SR; has_alpha_64 is Wizardry-only (SR sizeof 0x64). */
class stTextureFile : public srClassSupport<stTextureFile, srTexture, 0, 0x10001> {
public:
    static const char* sGetClassName()
    {
        return "stTextureFile";
    }

    stTextureFile(const char* file_name, int cached); /* 0x0047C630 */
    stTextureFile& operator=(const stTextureFile& other);

    const char* getFileName() const
    {
        return file_name_58;
    }
    void setFileName(const char* file_name); /* 0x0047C830 */
    void setCached(int cached)
    {
        cached_54 = cached;
    }
    int isSurfaceLoaded() const
    {
        return surface_5c != 0;
    }
    unsigned char hasAlpha() const
    {
        return has_alpha_64;
    }
    void loadSurface(); /* 0x0047BBF0 */
    void releaseSurface();

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;                      /* 0x0047C7A0 */
    virtual unsigned long getTextureFrameHandle() override;     /* 0x0047C5F0 */
    virtual void getMipmapData(MultiRequest& request) override; /* 0x0047CA50 */
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void invalidate() override; /* 0x0047C8B0 */

protected:
    virtual ~stTextureFile() override;          /* 0x0047C8E0 */
    virtual void setupDefaultValues() override; /* 0x0047C600 */

private:
    enum TextureState { LOAD_FAILED = 0x01, DEFAULTS_PENDING = 0x02 };

    srColorSurface* LoadSurface0047C090(int handle, int* image_type);

    int cached_54;
    char* file_name_58;
    srColorSurface* surface_5c;
    unsigned long frame_handle_60;
    unsigned char has_alpha_64;
    unsigned char padding_65[3];
};

static_assert(sizeof(stTextureFile) == 0x68, "stTextureFile_must_be_0x68");
