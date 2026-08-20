#include "wiz8/engine_code/stTextureFile.h"

#include "FileMan.h"
#include "surrender/srPalette.h"
#include "wiz8/virtual_file.h"

#include <string.h>

extern unsigned char Function489A80(const void* texture);
extern void Function4881D0();

// VTABLE: WIZ8 0x005EC5F8
// class stTextureFile

// VTABLE: WIZ8 0x005EC63C
// class srClassSupport<stTextureFile,srTexture,0,65537>

// TEMPLATE: WIZ8 0x0047D6D0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassID

// TEMPLATE: WIZ8 0x0047D6E0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassName

// TEMPLATE: WIZ8 0x0047D6F0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassNode

// TEMPLATE: WIZ8 0x0047D790
// srClassSupport<stTextureFile,srTexture,0,65537>::clone

// TEMPLATE: WIZ8 0x0047D870
// srClassSupport<stTextureFile,srTexture,0,65537>::~srClassSupport<stTextureFile,srTexture,0,65537>

// SYNTHETIC: WIZ8 0x0047D970
// srClassSupport<stTextureFile,srTexture,0,65537>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0047D9A0
// srClassSupport<stTextureFile,srTexture,0,65537>::sGetClassNode

// FUNCTION: WIZ8 0x0047C630
stTextureFile::stTextureFile(const char* file_name, int cached)
    : cached_54(cached),
      file_name_58(0),
      surface_5c(0),
      frame_handle_60(getNewFrameHandle()),
      has_alpha_64(0)
{
    invalidate();
    setFileName(file_name);
    if (file_name != 0) {
        setName(file_name);
    }
    if (cached_54 != 0 && file_name_58 != 0) {
        setupDefaultValues();
    }
}

stTextureFile& stTextureFile::operator=(const stTextureFile& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        setFileName(other.file_name_58);
        cached_54 = other.cached_54;
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047C5F0
unsigned long stTextureFile::getTextureFrameHandle()
{
    if ((texture_flags_ & LOAD_FAILED) != 0) {
        return 0;
    }
    return frame_handle_60;
}

// FUNCTION: WIZ8 0x0047C600
void stTextureFile::setupDefaultValues()
{
    if ((texture_flags_ & DEFAULTS_PENDING) == 0) {
        return;
    }

    texture_flags_ &= ~DEFAULTS_PENDING;
    if (surface_5c == 0) {
        loadSurface();
    }
    setupDefaultValuesFromSurface(surface_5c);
}

// FUNCTION: WIZ8 0x0047C7A0
srClass* stTextureFile::vInstance()
{
    return new stTextureFile(0, 0);
}

// SYNTHETIC: WIZ8 0x0047C800
// stTextureFile::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0047C830
void stTextureFile::setFileName(const char* file_name)
{
    invalidate();
    delete[] file_name_58;
    file_name_58 = 0;

    if (file_name != 0 && file_name[0] != 0) {
        file_name_58 = new char[strlen(file_name) + 1];
        strcpy(file_name_58, file_name);
    }

    texture_flags_ &= ~LOAD_FAILED;
    texture_flags_ |= DEFAULTS_PENDING;
}

void stTextureFile::releaseSurface()
{
    if (surface_5c != 0) {
        surface_5c->release();
        surface_5c = 0;
    }
}

// FUNCTION: WIZ8 0x0047C8B0
void stTextureFile::invalidate()
{
    releaseSurface();
    texture_flags_ |= DEFAULTS_PENDING;
    invalidateFrameHandle(frame_handle_60);
    texture_flags_ &= ~LOAD_FAILED;
}

// FUNCTION: WIZ8 0x0047C8E0
stTextureFile::~stTextureFile()
{
    if (Function489A80(this) != 0) {
        Function4881D0();
    }
    invalidate();
    delete[] file_name_58;
    file_name_58 = 0;
    texture_flags_ &= ~LOAD_FAILED;
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047BBF0
void stTextureFile::loadSurface()
{
    if (surface_5c != 0) {
        invalidate();
    }
    if (file_name_58 == 0) {
        return;
    }

    int image_type = 0;
    int handle = FileOpen(file_name_58, 0x41, 0);
    if (handle != 0) {
        surface_5c = LoadSurface0047C090(handle, &image_type);
        CloseVirtualFile(handle);
    }

    if (surface_5c == 0) {
        texture_flags_ |= LOAD_FAILED;
        return;
    }

    setupDefaultValues();
    surface_5c->setFilter(getFilter());
}

// FUNCTION: WIZ8 0x0047CA50
void stTextureFile::getMipmapData(MultiRequest& request)
{
    if (surface_5c == 0) {
        loadSurface();
    }
    if (surface_5c == 0) {
        return;
    }

    long level = request.mipmap_level;
    if (request.destinations[level] != 0) {
        request.destinations[level]->copy(*surface_5c);
    }
    for (++level; level <= static_cast<long>(request.unknown_04); ++level) {
        if (request.destinations[level] != 0 &&
            request.destinations[level - 1] != 0) {
            request.destinations[level]->copy(
                *request.destinations[level - 1]);
        }
    }

    if (cached_54 == 0) {
        releaseSurface();
        texture_flags_ |= DEFAULTS_PENDING;
    }
}

void stTextureFile::getMipmapLevelPartial(PartialRequest&)
{
}

void stTextureFile::dump(std::ostream&)
{
}
