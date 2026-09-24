#include "surrender/srTextureFile.h"

#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srImporter.h"

// FUNCTION: SURRENDER 0x1005F8E0
srTextureFile::srTextureFile(const char* file_name, int cached)
    : cached_54(0), file_name_58(0), surface_5c(0), frame_handle_60(getNewFrameHandle())
{
    cached_54 = 0;
    if (cached != 0) {
        cached_54 = 1;
    }
    setFileName(file_name);
    if (file_name != 0) {
        setName(file_name);
    }
    if (cached != 0 && file_name != 0) {
        setupDefaultValues();
    }
}

/* Retail delegates to operator= then overwrites the fresh members with a
   raw memberwise copy of the source — file_name_58 is shared, not
   re-duplicated. */
// FUNCTION: SURRENDER 0x1005FF80
srTextureFile::srTextureFile(const srTextureFile& other)
{
    *this = other;
    cached_54 = other.cached_54;
    file_name_58 = other.file_name_58;
    surface_5c = other.surface_5c;
    frame_handle_60 = other.frame_handle_60;
}

// FUNCTION: SURRENDER 0x1005F780
srTextureFile& srTextureFile::operator=(const srTextureFile& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        setFileName(0);
        if (other.file_name_58 != 0) {
            setFileName(other.file_name_58);
        }
        cached_54 = other.cached_54;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005FAC0
srTextureFile::~srTextureFile()
{
    invalidate();
    setFileName(0);
}

// FUNCTION: SURRENDER 0x1005FF10
const char* srTextureFile::sGetClassName()
{
    return "srTextureFile";
}

// FUNCTION: SURRENDER 0x1005FF20
srClass* srTextureFile::vInstance()
{
    return new srTextureFile(0, 0);
}

// FUNCTION: SURRENDER 0x1005F9F0
const char* srTextureFile::getFileName() const
{
    return file_name_58;
}

/* The filename is an ordinary CRT string: retail allocates it with the
   scalar global operator new, not the SurRender heap or vector new. */
// FUNCTION: SURRENDER 0x1005FA00
void srTextureFile::setFileName(const char* file_name)
{
    invalidate();
    if (file_name_58 != 0) {
        delete file_name_58;
    }
    if (file_name == 0 || *file_name == 0) {
        file_name_58 = 0;
    } else {
        file_name_58 = static_cast<char*>(::operator new(strlen(file_name) + 1));
        memcpy(file_name_58, file_name, strlen(file_name) + 1);
    }
    texture_flags_ &= ~1;
    texture_flags_ |= 1 << FLAG_DIRTY_DEFAULTS;
}

// FUNCTION: SURRENDER 0x1005FAA0
void srTextureFile::setCached(int cached)
{
    if (cached != 0) {
        cached_54 |= 1;
        return;
    }
    cached_54 &= ~1;
}

// FUNCTION: SURRENDER 0x1005FF00
int srTextureFile::isSurfaceLoaded() const
{
    return surface_5c != 0;
}

// FUNCTION: SURRENDER 0x1005F7E0
void srTextureFile::loadSurface()
{
    if (surface_5c != 0) {
        invalidate();
    }
    if (file_name_58 != 0) {
        surface_5c = 0;
        srSurfaceIOManager::ImportInfo info;
        info.unknown_00 = 0;
        surface_5c = srCore.getSurfaceIOManager()->importSurface(file_name_58, info);
    }
}

// FUNCTION: SURRENDER 0x1005F7C0
void srTextureFile::releaseSurface()
{
    if (surface_5c != 0) {
        surface_5c->release();
        surface_5c = 0;
    }
}

// FUNCTION: SURRENDER 0x1005F8A0
unsigned long srTextureFile::getTextureFrameHandle()
{
    if ((texture_flags_ & (1 << FLAG_GENERATESURFACE_FAILURE)) != 0) {
        return 0;
    }
    return frame_handle_60;
}

// FUNCTION: SURRENDER 0x1005FA80
void srTextureFile::invalidate()
{
    releaseSurface();
    invalidateFrameHandle(frame_handle_60);
    texture_flags_ &= ~1;
}

// FUNCTION: SURRENDER 0x1005F8B0
void srTextureFile::setupDefaultValues()
{
    if ((texture_flags_ & (1 << FLAG_DIRTY_DEFAULTS)) != 0) {
        texture_flags_ &= ~(1 << FLAG_DIRTY_DEFAULTS);
        if (surface_5c == 0) {
            loadSurface();
        }
        setupDefaultValuesFromSurface(surface_5c);
    }
}

// FUNCTION: SURRENDER 0x1005FCB0
void srTextureFile::getMipmapData(MultiRequest& request)
{
    if (surface_5c == 0) {
        loadSurface();
    }
    if (surface_5c != 0 && request.destinations[request.mipmap_level] != 0) {
        request.destinations[request.mipmap_level]->copy(*surface_5c);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
        /* Retail compares the signed level against the unsigned last level
           (JA branch); the mixed-sign spelling is part of the body. */
        for (long level = request.mipmap_level + 1; level <= request.last_level_04; ++level) {
            if (request.destinations[level] != 0 && request.destinations[level - 1] != 0) {
                request.destinations[level]->copy(*request.destinations[level - 1]);
            }
        }
#pragma clang diagnostic pop
        if ((cached_54 & 1) == 0) {
            releaseSurface();
        }
    }
}

// FUNCTION: SURRENDER 0x1005FD30
void srTextureFile::getMipmapLevelPartial(PartialRequest& request) {}

// FUNCTION: SURRENDER 0x1005FBD0
void srTextureFile::dump(std::ostream& stream)
{
    srTexture::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Filename: " << file_name_58 << '\n';
    stream.width(0x20);
    stream << "  Caching enabled: " << ((cached_54 & 1) != 0 ? "yes\n" : "no\n");
    stream.width(0x20);
    stream << "  Cached: " << (surface_5c != 0 ? "yes\n" : "no\n");
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}
