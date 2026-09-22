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
    srMatrix4T<float>* matrix = &current_matrix_390_[matrix_mode_1650_];
    MatrixStack& stack = matrix_stacks_410_[matrix_mode_1650_];
    if (stack.depth_800 != 0) {
        stack.depth_800--;
        *matrix = stack.entries_00[stack.depth_800];
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

// FUNCTION: SURRENDER 0x100213A0
srMatrix4T<float>::e_scaleType srGERD::getModelViewScaleType()
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    return modelview_scale_type_1744_;
}

// FUNCTION: SURRENDER 0x100213C0
void srGERD::getNormalMatrix(srMatrix4T<float>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    if ((enable_flags_20_.value & 8) != 0) {
        srVector4T<float> negated;
        negated.Set(-normal_matrix_1704_.vectors[0].x, -normal_matrix_1704_.vectors[0].y,
                    -normal_matrix_1704_.vectors[0].z, -normal_matrix_1704_.vectors[0].w);
        matrix.vectors[0] = negated;
        negated.Set(-normal_matrix_1704_.vectors[1].x, -normal_matrix_1704_.vectors[1].y,
                    -normal_matrix_1704_.vectors[1].z, -normal_matrix_1704_.vectors[1].w);
        matrix.vectors[1] = negated;
        negated.Set(-normal_matrix_1704_.vectors[2].x, -normal_matrix_1704_.vectors[2].y,
                    -normal_matrix_1704_.vectors[2].z, -normal_matrix_1704_.vectors[2].w);
        matrix.vectors[2] = negated;
        matrix.vectors[3] = normal_matrix_1704_.vectors[3];
        return;
    }
    matrix = normal_matrix_1704_;
}

// FUNCTION: SURRENDER 0x100214F0
void srGERD::getProjectClipNearMatrix(srMatrix4T<float>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    matrix = project_clip_near_16c4_;
}

