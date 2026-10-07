#include "surrender/srTextureMap.h"

#include "surrender/srHeap.h"

#include <ostream>

// FUNCTION: SURRENDER 0x10060270
srTextureMap::srTextureMap(srColorSurfaceIFace* surface)
{
    frame_handle = getNewFrameHandle();
    setSurfacePtr(surface);
}

// FUNCTION: SURRENDER 0x10060210
srTextureMap& srTextureMap::operator=(const srTextureMap& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        surface = other.surface;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10060380
srTextureMap::~srTextureMap()
{
    invalidate();
}

// FUNCTION: SURRENDER 0x100601C0
void srTextureMap::getMipmapData(MultiRequest& request)
{
    if (surface != 0) {
        request.destinations[request.mipmap_level]->copy(*surface.get());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
        for (long level = request.mipmap_level + 1; level <= request.last_level; ++level) {
            request.destinations[level]->copy(*request.destinations[level - 1]);
        }
#pragma clang diagnostic pop
    }
}

/* The guard repeats inside srPtr::operator=(T*), which is also the re-check
   after invalidate() — a recursive invalidation can swap the surface first. */
// FUNCTION: SURRENDER 0x100604E0
void srTextureMap::setSurfacePtr(srColorSurfaceIFace* surface)
{
    if (surface != this->surface) {
        invalidate();
        this->surface = surface;
        setupDefaultValues();
    }
}

// FUNCTION: SURRENDER 0x10060490
srColorSurfaceIFace* srTextureMap::getSurfacePtr() const
{
    return surface;
}

// FUNCTION: SURRENDER 0x100604A0
void srTextureMap::invalidate()
{
    invalidateFrameHandle(frame_handle);
}

// FUNCTION: SURRENDER 0x100604B0
unsigned long srTextureMap::getTextureFrameHandle()
{
    return frame_handle;
}

// FUNCTION: SURRENDER 0x100604C0
void srTextureMap::setupDefaultValues()
{
    setupDefaultValuesFromSurface(surface);
    texture_flags_ &= ~(1 << FLAG_DIRTY_DEFAULTS);
}

// FUNCTION: SURRENDER 0x10060520
void srTextureMap::dump(std::ostream& stream)
{
    srTexture::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Surface: " << surface->getName() << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x10060780
srClass* srTextureMap::vInstance()
{
    return new srTextureMap(0);
}
