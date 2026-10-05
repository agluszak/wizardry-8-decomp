#include "surrender/srGERD.h"

#include "surrender/srCriticalSection.h"

// FUNCTION: SURRENDER 0x10018550
void srGERD::setTextureDefaultCorrection(srTextureIFace::e_correction correction)
{
    srCriticalSectionAccess access(state_section);
    if (correction == srTextureIFace::CORRECTION_DEFAULT) {
        correction = srTextureIFace::CORRECTION_GOOD;
    }
    texture_state.default_correction = correction;
    texture_state.correction_map[srTextureIFace::CORRECTION_DEFAULT] =
        texture_state.correction_map[correction];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100185D0
void srGERD::setTextureDefaultCompression(srTextureIFace::e_compression compression)
{
    srCriticalSectionAccess access(state_section);
    /* COMPRESSION_DEFAULT (4) collapses to entry 0 through the mask. */
    compression = static_cast<srTextureIFace::e_compression>(
        compression & ((compression == srTextureIFace::COMPRESSION_DEFAULT) - 1));
    texture_state.default_compression = compression;
    texture_state.default_texture_params[4] = texture_state.default_texture_params[compression];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100187D0
srTextureIFace::e_compression srGERD::getTextureDefaultCompression() const
{
    srCriticalSectionAccess access(state_section);
    return texture_state.default_compression;
}

// FUNCTION: SURRENDER 0x100187F0
srTextureIFace::e_correction srGERD::getTextureDefaultCorrection() const
{
    srCriticalSectionAccess access(state_section);
    return texture_state.default_correction;
}

// FUNCTION: SURRENDER 0x10018810
srTextureIFace::e_filter srGERD::getTextureDefaultMagFilter() const
{
    srCriticalSectionAccess access(state_section);
    return texture_state.default_mag_filter;
}

// FUNCTION: SURRENDER 0x10018830
srTextureIFace::e_filter srGERD::getTextureDefaultMinFilter() const
{
    srCriticalSectionAccess access(state_section);
    return texture_state.default_min_filter;
}

// FUNCTION: SURRENDER 0x10018850
srTextureIFace::e_mipmap srGERD::getTextureDefaultMipmap() const
{
    srCriticalSectionAccess access(state_section);
    return texture_state.default_mipmap;
}

// FUNCTION: SURRENDER 0x10018650
void srGERD::setTextureDefaultMagFilter(srTextureIFace::e_filter filter)
{
    srCriticalSectionAccess access(state_section);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    texture_state.default_mag_filter = filter;
    texture_state.mag_filter_map[srTextureIFace::FILTER_DEFAULT] =
        texture_state.mag_filter_map[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x100186D0
void srGERD::setTextureDefaultMinFilter(srTextureIFace::e_filter filter)
{
    srCriticalSectionAccess access(state_section);
    if (filter == srTextureIFace::FILTER_DEFAULT) {
        filter = srTextureIFace::FILTER_GOOD;
    }
    texture_state.default_min_filter = filter;
    texture_state.min_filter_map[srTextureIFace::FILTER_DEFAULT] =
        texture_state.min_filter_map[filter];
    resetTexture();
}

// FUNCTION: SURRENDER 0x10018750
void srGERD::setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap)
{
    srCriticalSectionAccess access(state_section);
    if (mipmap == srTextureIFace::MIPMAP_DEFAULT) {
        mipmap = srTextureIFace::MIPMAP_FASTEST;
    }
    texture_state.default_mipmap = mipmap;
    texture_state.mipmap_map[3] = texture_state.mipmap_map[mipmap];
    resetTexture();
}
