#pragma once

#include "surrender/srTexture.h"

class W8ColorSurface005EBD10;
class stTextureFile;

/* Wizardry's virtual-file-backed texture. The method names are corroborated
   by the same 17-slot srTextureFile surface exported from SR.DLL; the distinct
   stTextureFile class name, class id and bodies are owned by the executable. */
class stTextureFile
    : public srClassSupport<stTextureFile, srTexture, 0, 0x10001> {
public:
    static const char* sGetClassName() { return "stTextureFile"; }

    stTextureFile(const char* file_name, int cached);       /* 0x0047C630 */
    stTextureFile& operator=(const stTextureFile& other);

    const char* getFileName() const { return file_name_58; }
    void setFileName(const char* file_name);                /* 0x0047C830 */
    void setCached(int cached) { cached_54 = cached; }
    int isSurfaceLoaded() const { return surface_5c != 0; }
    unsigned char hasAlpha() const { return has_alpha_64; }
    void loadSurface();                                    /* 0x0047BBF0 */
    void releaseSurface();

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;                  /* 0x0047C7A0 */
    virtual unsigned long getTextureFrameHandle() override; /* 0x0047C5F0 */
    virtual void getMipmapData(MultiRequest& request) override; /* 0x0047CA50 */
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void invalidate() override;                     /* 0x0047C8B0 */

protected:
    virtual ~stTextureFile() override;                      /* 0x0047C8E0 */
    virtual void setupDefaultValues() override;             /* 0x0047C600 */

private:
    enum TextureState {
        LOAD_FAILED = 0x01,
        DEFAULTS_PENDING = 0x02
    };

    W8ColorSurface005EBD10* LoadSurface0047C090(
        int handle, int* image_type);

    int cached_54;
    char* file_name_58;
    W8ColorSurface005EBD10* surface_5c;
    unsigned long frame_handle_60;
    unsigned char has_alpha_64;
    unsigned char padding_65[3];
};

static_assert(sizeof(stTextureFile) == 0x68,
              "stTextureFile_must_be_0x68");
