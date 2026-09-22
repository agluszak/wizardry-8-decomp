#include "surrender/srGERD.h"

#include "surrender/srCriticalSection.h"
#include "surrender/srDebugDD.h"
#include "surrender/srWindow.h"

// GLOBAL: SURRENDER 0x100A4780
srGERD* srGERD::first;

// GLOBAL: SURRENDER 0x100A4784
srGERD* srGERD::firstOpen;

// FUNCTION: SURRENDER 0x10017920
srDD* srGERD::getDD()
{
    ++statistics_1a78_.value_68;
    return dd_40_;
}

// FUNCTION: SURRENDER 0x10017A50
void srGERD::setTextureReduction(long reduction)
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    if (reduction < 0) {
        texture_reduction_2040_ = 0;
        section->releaseAccess();
        return;
    }
    if (reduction > 7) {
        reduction = 7;
    }
    texture_reduction_2040_ = reduction;
    section->releaseAccess();
}

// FUNCTION: SURRENDER 0x10017F30
unsigned long srGERD::getTextureCacheSize() const
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    unsigned long size = texture_cache_size_2038_;
    section->releaseAccess();
    return size;
}

// FUNCTION: SURRENDER 0x10017F50
unsigned long srGERD::getTextureCacheUsed() const
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    unsigned long used = texture_cache_used_2034_;
    section->releaseAccess();
    return used;
}

// FUNCTION: SURRENDER 0x1001AA60
long srGERD::getDisplayMode(unsigned long width, unsigned long height, unsigned long depth) const
{
    if ((state_flags_28_ & 1) != 0) {
        for (long i = 0; i < display_mode_count_36c_; i++) {
            unsigned long* mode = display_modes_368_ + i * 3;
            if (mode[0] == width && mode[1] == height && mode[2] == depth) {
                return i;
            }
        }
    }
    return -1;
}

// FUNCTION: SURRENDER 0x1001ADD0
void srGERD::toggle(e_enable option)
{
    enable_flags_20_.value ^= 1UL << option;
    if (option == 0) {
        dirty_24_ |= 1;
        return;
    }
    if (option == 5) {
        if ((enable_flags_20_.value & 0x20) == 0) {
            dd_40_ = real_dd_48_;
            if (debug_dd_44_ != 0) {
                delete debug_dd_44_;
            }
            real_dd_48_ = 0;
            debug_dd_44_ = 0;
            return;
        }
        real_dd_48_ = dd_40_;
        debug_dd_44_ = new srDebugDD(real_dd_48_);
        dd_40_ = debug_dd_44_;
    }
}

