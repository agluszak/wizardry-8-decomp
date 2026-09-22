#include "surrender/srGERD.h"

#include "surrender/srCriticalSection.h"

namespace {

// RAII critical-section guard: the locked setters carry an EH funclet that
// runs LeaveCriticalSection on unwind, matching an object of this shape.
class GerdAccess {
public:
    GerdAccess(srCriticalSection* critical_section) : critical_section_(critical_section)
    {
        critical_section_->getAccess();
    }

    ~GerdAccess()
    {
        critical_section_->releaseAccess();
    }

private:
    srCriticalSection* critical_section_;
};

} // namespace

// FUNCTION: SURRENDER 0x10018650
void srGERD::setTextureDefaultMagFilter(srTextureIFace::e_filter filter)
{
    GerdAccess access(state_section_18_);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    default_mag_filter_1fb8_ = filter;
    mag_filter_param_1f7c_ = mag_filter_map_1f6c_[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100186D0
void srGERD::setTextureDefaultMinFilter(srTextureIFace::e_filter filter)
{
    GerdAccess access(state_section_18_);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    default_min_filter_1fbc_ = filter;
    min_filter_param_1f90_ = min_filter_map_1f80_[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x10018750
void srGERD::setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap)
{
    GerdAccess access(state_section_18_);
    if (mipmap == srTextureIFace::MIPMAP_DEFAULT) {
        mipmap = srTextureIFace::MIPMAP_FASTEST;
    }
    default_mipmap_1fc0_ = mipmap;
    mipmap_param_1fa0_ = mipmap_map_1f94_[mipmap];
    resetTexture();
}
