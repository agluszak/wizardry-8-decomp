#include "surrender/srTexture.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srPixelConvert.h"

// GLOBAL: SURRENDER 0x100A4A20
// Lazy e_flag names, "GENERATESURFACE_FAILURE,DIRTY_DEFAULTS"
static const char* s_flag_names_100a4a20;

// GLOBAL: SURRENDER 0x100A4A1C
// srTexture::_frameHandle
unsigned long srTexture::_frameHandle;

/* Same comma-separated bit-name walker srNode::dump uses; each TU keeps its
   own copy. */
static void dumpFlags(std::ostream& stream, unsigned long flags, const char* names)
{
    if (flags == 0) {
        stream << "[NONE]";
        return;
    }
    stream << '[';
    bool first = true;
    for (unsigned long bit = 0; bit < 0x20; ++bit) {
        if ((flags & (1 << bit)) != 0) {
            if (first) {
                first = false;
            } else {
                stream << ',';
            }
            if (names == 0 || *names == 0) {
                stream << bit;
            } else {
                while (*names != 0 && *names != ',') {
                    stream << *names++;
                }
                if (*names == ',') {
                    ++names;
                }
            }
        } else if (names != 0) {
            while (*names != 0 && *names != ',') {
                ++names;
            }
            if (*names == ',') {
                ++names;
            }
        }
    }
    stream << ']';
}

// FUNCTION: SURRENDER 0x1005EDE0
const char* srTexture::sGetClassName()
{
    return "srTexture";
}

// FUNCTION: SURRENDER 0x1005E440
srTexture::srTexture()
{
    texture_dimensions_.format.flags = 0;
    texture_dimensions_.hints = 0;
    texture_dimensions_.width = 0x40;
    texture_dimensions_.height = 0x40;
    texture_dimensions_.palette = srCore.getPalette();
    texture_dimensions_.filter = srCore.getFilter();
    texture_dimensions_.hints = 0;
    texture_dimensions_.compression = srTextureIFace::COMPRESSION_DEFAULT;
    srPixelConvert::mapPixelFormat(static_cast<srPixelConvert::e_surfaceType>(0xb),
                                   texture_dimensions_.format);
    texture_flags_ = 0;
    if (s_flag_names_100a4a20 == 0) {
        s_flag_names_100a4a20 = "GENERATESURFACE_FAILURE,DIRTY_DEFAULTS";
    }
    mipmap_bias_1c = 0.0f;
    packed_state_18 = 0xe43;
    texture_priority_4c = 0.5f;
    texture_flags_ |= 1 << FLAG_DIRTY_DEFAULTS;
}

// FUNCTION: SURRENDER 0x1005E570
srTexture::~srTexture() {}