// FUNCTION: SURRENDER 0x1001AE60
void srGERD::popEnable()
{
    unsigned long flags = 0;
    if (enable_depth_21ac_ != 0) {
        enable_depth_21ac_ -= 1;
        flags = enable_stack_216c_[enable_depth_21ac_];
    }
    if (flags != enable_flags_20_.value) {
        for (e_enable option = static_cast<e_enable>(0); static_cast<int>(option) < 7;
             option = static_cast<e_enable>(static_cast<int>(option) + 1)) {
            unsigned long bit = 1UL << option;
            if (((flags & bit) != 0) != ((enable_flags_20_.value & bit) != 0)) {
                toggle(option);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1001AEC0
void srGERD::setSwapInterval(unsigned long interval)
{
    if (interval != swap_interval_1764_) {
        swap_interval_1764_ = interval;
        dirty_24_ |= 4;
    }
}

// FUNCTION: SURRENDER 0x1001AEE0
void srGERD::setGamma(const srVector3T<float>& gamma)
{
    srVector3T<float> adjusted;
    adjusted.x = gamma.x;
    adjusted.y = gamma.y;
    adjusted.z = gamma.z;
    if (adjusted.x < 0.0f) {
        adjusted.x = 0.0f;
    }
    if (adjusted.y < 0.0f) {
        adjusted.y = 0.0f;
    }
    if (adjusted.z < 0.0f) {
        adjusted.z = 0.0f;
    }
    if (adjusted.x != gamma_1758_.x || adjusted.y != gamma_1758_.y || adjusted.z != gamma_1758_.z) {
        gamma_1758_ = adjusted;
        dirty_24_ |= 2;
    }
}

// FUNCTION: SURRENDER 0x1001AFB0
void srGERD::setAntiAlias(e_antiAlias mode)
{
    if (mode != antialias_1768_) {
        antialias_1768_ = mode;
        dirty_24_ |= 8;
    }
}

// FUNCTION: SURRENDER 0x1001BE70
void srGERD::setClipState(srFlags<srRendererDefs::e_clip> state)
{
    if (state.value != clip_state_21cc_.value) {
        clip_state_21cc_ = state;
        dirty_21c0_ |= 1;
    }
}

// FUNCTION: SURRENDER 0x1001C380
srGERD::e_winding srGERD::getWinding() const
{
    return winding_164c_;
}

// FUNCTION: SURRENDER 0x1001C390
void srGERD::setWinding(e_winding winding)
{
    if (winding_164c_ != winding) {
        winding_164c_ = winding;
        dirty_24_ |= 0x2000;
    }
}

// FUNCTION: SURRENDER 0x1001C3B0
void srGERD::setAmbientLight(const srVector4T<float>& light)
{
    ambient_light_2048_ = light;
}

// FUNCTION: SURRENDER 0x1001C410
void srGERD::getAmbientLight(srVector4T<float>& light)
{
    light = ambient_light_2048_;
}

// FUNCTION: SURRENDER 0x1001C480
void srGERD::setAmbientLight(float red, float green, float blue, float alpha)
{
    srVector4T<float> light;
    light.x = red;
    light.y = green;
    light.z = blue;
    light.w = alpha;
    setAmbientLight(light);
}

// FUNCTION: SURRENDER 0x1001C4C0
srGERD::e_cullMode srGERD::getCullMode() const
{
    return cull_mode_1648_;
}

// FUNCTION: SURRENDER 0x1001C4F0
void srGERD::getEnvironmentRange(float& minimum, float& maximum) const
{
    minimum = environment_min_2058_;
    maximum = environment_max_205c_;
}

// FUNCTION: SURRENDER 0x1001C5A0
void srGERD::getEnvironmentScaleFactor(float& scale, float& inverse_scale)
{
    scale = environment_scale_2060_;
    inverse_scale = environment_inv_scale_2064_;
}

// FUNCTION: SURRENDER 0x1001C8D0
long srGERD::getPolygonOffset() const
{
    return polygon_offset_1fe4_;
}

// FUNCTION: SURRENDER 0x1001CA30
void srGERD::setClearColor(const srVector4T<float>& color)
{
    clear_color_1b08_ = color;
    float* clear = &clear_color_1b08_.x;
    if (clear[0] <= 0.0f) {
        clear[0] = 0.0f;
    } else if (clear[0] >= 1.0f) {
        clear[0] = 1.0f;
    }
    if (clear[1] <= 0.0f) {
        clear[1] = 0.0f;
    } else if (clear[1] >= 1.0f) {
        clear[1] = 1.0f;
    }
    if (clear[2] <= 0.0f) {
        clear[2] = 0.0f;
    } else if (clear[2] >= 1.0f) {
        clear[2] = 1.0f;
    }
    if (clear[3] <= 0.0f) {
        clear[3] = 0.0f;
    } else if (clear[3] >= 1.0f) {
        clear[3] = 1.0f;
    }
}

// FUNCTION: SURRENDER 0x1001CB00
void srGERD::setClearColor(float red, float green, float blue, float alpha)
{
    srVector4T<float> color;
    color.x = red;
    color.y = green;
    color.z = blue;
    color.w = alpha;
    setClearColor(color);
}

// FUNCTION: SURRENDER 0x1001CB70
void srGERD::setClearDepth(double depth)
{
    if (depth <= 0.0) {
        clear_depth_1b28_ = 0.0;
        return;
    }
    if (depth >= 1.0) {
        clear_depth_1b28_ = 1.0;
        return;
    }
    clear_depth_1b28_ = depth;
}

// FUNCTION: SURRENDER 0x1001CEC0
unsigned long srGERD::getVertexProcessorCount() const
{
    return vertex_processor_count_21b8_;
}

// FUNCTION: SURRENDER 0x1001CED0
void srGERD::getVertexProcessors(srVertexProcessor** processors) const
{
    if (processors != 0) {
        unsigned long count = vertex_processor_count_21b8_;
        for (unsigned long i = 0; i < count; i++) {
            processors[i] = vertex_processors_21b0_[i];
        }
    }
}

// FUNCTION: SURRENDER 0x1001CFC0
void srGERD::setError(e_error error)
{
    last_error_2c_ = error;
}

// FUNCTION: SURRENDER 0x1001CF80
srGERD* srGERD::getNext() const
{
    return next_34_;
}

// FUNCTION: SURRENDER 0x1001CF50
srGERD* srGERD::getFirstOpen()
{
    return firstOpen;
}

// FUNCTION: SURRENDER 0x1001CF90
srGERD* srGERD::getNextOpen() const
{
    return next_open_3c_;
}

// FUNCTION: SURRENDER 0x10018330
void srGERD::invalidateTexture(srTextureIFace* texture)
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    if (texture != 0) {
        invalidateTextureByFrameHandle(texture->getTextureFrameHandle());
    }
    section->releaseAccess();
}

// FUNCTION: SURRENDER 0x100281E0
void srGERD::invalidateTexture(Texture& texture)
{
    Texture* candidate = &texture;
    if (candidate != 0 && candidate != current_texture_2030_) {
        markTextureAsDeleted(texture);
    }
}

// FUNCTION: SURRENDER 0x10028150
void srGERD::markTextureAsDeleted(Texture& texture)
{
    if (texture.deleted_9c_ == 0) {
        if (texture.next_00 != 0) {
            texture.next_00->prev_04 = texture.prev_04;
        }
        if (texture.prev_04 != 0) {
            texture.prev_04->next_00 = texture.next_00;
        }
        if (&texture == active_textures_202c_) {
            active_textures_202c_ = texture.prev_04;
        }
        texture.next_00 = 0;
        Texture* previous = deleted_textures_2028_;
        texture.prev_04 = previous;
        if (previous != 0) {
            previous->next_00 = &texture;
        }
        deleted_textures_2028_ = &texture;
        texture.deleted_9c_ = 1;
    }
}

// FUNCTION: SURRENDER 0x10018390
void srGERD::invalidateTextureByFrameHandle(unsigned long handle)
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    if (handle != 0 && texture_hash_enabled_2044_) {
        long index = texture_hash_heads_2004_[((handle >> 10 ^ handle) >> 10 ^ handle) &
                                              (texture_hash_size_2010_ - 1)];
        if (index != -1) {
            TextureEntry* entries = texture_hash_entries_2008_;
            while (entries[index].handle_04 != handle) {
                index = entries[index].next_00;
                if (index == -1) {
                    section->releaseAccess();
                    return;
                }
            }
            Texture* texture = entries[index].texture_08;
            if (texture != 0) {
                invalidateTexture(*texture);
            }
        }
    }
    section->releaseAccess();
}

// FUNCTION: SURRENDER 0x10023550
void srGERD::matrixMode(e_matrixMode mode)
{
    matrix_mode_1650_ = mode;
}

// FUNCTION: SURRENDER 0x100235B0
void srGERD::pushMatrix()
{
    MatrixStack& stack = matrix_stacks_410_[matrix_mode_1650_];
    if (stack.depth_800 < 0x20) {
        stack.entries_00[stack.depth_800] = current_matrix_390_[matrix_mode_1650_];
        stack.depth_800++;
    }
}

// FUNCTION: SURRENDER 0x10023560
void srGERD::popMatrix()
{
    MatrixStack& stack = matrix_stacks_410_[matrix_mode_1650_];
    if (stack.depth_800 != 0) {
        stack.depth_800--;
        current_matrix_390_[matrix_mode_1650_] = stack.entries_00[stack.depth_800];
    }
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x100236D0
void srGERD::setMatrixDirty()
{
    if (matrix_mode_1650_ == MATRIX_PROJECTION) {
        dirty_24_ |= 0x10000;
    }
    dirty_24_ |= 1 << (matrix_mode_1650_ + 5);
}

// FUNCTION: SURRENDER 0x1001D2D0
void srGERD::checkFrameStateChanges()
{
    if ((dirty_24_ & 0xf) != 0) {
        applyFrameStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001B450
void srGERD::applyFrameStateChanges()
{
    srDD::Update update;
    update.gamma_04 = gamma_1758_;
    update.enabled_1c = static_cast<unsigned long>(static_cast<char>(enable_flags_20_.value) & 1);
    update.swap_interval_14 = swap_interval_1764_;
    update.antialias_18 = antialias_1768_;
    update.flags_00 = 0;
    update.value_10 = 1.0f;
    if ((dirty_24_ & 8) != 0) {
        update.flags_00 |= 8;
    }
    if ((dirty_24_ & 1) != 0) {
        update.flags_00 |= 1;
    }
    if ((dirty_24_ & 2) != 0) {
        update.flags_00 |= 4;
    }
    if ((dirty_24_ & 4) != 0) {
        update.flags_00 |= 2;
    }
    getDD()->update(update);
    dirty_24_ &= ~0xfUL;
    statistics_1a78_.frame_state_count_48++;
}

// FUNCTION: SURRENDER 0x1001A790
srGERD::e_error srGERD::beginFrame()
{
    if ((state_flags_28_ & 1) == 0) {
        return static_cast<e_error>(9);
    }
    if (isWindowOpen() == 0) {
        return static_cast<e_error>(4);
    }
    if (srWindow::isWindow(window_374_) == 0) {
        return static_cast<e_error>(6);
    }
    if ((state_flags_28_ & 4) == 0) {
        if ((enable_flags_20_.value & 0x10) != 0 && (state_flags_28_ & 8) == 0) {
            flipFrame();
        }
        checkFrameStateChanges();
        getDD()->beginFrame();
        state_flags_28_ |= 4;
        state_flags_28_ &= ~8UL;
    }
    return static_cast<e_error>(0);
}

// FUNCTION: SURRENDER 0x1001A810
void srGERD::endFrame()
{
    if (isWindowOpen() == 0) {
        setError(static_cast<e_error>(4));
        return;
    }
    if ((state_flags_28_ & 4) != 0) {
        flushRenderers();
        getDD()->endFrame();
        state_flags_28_ &= ~4UL;
    }
}

// FUNCTION: SURRENDER 0x1001CD20
int srGERD::isContextCreated() const
{
    return state_flags_28_ & 1;
}

// FUNCTION: SURRENDER 0x1001D020
long srGERD::getWidth() const
{
    return width_380_;
}

// FUNCTION: SURRENDER 0x1001D0A0
long srGERD::getHeight() const
{
    return height_384_;
}

// FUNCTION: SURRENDER 0x1001D0B0
unsigned long srGERD::getWindowHandle() const
{
    return window_374_;
}

// FUNCTION: SURRENDER 0x1001D0D0
int srGERD::isWindowOpen() const
{
    return (state_flags_28_ >> 1) & 1;
}

// FUNCTION: SURRENDER 0x1001D1A0
srGERD* srGERD::getFirst()
{
    return first;
}

// FUNCTION: SURRENDER 0x1001D1F0
void srGERD::pushEnable()
{
    unsigned long depth = enable_depth_21ac_;
    if (depth < 0x10) {
        enable_depth_21ac_ = depth + 1;
        enable_stack_216c_[depth] = enable_flags_20_.value;
    }
}

// FUNCTION: SURRENDER 0x1001D410
void srGERD::setViewPort(unsigned long x, unsigned long y, unsigned long width,
                         unsigned long height)
{
    if (width < 1) {
        width = 1;
    }
    if (height < 1) {
        height = 1;
    }
    view_top_163c_ = y;
    view_left_1638_ = x;
    view_bottom_1644_ = y + height;
    view_right_1640_ = x + width;
    if (view_left_1638_ >= (unsigned long)getWidth()) {
        view_left_1638_ = getWidth();
    }
    if (view_top_163c_ >= (unsigned long)getHeight()) {
        view_top_163c_ = getHeight();
    }
    if (view_right_1640_ >= (unsigned long)getWidth()) {
        view_right_1640_ = getWidth();
    }
    if (view_bottom_1644_ >= (unsigned long)getHeight()) {
        view_bottom_1644_ = getHeight();
    }
    dirty_24_ |= 0x80;
}

// FUNCTION: SURRENDER 0x1001DAA0
void srGERD::pushPick(const Pick& pick)
{
    if (pick_depth_19ec_ < 0x20) {
        pick_stack_176c_[pick_depth_19ec_] = pick;
        pick_depth_19ec_ += 1;
    }
}

// FUNCTION: SURRENDER 0x1001DAE0
void srGERD::popPick(Pick& pick)
{
    if (pick_depth_19ec_ != 0) {
        pick_depth_19ec_ -= 1;
        pick = pick_stack_176c_[pick_depth_19ec_];
    }
}

// FUNCTION: SURRENDER 0x1001DB10
void srGERD::setPickKey(unsigned long key)
{
    pick_key_19f0_ = key;
}

// FUNCTION: SURRENDER 0x1001EE50
void srGERD::setExclusionMask(unsigned long mask)
{
    exclusion_mask_21bc_ = mask;
}

// FUNCTION: SURRENDER 0x1001EE60
unsigned long srGERD::getExclusionMask() const
{
    return exclusion_mask_21bc_;
}

// FUNCTION: SURRENDER 0x100293E0
void srGERD::resetTexture()
{
    dirty_24_ |= 0x400;
    dirty_24_ |= 0x800;
}
