#include "surrender/srTextureMap.h"

#include "surrender/srHeap.h"

// FUNCTION: SURRENDER 0x10060270
srTextureMap::srTextureMap(srColorSurfaceIFace* surface)
{
    frame_handle_58_ = getNewFrameHandle();
    setSurfacePtr(surface);
}

/* Retail delegates to operator= then memberwise-copies its own members;
   the surface_54_ tail uses srPtr copy-constructor semantics (addref, no
   release), which a source-level member assignment cannot reproduce. */
// SYNTHETIC: SURRENDER 0x100607E0
// srTextureMap::srTextureMap (implicit copy constructor)

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

// SYNTHETIC: SURRENDER 0x100608C0
// srTextureMap default constructor closure

// TEMPLATE: SURRENDER 0x100605A0
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassID

// TEMPLATE: SURRENDER 0x100605B0
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassName

// TEMPLATE: SURRENDER 0x100605C0
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassNode

// TEMPLATE: SURRENDER 0x10060650
// srClassSupport<srTextureMap, srTexture, 0, 0x2111>::vClone

// TEMPLATE: SURRENDER 0x10060670
// srClientSupport<srTextureMap, 0x2111>::~srClientSupport

// SYNTHETIC: SURRENDER 0x10060760
// srPtr element destructor emission

// SYNTHETIC: SURRENDER 0x100608D0
// srTextureMap scalar deleting destructor

// SYNTHETIC: SURRENDER 0x100608F0
// srTextureMap::`vector deleting destructor'

// SYNTHETIC: SURRENDER 0x10060950
// srClassSupport<srTextureMap,srTexture,0,8465>::`scalar deleting destructor'

// SYNTHETIC: SURRENDER 0x10060980
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0x10060990
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0x100609C0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0x100609D0
// std::_Winit global atexit registrar
