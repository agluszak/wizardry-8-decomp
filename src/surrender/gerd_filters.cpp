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

// FUNCTION: SURRENDER 0x10018550
void srGERD::setTextureDefaultCorrection(srTextureIFace::e_correction correction)
{
    GerdAccess access(state_section_18_);
    if (correction == srTextureIFace::CORRECTION_DEFAULT) {
        correction = srTextureIFace::CORRECTION_GOOD;
    }
    default_correction_1fb4_ = correction;
    correction_map_1f5c_[srTextureIFace::CORRECTION_DEFAULT] = correction_map_1f5c_[correction];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100185D0
void srGERD::setTextureDefaultCompression(srTextureIFace::e_compression compression)
{
    GerdAccess access(state_section_18_);
    /* COMPRESSION_DEFAULT (4) collapses to entry 0 through the mask. */
    compression = static_cast<srTextureIFace::e_compression>(
        compression & ((compression == srTextureIFace::COMPRESSION_DEFAULT) - 1));
    default_compression_1fd8_ = compression;
    default_texture_params_1fc4_[4] = default_texture_params_1fc4_[compression];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100187D0
srTextureIFace::e_compression srGERD::getTextureDefaultCompression() const
{
    GerdAccess access(state_section_18_);
    return default_compression_1fd8_;
}

// FUNCTION: SURRENDER 0x100187F0
srTextureIFace::e_correction srGERD::getTextureDefaultCorrection() const
{
    GerdAccess access(state_section_18_);
    return default_correction_1fb4_;
}

// FUNCTION: SURRENDER 0x10018810
srTextureIFace::e_filter srGERD::getTextureDefaultMagFilter() const
{
    GerdAccess access(state_section_18_);
    return default_mag_filter_1fb8_;
}

// FUNCTION: SURRENDER 0x10018830
srTextureIFace::e_filter srGERD::getTextureDefaultMinFilter() const
{
    GerdAccess access(state_section_18_);
    return default_min_filter_1fbc_;
}

// FUNCTION: SURRENDER 0x10018850
srTextureIFace::e_mipmap srGERD::getTextureDefaultMipmap() const
{
    GerdAccess access(state_section_18_);
    return default_mipmap_1fc0_;
}

// FUNCTION: SURRENDER 0x10018650
void srGERD::setTextureDefaultMagFilter(srTextureIFace::e_filter filter)
{
    GerdAccess access(state_section_18_);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    default_mag_filter_1fb8_ = filter;
    mag_filter_map_1f6c_[srTextureIFace::FILTER_DEFAULT] = mag_filter_map_1f6c_[filter];
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
    min_filter_map_1f80_[srTextureIFace::FILTER_DEFAULT] = min_filter_map_1f80_[filter];
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
    mipmap_map_1f94_[3] = mipmap_map_1f94_[mipmap];
    resetTexture();
}
