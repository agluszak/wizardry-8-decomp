#include "surrender/srTextureMap.h"

#include "surrender/srHeap.h"

// FUNCTION: SURRENDER 0x10060270
srTextureMap::srTextureMap(srColorSurfaceIFace* surface)
{
    frame_handle_58_ = getNewFrameHandle();
    setSurfacePtr(surface);
}

// FUNCTION: SURRENDER 0x10060210
srTextureMap& srTextureMap::operator=(const srTextureMap& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        surface_54_ = other.surface_54_;
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
    if (surface_54_ != 0) {
        request.destinations[request.mipmap_level]->copy(*surface_54_.get());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
        /* Retail compares the signed level against the unsigned last level
           (JA branch); the mixed-sign spelling is part of the body. */
        for (long level = request.mipmap_level + 1; level <= request.last_level_04; ++level) {
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
    if (surface != surface_54_) {
        invalidate();
        surface_54_ = surface;
        setupDefaultValues();
    }
}

// FUNCTION: SURRENDER 0x10060490
srColorSurfaceIFace* srTextureMap::getSurfacePtr() const
{
    return surface_54_;
}

// FUNCTION: SURRENDER 0x100604A0
void srTextureMap::invalidate()
{
    invalidateFrameHandle(frame_handle_58_);
}

// FUNCTION: SURRENDER 0x100604B0
unsigned long srTextureMap::getTextureFrameHandle()
{
    return frame_handle_58_;
}

// FUNCTION: SURRENDER 0x100604C0
void srTextureMap::setupDefaultValues()
{
    setupDefaultValuesFromSurface(surface_54_);
    texture_flags_ &= ~(1 << FLAG_DIRTY_DEFAULTS);
}

// FUNCTION: SURRENDER 0x10060520
void srTextureMap::dump(std::ostream& stream)
{
    srTexture::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Surface: " << surface_54_->getName() << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x10060780
srClass* srTextureMap::vInstance()
{
    return new srTextureMap(0);
}