// FUNCTION: SURRENDER 0x10022190
void srGERD::ortho(const Frustum& frustum)
{
    srMatrix4T<double> ortho_matrix;
    ortho_matrix.vectors[0].x = 2.0 / (frustum.right - frustum.left);
    ortho_matrix.vectors[0].w = -((frustum.right + frustum.left) / (frustum.right - frustum.left));
    ortho_matrix.vectors[1].y = 2.0 / (frustum.top - frustum.bottom);
    ortho_matrix.vectors[1].w = -((frustum.bottom + frustum.top) / (frustum.top - frustum.bottom));
    ortho_matrix.vectors[2].z = -2.0 / (frustum.far_plane - frustum.near_plane);
    ortho_matrix.vectors[2].w =
        -((frustum.near_plane + frustum.far_plane) / (frustum.far_plane - frustum.near_plane));
    srMatrix4T<float>& matrix = current_matrix_390_[matrix_mode_1650_];
    matrix.vectors[0].w = matrix.vectors[0].x * ortho_matrix.vectors[0].w +
                          matrix.vectors[0].y * ortho_matrix.vectors[1].w +
                          matrix.vectors[0].z * ortho_matrix.vectors[2].w + matrix.vectors[0].w;
    matrix.vectors[0].x *= ortho_matrix.vectors[0].x;
    matrix.vectors[0].y *= ortho_matrix.vectors[1].y;
    matrix.vectors[0].z *= ortho_matrix.vectors[2].z;
    matrix.vectors[1].w = matrix.vectors[1].x * ortho_matrix.vectors[0].w +
                          matrix.vectors[1].y * ortho_matrix.vectors[1].w +
                          matrix.vectors[1].z * ortho_matrix.vectors[2].w + matrix.vectors[1].w;
    matrix.vectors[1].x *= ortho_matrix.vectors[0].x;
    matrix.vectors[1].y *= ortho_matrix.vectors[1].y;
    matrix.vectors[1].z *= ortho_matrix.vectors[2].z;
    matrix.vectors[2].w = matrix.vectors[2].x * ortho_matrix.vectors[0].w +
                          matrix.vectors[2].y * ortho_matrix.vectors[1].w +
                          matrix.vectors[2].z * ortho_matrix.vectors[2].w + matrix.vectors[2].w;
    matrix.vectors[2].x *= ortho_matrix.vectors[0].x;
    matrix.vectors[2].y *= ortho_matrix.vectors[1].y;
    matrix.vectors[2].z *= ortho_matrix.vectors[2].z;
    matrix.vectors[3].w = matrix.vectors[3].x * ortho_matrix.vectors[0].w +
                          matrix.vectors[3].y * ortho_matrix.vectors[1].w +
                          matrix.vectors[3].z * ortho_matrix.vectors[2].w + matrix.vectors[3].w;
    matrix.vectors[3].x *= ortho_matrix.vectors[0].x;
    matrix.vectors[3].y *= ortho_matrix.vectors[1].y;
    matrix.vectors[3].z *= ortho_matrix.vectors[2].z;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022330
void srGERD::ortho(double left, double right, double bottom, double top, double near_plane,
                   double far_plane)
{
    Frustum frustum;
    frustum.left = left;
    frustum.right = right;
    frustum.bottom = bottom;
    frustum.top = top;
    frustum.near_plane = near_plane;
    frustum.far_plane = far_plane;
    ortho(frustum);
}

// FUNCTION: SURRENDER 0x100223A0
void srGERD::rotate(double angle, const srVector3T<double>& axis)
{
    double length_sq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    if (length_sq != 0.0) {
        srMatrix4T<double> rotation;
        srVector3T<double> unit_axis = axis;
        if (length_sq != 1.0) {
            double inverse = 1.0 / sqrt(length_sq);
            unit_axis.Set(axis.x * inverse, axis.y * inverse, axis.z * inverse);
        }
        double sine = sin(angle);
        double cosine = cos(angle);
        double complement = 1.0 - cosine;
        rotation.vectors[0].x = unit_axis.x * unit_axis.x * complement + cosine;
        rotation.vectors[0].y = unit_axis.x * unit_axis.y * complement - unit_axis.z * sine;
        rotation.vectors[0].z = unit_axis.x * unit_axis.z * complement + unit_axis.y * sine;
        rotation.vectors[1].x = unit_axis.x * unit_axis.y * complement + unit_axis.z * sine;
        rotation.vectors[1].y = unit_axis.y * unit_axis.y * complement + cosine;
        rotation.vectors[1].z = unit_axis.y * unit_axis.z * complement - unit_axis.x * sine;
        rotation.vectors[2].x = unit_axis.x * unit_axis.z * complement - unit_axis.y * sine;
        rotation.vectors[2].y = unit_axis.y * unit_axis.z * complement + unit_axis.x * sine;
        rotation.vectors[2].z = unit_axis.z * unit_axis.z * complement + cosine;
        srMatrix4T<float>& matrix = current_matrix_390_[matrix_mode_1650_];
        float x = matrix.vectors[0].x;
        float y = matrix.vectors[0].y;
        float z = matrix.vectors[0].z;
        matrix.vectors[0].x =
            rotation.vectors[0].x * x + rotation.vectors[1].x * y + rotation.vectors[2].x * z;
        matrix.vectors[0].y =
            rotation.vectors[0].y * x + rotation.vectors[1].y * y + rotation.vectors[2].y * z;
        matrix.vectors[0].z =
            rotation.vectors[0].z * x + rotation.vectors[1].z * y + rotation.vectors[2].z * z;
        x = matrix.vectors[1].x;
        y = matrix.vectors[1].y;
        z = matrix.vectors[1].z;
        matrix.vectors[1].x =
            rotation.vectors[0].x * x + rotation.vectors[1].x * y + rotation.vectors[2].x * z;
        matrix.vectors[1].y =
            rotation.vectors[0].y * x + rotation.vectors[1].y * y + rotation.vectors[2].y * z;
        matrix.vectors[1].z =
            rotation.vectors[0].z * x + rotation.vectors[1].z * y + rotation.vectors[2].z * z;
        x = matrix.vectors[2].x;
        y = matrix.vectors[2].y;
        z = matrix.vectors[2].z;
        matrix.vectors[2].x =
            rotation.vectors[0].x * x + rotation.vectors[1].x * y + rotation.vectors[2].x * z;
        matrix.vectors[2].y =
            rotation.vectors[0].y * x + rotation.vectors[1].y * y + rotation.vectors[2].y * z;
        matrix.vectors[2].z =
            rotation.vectors[0].z * x + rotation.vectors[1].z * y + rotation.vectors[2].z * z;
        x = matrix.vectors[3].x;
        y = matrix.vectors[3].y;
        z = matrix.vectors[3].z;
        matrix.vectors[3].x =
            rotation.vectors[0].x * x + rotation.vectors[1].x * y + rotation.vectors[2].x * z;
        matrix.vectors[3].y =
            rotation.vectors[0].y * x + rotation.vectors[1].y * y + rotation.vectors[2].y * z;
        matrix.vectors[3].z =
            rotation.vectors[0].z * x + rotation.vectors[1].z * y + rotation.vectors[2].z * z;
        setMatrixDirty();
        return;
    }
    loadIdentity();
}

// FUNCTION: SURRENDER 0x100226F0
void srGERD::rotate(double angle, const srVector3T<float>& axis)
{
    rotate(angle, srVector3T<double>(axis.x, axis.y, axis.z));
}

// FUNCTION: SURRENDER 0x10022780
void srGERD::scale(const srVector3T<double>& factors)
{
    srMatrix4T<float>& matrix = current_matrix_390_[matrix_mode_1650_];
    matrix.vectors[0].x *= factors.x;
    matrix.vectors[1].x *= factors.x;
    matrix.vectors[2].x *= factors.x;
    matrix.vectors[3].x *= factors.x;
    matrix.vectors[0].y *= factors.y;
    matrix.vectors[1].y *= factors.y;
    matrix.vectors[2].y *= factors.y;
    matrix.vectors[3].y *= factors.y;
    matrix.vectors[0].z *= factors.z;
    matrix.vectors[1].z *= factors.z;
    matrix.vectors[2].z *= factors.z;
    matrix.vectors[3].z *= factors.z;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022870
void srGERD::scale(double x, double y, double z)
{
    scale(srVector3T<double>(x, y, z));
}

// FUNCTION: SURRENDER 0x10022960
void srGERD::translate(const srVector3T<double>& offset)
{
    srMatrix4T<float>& matrix = current_matrix_390_[matrix_mode_1650_];
    matrix.vectors[0].w = matrix.vectors[0].x * offset.x + matrix.vectors[0].y * offset.y +
                          matrix.vectors[0].z * offset.z + matrix.vectors[0].w;
    matrix.vectors[1].w = matrix.vectors[1].x * offset.x + matrix.vectors[1].y * offset.y +
                          matrix.vectors[1].z * offset.z + matrix.vectors[1].w;
    matrix.vectors[2].w = matrix.vectors[2].x * offset.x + matrix.vectors[2].y * offset.y +
                          matrix.vectors[2].z * offset.z + matrix.vectors[2].w;
    matrix.vectors[3].w = matrix.vectors[3].x * offset.x + matrix.vectors[3].y * offset.y +
                          matrix.vectors[3].z * offset.z + matrix.vectors[3].w;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x100229F0
void srGERD::translate(const srVector3T<float>& offset)
{
    translate(srVector3T<double>(offset.x, offset.y, offset.z));
}

// FUNCTION: SURRENDER 0x10022A20
void srGERD::translate(double x, double y, double z)
{
    translate(srVector3T<double>(x, y, z));
}

// FUNCTION: SURRENDER 0x100231D0
void srGERD::loadIdentity()
{
    srMatrix4T<float>& matrix = current_matrix_390_[matrix_mode_1650_];
    if (matrix.vectors[0].x != 1.0f || matrix.vectors[1].y != 1.0f || matrix.vectors[2].z != 1.0f ||
        matrix.vectors[3].w != 1.0f || matrix.vectors[0].y != 0.0f || matrix.vectors[0].z != 0.0f ||
        matrix.vectors[0].w != 0.0f || matrix.vectors[1].x != 0.0f || matrix.vectors[1].z != 0.0f ||
        matrix.vectors[1].w != 0.0f || matrix.vectors[2].x != 0.0f || matrix.vectors[2].y != 0.0f ||
        matrix.vectors[2].w != 0.0f || matrix.vectors[3].x != 0.0f || matrix.vectors[3].y != 0.0f ||
        matrix.vectors[3].z != 0.0f) {
        matrix.vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
        matrix.vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
        matrix.vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
        matrix.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
        setMatrixDirty();
    }
}

// FUNCTION: SURRENDER 0x10023320
void srGERD::getMatrix(e_matrixMode mode, srMatrix4T<float>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    matrix = current_matrix_390_[mode];
}

// FUNCTION: SURRENDER 0x10023510
void srGERD::getInverseModelViewMatrix(srMatrix4T<float>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    matrix = inverse_modelview_1684_;
}

/* Locked-buffer surface created by lockBuffer: a srColorSurfaceIFace-derived
   class of 0x60 bytes (ctor 0x100205d0) that keeps its own scissor rect. */
class srGERD::LockSurface {
public:
    void setScissor(unsigned long left, unsigned long top, unsigned long right,
                    unsigned long bottom);

private:
    unsigned char unknown_00_[0x48];
    unsigned long left_48_;
    unsigned long top_4c_;
    unsigned long right_50_;
    unsigned long bottom_54_;
};

// FUNCTION: SURRENDER 0x10020780
void srGERD::LockSurface::setScissor(unsigned long left, unsigned long top, unsigned long right,
                                     unsigned long bottom)
{
    left_48_ = left;
    right_50_ = right;
    top_4c_ = top;
    bottom_54_ = bottom;
}

// FUNCTION: SURRENDER 0x1001BC70
void srGERD::applyViewStateChanges()
{
    unsigned long dirty = dirty_24_;
    statistics_1a78_.value_40 += 1;
    if ((dirty & 0x20) != 0) {
        classifyMatrix(MATRIX_MODELVIEW);
        dirty = dirty_24_ & ~0x20UL;
        dirty_24_ = dirty;
        if (dirty == 0) {
            return;
        }
    }
    if ((dirty & 0x180) != 0) {
        srDD::ViewPort viewport;
        viewport.x = view_left_1638_;
        viewport.y = view_top_163c_;
        viewport.width = view_right_1640_ - viewport.x;
        viewport.height = view_bottom_1644_ - viewport.y;
        viewport.extra[0] = viewport_extra_1618_[0];
        viewport.extra[1] = viewport_extra_1618_[1];
        viewport.extra[2] = viewport_extra_1618_[2];
        viewport.extra[3] = viewport_extra_1618_[3];
        if ((state_flags_28_ & 0x10) == 0) {
            getDD()->setViewPort(viewport);
        }
    }
    if ((dirty_24_ & 0x40) != 0) {
        classifyMatrix(MATRIX_PROJECTION);
        project_clip_near_16c4_ = current_matrix_390_[MATRIX_PROJECTION];
        srMatrix4T<float>& projection = current_matrix_390_[MATRIX_PROJECTION];
        if (((projection.vectors[3].x != 0.0f) || (projection.vectors[3].y != 0.0f) ||
             (projection.vectors[3].z != 0.0f)) &&
            ((projection.vectors[3].w + projection.vectors[2].w != 0.0f) &&
             (fabs((projection.vectors[3].z + projection.vectors[2].z) /
                   (projection.vectors[3].w + projection.vectors[2].w)) != 1.0))) {
            float scale = (float)fabs((projection.vectors[3].z + projection.vectors[2].z) /
                                      (projection.vectors[3].w + projection.vectors[2].w));
            project_clip_near_16c4_.vectors[0].x *= scale;
            project_clip_near_16c4_.vectors[0].y *= scale;
            project_clip_near_16c4_.vectors[0].z *= scale;
            project_clip_near_16c4_.vectors[0].w *= scale;
            project_clip_near_16c4_.vectors[1].x *= scale;
            project_clip_near_16c4_.vectors[1].y *= scale;
            project_clip_near_16c4_.vectors[1].z *= scale;
            project_clip_near_16c4_.vectors[1].w *= scale;
            project_clip_near_16c4_.vectors[2].x *= scale;
            project_clip_near_16c4_.vectors[2].y *= scale;
            project_clip_near_16c4_.vectors[2].z *= scale;
            project_clip_near_16c4_.vectors[2].w *= scale;
            project_clip_near_16c4_.vectors[3].x *= scale;
            project_clip_near_16c4_.vectors[3].y *= scale;
            project_clip_near_16c4_.vectors[3].z *= scale;
            project_clip_near_16c4_.vectors[3].w *= scale;
        }
        if ((state_flags_28_ & 0x10) == 0) {
            getDD()->setProjectionMatrix(project_clip_near_16c4_,
                                         matrix_type_174c_[MATRIX_PROJECTION]);
        }
    }
    if ((dirty_24_ & 0x10) != 0) {
        recalcScissor();
    }
    dirty_24_ &= ~0x1f0UL;
}

// FUNCTION: SURRENDER 0x1001D580
void srGERD::setScissor(unsigned long x, unsigned long y, unsigned long width, unsigned long height)
{
    scissor_1628_.left = x;
    scissor_1628_.right = x + width;
    scissor_1628_.top = y;
    scissor_1628_.bottom = y + height;
    if (scissor_1628_.left >= (unsigned long)getWidth()) {
        scissor_1628_.left = getWidth();
    }
    if (scissor_1628_.top >= (unsigned long)getHeight()) {
        scissor_1628_.top = getHeight();
    }
    if (scissor_1628_.right >= (unsigned long)getWidth()) {
        scissor_1628_.right = getWidth();
    }
    if (scissor_1628_.bottom >= (unsigned long)getHeight()) {
        scissor_1628_.bottom = getHeight();
    }
    recalcScissor();
    dirty_24_ |= 0x10;
}

// FUNCTION: SURRENDER 0x100204C0
void srGERD::recalcScissor()
{
    if (scissor_1628_.left == 0 && scissor_1628_.right == (unsigned long)getWidth() &&
        scissor_1628_.top == 0 && scissor_1628_.bottom == (unsigned long)getHeight()) {
        scissor_state_1680_ |= 2;
    } else {
        scissor_state_1680_ &= ~2UL;
    }
    getDD()->setScissor(scissor_1628_);
    if (lock_surface_1b00_ != 0) {
        lock_surface_1b00_->setScissor(scissor_1628_.left, scissor_1628_.top, scissor_1628_.right,
                                       scissor_1628_.bottom);
    }
}

// FUNCTION: SURRENDER 0x100215A0
void srGERD::classifyMatrix(e_matrixMode mode)
{
    statistics_1a78_.value_7c += 1;
    if (mode == MATRIX_MODELVIEW) {
        srMatrix4T<float>* modelview = &current_matrix_390_[MATRIX_MODELVIEW];
        matrix_type_174c_[MATRIX_MODELVIEW] = (srMatrix4T<float>::e_type)0;
        srMatrix4T<float>* inverse = &inverse_modelview_1684_;
        double length0 = modelview->vectors[2].x * modelview->vectors[2].x +
                         modelview->vectors[1].x * modelview->vectors[1].x +
                         modelview->vectors[0].x * modelview->vectors[0].x;
        double length1 = modelview->vectors[2].y * modelview->vectors[2].y +
                         modelview->vectors[1].y * modelview->vectors[1].y +
                         modelview->vectors[0].y * modelview->vectors[0].y;
        double length2 = modelview->vectors[2].z * modelview->vectors[2].z +
                         modelview->vectors[1].z * modelview->vectors[1].z +
                         modelview->vectors[0].z * modelview->vectors[0].z;
        if (fabs(length0 - length1) <= 1e-05 && fabs(length0 - length2) <= 1e-05) {
            if (fabs(length0 - 1.0) > 1e-05) {
                modelview_scale_type_1744_ = srMatrix4T<float>::SCALE_TYPE_POSITIONAL_1;
                modelview_scale_1748_ = (float)sqrt(length0);
                length0 = 1.0 / length0;
                inverse->vectors[0].x = length0 * modelview->vectors[0].x;
                inverse->vectors[1].x = length0 * modelview->vectors[0].y;
                inverse->vectors[2].x = length0 * modelview->vectors[0].z;
                inverse->vectors[0].y = length0 * modelview->vectors[1].x;
                inverse->vectors[1].y = length0 * modelview->vectors[1].y;
                inverse->vectors[2].y = length0 * modelview->vectors[1].z;
                inverse->vectors[0].z = length0 * modelview->vectors[2].x;
                inverse->vectors[1].z = length0 * modelview->vectors[2].y;
                inverse->vectors[2].z = length0 * modelview->vectors[2].z;
                float scale = modelview_scale_1748_;
                normal_matrix_1704_.vectors[0].x = scale * inverse->vectors[0].x;
                normal_matrix_1704_.vectors[0].y = scale * inverse->vectors[1].x;
                normal_matrix_1704_.vectors[0].z = scale * inverse->vectors[2].x;
                normal_matrix_1704_.vectors[1].x = scale * inverse->vectors[0].y;
                normal_matrix_1704_.vectors[1].y = scale * inverse->vectors[1].y;
                normal_matrix_1704_.vectors[1].z = scale * inverse->vectors[2].y;
                normal_matrix_1704_.vectors[2].x = scale * inverse->vectors[0].z;
                normal_matrix_1704_.vectors[2].y = scale * inverse->vectors[1].z;
                normal_matrix_1704_.vectors[2].z = scale * inverse->vectors[2].z;
            } else {
                modelview_scale_type_1744_ = srMatrix4T<float>::SCALE_TYPE_POSITIONAL_0;
                modelview_scale_1748_ = 1.0f;
                inverse->vectors[0].x = modelview->vectors[0].x;
                normal_matrix_1704_.vectors[0].x = modelview->vectors[0].x;
                inverse->vectors[1].x = modelview->vectors[0].y;
                normal_matrix_1704_.vectors[0].y = modelview->vectors[0].y;
                inverse->vectors[2].x = modelview->vectors[0].z;
                normal_matrix_1704_.vectors[0].z = modelview->vectors[0].z;
                inverse->vectors[0].y = modelview->vectors[1].x;
                normal_matrix_1704_.vectors[1].x = modelview->vectors[1].x;
                inverse->vectors[1].y = modelview->vectors[1].y;
                normal_matrix_1704_.vectors[1].y = modelview->vectors[1].y;
                inverse->vectors[2].y = modelview->vectors[1].z;
                normal_matrix_1704_.vectors[1].z = modelview->vectors[1].z;
                inverse->vectors[0].z = modelview->vectors[2].x;
                normal_matrix_1704_.vectors[2].x = modelview->vectors[2].x;
                inverse->vectors[1].z = modelview->vectors[2].y;
                normal_matrix_1704_.vectors[2].y = modelview->vectors[2].y;
                inverse->vectors[2].z = modelview->vectors[2].z;
                normal_matrix_1704_.vectors[2].z = modelview->vectors[2].z;
            }
            inverse->vectors[0].w = -(modelview->vectors[0].w * inverse->vectors[0].x +
                                      modelview->vectors[1].w * inverse->vectors[0].y +
                                      modelview->vectors[2].w * inverse->vectors[0].z);
            inverse->vectors[1].w = -(inverse->vectors[1].x * modelview->vectors[0].w +
                                      inverse->vectors[1].y * modelview->vectors[1].w +
                                      inverse->vectors[1].z * modelview->vectors[2].w);
            inverse->vectors[2].w = -(inverse->vectors[2].x * modelview->vectors[0].w +
                                      inverse->vectors[2].y * modelview->vectors[1].w +
                                      inverse->vectors[2].z * modelview->vectors[2].w);
            inverse->vectors[3].x = 0.0f;
            inverse->vectors[3].y = 0.0f;
            inverse->vectors[3].z = 0.0f;
            inverse->vectors[3].w = 1.0f;
            return;
        }
        float inv0 = (float)(1.0f / sqrt(length0));
        float inv1 = (float)(1.0f / sqrt(length1));
        float inv2 = (float)(1.0f / sqrt(length2));
        if (length0 < length1) {
            length0 = length1;
        }
        if (length0 < length2) {
            length0 = length2;
        }
        modelview_scale_type_1744_ = srMatrix4T<float>::SCALE_TYPE_POSITIONAL_2;
        modelview_scale_1748_ = (float)sqrt(length0);
        if (modelview == inverse) {
            inverse->Invert();
        } else {
            inverse->AdjugateFrom(&modelview->vectors[0].x);
            float det = inverse->Det();
            if (det != 1.0f) {
                inverse->Scale(1.0f / det);
            }
        }
        srMatrix4T<float> normalized = *modelview;
        normalized.vectors[0].x *= inv0;
        normalized.vectors[0].y *= inv1;
        normalized.vectors[0].z *= inv2;
        normalized.vectors[1].x *= inv0;
        normalized.vectors[1].y *= inv1;
        normalized.vectors[1].z *= inv2;
        normalized.vectors[2].x *= inv0;
        normalized.vectors[2].y *= inv1;
        normalized.vectors[2].z *= inv2;
        normalized.vectors[3].x *= inv0;
        normalized.vectors[3].y *= inv1;
        normalized.vectors[3].z *= inv2;
        srMatrix4T<float> adjugate;
        adjugate.AdjugateFrom(&normalized.vectors[0].x);
        float det = adjugate.Det();
        if (det != 1.0f) {
            adjugate.Scale(1.0f / det);
        }
        normal_matrix_1704_.vectors[0].x = adjugate.vectors[0].x;
        normal_matrix_1704_.vectors[0].y = adjugate.vectors[1].x;
        normal_matrix_1704_.vectors[0].z = adjugate.vectors[2].x;
        normal_matrix_1704_.vectors[1].x = adjugate.vectors[0].y;
        normal_matrix_1704_.vectors[1].y = adjugate.vectors[1].y;
        normal_matrix_1704_.vectors[1].z = adjugate.vectors[2].y;
        normal_matrix_1704_.vectors[2].x = adjugate.vectors[0].z;
        normal_matrix_1704_.vectors[2].y = adjugate.vectors[1].z;
        normal_matrix_1704_.vectors[2].z = adjugate.vectors[2].z;
        return;
    }
    srMatrix4T<float>& matrix = current_matrix_390_[mode];
    unsigned long mask = 0;
    unsigned long bit = 1;
    for (long row = 0; row < 4; row++) {
        if (matrix.vectors[row].x == 0.0f) {
            mask |= bit;
        }
        if (matrix.vectors[row].y == 0.0f) {
            mask |= bit * 2;
        }
        if (matrix.vectors[row].z == 0.0f) {
            mask |= bit * 4;
        }
        if (matrix.vectors[row].w == 0.0f) {
            mask |= bit * 8;
        }
        bit *= 0x10;
    }
    unsigned long type;
    if (mask == 0x7bde && matrix.vectors[0].x == 1.0f && matrix.vectors[1].y == 1.0f &&
        matrix.vectors[2].z == 1.0f && matrix.vectors[3].w == 1.0f) {
        type = 4;
    } else if ((mask & 0xb39a) == 0xb39a) {
        type = 6;
    } else if ((mask & 0x7356) == 0x7356) {
        type = 5;
    } else if ((mask & 0x7000) == 0x7000 && matrix.vectors[3].w == 1.0f) {
        type = 3;
    } else {
        type = 0;
    }
    matrix_type_174c_[mode] = (srMatrix4T<float>::e_type)type;
}
