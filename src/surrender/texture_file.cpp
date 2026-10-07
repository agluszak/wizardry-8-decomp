#include "surrender/srTextureFile.h"

#include <ostream>
#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srImporter.h"

// FUNCTION: SURRENDER 0x1005F8E0
srTextureFile::srTextureFile(const char* file_name, int cached)
    : cached(0), file_name(0), surface(0), frame_handle(getNewFrameHandle())
{
    this->cached = 0;
    if (cached != 0) {
        this->cached = 1;
    }
    setFileName(file_name);
    if (file_name != 0) {
        setName(file_name);
    }
    if (cached != 0 && file_name != 0) {
        setupDefaultValues();
    }
}

// FUNCTION: SURRENDER 0x1005F780
srTextureFile& srTextureFile::operator=(const srTextureFile& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        setFileName(0);
        if (other.file_name != 0) {
            setFileName(other.file_name);
        }
        cached = other.cached;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005FAC0
srTextureFile::~srTextureFile()
{
    invalidate();
    setFileName(0);
}

// FUNCTION: SURRENDER 0x1005FF20
srClass* srTextureFile::vInstance()
{
    return new srTextureFile(0, 0);
}

// FUNCTION: SURRENDER 0x1005F9F0
const char* srTextureFile::getFileName() const
{
    return file_name;
}

// FUNCTION: SURRENDER 0x1005FA00
void srTextureFile::setFileName(const char* file_name)
{
    invalidate();
    if (this->file_name != 0) {
        delete this->file_name;
    }
    if (file_name == 0 || *file_name == 0) {
        this->file_name = 0;
    } else {
        this->file_name = static_cast<char*>(::operator new(strlen(file_name) + 1));
        memcpy(this->file_name, file_name, strlen(file_name) + 1);
    }
    texture_flags_ &= ~1;
    texture_flags_ |= 1 << FLAG_DIRTY_DEFAULTS;
}

// FUNCTION: SURRENDER 0x1005FAA0
void srTextureFile::setCached(int cached)
{
    if (cached != 0) {
        this->cached |= 1;
        return;
    }
    this->cached &= ~1;
}

// FUNCTION: SURRENDER 0x1005FF00
int srTextureFile::isSurfaceLoaded() const
{
    return surface != 0;
}

// FUNCTION: SURRENDER 0x1005F7E0
void srTextureFile::loadSurface()
{
    if (surface != 0) {
        invalidate();
    }
    if (file_name != 0) {
        surface = 0;
        try {
            srSurfaceIOManager::ImportInfo info;
            info.unknown_00 = 0;
            surface = srCore.getSurfaceIOManager()->importSurface(file_name, info);
        } catch (const srIOManager::Error&) {
            texture_flags_ |= 1 << FLAG_GENERATESURFACE_FAILURE;
            surface = 0;
        } catch (...) {
            texture_flags_ |= 1 << FLAG_GENERATESURFACE_FAILURE;
            surface = 0;
        }
        if (surface != 0) {
            setupDefaultValues();
            surface->setFilter(getFilter());
        }
    }
}

// FUNCTION: SURRENDER 0x1005F7C0
void srTextureFile::releaseSurface()
{
    if (surface != 0) {
        surface->release();
        surface = 0;
    }
}

// FUNCTION: SURRENDER 0x1005F8A0
unsigned long srTextureFile::getTextureFrameHandle()
{
    if ((texture_flags_ & (1 << FLAG_GENERATESURFACE_FAILURE)) != 0) {
        return 0;
    }
    return frame_handle;
}

// FUNCTION: SURRENDER 0x1005FA80
void srTextureFile::invalidate()
{
    releaseSurface();
    invalidateFrameHandle(frame_handle);
    texture_flags_ &= ~1;
}

// FUNCTION: SURRENDER 0x1005F8B0
void srTextureFile::setupDefaultValues()
{
    if ((texture_flags_ & (1 << FLAG_DIRTY_DEFAULTS)) != 0) {
        texture_flags_ &= ~(1 << FLAG_DIRTY_DEFAULTS);
        if (surface == 0) {
            loadSurface();
        }
        setupDefaultValuesFromSurface(surface);
    }
}

// FUNCTION: SURRENDER 0x1005FCB0
void srTextureFile::getMipmapData(MultiRequest& request)
{
    if (surface == 0) {
        loadSurface();
    }
    if (surface != 0 && request.destinations[request.mipmap_level] != 0) {
        request.destinations[request.mipmap_level]->copy(*surface);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
        for (long level = request.mipmap_level + 1; level <= request.last_level; ++level) {
            if (request.destinations[level] != 0 && request.destinations[level - 1] != 0) {
                request.destinations[level]->copy(*request.destinations[level - 1]);
            }
        }
#pragma clang diagnostic pop
        if ((cached & 1) == 0) {
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
    stream << "  Filename: " << file_name << '\n';
    stream.width(0x20);
    stream << "  Caching enabled: " << ((cached & 1) != 0 ? "yes\n" : "no\n");
    stream.width(0x20);
    stream << "  Cached: " << (surface != 0 ? "yes\n" : "no\n");
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}
