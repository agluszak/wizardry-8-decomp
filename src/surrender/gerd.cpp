#include "surrender/srGERD.h"

#include "surrender/srColorSurface.h"
#include "surrender/srCore.h"
#include "surrender/srCriticalSection.h"
#include "surrender/srDebugDD.h"
#include "surrender/srThread.h"
#include "surrender/srWindow.h"
#include "surrender/srHeap.h"
#include "surrender/srPalette.h"
#include "surrender/srVectorProcessor.h"

#include <string.h>

// GLOBAL: SURRENDER 0x100A4780
srGERD* srGERD::first;

// GLOBAL: SURRENDER 0x100A4784
srGERD* srGERD::firstOpen;

/* Retail EH funclet FUN_10010660 proves a scoped guard whose dtor releases
   the renderers critical section; every renderer-list function enters it
   once per lock acquisition. */
namespace {

class SectionAccess {
public:
    SectionAccess(srCriticalSection* section) : section_(section)
    {
        section_->getAccess();
    }
    ~SectionAccess()
    {
        section_->releaseAccess();
    }

private:
    srCriticalSection* section_;
};

} // namespace

// FUNCTION: SURRENDER 0x10017920
srDD* srGERD::getDD() const
{
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

// FUNCTION: SURRENDER 0x10017AC0
void srGERD::setTexture(srTextureIFace* texture, unsigned long layer)
{
    SectionAccess access(state_section_18_);
    if (layer < static_cast<unsigned long>(max_texture_stages_78_) &&
        texture_iface_1ffc_[layer] != texture) {
        /* Retail keeps this redundant re-test (JZ on the same pair). */
        if (texture != texture_iface_1ffc_[layer]) {
            if (texture != 0) {
                texture->addReference();
            }
            if (texture_iface_1ffc_[layer] != 0) {
                texture_iface_1ffc_[layer]->release();
            }
            texture_iface_1ffc_[layer] = texture;
        }
        changeTexture(texture, layer, 0);
        dirty_24_ |= 1 << (layer + 0xa);
    }
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
    if (state.value != vertex_arrays_21c4_.clip_08.value) {
        vertex_arrays_21c4_.clip_08 = state;
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
    minimum = environment_2058_.x;
    maximum = environment_2058_.y;
}

// FUNCTION: SURRENDER 0x1001C5A0
void srGERD::getEnvironmentScaleFactor(float& scale, float& inverse_scale)
{
    scale = environment_2058_.z;
    inverse_scale = environment_2058_.w;
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

// FUNCTION: SURRENDER 0x1001CC10
void srGERD::drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count)
{
    if ((enable_flags_20_.value & 4) == 0) {
        if ((dirty_24_ & 0x1f0) != 0) {
            applyViewStateChanges();
        }
        if ((dirty_24_ & 0xfe00) != 0) {
            applyDrawStateChanges();
        }
        if ((dirty_21c0_ & 1) != 0) {
            getDD()->setVertexArrayInfo(&vertex_arrays_21c4_);
            dirty_21c0_ &= ~1UL;
        }
        getDD()->drawArrays(primitive, first, count);
        statistics_1a78_.draw_calls_60++;
    }
}

// FUNCTION: SURRENDER 0x1001CC90
void srGERD::drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                          srRendererDefs::e_indexType type, const void* indices)
{
    if ((enable_flags_20_.value & 4) == 0) {
        if ((dirty_24_ & 0x1f0) != 0) {
            applyViewStateChanges();
        }
        if ((dirty_24_ & 0xfe00) != 0) {
            applyDrawStateChanges();
        }
        if ((dirty_21c0_ & 1) != 0) {
            getDD()->setVertexArrayInfo(&vertex_arrays_21c4_);
            dirty_21c0_ &= ~1UL;
        }
        getDD()->drawElements(primitive, count, type, indices);
        statistics_1a78_.draw_calls_60++;
    }
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
            processors[i] = vertex_processors_21b0_.data[i];
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
    if (candidate != 0 && candidate != texture_default_2030_) {
        markTextureAsDeleted(texture);
    }
}

// FUNCTION: SURRENDER 0x10028150
void srGERD::markTextureAsDeleted(Texture& texture)
{
    if (texture.device_2c.deleted_70 == 0) {
        if (texture.prev_00 != 0) {
            texture.prev_00->next_04 = texture.next_04;
        }
        if (texture.next_04 != 0) {
            texture.next_04->prev_00 = texture.prev_00;
        }
        if (&texture == texture_head_202c_) {
            texture_head_202c_ = texture.next_04;
        }
        texture.prev_00 = 0;
        Texture* previous = texture_deleted_2028_;
        texture.next_04 = previous;
        if (previous != 0) {
            previous->prev_00 = &texture;
        }
        texture_deleted_2028_ = &texture;
        texture.device_2c.deleted_70 = 1;
    }
}

// FUNCTION: SURRENDER 0x10018390
void srGERD::invalidateTextureByFrameHandle(unsigned long handle)
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    if (handle != 0 && texture_hash_enabled_2044_) {
        long index =
            texture_lookup_2004_
                .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
        if (index != -1) {
            srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
            while (entries[index].key != handle) {
                index = entries[index].next_index;
                if (index == -1) {
                    section->releaseAccess();
                    return;
                }
            }
            Texture* texture = entries[index].value;
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
        stack.stack_00[stack.depth_800] = matrix_current_390_[matrix_mode_1650_];
        stack.depth_800++;
    }
}

// FUNCTION: SURRENDER 0x10023560
void srGERD::popMatrix()
{
    srMatrix4T<float>* matrix = &matrix_current_390_[matrix_mode_1650_];
    MatrixStack& stack = matrix_stacks_410_[matrix_mode_1650_];
    if (stack.depth_800 != 0) {
        stack.depth_800--;
        *matrix = stack.stack_00[stack.depth_800];
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

// FUNCTION: SURRENDER 0x10019A40
void srGERD::flushRenderers()
{
    if (srThread::getHandle() == owner_thread_1c_) {
        flushImmediateRenderers();
        flushSort();
        SectionAccess access(renderers_section_14_);
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            while (entry->busy_0c != 0) {
                srThread::yield(0);
            }
            entry->renderer_08->reset(0);
        }
    }
}

// FUNCTION: SURRENDER 0x10019AD0
void srGERD::flushSort()
{
    if (srThread::getHandle() == owner_thread_1c_) {
        SectionAccess access(renderers_section_14_);
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            if (entry->renderer_08->sorted_d8_ == 1) {
                while (entry->busy_0c != 0) {
                    srThread::yield(0);
                }
                entry->renderer_08->submit();
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10019B60
void srGERD::flushImmediateRenderers()
{
    if (srThread::getHandle() == owner_thread_1c_) {
        SectionAccess access(renderers_section_14_);
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            if (entry->renderer_08->sorted_d8_ == 0) {
                while (entry->busy_0c != 0) {
                    srThread::yield(0);
                }
                entry->renderer_08->submit();
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10019BF0
srGERD::RendererEntry* srGERD::createRenderer(int sorted)
{
    SectionAccess access(renderers_section_14_);
    Renderer::Parameters parameters;
    parameters.gerd = this;
    parameters.sorted = sorted != 0;
    parameters.batch_limit = renderer_batch_limit_6c_;
    parameters.texture_stages = max_texture_stages_78_;
    RendererEntry* entry = new RendererEntry;
    entry->renderer_08 = new Renderer(parameters);
    entry->busy_0c = 0;
    entry->prev_00 = 0;
    entry->next_04 = renderers_10_;
    if (renderers_10_ != 0) {
        renderers_10_->prev_00 = entry;
    }
    renderers_10_ = entry;
    return entry;
}

// FUNCTION: SURRENDER 0x10019CC0
srGERD::Renderer* srGERD::lockRenderer()
{
    int sorted = 0;
    if ((enable_flags_20_.value & 2) != 0) {
        sorted = 1;
    }
    for (;;) {
        SectionAccess access(renderers_section_14_);
        flushNonBusyRenderers();
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            if (entry->busy_0c == 0 && entry->renderer_08->sorted_d8_ == sorted) {
                return _lockRenderer(entry);
            }
        }
        if (sorted == 0) {
            return _lockRenderer(createRenderer((enable_flags_20_.value >> 1) & 1));
        }
    }
}

// FUNCTION: SURRENDER 0x10019D70
srGERD::Renderer* srGERD::_lockRenderer(RendererEntry* entry)
{
    entry->busy_0c = 1;
    return entry->renderer_08;
}

// FUNCTION: SURRENDER 0x10019D90
void srGERD::unlockRenderer(Renderer* renderer, int submit)
{
    SectionAccess access(renderers_section_14_);
    for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
        if (entry->renderer_08 == renderer) {
            entry->busy_0c = 0;
            if (srThread::getHandle() == owner_thread_1c_ &&
                (submit != 0 || renderer->isBatchFull() != 0)) {
                renderer->submit();
            }
            return;
        }
    }
}

// FUNCTION: SURRENDER 0x10019E30
void srGERD::flushNonBusyRenderers()
{
    if (srThread::getHandle() == owner_thread_1c_) {
        SectionAccess access(renderers_section_14_);
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            if (entry->busy_0c == 0 && entry->renderer_08->isBatchFull() != 0) {
                entry->renderer_08->submit();
            }
        }
    }
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
    srMatrix4T<float>& matrix = matrix_current_390_[matrix_mode_1650_];
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
        srMatrix4T<float>& matrix = matrix_current_390_[matrix_mode_1650_];
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
    srMatrix4T<float>& matrix = matrix_current_390_[matrix_mode_1650_];
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
    srMatrix4T<float>& matrix = matrix_current_390_[matrix_mode_1650_];
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
    srMatrix4T<float>& matrix = matrix_current_390_[matrix_mode_1650_];
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
    matrix = matrix_current_390_[mode];
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
    statistics_1a78_.view_state_applies_40 += 1;
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
        project_clip_near_16c4_ = matrix_current_390_[MATRIX_PROJECTION];
        srMatrix4T<float>& projection = matrix_current_390_[MATRIX_PROJECTION];
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
                                         matrix_class_174c_[MATRIX_PROJECTION]);
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
        scissor_flags_1680_ |= 2;
    } else {
        scissor_flags_1680_ &= ~2UL;
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
    statistics_1a78_.matrix_classifications_7c += 1;
    if (mode == MATRIX_MODELVIEW) {
        srMatrix4T<float>* modelview = &matrix_current_390_[MATRIX_MODELVIEW];
        matrix_class_174c_[MATRIX_MODELVIEW] = (srMatrix4T<float>::e_type)0;
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
                max_modelview_scale_1748_ = (float)sqrt(length0);
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
                float scale = max_modelview_scale_1748_;
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
                max_modelview_scale_1748_ = 1.0f;
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
        max_modelview_scale_1748_ = (float)sqrt(length0);
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
    srMatrix4T<float>& matrix = matrix_current_390_[mode];
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
    matrix_class_174c_[mode] = (srMatrix4T<float>::e_type)type;
}

// FUNCTION: SURRENDER 0x1001CF40
void srGERD::assertContext() const {}

// FUNCTION: SURRENDER 0x1001BB10
void srGERD::checkViewStateChanges()
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001BB30
void srGERD::checkClipPlaneChanges()
{
    if ((dirty_24_ & 0x10000) != 0) {
        applyClipPlaneChanges();
    }
}

// FUNCTION: SURRENDER 0x1001BB20
void srGERD::checkDrawStateChanges()
{
    if ((dirty_24_ & 0xfe00) != 0) {
        applyDrawStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001D2A0
void srGERD::checkAllStateChanges()
{
    if (dirty_24_ != 0) {
        if ((dirty_24_ & 0xfe00) != 0) {
            applyDrawStateChanges();
        }
        checkFrameStateChanges();
        if ((dirty_24_ & 0x1f0) != 0) {
            applyViewStateChanges();
        }
    }
}

// FUNCTION: SURRENDER 0x10023540
srGERD::e_matrixMode srGERD::getMatrixMode() const
{
    return matrix_mode_1650_;
}

// FUNCTION: SURRENDER 0x10023350
void srGERD::getMatrix(srMatrix4T<float>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    matrix = matrix_current_390_[matrix_mode_1650_];
}

// FUNCTION: SURRENDER 0x10023390
void srGERD::getMatrix(e_matrixMode mode, srMatrix4T<double>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    const srMatrix4T<float>& current = matrix_current_390_[mode];
    srMatrix4T<double> result;
    for (int row = 0; row != 4; ++row) {
        result.vectors[row].x = (double)current.vectors[row].x;
        result.vectors[row].y = (double)current.vectors[row].y;
        result.vectors[row].z = (double)current.vectors[row].z;
        result.vectors[row].w = (double)current.vectors[row].w;
    }
    matrix = result;
}

// FUNCTION: SURRENDER 0x10023450
void srGERD::getMatrix(srMatrix4T<double>& matrix)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    const srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    srMatrix4T<double> result;
    for (int row = 0; row != 4; ++row) {
        result.vectors[row].x = (double)current.vectors[row].x;
        result.vectors[row].y = (double)current.vectors[row].y;
        result.vectors[row].z = (double)current.vectors[row].z;
        result.vectors[row].w = (double)current.vectors[row].w;
    }
    matrix = result;
}

// FUNCTION: SURRENDER 0x10021380
float srGERD::getMaxModelViewScale()
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    return max_modelview_scale_1748_;
}

// FUNCTION: SURRENDER 0x10021D20
void srGERD::pushMultMatrix(const srMatrix4x3T<float>& matrix)
{
    MatrixStack& stack = matrix_stacks_410_[matrix_mode_1650_];
    if (stack.depth_800 < 0x20) {
        stack.stack_00[stack.depth_800] = matrix_current_390_[matrix_mode_1650_];
        stack.depth_800 += 1;
    }
    srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    for (int row = 0; row != 3; ++row) {
        float x = current.vectors[row].x;
        float y = current.vectors[row].y;
        float z = current.vectors[row].z;
        current.vectors[row].w = matrix.rows[0].w * x + matrix.rows[2].w * z +
                                 matrix.rows[1].w * y + current.vectors[row].w;
        current.vectors[row].x = matrix.rows[0].x * x + matrix.rows[2].x * z + matrix.rows[1].x * y;
        current.vectors[row].y = matrix.rows[1].y * y + matrix.rows[0].y * x + matrix.rows[2].y * z;
        current.vectors[row].z = matrix.rows[0].z * x + matrix.rows[2].z * z + matrix.rows[1].z * y;
    }
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022C50
void srGERD::multMatrix(const srMatrix4T<float>& matrix)
{
    assertContext();
    srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    for (int row = 0; row != 4; ++row) {
        float x = current.vectors[row].x;
        float y = current.vectors[row].y;
        float z = current.vectors[row].z;
        float w = current.vectors[row].w;
        current.vectors[row].x = matrix.vectors[1].x * y + matrix.vectors[2].x * z +
                                 x * matrix.vectors[0].x + matrix.vectors[3].x * w;
        current.vectors[row].y = x * matrix.vectors[0].y + y * matrix.vectors[1].y +
                                 matrix.vectors[2].y * z + matrix.vectors[3].y * w;
        current.vectors[row].z = matrix.vectors[1].z * y + z * matrix.vectors[2].z +
                                 w * matrix.vectors[3].z + x * matrix.vectors[0].z;
        current.vectors[row].w = matrix.vectors[0].w * x + matrix.vectors[1].w * y +
                                 w * matrix.vectors[3].w + z * matrix.vectors[2].w;
    }
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022F10
void srGERD::multMatrix(const srMatrix4T<double>& matrix)
{
    assertContext();
    srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    for (int row = 0; row != 4; ++row) {
        float x = current.vectors[row].x;
        float y = current.vectors[row].y;
        float z = current.vectors[row].z;
        float w = current.vectors[row].w;
        current.vectors[row].x = x * (float)matrix.vectors[0].x + y * (float)matrix.vectors[1].x +
                                 z * (float)matrix.vectors[2].x + w * (float)matrix.vectors[3].x;
        current.vectors[row].y = x * (float)matrix.vectors[0].y + y * (float)matrix.vectors[1].y +
                                 z * (float)matrix.vectors[2].y + w * (float)matrix.vectors[3].y;
        current.vectors[row].z = x * (float)matrix.vectors[0].z + y * (float)matrix.vectors[1].z +
                                 z * (float)matrix.vectors[2].z + w * (float)matrix.vectors[3].z;
        current.vectors[row].w = x * (float)matrix.vectors[0].w + y * (float)matrix.vectors[1].w +
                                 z * (float)matrix.vectors[2].w + w * (float)matrix.vectors[3].w;
    }
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022A70
void srGERD::loadMatrix(const srMatrix4T<double>& matrix)
{
    srMatrix4T<float> converted;
    for (int row = 0; row != 4; ++row) {
        converted.vectors[row].x = (float)matrix.vectors[row].x;
        converted.vectors[row].y = (float)matrix.vectors[row].y;
        converted.vectors[row].z = (float)matrix.vectors[row].z;
        converted.vectors[row].w = (float)matrix.vectors[row].w;
    }
    matrix_current_390_[matrix_mode_1650_] = converted;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022B20
void srGERD::loadMatrix(const srMatrix4T<float>& matrix)
{
    matrix_current_390_[matrix_mode_1650_] = matrix;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022B50
void srGERD::loadMatrix(const srMatrix3T<double>& matrix)
{
    srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    current.vectors[0].x = (float)matrix.vectors[0].x;
    current.vectors[0].y = (float)matrix.vectors[0].y;
    current.vectors[0].z = (float)matrix.vectors[0].z;
    current.vectors[0].w = 0.0f;
    current.vectors[1].x = (float)matrix.vectors[1].x;
    current.vectors[1].y = (float)matrix.vectors[1].y;
    current.vectors[1].z = (float)matrix.vectors[1].z;
    current.vectors[1].w = 0.0f;
    current.vectors[2].x = (float)matrix.vectors[2].x;
    current.vectors[2].y = (float)matrix.vectors[2].y;
    current.vectors[2].z = (float)matrix.vectors[2].z;
    current.vectors[2].w = 0.0f;
    current.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022BD0
void srGERD::loadMatrix(const srMatrix3T<float>& matrix)
{
    srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
    current.vectors[0].x = matrix.vectors[0].x;
    current.vectors[0].y = matrix.vectors[0].y;
    current.vectors[0].z = matrix.vectors[0].z;
    current.vectors[0].w = 0.0f;
    current.vectors[1].x = matrix.vectors[1].x;
    current.vectors[1].y = matrix.vectors[1].y;
    current.vectors[1].z = matrix.vectors[1].z;
    current.vectors[1].w = 0.0f;
    current.vectors[2].x = matrix.vectors[2].x;
    current.vectors[2].y = matrix.vectors[2].y;
    current.vectors[2].z = matrix.vectors[2].z;
    current.vectors[2].w = 0.0f;
    current.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022840
void srGERD::scale(const srVector3T<float>& factors)
{
    scale(srVector3T<double>((double)factors.x, (double)factors.y, (double)factors.z));
}

// FUNCTION: SURRENDER 0x100228C0
void srGERD::scale(double factor)
{
    if (factor != 1.0) {
        srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
        current.vectors[0].x = (float)(current.vectors[0].x * factor);
        current.vectors[1].x = (float)(current.vectors[1].x * factor);
        current.vectors[2].x = (float)(current.vectors[2].x * factor);
        current.vectors[3].x = (float)(current.vectors[3].x * factor);
        current.vectors[0].y = (float)(current.vectors[0].y * factor);
        current.vectors[1].y = (float)(current.vectors[1].y * factor);
        current.vectors[2].y = (float)(current.vectors[2].y * factor);
        current.vectors[3].y = (float)(current.vectors[3].y * factor);
        current.vectors[0].z = (float)(current.vectors[0].z * factor);
        current.vectors[1].z = (float)(current.vectors[1].z * factor);
        current.vectors[2].z = (float)(current.vectors[2].z * factor);
        current.vectors[3].z = (float)(current.vectors[3].z * factor);
        setMatrixDirty();
    }
}

// FUNCTION: SURRENDER 0x10022730
void srGERD::rotate(double angle, double x, double y, double z)
{
    rotate(angle, srVector3T<double>(x, y, z));
}

// FUNCTION: SURRENDER 0x10021DF0
void srGERD::frustum(const Frustum& bounds)
{
    if (0.0 < bounds.near_plane && 0.0 < bounds.far_plane) {
        srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
        srMatrix4T<double> projection;
        double double_near = bounds.near_plane + bounds.near_plane;
        projection.vectors[3].w = 0.0;
        projection.vectors[2].w = -1.0;
        projection.vectors[0].x = double_near / (bounds.right - bounds.left);
        double scale_y = double_near / (bounds.top - bounds.bottom);
        double m22 =
            -((bounds.far_plane + bounds.near_plane) / (bounds.far_plane - bounds.near_plane));
        double m20 = (bounds.right + bounds.left) / (bounds.right - bounds.left);
        double m21 = (bounds.bottom + bounds.top) / (bounds.top - bounds.bottom);
        double m32 =
            -((bounds.far_plane * bounds.near_plane + bounds.far_plane * bounds.near_plane) /
              (bounds.far_plane - bounds.near_plane));
        for (int row = 0; row != 4; ++row) {
            float z = current.vectors[row].z;
            current.vectors[row].z = (current.vectors[row].y * (float)m21 +
                                      current.vectors[row].x * (float)m20 + z * (float)m22) -
                                     current.vectors[row].w;
            current.vectors[row].w = (float)m32 * z;
            current.vectors[row].x = current.vectors[row].x * (float)projection.vectors[0].x;
            current.vectors[row].y = current.vectors[row].y * (float)scale_y;
        }
        setMatrixDirty();
    }
}

// FUNCTION: SURRENDER 0x10021FF0
void srGERD::frustum(double left, double right, double bottom, double top, double near_plane,
                     double far_plane)
{
    Frustum bounds;
    bounds.left = left;
    bounds.right = right;
    bounds.bottom = bottom;
    bounds.top = top;
    bounds.near_plane = near_plane;
    bounds.far_plane = far_plane;
    frustum(bounds);
}

// FUNCTION: SURRENDER 0x10022060
void srGERD::perspective(double fov_y, double aspect, double near_plane, double far_plane)
{
    if (0.0 < near_plane && 0.0 < far_plane) {
        srMatrix4T<float>& current = matrix_current_390_[matrix_mode_1650_];
        double scale_y = 1.0 / tan(fov_y * 0.5);
        double scale_x = scale_y / aspect;
        float m22 = (float)((near_plane + far_plane) / (near_plane - far_plane));
        double m32 = (near_plane * far_plane + near_plane * far_plane) / (near_plane - far_plane);
        for (int row = 0; row != 4; ++row) {
            float z = current.vectors[row].z;
            current.vectors[row].x = (float)(current.vectors[row].x * scale_x);
            current.vectors[row].y = (float)(current.vectors[row].y * scale_y);
            current.vectors[row].z = (float)(z * m22 - current.vectors[row].w);
            current.vectors[row].w = (float)(z * m32);
        }
        setMatrixDirty();
    }
}

// FUNCTION: SURRENDER 0x1001B820
void srGERD::applyClipPlaneChanges()
{
    const srMatrix4T<float>& projection = matrix_current_390_[MATRIX_PROJECTION];
    float slope = (projection.vectors[2].w + projection.vectors[3].w) /
                  (projection.vectors[2].z + projection.vectors[3].z);
    float bottom = projection.vectors[0].w + 1.0f;
    float near_left = -((bottom * projection.vectors[3].w +
                         (projection.vectors[0].z - 1.0f) * projection.vectors[3].z * slope) /
                        projection.vectors[0].x);
    float top = projection.vectors[1].w + 1.0f;
    float near_top = -((top * projection.vectors[3].w +
                        (projection.vectors[1].z - 1.0f) * projection.vectors[3].z * slope) /
                       projection.vectors[1].y);
    frustum_planes_1418_[0].Set(slope, 0.0f, near_left, 0.0f);
    frustum_planes_1418_[1].Set(
        -slope, 0.0f,
        -(((projection.vectors[0].w - 1.0f) * projection.vectors[3].w) / bottom +
          ((projection.vectors[0].z + 1.0f) * projection.vectors[3].z) /
              (1.0f - projection.vectors[0].z)) *
            near_left,
        0.0f);
    frustum_planes_1418_[2].Set(
        0.0f, -slope,
        -(((projection.vectors[1].w - 1.0f) * projection.vectors[3].w) / top +
          ((projection.vectors[1].z + 1.0f) * projection.vectors[3].z) /
              (1.0f - projection.vectors[1].z)) *
            near_top,
        0.0f);
    frustum_planes_1418_[3].Set(0.0f, slope, near_top, 0.0f);
    frustum_planes_1418_[4].Set(0.0f, 0.0f, -1.0f, slope);
    frustum_planes_1418_[5].Set(0.0f, 0.0f, 1.0f,
                                (projection.vectors[2].w - projection.vectors[3].w) /
                                    (projection.vectors[2].z - projection.vectors[3].z));
    for (int plane = 0; plane != 4; ++plane) {
        float length = sqrt(frustum_planes_1418_[plane].z * frustum_planes_1418_[plane].z +
                            frustum_planes_1418_[plane].y * frustum_planes_1418_[plane].y +
                            frustum_planes_1418_[plane].w * frustum_planes_1418_[plane].w +
                            frustum_planes_1418_[plane].x * frustum_planes_1418_[plane].x);
        if (length != 0.0f) {
            float inverse_length = 1.0f / length;
            frustum_planes_1418_[plane].x *= inverse_length;
            frustum_planes_1418_[plane].y *= inverse_length;
            frustum_planes_1418_[plane].z *= inverse_length;
            frustum_planes_1418_[plane].w *= inverse_length;
        }
    }
    dirty_24_ &= ~0x10000UL;
}

// FUNCTION: SURRENDER 0x1001D4E0
void srGERD::getScissor(unsigned long& x, unsigned long& y, unsigned long& width,
                        unsigned long& height) const
{
    x = scissor_1628_.left;
    y = scissor_1628_.top;
    width = scissor_1628_.right - scissor_1628_.left;
    height = scissor_1628_.bottom - scissor_1628_.top;
}

// FUNCTION: SURRENDER 0x1001D530
void srGERD::getViewPort(unsigned long& x, unsigned long& y, unsigned long& width,
                         unsigned long& height) const
{
    x = view_left_1638_;
    y = view_top_163c_;
    width = view_right_1640_ - view_left_1638_;
    height = view_bottom_1644_ - view_top_163c_;
}

// FUNCTION: SURRENDER 0x1001C010
void srGERD::pushClipPlane(const srVector4T<float>& plane, e_clipMode mode)
{
    if ((unsigned long)clip_plane_count_167c_ < 0x1a) {
        if ((dirty_24_ & 0x10000) != 0) {
            applyClipPlaneChanges();
        }
        if ((dirty_24_ & 0x1f0) != 0) {
            applyViewStateChanges();
        }
        unsigned long bit = 1UL << (clip_plane_count_167c_ + 6);
        float inverse_length =
            1.0f / sqrt(plane.z * plane.z + plane.x * plane.x + plane.y * plane.y);
        float x = plane.x * inverse_length;
        float y = plane.y * inverse_length;
        float z = plane.z * inverse_length;
        float w = inverse_length * plane.w;
        srVector4T<float>& eye_plane = user_clip_planes_1478_[clip_plane_count_167c_];
        eye_plane.x =
            x * inverse_modelview_1684_.vectors[0].x + y * inverse_modelview_1684_.vectors[1].x +
            z * inverse_modelview_1684_.vectors[2].x + w * inverse_modelview_1684_.vectors[3].x;
        eye_plane.y =
            x * inverse_modelview_1684_.vectors[0].y + y * inverse_modelview_1684_.vectors[1].y +
            z * inverse_modelview_1684_.vectors[2].y + w * inverse_modelview_1684_.vectors[3].y;
        eye_plane.z =
            x * inverse_modelview_1684_.vectors[0].z + y * inverse_modelview_1684_.vectors[1].z +
            z * inverse_modelview_1684_.vectors[2].z + w * inverse_modelview_1684_.vectors[3].z;
        eye_plane.w =
            x * inverse_modelview_1684_.vectors[0].w + y * inverse_modelview_1684_.vectors[1].w +
            z * inverse_modelview_1684_.vectors[2].w + w * inverse_modelview_1684_.vectors[3].w;
        clip_modes_165a_[clip_plane_count_167c_] = static_cast<unsigned char>(mode);
        clip_mask_1674_ |= bit;
        if (mode == 1) {
            clip_mode1_mask_1678_ |= bit;
        } else {
            clip_mode1_mask_1678_ &= ~bit;
        }
        clip_plane_count_167c_ += 1;
    }
}

// FUNCTION: SURRENDER 0x1001C1F0
void srGERD::popClipPlane()
{
    if (clip_plane_count_167c_ != 0) {
        clip_plane_count_167c_ -= 1;
        unsigned long mask = ~(1UL << (clip_plane_count_167c_ + 6));
        clip_mask_1674_ &= mask;
        clip_mode1_mask_1678_ &= mask;
    }
}

// FUNCTION: SURRENDER 0x1001C230
void srGERD::getClipPlanes(ClipPlanes& planes)
{
    if ((dirty_24_ & 0x10000) != 0) {
        applyClipPlaneChanges();
    }
    for (int plane = 0; plane != 6; ++plane) {
        planes.planes_000[plane] = frustum_planes_1418_[plane];
    }
    if ((clip_mask_1674_ & 0xffffffc0) != 0) {
        for (unsigned long index = 6; index < 0x20; ++index) {
            if ((clip_mask_1674_ & (1UL << index)) != 0) {
                planes.planes_000[index] = user_clip_planes_1478_[index - 6];
            }
        }
    }
    planes.mask_200 = clip_mask_1674_;
    planes.value_204 = clip_mode1_mask_1678_;
}

// FUNCTION: SURRENDER 0x1001C5C0
void srGERD::pushEnvironment()
{
    if (environment_depth_2168_ < 0x10) {
        environment_stack_2068_[environment_depth_2168_] = environment_2058_;
        environment_depth_2168_ += 1;
    }
}

// FUNCTION: SURRENDER 0x1001C600
void srGERD::popEnvironment()
{
    if (environment_depth_2168_ != 0) {
        environment_depth_2168_ -= 1;
        environment_2058_ = environment_stack_2068_[environment_depth_2168_];
    }
}

// FUNCTION: SURRENDER 0x1001C4D0
void srGERD::setEnvironmentRange(float minimum, float maximum)
{
    environment_2058_.x = minimum;
    environment_2058_.y = maximum;
}

// FUNCTION: SURRENDER 0x1001C510
void srGERD::setEnvironmentScaleFactor(float scale, float inverse_scale)
{
    if (0.0f < scale) {
        if (1.0f <= scale) {
            scale = 1.0f;
        }
    } else {
        scale = 0.0f;
    }
    environment_2058_.z = scale;
    if (0.0f < inverse_scale) {
        if (inverse_scale < 1.0f) {
            environment_2058_.w = inverse_scale;
            return;
        }
        environment_2058_.w = 1.0f;
        return;
    }
    environment_2058_.w = 0.0f;
}

// FUNCTION: SURRENDER 0x1001CE00
void srGERD::pushVertexProcessor(srVertexProcessor& processor)
{
    unsigned long count = vertex_processor_count_21b8_;
    if (vertex_processors_21b0_.capacity <= count) {
        vertex_processors_21b0_.setCapacity(vertex_processors_21b0_.capacity + 8 + count);
    }
    vertex_processors_21b0_[count] = &processor;
    vertex_processor_count_21b8_ += 1;
}

// FUNCTION: SURRENDER 0x1001CEA0
void srGERD::popVertexProcessor()
{
    if (vertex_processor_count_21b8_ > 0) {
        vertex_processor_count_21b8_--;
    }
}

// FUNCTION: SURRENDER 0x1001C640
void srGERD::setFogColor(const srVector3T<float>& color)
{
    srVector4T<float> clamped;
    clamped.Set(color.x, color.y, color.z, 0.0f);
    setFogColor(clamped);
}

/* Each component clamps through (0,1) — strictly positive keeps the value,
   1.0 or above saturates, anything else becomes 0. The color only updates
   (and dirties state) when a clamped component differs. */
// FUNCTION: SURRENDER 0x1001C6B0
void srGERD::setFogColor(const srVector4T<float>& color)
{
    srVector4T<float> clamped = color;
    if (clamped.x <= 0.0f) {
        clamped.x = 0.0f;
    } else if (clamped.x >= 1.0f) {
        clamped.x = 1.0f;
    }
    if (clamped.y <= 0.0f) {
        clamped.y = 0.0f;
    } else if (clamped.y >= 1.0f) {
        clamped.y = 1.0f;
    }
    if (clamped.z <= 0.0f) {
        clamped.z = 0.0f;
    } else if (clamped.z >= 1.0f) {
        clamped.z = 1.0f;
    }
    if (clamped.w <= 0.0f) {
        clamped.w = 0.0f;
    } else if (clamped.w >= 1.0f) {
        clamped.w = 1.0f;
    }
    if (clamped.x != fog_color_1fe8_.x || clamped.y != fog_color_1fe8_.y ||
        clamped.z != fog_color_1fe8_.z || clamped.w != fog_color_1fe8_.w) {
        flushImmediateRenderers();
        fog_color_1fe8_ = clamped;
        dirty_24_ |= 0x200;
    }
}

// FUNCTION: SURRENDER 0x1001C680
void srGERD::getFogColor(srVector4T<float>& color) const
{
    color = fog_color_1fe8_;
}

// FUNCTION: SURRENDER 0x1001C440
void srGERD::setAmbientLight(const srVector3T<float>& light)
{
    srVector4T<float> expanded;
    expanded.x = light.x;
    expanded.w = 0.0f;
    expanded.y = light.y;
    expanded.z = light.z;
    setAmbientLight(expanded);
}

// FUNCTION: SURRENDER 0x1001DB20
unsigned long srGERD::getPickKey() const
{
    return pick_key_19f0_;
}

// FUNCTION: SURRENDER 0x1001DB30
srGERD::e_visibility srGERD::testBoundingSphere(const srVector3T<float>& center, float radius)
{
    statistics_1a78_.sphere_tests_6c++;
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    if ((dirty_24_ & 0x10000) != 0) {
        applyClipPlaneChanges();
    }
    const srMatrix4T<float>& modelview = matrix_current_390_[MATRIX_MODELVIEW];
    float eye_z = modelview.vectors[2].z * center.z + modelview.vectors[2].y * center.y +
                  modelview.vectors[2].x * center.x + modelview.vectors[2].w;
    float negative_radius = -(radius * max_modelview_scale_1748_);
    if (eye_z * frustum_planes_1418_[4].z + frustum_planes_1418_[4].w <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    if (eye_z * frustum_planes_1418_[5].z + frustum_planes_1418_[5].w <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    float eye_x = modelview.vectors[0].x * center.x + modelview.vectors[0].z * center.z +
                  modelview.vectors[0].y * center.y + modelview.vectors[0].w;
    if (eye_x * frustum_planes_1418_[0].x + eye_z * frustum_planes_1418_[0].z <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    if (eye_z * frustum_planes_1418_[1].z + eye_x * frustum_planes_1418_[1].x <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    float eye_y = modelview.vectors[1].z * center.z + modelview.vectors[1].y * center.y +
                  modelview.vectors[1].x * center.x + modelview.vectors[1].w;
    if (eye_z * frustum_planes_1418_[2].z + eye_y * frustum_planes_1418_[2].y <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    if (eye_z * frustum_planes_1418_[3].z + eye_y * frustum_planes_1418_[3].y <= negative_radius) {
        return VISIBILITY_POSITIONAL_0;
    }
    unsigned long remaining = clip_mask_1674_ & 0xffffffc0;
    if (remaining != 0) {
        for (unsigned long index = 6; index < 0x20; ++index) {
            if (remaining == 0) {
                break;
            }
            unsigned long bit = 1UL << index;
            if ((remaining & bit) != 0) {
                const srVector4T<float>& plane = user_clip_planes_1478_[index - 6];
                if (eye_z * plane.z + eye_x * plane.x + eye_y * plane.y + plane.w <=
                    negative_radius) {
                    return VISIBILITY_POSITIONAL_0;
                }
                remaining &= ~bit;
            }
        }
    }
    statistics_1a78_.sphere_visible_70++;
    return static_cast<e_visibility>(1);
}

// FUNCTION: SURRENDER 0x1001DD60
srGERD::e_visibility srGERD::testBoundingBox(const srVector3T<float>& minimum,
                                             const srVector3T<float>& maximum)
{
    statistics_1a78_.box_tests_74++;
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    srMatrix4T<float> combined = matrix_current_390_[MATRIX_PROJECTION];
    combined.MultiplyBy(matrix_current_390_[MATRIX_MODELVIEW]);
    if (srVectorProcessor::vp->_srTestBoundingBox(combined, minimum, maximum) != 0) {
        statistics_1a78_.box_visible_78++;
        return static_cast<e_visibility>(1);
    }
    return VISIBILITY_POSITIONAL_0;
}

// FUNCTION: SURRENDER 0x100235F0
srVector4T<float> srGERD::getEyeSpaceLocation(const srVector3T<float>& object_location)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    return matrix_current_390_[MATRIX_MODELVIEW].Transform(object_location);
}

// FUNCTION: SURRENDER 0x1001DDF0
void srGERD::getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                               const srVector3T<float>& object_center, float object_radius)
{
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    const srMatrix4T<float>& modelview = matrix_current_390_[MATRIX_MODELVIEW];
    center.x = modelview.vectors[0].x * object_center.x + modelview.vectors[0].y * object_center.y +
               modelview.vectors[0].z * object_center.z + modelview.vectors[0].w;
    center.z = modelview.vectors[2].y * object_center.y + modelview.vectors[2].z * object_center.z +
               modelview.vectors[2].x * object_center.x + modelview.vectors[2].w;
    center.y = modelview.vectors[1].y * object_center.y + modelview.vectors[1].z * object_center.z +
               modelview.vectors[1].x * object_center.x + modelview.vectors[1].w;
    radius = object_radius * max_modelview_scale_1748_;
}

namespace {

/* RAII state-lock guard: applyDrawStateChanges carries an EH funclet that
   releases the section on unwind, matching an object of this shape. */
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

// FUNCTION: SURRENDER 0x1001B570
void srGERD::applyDrawStateChanges()
{
    GerdAccess access(state_section_18_);
    if ((dirty_24_ & 0x200) != 0) {
        getDD()->setFogColor(fog_color_1fe8_);
    }
    if ((dirty_24_ & 0x1000) != 0) {
        getDD()->setShader(shader_1ff8_);
        statistics_1a78_.shader_sets_5c++;
    }
    removeDeletedTextures();
    if ((dirty_24_ & 0x400) != 0) {
        changeTexture(texture_iface_1ffc_[0], 0, 1);
    }
    if (((dirty_24_ & 0x800) != 0) && (1 < (unsigned long)max_texture_stages_78_)) {
        changeTexture(texture_iface_1ffc_[1], 1, 1);
    }
    if ((dirty_24_ & 0x2000) != 0) {
        int apply_cull = 1;
        long cull;
        if (cull_mode_1648_ == CULL_NONE) {
            cull = (winding_164c_ != 0) + 1;
        } else if (cull_mode_1648_ == CULL_BACK) {
            cull = (winding_164c_ == 0) + 1;
        } else if (cull_mode_1648_ == CULL_FRONT) {
            cull = 0;
        } else {
            apply_cull = 0;
        }
        if (apply_cull != 0) {
            getDD()->setCullMode(static_cast<srDD::e_cullMode>(cull));
        }
    }
    if ((dirty_24_ & 0x4000) != 0) {
        if (polygon_mode_1fe0_ <= 2) {
            getDD()->setPolygonMode(static_cast<srDD::e_polygonMode>(polygon_mode_1fe0_));
        }
    }
    if ((dirty_24_ & 0x8000) != 0) {
        getDD()->setPolygonOffset(polygon_offset_1fe4_);
    }
    dirty_24_ &= 0xffff01ff;
    statistics_1a78_.draw_state_applies_44++;
}

// FUNCTION: SURRENDER 0x100281B0
void srGERD::removeDeletedTextures()
{
    Texture* texture = texture_deleted_2028_;
    while (texture != 0) {
        Texture* next = texture->next_04;
        deleteTexture(*texture);
        texture = next;
    }
}

// FUNCTION: SURRENDER 0x100286B0
void srGERD::releaseTextureSurfaceData(Texture& texture)
{
    for (long index = 0; index < 12; ++index) {
        texture.device_2c.levels_38[index] = 0;
    }
    if (texture.surface_data_20 != 0) {
        srHeap.free(texture.surface_data_20);
        texture.surface_data_20 = 0;
    }
    texture_cache_used_2034_ -= texture.device_2c.size_1c;
    texture.device_2c.size_1c = 0;
}

// FUNCTION: SURRENDER 0x100286F0
void srGERD::deleteTexture(Texture& texture)
{
    for (unsigned long stage = 0; stage < (unsigned long)max_texture_stages_78_; ++stage) {
        if (&texture == texture_slots_1f38_[stage]) {
            texture_slots_1f38_[stage] = 0;
            dirty_24_ |= 1UL << (stage + 10);
        }
    }
    if (texture.prev_00 != 0) {
        texture.prev_00->next_04 = texture.next_04;
    }
    if (texture.next_04 != 0) {
        texture.next_04->prev_00 = texture.prev_00;
    }
    if (&texture == texture_deleted_2028_) {
        texture_deleted_2028_ = texture.next_04;
    }
    texture_lookup_2004_.Remove(&texture.id_08);
    getDD()->deleteTexture(texture.device_2c);
    if (texture.name_28 != 0) {
        srHeap.free(texture.name_28);
        texture.name_28 = 0;
    }
    texture.device_2c.unknown_68 = 0;
    texture.device_2c.unknown_6c = 0;
    texture.prev_00 = 0;
    texture.next_04 = 0;
    texture.device_2c.deleted_70 = 0;
    releaseTextureSurfaceData(texture);
    if (texture.palette_24 != 0) {
        texture.palette_24->release();
        texture.palette_24 = 0;
    }
    texture.id_08 = 0;
    texture.device_2c.flags_00 = 0;
    texture.device_2c.size_1c = 0;
    texture.device_2c.last_use_18 = 0;
    texture.device_2c.priority_14 = 0.5f;
    texture.device_2c.unknown_68 = 0;
    texture.device_2c.width_20 = 0;
    texture.device_2c.height_24 = 0;
    texture.device_2c.first_level_28 = 0;
    texture.device_2c.last_level_2c = 0;
    texture.device_2c.format_index_30 = 0;
    texture.device_2c.parameter_34 = 0;
    texture.device_2c.unknown_6c = 0;
    for (long level = 0; level < 12; ++level) {
        texture.device_2c.levels_38[level] = 0;
    }
    --texture_count_2014_;
    texture.prev_00 = texture_free_2018_;
    texture_free_2018_ = &texture;
    if (texture_count_2014_ == 0) {
        for (unsigned long chunk = 0; chunk < texture_pool_count_2024_; ++chunk) {
            srHeap.free(texture_pool_201c_[chunk]);
        }
        texture_pool_201c_.release();
        texture_free_2018_ = 0;
        texture_pool_count_2024_ = 0;
        texture_count_2014_ = 0;
    }
}

// FUNCTION: SURRENDER 0x10028910
srGERD::Texture* srGERD::findLowestPriority()
{
    Texture* texture = texture_head_202c_;
    Texture* found = 0;
    unsigned long last_use = texture_sequence_203c_ + 1;
    float lowest = 1.01f;
    if (texture == 0) {
        return 0;
    }
    do {
        if (texture->device_2c.priority_14 < lowest ||
            (texture->device_2c.priority_14 == lowest &&
             texture->device_2c.last_use_18 < last_use)) {
            int bound = 0;
            if (max_texture_stages_78_ != 0) {
                Texture** slot = texture_slots_1f38_;
                unsigned long stage = 0;
                do {
                    if (*slot == texture) {
                        bound = 1;
                        break;
                    }
                    ++stage;
                    ++slot;
                } while (stage < (unsigned long)max_texture_stages_78_);
            }
            if (bound == 0) {
                lowest = texture->device_2c.priority_14;
                last_use = texture->device_2c.last_use_18;
                found = texture;
            }
        }
        texture = texture->next_04;
        if (texture == 0) {
            return found;
        }
    } while (true);
}

// FUNCTION: SURRENDER 0x10028990
srGERD::Texture* srGERD::allocTexture(unsigned long id)
{
    if (texture_free_2018_ == 0) {
        unsigned long chunk_count = texture_count_2014_;
        if (chunk_count < 2) {
            chunk_count = 1;
        } else if (0xff < (long)chunk_count) {
            chunk_count = 0x100;
        }
        Texture* chunk = static_cast<Texture*>(srHeap.allocate(chunk_count * sizeof(Texture)));
        texture_pool_201c_[texture_pool_count_2024_++] = chunk;
        Texture* link = chunk;
        for (unsigned long index = chunk_count; index != 0; --index) {
            link->prev_00 = link + 1;
            ++link;
        }
        chunk[chunk_count - 1].prev_00 = 0;
    }
    Texture* texture = texture_free_2018_;
    texture_free_2018_ = texture->prev_00;
    ++texture_count_2014_;
    /* Retail zeroes 0xa4 bytes: the aligned/unaligned dword-and-byte fill is
       memset lowering; the trailing dword is the free-list link, already
       consumed above. */
    memset(texture, 0, 0xa4);
    texture->id_08 = id;
    if (texture->palette_24 != 0) {
        texture->palette_24->release();
        texture->palette_24 = 0;
    }
    texture_lookup_2004_.Insert(&texture->id_08, &texture);
    texture->next_04 = texture_head_202c_;
    texture->prev_00 = 0;
    if (texture_head_202c_ != 0) {
        texture_head_202c_->prev_00 = texture;
    }
    texture_head_202c_ = texture;
    texture->device_2c.flags_00 = 0;
    texture->device_2c.size_1c = 0;
    texture->device_2c.last_use_18 = 0;
    texture->device_2c.priority_14 = 0.5f;
    texture->device_2c.unknown_68 = 0;
    texture->device_2c.width_20 = 0;
    texture->device_2c.height_24 = 0;
    texture->device_2c.first_level_28 = 0;
    texture->device_2c.last_level_2c = 0;
    texture->device_2c.format_index_30 = 0;
    texture->device_2c.parameter_34 = 0;
    texture->device_2c.unknown_6c = 0;
    for (long level = 0; level < 12; ++level) {
        texture->device_2c.levels_38[level] = 0;
    }
    texture->name_28 = 0;
    texture->device_2c.resident_74 = 0;
    texture->device_2c.deleted_70 = 0;
    return texture;
}

// FUNCTION: SURRENDER 0x10029390
void srGERD::invalidatePalette()
{
    if (palette_1fdc_ != 0) {
        getDD()->deletePalette(palette_1f50_);
        if (palette_1fdc_ != 0) {
            palette_1fdc_->release();
            palette_1fdc_ = 0;
        }
        palette_1f50_.data_00 = 0;
        palette_1f50_.size_04 = 0;
    }
}

// FUNCTION: SURRENDER 0x10028FB0
void srGERD::setTextureParameters(unsigned long stage, const srTextureIFace::Parameters& parameters)
{
    unsigned long state = parameters.packed_state_00;
    float bias = parameters.mipmap_bias_04;
    unsigned long packed = ((((correction_map_1fa4_[(state >> 0xc) & 1] & 0xfffffff3) |
                              (detail_map_1fac_[(state >> 0xd) & 1] << 2))
                                 << 2 |
                             (mipmap_map_1f94_[(state >> 10) & 3] & 0xffffffc3))
                                << 2 |
                            (min_filter_map_1f80_[(state >> 7) & 7] & 0xffffff03))
                               << 2 |
                           (mag_filter_map_1f6c_[(state >> 4) & 7] & 0xfffffc0f);
    packed = (packed << 4) | (wrap_map_1f5c_[state & 3] & 0xffffc00f);
    srDD::TexParms* parms = &texture_parms_1f40_[stage];
    if (packed == parms->packed_00 && bias == parms->mipmap_bias_04) {
        return;
    }
    parms->packed_00 = packed;
    parms->mipmap_bias_04 = bias;
    ++statistics_1a78_.texture_parameter_sets_50;
    getDD()->setTextureParameters(stage, *parms);
}

// FUNCTION: SURRENDER 0x10029400
void srGERD::changeTexture(srTextureIFace* texture, unsigned long stage, int apply_parms)
{
    if ((state_flags_28_ & 0x10) != 0) {
        return;
    }
    if ((unsigned long)max_texture_stages_78_ <= stage) {
        return;
    }
    Texture* found;
    if (texture == 0) {
        if (texture_slots_1f38_[stage] == texture_default_2030_) {
            return;
        }
    } else {
        unsigned long id = texture->getTextureFrameHandle();
        if (id != 0) {
            int slot = texture_lookup_2004_.FindNextEntry(&id, -1);
            if (slot != -1) {
                found = texture_lookup_2004_.entries[slot].value;
                if (found != 0) {
                    goto bound;
                }
            }
            found = createNewTexture(texture);
            if (found != 0) {
                goto bound;
            }
        }
    }
    found = texture_default_2030_;
    texture = srCore.getTexture();
bound:
    ++texture_sequence_203c_;
    found->device_2c.last_use_18 = texture_sequence_203c_;
    if (apply_parms == 0) {
        return;
    }
    if (found->palette_24 != 0 && found->palette_24 != palette_1fdc_) {
        ++statistics_1a78_.palette_binds_58;
        invalidatePalette();
        srPalette* palette = found->palette_24;
        palette_1f50_.flags_08 = 0;
        palette_1f50_.data_00 = palette->getPaletteDataPtr();
        unsigned long size = palette->getPaletteSize();
        palette_1f50_.size_04 = size;
        if (0x100 < size) {
            palette_1f50_.size_04 = 0x100;
        }
        getDD()->bindPalette(palette_1f50_);
        if (palette != palette_1fdc_) {
            if (palette != 0) {
                palette->addReference();
            }
            if (palette_1fdc_ != 0) {
                palette_1fdc_->release();
            }
            palette_1fdc_ = palette;
        }
    }
    Texture* previous = texture_slots_1f38_[stage];
    texture_slots_1f38_[stage] = found;
    if (previous != found) {
        getDD()->bindTexture(stage, found->device_2c);
        if (found != texture_default_2030_ && found->device_2c.resident_74 == 0 &&
            found->surface_data_20 != 0 && (flags_68_ & 0x20) != 0) {
            releaseTextureSurfaceData(*found);
        }
        ++statistics_1a78_.texture_binds_4c;
    }
    srTextureIFace::Parameters parameters;
    texture->getTextureParms(parameters);
    setTextureParameters(stage, parameters);
}

namespace {

/* evaluateTextureDimensions' next-power-of-two clamp emits the unrolled
   bit-scan both times the pattern appears. */
unsigned long nextTextureDimension(unsigned long value)
{
    if (value < 2) {
        return 1;
    }
    unsigned long v = value - 1;
    if (v == 0) {
        return 1;
    }
    unsigned char shift = 0;
    if ((v & 0xffff0000) != 0) {
        shift = 16;
        v >>= 16;
    }
    if ((v >> 8) != 0) {
        shift += 8;
        v >>= 8;
    }
    if ((v & 0xf0) != 0) {
        shift += 4;
        v >>= 4;
    }
    if ((v & 0xc) != 0) {
        shift += 2;
        v >>= 2;
    }
    if ((v & 2) != 0) {
        shift += 1;
    }
    return 1UL << (shift + 1U & 0x1f);
}

} // namespace

// FUNCTION: SURRENDER 0x10018EE0
void srGERD::convertPixelFormat(srDD::PixelFormat& device,
                                const srPixelConvert::PixelFormat& format)
{
    device.red_bits = format.red_bits;
    device.green_bits = format.green_bits;
    device.blue_bits = format.blue_bits;
    device.alpha_bits = format.alpha_bits;
    device.red_shift = format.red_shift;
    device.green_shift = format.green_shift;
    device.blue_shift = format.blue_shift;
    device.alpha_shift = format.alpha_shift;
    device.conversion_class = format.conversion_class;
    device.bytes_per_pixel_minus_one = format.bytes_per_pixel_minus_one;
}

// FUNCTION: SURRENDER 0x10028B80
void srGERD::evaluateTextureDimensions(srDD::Texture& device,
                                       const srTextureIFace::Dimensions& dimensions)
{
    unsigned long min_dim = texture_min_dim_7c_;
    unsigned char reduction = 0;
    if ((dimensions.hints & 0x80) == 0) {
        reduction = (unsigned char)texture_reduction_2040_;
    }
    unsigned long width = nextTextureDimension(dimensions.width) >> (reduction & 0x1f);
    unsigned long height = nextTextureDimension(dimensions.height) >> (reduction & 0x1f);
    unsigned long device_width = min_dim;
    if (min_dim <= width && width <= texture_max_dim_80_) {
        device_width = width;
    }
    unsigned long device_height = min_dim;
    if (min_dim <= height && height <= texture_max_dim_80_) {
        device_height = height;
    }
    int unbalanced = 0;
    if (device_height < device_width) {
        while (texture_max_aspect_84_ < device_width / device_height &&
               device_height < texture_max_dim_80_) {
            device_height *= 2;
        }
        unbalanced = device_height < device_width;
    }
    if (unbalanced == 0 && device_height != device_width) {
        while (texture_max_aspect_84_ < device_height / device_width &&
               device_width < texture_max_dim_80_) {
            device_width *= 2;
        }
    }
    device.width_20 = device_width;
    device.height_24 = device_height;
    device.first_level_28 = 0;
    device.last_level_2c = 0;
    if (((dimensions.hints & 8) == 0 || (flags_68_ & 0x40) != 0) &&
        texture_min_dim_7c_ < device_width) {
        do {
            if (device_height <= texture_min_dim_7c_) {
                return;
            }
            ++device.last_level_2c;
            device_width >>= 1;
            device_height >>= 1;
        } while (texture_min_dim_7c_ < device_width);
    }
}

// FUNCTION: SURRENDER 0x10028D20
void srGERD::evaluateTexturePixelFormat(Texture& texture,
                                        const srTextureIFace::Dimensions& dimensions)
{
    unsigned long flags = dimensions.hints;
    srPixelConvert::PixelFormat format = dimensions.format;
    if ((flags & 0x100) != 0) {
        srPixelConvert::mapPixelFormat(
            static_cast<srPixelConvert::e_surfaceType>(format.alpha_bits != 0 ? 6 : 2), format);
    }
    if ((dimensions.hints & 0x10) != 0) {
        srPixelConvert::mapPixelFormat(static_cast<srPixelConvert::e_surfaceType>(3), format);
    }
    flags = dimensions.hints;
    if ((flags & 2) == 0) {
        if ((flags & 4) != 0) {
            format.alpha_bits = 1;
        }
    } else {
        format.alpha_bits = 0;
    }
    if ((flags & 0x40) != 0) {
        texture.device_2c.flags_00 |= 1;
    }
    texture.device_2c.resident_74 = (dimensions.hints >> 5) & 1;
    convertPixelFormat(texture.device_2c.format_04, format);
    unsigned long index =
        format.match(texture_formats_360_, (unsigned long)texture_format_count_364_);
    texture.device_2c.format_index_30 = index;
    texture.pixel_format_0c = texture_formats_360_[index];
    if (texture.pixel_format_0c.conversion_class == 3) {
        if (dimensions.palette != 0) {
            dimensions.palette->addReference();
        }
        if (texture.palette_24 != 0) {
            texture.palette_24->release();
        }
        texture.palette_24 = dimensions.palette;
    } else if (texture.palette_24 != 0) {
        texture.palette_24->release();
        texture.palette_24 = 0;
    }
    texture.device_2c.parameter_34 = default_texture_params_1fc4_[dimensions.compression];
}

// FUNCTION: SURRENDER 0x10028E60
void srGERD::releaseTextureMemory(long bytes)
{
    if (0 < bytes) {
        Texture* texture = findLowestPriority();
        while (texture != 0) {
            bytes -= (long)texture->device_2c.size_1c;
            markTextureAsDeleted(*texture);
            deleteTexture(*texture);
            if (bytes < 1) {
                return;
            }
            texture = findLowestPriority();
        }
    }
}

// FUNCTION: SURRENDER 0x10028EB0
unsigned long srGERD::getTextureBytesNeeded(const Texture& texture) const
{
    unsigned long width = texture.device_2c.width_20;
    unsigned long height = texture.device_2c.height_24;
    unsigned long bytes = 0;
    if (texture.device_2c.first_level_28 <= texture.device_2c.last_level_2c) {
        long count = (long)(texture.device_2c.last_level_2c - texture.device_2c.first_level_28) + 1;
        do {
            bytes += height * width * (texture.pixel_format_0c.bytes_per_pixel_minus_one + 1);
            width >>= 1;
            height >>= 1;
            --count;
        } while (count != 0);
    }
    return bytes;
}

// FUNCTION: SURRENDER 0x10028EF0
void srGERD::allocTextureData(Texture& texture)
{
    unsigned long size = getTextureBytesNeeded(texture);
    if (texture_cache_size_2038_ != 0 &&
        texture_cache_size_2038_ < size + texture_cache_used_2034_) {
        float priority = texture.device_2c.priority_14;
        texture.device_2c.priority_14 = 1.2f;
        releaseTextureMemory((long)(size - texture_cache_size_2038_ + texture_cache_used_2034_));
        texture.device_2c.priority_14 = priority;
    }
    texture.surface_data_20 = srHeap.allocate(size);
    texture.device_2c.size_1c = size;
    texture_cache_used_2034_ += size;
    long bytes_per_pixel = texture.pixel_format_0c.bytes_per_pixel_minus_one;
    unsigned char* data = (unsigned char*)texture.surface_data_20;
    unsigned long width = texture.device_2c.width_20;
    unsigned long height = texture.device_2c.height_24;
    unsigned long level = texture.device_2c.first_level_28;
    if (level <= texture.device_2c.last_level_2c) {
        void** levels = &texture.device_2c.levels_38[level];
        do {
            *levels = data;
            data += height * width * (unsigned long)(bytes_per_pixel + 1);
            width >>= 1;
            height >>= 1;
            ++level;
            ++levels;
        } while (level <= texture.device_2c.last_level_2c);
    }
}

// FUNCTION: SURRENDER 0x10029090
srGERD::Texture* srGERD::createNewTexture(srTextureIFace* texture)
{
    srTextureIFace::Dimensions dimensions;
    dimensions.palette = srCore.getPalette();
    dimensions.filter = srCore.getFilter();
    dimensions.hints = 0;
    dimensions.compression = srTextureIFace::COMPRESSION_DEFAULT;
    srPixelConvert::mapPixelFormat(static_cast<srPixelConvert::e_surfaceType>(0xb),
                                   dimensions.format);
    Texture* result = allocTexture(texture->getTextureFrameHandle());
    const char* name = texture->getName();
    if (name == 0 || *name == 0) {
        result->name_28 = 0;
    } else {
        char* copy = (char*)srHeap.allocate(strlen(name) + 1);
        result->name_28 = copy;
        strcpy(copy, name);
    }
    dimensions.compression =
        static_cast<srTextureIFace::e_compression>(default_parameter_index_1fd8_);
    dimensions.width = 1;
    dimensions.height = 1;
    if (dimensions.palette != 0) {
        dimensions.palette->release();
        dimensions.palette = 0;
    }
    dimensions.format = *texture_formats_360_;
    dimensions.filter = 0;
    dimensions.hints = 0;
    texture->getDimensions(dimensions);
    evaluateTextureDimensions(result->device_2c, dimensions);
    evaluateTexturePixelFormat(*result, dimensions);
    allocTextureData(*result);
    srTextureIFace::MultiRequest request;
    request.mipmap_level = (long)result->device_2c.first_level_28;
    request.last_level_04 = result->device_2c.last_level_2c;
    unsigned long width = result->device_2c.width_20;
    unsigned long height = result->device_2c.height_24;
    long level = request.mipmap_level;
    for (; level <= (long)request.last_level_04; ++level) {
        srColorSurface* surface = new srColorSurface(
            result->pixel_format_0c, result->device_2c.levels_38[level], width, height,
            (unsigned long)(result->pixel_format_0c.bytes_per_pixel_minus_one + 1) * width);
        request.destinations[level] = surface;
        if (result->palette_24 != 0) {
            surface->setPalette(result->palette_24);
        }
        surface->setFilter(dimensions.filter);
        width >>= 1;
        height >>= 1;
    }
    texture->getMipmapData(request);
    for (level = request.mipmap_level; level <= (long)request.last_level_04; ++level) {
        request.destinations[level]->release();
    }
    result->device_2c.unknown_68 = 0;
    result->device_2c.priority_14 = texture->getPriority();
    ++statistics_1a78_.textures_created_54;
    if (dimensions.palette != 0) {
        dimensions.palette->release();
    }
    return result;
}
