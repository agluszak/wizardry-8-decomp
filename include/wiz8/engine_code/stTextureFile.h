#pragma once

#include "surrender/srColorSurface.h"
#include "surrender/srTexture.h"

class stTextureFile;

/* The Truevision TGA file header consumed by the texture loader. The struct
   is naturally aligned, not packed: LoadSurfacePixels reads width at +0x0e,
   height at +0x10, pixel_depth at +0x12 and image_descriptor at +0x13, which
   fixes the 0x14-byte layout with pad bytes at +0x03 and +0x09. */
struct W8TgaHeader {
    unsigned char id_length;
    unsigned char color_map_type;
    unsigned char image_type;
    unsigned short color_map_origin;
    unsigned short color_map_length;
    unsigned char color_map_entry_size;
    unsigned short x_origin;
    unsigned short y_origin;
    unsigned short width;
    unsigned short height;
    unsigned char pixel_depth;
    unsigned char image_descriptor;
};

static_assert(sizeof(W8TgaHeader) == 20, "W8TgaHeader_must_be_20");

/* Both loaders clean their own stack arguments. LoadSurface's second
   parameter is a pointer the callee never dereferences; the sole caller
   passes the address of a zeroed dword. */
srColorSurface* __stdcall LoadSurface(int handle, long* unused_out);
void __stdcall LoadSurfacePixels(int handle, srColorSurface* surface, const W8TgaHeader* header);

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
    srColorSurface* getSurface() const
    {
        return surface_5c;
    }
    unsigned char hasAlpha() const
    {
        return has_alpha_64;
    }
    void loadSurface(); /* 0x0047BBF0 */
    /* Emitted at 0x47BBD0 for the materials.cpp probe caller and inlined at
       the invalidate/getMipmapData sites; the pending-defaults flag is part
       of the release sequence itself. */
    void releaseSurface(); /* 0x0047BBD0 */

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
    /* Masks for srTexture::e_flag dump bits. */
    enum TextureState {
        LOAD_FAILED = 1UL << FLAG_GENERATESURFACE_FAILURE,
        DEFAULTS_PENDING = 1UL << FLAG_DIRTY_DEFAULTS
    };

    int cached_54;
    char* file_name_58;
    srColorSurface* surface_5c;
    unsigned long frame_handle_60;
    bool has_alpha_64;
    unsigned char padding_65[3];
};

static_assert(sizeof(stTextureFile) == 0x68, "stTextureFile_must_be_0x68");
