#include "surrender/srTextureMap.h"

// FUNCTION: SURRENDER 0x10060270
srTextureMap::srTextureMap(srColorSurfaceIFace* surface)
{
    surface_54_ = 0;
    frame_handle_58_ = getNewFrameHandle();
    setSurfacePtr(surface);
}

/* invalidate() runs before the swap and setupDefaultValues() after; the
   re-check inside guards against a recursive invalidation swapping first. */
// FUNCTION: SURRENDER 0x100604E0
void srTextureMap::setSurfacePtr(srColorSurfaceIFace* surface)
{
    if (surface != surface_54_) {
        invalidate();
        if (surface != surface_54_) {
            if (surface != 0) {
                surface->addReference();
            }
            if (surface_54_ != 0) {
                surface_54_->release();
            }
            surface_54_ = surface;
        }
        setupDefaultValues();
    }
}
