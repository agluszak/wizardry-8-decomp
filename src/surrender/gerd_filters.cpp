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
    texture_state_1f5c_.default_correction_58_ = correction;
    texture_state_1f5c_.correction_map_00_[srTextureIFace::CORRECTION_DEFAULT] = texture_state_1f5c_.correction_map_00_[correction];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100185D0
void srGERD::setTextureDefaultCompression(srTextureIFace::e_compression compression)
{
    GerdAccess access(state_section_18_);
    /* COMPRESSION_DEFAULT (4) collapses to entry 0 through the mask. */
    compression = static_cast<srTextureIFace::e_compression>(
        compression & ((compression == srTextureIFace::COMPRESSION_DEFAULT) - 1));
    texture_state_1f5c_.default_compression_7c_ = compression;
    texture_state_1f5c_.default_texture_params_68_[4] = texture_state_1f5c_.default_texture_params_68_[compression];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100187D0
srTextureIFace::e_compression srGERD::getTextureDefaultCompression() const
{
    GerdAccess access(state_section_18_);
    return texture_state_1f5c_.default_compression_7c_;
}

// FUNCTION: SURRENDER 0x100187F0
srTextureIFace::e_correction srGERD::getTextureDefaultCorrection() const
{
    GerdAccess access(state_section_18_);
    return texture_state_1f5c_.default_correction_58_;
}

// FUNCTION: SURRENDER 0x10018810
srTextureIFace::e_filter srGERD::getTextureDefaultMagFilter() const
{
    GerdAccess access(state_section_18_);
    return texture_state_1f5c_.default_mag_filter_5c_;
}

// FUNCTION: SURRENDER 0x10018830
srTextureIFace::e_filter srGERD::getTextureDefaultMinFilter() const
{
    GerdAccess access(state_section_18_);
    return texture_state_1f5c_.default_min_filter_60_;
}

// FUNCTION: SURRENDER 0x10018850
srTextureIFace::e_mipmap srGERD::getTextureDefaultMipmap() const
{
    GerdAccess access(state_section_18_);
    return texture_state_1f5c_.default_mipmap_64_;
}

// FUNCTION: SURRENDER 0x10018650
void srGERD::setTextureDefaultMagFilter(srTextureIFace::e_filter filter)
{
    GerdAccess access(state_section_18_);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    texture_state_1f5c_.default_mag_filter_5c_ = filter;
    texture_state_1f5c_.mag_filter_map_10_[srTextureIFace::FILTER_DEFAULT] = texture_state_1f5c_.mag_filter_map_10_[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100186D0
void srGERD::setTextureDefaultMinFilter(srTextureIFace::e_filter filter)
{
    GerdAccess access(state_section_18_);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    texture_state_1f5c_.default_min_filter_60_ = filter;
    texture_state_1f5c_.min_filter_map_24_[srTextureIFace::FILTER_DEFAULT] = texture_state_1f5c_.min_filter_map_24_[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x10018750
void srGERD::setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap)
{
    GerdAccess access(state_section_18_);
    if (mipmap == srTextureIFace::MIPMAP_DEFAULT) {
        mipmap = srTextureIFace::MIPMAP_FASTEST;
    }
    texture_state_1f5c_.default_mipmap_64_ = mipmap;
    texture_state_1f5c_.mipmap_map_38_[3] = texture_state_1f5c_.mipmap_map_38_[mipmap];
    resetTexture();
}