// FUNCTION: SURRENDER 0x1005E390
srTexture& srTexture::operator=(const srTexture& other)
{
    if (this != &other) {
        invalidate();
        srClass::operator=(other);
        packed_state_18 = other.packed_state_18;
        mipmap_bias_1c = other.mipmap_bias_1c;
        texture_priority_4c = other.texture_priority_4c;
        texture_dimensions_ = other.texture_dimensions_;
        texture_flags_ = other.texture_flags_;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1005E690
void srTexture::dump(std::ostream& stream)
{
    srClass::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Filter: " << getFilter()->getName() << '\n';
    const char* name;
    e_correction correction = getCorrection();
    e_filter mag_filter = getMagFilter();
    e_filter min_filter = getMinFilter();
    e_mipmap mipmap = getMipmap();
    e_wrap wrap_s = getWrapS();
    e_wrap wrap_t = getWrapT();
    stream.width(0x20);
    stream << "  Correction: ";
    switch (correction) {
    case CORRECTION_FASTEST:
        name = "FASTEST";
        break;
    case CORRECTION_GOOD:
        name = "GOOD";
        break;
    case CORRECTION_BEST:
        name = "BEST";
        break;
    default:
        name = "DEFAULT";
        break;
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Mag. filter: ";
    switch (mag_filter) {
    case FILTER_NONE:
        name = "NONE";
        break;
    case FILTER_FASTEST:
        name = "FASTEST";
        break;
    case FILTER_GOOD:
        name = "GOOD";
        break;
    case FILTER_BEST:
        name = "BEST";
        break;
    default:
        name = "DEFAULT";
        break;
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Min. filter: ";
    switch (min_filter) {
    case FILTER_NONE:
        name = "NONE";
        break;
    case FILTER_FASTEST:
        name = "FASTEST";
        break;
    case FILTER_GOOD:
        name = "GOOD";
        break;
    case FILTER_BEST:
        name = "BEST";
        break;
    default:
        name = "DEFAULT";
        break;
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Mipmap: ";
    switch (mipmap) {
    case MIPMAP_NONE:
        name = "NONE";
        break;
    case MIPMAP_FASTEST:
        name = "FASTEST";
        break;
    case MIPMAP_BEST:
        name = "BEST";
        break;
    default:
        name = "DEFAULT";
        break;
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Wrap S: ";
    if (wrap_s == WRAP_REPEAT) {
        name = "REPEAT";
    } else {
        name = "CLAMP";
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Wrap T: ";
    if (wrap_t == WRAP_REPEAT) {
        name = "REPEAT";
    } else {
        name = "CLAMP";
    }
    stream << name << '\n';
    stream.width(0x20);
    stream << "  Mipmap bias: " << mipmap_bias_1c << '\n';
    stream.width(0x20);
    stream << "  Priority: ";
    stream.width(0x20);
    stream << getPriority() << '\n';
    stream.width(0x20);
    stream << "  Flags: ";
    dumpFlags(stream, texture_flags_, s_flag_names_100a4a20);
    stream << '\n';
    stream.flags(static_cast<std::ios::fmtflags>(flags & 0x7fff));
}

// FUNCTION: SURRENDER 0x1005EEF0
unsigned long srTexture::getNewFrameHandle()
{
    _frameHandle = _frameHandle + 1;
    return _frameHandle;
}

// FUNCTION: SURRENDER 0x1005EA90
void srTexture::invalidateFrameHandle(unsigned long handle)
{
    for (srGERD* device = srGERD::getFirstOpen(); device != 0; device = device->getNextOpen()) {
        device->invalidateTextureByFrameHandle(handle);
    }
}

// FUNCTION: SURRENDER 0x1005EB50
void srTexture::setDimensions(const Dimensions& dimensions)
{
    texture_dimensions_ = dimensions;
    texture_flags_ &= ~(1 << FLAG_DIRTY_DEFAULTS);
}

// FUNCTION: SURRENDER 0x1005EBE0
void srTexture::setupDefaultValuesFromSurface(srColorSurfaceIFace* surface)
{
    if (surface != 0) {
        texture_dimensions_.width = surface->getWidth();
        texture_dimensions_.height = surface->getHeight();
        surface->getPixelFormat(texture_dimensions_.format);
        texture_dimensions_.palette = surface->getPalette();
    } else {
        texture_dimensions_.width = 1;
        texture_dimensions_.height = 1;
        texture_dimensions_.palette = 0;
    }
}

// FUNCTION: SURRENDER 0x1005EB00
srFilter* srTexture::getFilter() const
{
    return texture_dimensions_.filter;
}

// FUNCTION: SURRENDER 0x1005EB20
float srTexture::getPriority()
{
    return texture_priority_4c;
}

// FUNCTION: SURRENDER 0x1005EB40
void srTexture::setMipmapBias(float bias)
{
    mipmap_bias_1c = bias;
}

// FUNCTION: SURRENDER 0x1005EC60
void srTexture::setCorrection(e_correction correction)
{
    packed_state_18 = (packed_state_18 & ~3) | correction;
}

// FUNCTION: SURRENDER 0x1005EC80
void srTexture::setMagFilter(e_filter filter)
{
    packed_state_18 = (packed_state_18 & ~0x70) | (filter << 4);
}

// FUNCTION: SURRENDER 0x1005ECA0
void srTexture::setMinFilter(e_filter filter)
{
    packed_state_18 = (packed_state_18 & ~0x380) | (filter << 7);
}

// FUNCTION: SURRENDER 0x1005ECC0
void srTexture::setMipmap(e_mipmap mipmap)
{
    packed_state_18 = (packed_state_18 & ~0xc00) | (mipmap << 10);
}

// FUNCTION: SURRENDER 0x1005ECE0
void srTexture::setWrapS(e_wrap wrap)
{
    packed_state_18 = (packed_state_18 & ~0x1000) | (wrap << 12);
}

// FUNCTION: SURRENDER 0x1005ED00
void srTexture::setWrapT(e_wrap wrap)
{
    packed_state_18 = (packed_state_18 & ~0x2000) | (wrap << 13);
}

// FUNCTION: SURRENDER 0x1005ED20
srTextureIFace::e_correction srTexture::getCorrection() const
{
    return static_cast<e_correction>(packed_state_18 & 3);
}

// FUNCTION: SURRENDER 0x1005ED30
srTextureIFace::e_filter srTexture::getMagFilter() const
{
    return static_cast<e_filter>((packed_state_18 >> 4) & 7);
}

// FUNCTION: SURRENDER 0x1005ED40
srTextureIFace::e_filter srTexture::getMinFilter() const
{
    return static_cast<e_filter>((packed_state_18 >> 7) & 7);
}

// FUNCTION: SURRENDER 0x1005ED50
srTextureIFace::e_mipmap srTexture::getMipmap() const
{
    return static_cast<e_mipmap>((packed_state_18 >> 10) & 3);
}

// FUNCTION: SURRENDER 0x1005ED60
srTextureIFace::e_wrap srTexture::getWrapS() const
{
    return static_cast<e_wrap>((packed_state_18 >> 12) & 1);
}

// FUNCTION: SURRENDER 0x1005ED70
srTextureIFace::e_wrap srTexture::getWrapT() const
{
    return static_cast<e_wrap>((packed_state_18 >> 13) & 1);
}

// FUNCTION: SURRENDER 0x1005ED80
void srTexture::enableHint(e_hint hint)
{
    texture_dimensions_.hints |= 1 << hint;
}

// FUNCTION: SURRENDER 0x1005EDA0
void srTexture::disableHint(e_hint hint)
{
    texture_dimensions_.hints &= ~(1 << hint);
}

// FUNCTION: SURRENDER 0x1005EDF0
unsigned long srTexture::getTextureFrameHandle()
{
    return 0;
}

// FUNCTION: SURRENDER 0x1005EE00
void srTexture::getDimensions(Dimensions& dimensions)
{
    if ((texture_flags_ & (1 << FLAG_DIRTY_DEFAULTS)) != 0) {
        setupDefaultValues();
    }
    dimensions = texture_dimensions_;
}

// FUNCTION: SURRENDER 0x1005EE90
void srTexture::getMipmapData(MultiRequest& request) {}

// FUNCTION: SURRENDER 0x1005EEA0
void srTexture::getMipmapLevelPartial(PartialRequest& request) {}

// FUNCTION: SURRENDER 0x1005EEB0
void srTexture::getTextureParms(Parameters& parameters)
{
    parameters.packed_state_00 = packed_state_18;
    parameters.mipmap_bias_04 = mipmap_bias_1c;
}

// FUNCTION: SURRENDER 0x1005F140
const char* srTextureIFace::getTextureName()
{
    return getName();
}

// TEMPLATE: SURRENDER 0x1005F470
// srClassSupport<srTextureIFace, srClass, true, 0x2100>::sGetClassNode

// TEMPLATE: SURRENDER 0x1005F4B0
// srClassSupport<srTextureIFace, srClass, true, 0x2100>::srClassSupport
