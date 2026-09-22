#include "surrender/srGERD.h"

#include "surrender/srColorSurface.h"
#include "surrender/srConfig.h"
#include "surrender/srCore.h"
#include "surrender/srCriticalSection.h"
#include "surrender/srDebug.h"
#include "surrender/srDebugDD.h"
#include "surrender/srDynamicLibrary.h"
#include "surrender/srStringTable.h"
#include "surrender/srSystem.h"
#include "surrender/srThread.h"
#include "surrender/srWindow.h"
#include "surrender/srHeap.h"
#include "surrender/srPalette.h"
#include "surrender/srVectorProcessor.h"

#include <ctype.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIZ8_CLANG_LINT)
/* The lint lane's stub <ostream> declares only the operator<< overloads the
   recovered ABI references. dump calls std::endl - the real VC6 header
   resolves it to the _CRTIMP char overload imported from MSVCP60 - so the
   compile-only lane needs this declaration to parse. */
namespace std {
ostream& endl(ostream& stream);
}
#endif

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
    if (layer < info_50_.max_texture_stages_28_ && texture_iface_1ffc_[layer] != texture) {
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

// FUNCTION: SURRENDER 0x1001AAF0
void srGERD::getStatistics(Statistics& statistics)
{
    SectionAccess access(renderers_section_14_);
    statistics_1a78_.value_34 = 0;
    statistics_1a78_.value_3c = 0;
    statistics_1a78_.value_30 = 0;
    statistics_1a78_.value_38 = 0;
    statistics_1a78_.value_64 = 0;
    for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
        Renderer* renderer = entry->renderer_08;
        if (renderer != 0) {
            unsigned long renderer_stats[7];
            renderer->getStatistics(renderer_stats);
            statistics_1a78_.value_34 += renderer_stats[1];
            statistics_1a78_.value_64 += renderer_stats[2];
            statistics_1a78_.value_3c += renderer_stats[3];
            statistics_1a78_.value_30 += renderer_stats[0];
            if (renderer->sorted_d8_ == 1) {
                statistics_1a78_.value_38 += renderer_stats[6];
            }
        }
    }
    srDD::Statistics device;
    memset(&device, 0, sizeof(device));
    getDD()->getStatistics(device);
    statistics_1a78_.value_08 = device.value_00;
    statistics_1a78_.value_0c = device.value_04;
    statistics_1a78_.value_10 = device.value_08;
    statistics_1a78_.value_18 = device.value_10;
    statistics_1a78_.value_1c = device.value_14;
    statistics_1a78_.value_20 = device.value_18;
    statistics_1a78_.value_24 = device.value_1c;
    statistics_1a78_.value_28 = device.value_20;
    statistics = statistics_1a78_;
    statistics.elapsed_00 =
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - statistics.elapsed_00;
}

// FUNCTION: SURRENDER 0x1001ACD0
void srGERD::resetStatistics()
{
    memset(&statistics_1a78_, 0, sizeof(statistics_1a78_));
    getDD()->resetStatistics();
    SectionAccess access(renderers_section_14_);
    for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
        while (entry->busy_0c != 0) {
            srThread::yield(0);
        }
        entry->renderer_08->resetStatistics();
    }
    statistics_1a78_.elapsed_00 = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
    statistics_19f8_ = statistics_1a78_;
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

// FUNCTION: SURRENDER 0x1001C8A0
void srGERD::setPolygonOffset(long offset)
{
    if (offset != polygon_offset_1fe4_) {
        flushImmediateRenderers();
        polygon_offset_1fe4_ = offset;
        dirty_24_ |= 0x8000;
    }
}

// FUNCTION: SURRENDER 0x1001C8D0
long srGERD::getPolygonOffset() const
{
    return polygon_offset_1fe4_;
}

// FUNCTION: SURRENDER 0x1001CA30
void srGERD::setClearColor(const srVector4T<float>& color)
{
    clear_values_1b08_.color_00 = color;
    float* clear = &clear_values_1b08_.color_00.x;
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
        clear_values_1b08_.depth_20 = 0.0;
        return;
    }
    if (depth >= 1.0) {
        clear_values_1b08_.depth_20 = 1.0;
        return;
    }
    clear_values_1b08_.depth_20 = depth;
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

// FUNCTION: SURRENDER 0x10028050
void srGERD::invalidateResidentTexture(Texture& texture)
{
    if (texture.device_2c.resident_data_68 != 0) {
        if (texture.surface_data_20 == 0 || srThread::getHandle() != owner_thread_1c_) {
            invalidateTexture(texture);
        } else {
            getDD()->deleteTexture(texture.device_2c);
            texture.device_2c.resident_data_68 = 0;
            texture.device_2c.resident_size_6c = 0;
            for (unsigned long stage = 0; stage < info_50_.max_texture_stages_28_; ++stage) {
                if (texture_slots_1f38_[stage] == &texture) {
                    texture_slots_1f38_[stage] = 0;
                }
            }
        }
        dirty_24_ |= 0x400;
        dirty_24_ |= 0x800;
    }
}

// FUNCTION: SURRENDER 0x100280F0
void srGERD::resetCurrentTexPointers()
{
    for (unsigned long stage = 0; stage < info_50_.max_texture_stages_28_; ++stage) {
        if (texture_iface_1ffc_[stage] != 0) {
            texture_iface_1ffc_[stage]->release();
            texture_iface_1ffc_[stage] = 0;
        }
        texture_slots_1f38_[stage] = 0;
    }
    dirty_24_ |= 0x400;
    dirty_24_ |= 0x800;
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
    SectionAccess access(state_section_18_);
    if (handle != 0 && texture_hash_enabled_2044_) {
        long index =
            texture_lookup_2004_
                .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
        if (index != -1) {
            srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
            while (entries[index].key != handle) {
                index = entries[index].next_index;
                if (index == -1) {
                    return;
                }
            }
            Texture* texture = entries[index].value;
            if (texture != 0) {
                invalidateTexture(*texture);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10017BC0
void srGERD::invalidateResidentTextures()
{
    flushImmediateRenderers();
    SectionAccess access(state_section_18_);
    Texture* texture = texture_head_202c_;
    while (texture != 0) {
        Texture* next = texture->next_04;
        invalidateResidentTexture(*texture);
        texture = next;
    }
    resetCurrentTexPointers();
}

// FUNCTION: SURRENDER 0x10017C40
void srGERD::invalidateResidentTexture(srTextureIFace* texture)
{
    flushImmediateRenderers();
    SectionAccess access(state_section_18_);
    if (texture != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        long index =
            texture_lookup_2004_
                .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
        if (index != -1) {
            srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
            while (entries[index].key != handle) {
                index = entries[index].next_index;
                if (index == -1) {
                    return;
                }
            }
            Texture* found = entries[index].value;
            if (found != 0) {
                invalidateResidentTexture(*found);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10017EC0
void srGERD::setTextureCacheSize(unsigned long bytes)
{
    SectionAccess access(state_section_18_);
    texture_cache_size_2038_ = bytes;
    if (bytes != 0 && bytes < texture_cache_used_2034_) {
        releaseTextureMemory(texture_cache_used_2034_ - bytes);
    }
}

// FUNCTION: SURRENDER 0x100180A0
void srGERD::setTextureSubImage(srTextureIFace* texture, long mipmap, long x, long y, long width,
                                long height)
{
    SectionAccess access(state_section_18_);
    if (isWindowOpen() == 0) {
        return;
    }
    unsigned long handle = texture->getTextureFrameHandle();
    long index = texture_lookup_2004_
                     .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
    if (index == -1) {
        return;
    }
    srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
    while (entries[index].key != handle) {
        index = entries[index].next_index;
        if (index == -1) {
            return;
        }
    }
    Texture* resident = entries[index].value;
    if (resident != 0 && resident->device_2c.first_level_28 <= (unsigned long)mipmap &&
        (unsigned long)mipmap <= resident->device_2c.last_level_2c) {
        srTextureIFace::PartialRequest request;
        request.width_00 = resident->device_2c.width_20;
        request.height_04 = resident->device_2c.height_24;
        request.mipmap_level_08 = mipmap;
        request.source_right = x + width;
        request.source_bottom = y + height;
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        request.destination_x = x;
        request.destination_y = y;
        unsigned long shift =
            (unsigned long)((char)mipmap - (char)resident->device_2c.first_level_28);
        unsigned long level_width = request.width_00 >> (shift & 0x1f);
        if (level_width == 0) {
            level_width = 1;
        }
        unsigned long level_height = request.height_04 >> (shift & 0x1f);
        if (level_height == 0) {
            level_height = 1;
        }
        if (level_width < (unsigned long)request.source_right) {
            request.source_right = (long)level_width;
        }
        if (level_height < (unsigned long)request.source_bottom) {
            request.source_bottom = (long)level_height;
        }
        unsigned long bytes_per_pixel =
            (unsigned long)resident->pixel_format_0c.bytes_per_pixel_minus_one + 1;
        unsigned long pitch = bytes_per_pixel * level_width;
        if (resident->surface_data_20 == 0) {
            void* staging =
                srCore.getGlobalRecycler()->allocate(bytes_per_pixel * level_height * level_width);
            request.destination = new srColorSurface(resident->pixel_format_0c, staging,
                                                     level_width, level_height, pitch);
            texture->getMipmapLevelPartial(request);
            request.destination->release();
            resident->device_2c.levels_38[mipmap] = staging;
            getDD()->texSubImage(resident->device_2c, mipmap, request.destination_x,
                                 request.destination_y, request.source_right,
                                 request.source_bottom);
            resident->device_2c.levels_38[mipmap] = 0;
            srCore.getGlobalRecycler()->free(staging);
        } else {
            request.destination =
                new srColorSurface(resident->pixel_format_0c, resident->device_2c.levels_38[mipmap],
                                   level_width, level_height, pitch);
            texture->getMipmapLevelPartial(request);
            request.destination->release();
            getDD()->texSubImage(resident->device_2c, mipmap, request.destination_x,
                                 request.destination_y, request.source_right,
                                 request.source_bottom);
        }
    }
}

// FUNCTION: SURRENDER 0x10018480
void srGERD::invalidateTextureCache()
{
    SectionAccess access(state_section_18_);
    if (texture_hash_enabled_2044_) {
        Texture* texture = texture_head_202c_;
        while (texture != 0) {
            Texture* next = texture->next_04;
            invalidateTexture(*texture);
            texture = next;
        }
        resetCurrentTexPointers();
        texture_sequence_203c_ = 0;
    }
}

// FUNCTION: SURRENDER 0x10018500
unsigned long srGERD::getResidentTextureMemUsed() const
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    unsigned long used = 0;
    for (Texture* texture = texture_head_202c_; texture != 0; texture = texture->next_04) {
        if (texture->device_2c.resident_data_68 != 0 && texture->device_2c.resident_size_6c > 0) {
            used += texture->device_2c.resident_size_6c;
        }
    }
    section->releaseAccess();
    return used;
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
    parameters.batch_limit = info_50_.renderer_batch_limit_1c_;
    parameters.texture_stages = info_50_.max_texture_stages_28_;
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

// FUNCTION: SURRENDER 0x1001A850
void srGERD::flipFrame()
{
    flipFrame(0, 0, 0);
}

// FUNCTION: SURRENDER 0x1001A860
void srGERD::flipFrame(const Rectangle* first, const Rectangle* second, unsigned long count)
{
    if (isWindowOpen() == 0) {
        setError(static_cast<e_error>(4));
        return;
    }
    if ((state_flags_28_ & 8) != 0) {
        return;
    }
    flush();
    long width = getWidth();
    long height = getHeight();
    unsigned long window_width = srWindow::getWidth(getWindowHandle());
    unsigned long window_height = srWindow::getHeight(getWindowHandle());
    if (isFullScreen() == 0 && count != 0) {
        srDD::Scissor* first_scissors = new srDD::Scissor[count * 2];
        srDD::Scissor* second_scissors = first_scissors + count;
        for (unsigned long i = 0; i < count; i++) {
            first_scissors[i].left = first[i].x;
            first_scissors[i].top = first[i].y;
            first_scissors[i].right = first[i].x + first[i].width;
            first_scissors[i].bottom = first[i].y + first[i].height;
            second_scissors[i].left = second[i].x;
            second_scissors[i].top = second[i].y;
            second_scissors[i].right = second[i].x + second[i].width;
            second_scissors[i].bottom = second[i].y + second[i].height;
        }
        getDD()->flipFrame(first_scissors, second_scissors, count);
        delete[] first_scissors;
    } else {
        srDD::Scissor window_scissor;
        window_scissor.left = 0;
        window_scissor.top = 0;
        window_scissor.right = window_width;
        window_scissor.bottom = window_height;
        srDD::Scissor view_scissor;
        view_scissor.left = 0;
        view_scissor.top = 0;
        view_scissor.right = width;
        view_scissor.bottom = height;
        getDD()->flipFrame(&window_scissor, &view_scissor, 1);
    }
    statistics_1a78_.frames_2c += 1;
    state_flags_28_ |= 8;
    getStatistics(statistics_19f8_);
}

// FUNCTION: SURRENDER 0x1001AAB0
void srGERD::flush()
{
    if (isWindowOpen() == 0) {
        setError(static_cast<e_error>(4));
        return;
    }
    checkAllStateChanges();
    flushImmediateRenderers();
    getDD()->flushFrame();
}

// FUNCTION: SURRENDER 0x1001CD20
int srGERD::isContextCreated() const
{
    return state_flags_28_ & 1;
}

// FUNCTION: SURRENDER 0x1001D020
long srGERD::getWidth() const
{
    return open_info_378_.width_08;
}

// FUNCTION: SURRENDER 0x1001D0A0
long srGERD::getHeight() const
{
    return open_info_378_.height_0c;
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

// FUNCTION: SURRENDER 0x1001D0E0
int srGERD::isFullScreen() const
{
    return open_info_378_.display_mode_10 >= 0;
}

// FUNCTION: SURRENDER 0x1001D030
void srGERD::getPixelFormat(srPixelConvert::PixelFormat& format) const
{
    if (isWindowOpen() != 0) {
        /* Default 8888 format the device getBufferPixelFormat refines. */
        srDD::PixelFormat device = {8, 0x10, 8, 8, 8, 0, 8, 0x18, 0, 3};
        getDD()->getBufferPixelFormat(device);
        convertPixelFormat(format, device);
    }
}

// FUNCTION: SURRENDER 0x1001D100
const char* srGERD::getDeviceName() const
{
    return info_50_.text_3c_[0];
}

// FUNCTION: SURRENDER 0x1001D110
const char* srGERD::getDeviceVendor() const
{
    return info_50_.text_3c_[1];
}

// FUNCTION: SURRENDER 0x1001D120
const char* srGERD::getDevicePlatform() const
{
    return info_50_.text_3c_[2];
}

// FUNCTION: SURRENDER 0x1001D130
const char* srGERD::getDriverName() const
{
    const char* name = info_50_.text_3c_[3];
    if (isContextCreated() == 0) {
        name = driver_info_2cc_.name_14;
    }
    return name;
}

// FUNCTION: SURRENDER 0x1001D150
const char* srGERD::getDriverVendor() const
{
    return info_50_.text_3c_[4];
}

// FUNCTION: SURRENDER 0x1001D160
const char* srGERD::getDriverVersion() const
{
    return info_50_.text_3c_[5];
}

// FUNCTION: SURRENDER 0x1001D170
const char* srGERD::getHardwareChipset() const
{
    return info_50_.text_3c_[6];
}

// FUNCTION: SURRENDER 0x1001D180
const char* srGERD::getHardwareName() const
{
    return info_50_.text_3c_[7];
}

// FUNCTION: SURRENDER 0x1001D190
const char* srGERD::getHardwareVendor() const
{
    return info_50_.text_3c_[8];
}

// FUNCTION: SURRENDER 0x1001D1A0
srGERD* srGERD::getFirst()
{
    return first;
}

// FUNCTION: SURRENDER 0x1001D1D0
srDD::e_hardwareID srGERD::getHardwareID() const
{
    return static_cast<srDD::e_hardwareID>(info_50_.hardware_id_38_);
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

// FUNCTION: SURRENDER 0x1001D630
void srGERD::performPickTest(const PickInput& input)
{
    unsigned long depth = pick_depth_19ec_;
    if (depth == 0) {
        return;
    }
    srVector4T<float>* vertices = pick_vertices_2230_.ensure(input.vertex_count_14);
    for (unsigned long index = 0; index < input.vertex_count_14; index++) {
        float inv_w = 1.0f / input.positions_10[index].w;
        vertices[index].w = inv_w < 0.0f ? -1.0f : 1.0f;
        inv_w = fabs(inv_w);
        vertices[index].x = input.positions_10[index].x * inv_w;
        vertices[index].y = input.positions_10[index].y * inv_w;
        vertices[index].z = input.positions_10[index].z * inv_w;
    }
    Pick* pick = pick_stack_176c_;
    do {
        float pick_x = pick->x_00;
        float pick_y = pick->y_04;
        for (unsigned long index = 0; index < input.triangle_count_08; index++) {
            unsigned long triangle_index = input.indices_00[index];
            const srVector3i& triangle = input.triangles_04[triangle_index];
            const srVector4T<float>* corner0 = &vertices[input.vertices_0c[triangle.x]];
            const srVector4T<float>* corner1 = &vertices[input.vertices_0c[triangle.y]];
            const srVector4T<float>* corner2 = &vertices[input.vertices_0c[triangle.z]];
            float v0x = corner0->x;
            float v0y = corner0->y;
            float v0z = corner0->z;
            float v1x = corner1->x;
            float v1y = corner1->y;
            float v1z = corner1->z;
            float v2x = corner2->x;
            float v2y = corner2->y;
            float v2z = corner2->z;
            char winding = (v0y - pick_y) * (v0x - v1x) - (v0y - v1y) * (v0x - pick_x) > 0.0f;
            if ((v1y - pick_y) * (v1x - v2x) - (v1x - pick_x) * (v1y - v2y) > 0.0f) {
                winding++;
            }
            if (winding == 1) {
                continue;
            }
            if ((v2y - pick_y) * (v2x - v0x) - (v2y - v0y) * (v2x - pick_x) > 0.0f) {
                winding++;
            }
            if (winding != 0 && winding != 3) {
                continue;
            }
            /* Positions were normalized by |1/w|; undo the mirror for
               corners behind the eye before solving the plane. */
            if (corner0->w == -1.0f) {
                v0x = -v0x;
                v0y = -v0y;
                v0z = -v0z;
            }
            if (corner1->w == -1.0f) {
                v1x = -v1x;
                v1y = -v1y;
                v1z = -v1z;
            }
            if (corner2->w == -1.0f) {
                v2x = -v2x;
                v2y = -v2y;
                v2z = -v2z;
            }
            float a = (v1y - v0y) * (v2z - v0z) - (v1z - v0z) * (v2y - v0y);
            float b = (v1z - v0z) * (v2x - v0x) - (v2z - v0z) * (v1x - v0x);
            float c = (v2y - v0y) * (v1x - v0x) - (v1y - v0y) * (v2x - v0x);
            float hit = -((a * pick_x + b * pick_y - (a * v0x + b * v0y + c * v0z)) / c);
            if (hit >= -1.0f && hit < pick->z_08) {
                pick->z_08 = hit;
                /* reinterpret-ok: the public pick key arrives as ulong bits
                   naming the selected model instance. */
                pick->selected_model_0c = reinterpret_cast<srModelInstance*>(pick_key_19f0_);
                pick->value_10 = triangle_index;
            }
        }
        pick++;
        depth--;
    } while (depth != 0);
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

// FUNCTION: SURRENDER 0x1001F790
short srGERD::accumConvert(float value) const
{
    if (value <= -1.0f) {
        return -32767;
    }
    if (value >= 1.0f) {
        return 32767;
    }
    return static_cast<short>(value * 32767.0f);
}

// FUNCTION: SURRENDER 0x1001F7F0
void srGERD::accumAlloc()
{
    if (getWidth() != 0) {
        if (getHeight() != 0) {
            accum_buffer_1af8_ =
                static_cast<AccumPixel*>(srHeap.allocate(getWidth() * getHeight() * 8));
            unsigned long* scratch = static_cast<unsigned long*>(srHeap.allocate(getWidth() * 4));
            if (scratch != 0) {
                accum_scratch_1afc_ = scratch;
                accumClear();
                return;
            }
            accum_scratch_1afc_ = 0;
            accumClear();
        }
    }
}

// FUNCTION: SURRENDER 0x1001F8D0
void srGERD::accumClear()
{
    if (accum_buffer_1af8_ == 0) {
        accumAlloc();
    }
    if (accum_buffer_1af8_ == 0) {
        return;
    }
    short first = accumConvert(clear_values_1b08_.accum_10.x);
    short second = accumConvert(clear_values_1b08_.accum_10.y);
    short third = accumConvert(clear_values_1b08_.accum_10.z);
    short fourth = accumConvert(clear_values_1b08_.accum_10.w);
    unsigned long left = scissor_1628_.left;
    unsigned long top = scissor_1628_.top;
    unsigned long width = scissor_1628_.right - left;
    unsigned long height = scissor_1628_.bottom - top;
    /* reinterpret-ok: 16-bit accumulation pixels are filled as dword lanes. */
    SRDWORD* row = reinterpret_cast<SRDWORD*>(accum_buffer_1af8_) + (getWidth() * top + left) * 2;
    if (width == 0 || height == 0) {
        return;
    }
    if (first == third && second == fourth) {
        SRDWORD constant = first * 0x10000 + second;
        for (; height != 0; height--) {
            if (width * 2 != 0) {
                srVectorProcessor::copy(row, constant, width * 2);
            }
            row += getWidth() * 2;
        }
        return;
    }
    for (; height != 0; height--) {
        if (width != 0) {
            row[0] = (first << 16) | (fourth & 0xffff);
            row[1] = (third << 16) | (second & 0xffff);
            unsigned long words = (width * 8 - 5) >> 2;
            for (unsigned long i = 0; i < words; i++) {
                row[i + 2] = row[i];
            }
        }
        row += getWidth() * 2;
    }
}

/* Locked-buffer surface created by lockBuffer: a 0x60-byte surface class
   (ctor 0x100205D0, registered as "srGERD::Surface" class 0x3111) that keeps
   its own scissor rect and proxies pixel access through device bufferOp
   commands, staging through a 1-pixel-high srColorSurface scratch buffer when
   the locked format is not 32-bit ARGB. Retail vtable 0x100767F8. */
class srGERD::LockSurface : public srClassSupport<LockSurface, srColorSurfaceIFace, false, 0x3111> {
public:
    // 0x100205D0
    LockSurface(srGERD* gerd, const srPixelConvert::PixelFormat& format);
    // 0x1001F610
    virtual ~LockSurface() override;
    // 0x1001F780
    static const char* sGetClassName()
    {
        return "srGERD::Surface";
    }
    /* Retail 0x1001F770: vInstance cannot construct a LockSurface (the ctor
       needs the owning GERD and pixel format) and returns null. */
    virtual srClass* vInstance() override;
    /* Retail vtable 0x100767F8 override slots. */
    virtual void getPixelColumn(unsigned long* pixels, long x, long y0,
                                long y1) override; // 0x10020BB0
    virtual void setPixelColumn(const unsigned long* pixels, long x, long y0,
                                long y1) override; // 0x10020AF0
    virtual void* getDataPtr() override;           // 0x1001F750
    virtual long getDataSize() override;           // 0x1001F760
    virtual void setHLine(long y, long x0, long x1, unsigned long pixel) override;
    // 0x100207A0
    virtual void getPixelRow(unsigned long* pixels, long y, long x0,
                             long x1) override; // 0x10020990
    virtual void setPixelRow(const unsigned long* pixels, long y, long x0,
                             long x1) override; // 0x10020840
    virtual void getPixelRowRaw(void* pixels, long y, long x0,
                                long x1) override; // 0x10020A70
    virtual void setPixelRowRaw(const void* pixels, long y, long x0,
                                long x1) override; // 0x10020900
    void setScissor(unsigned long left, unsigned long top, unsigned long right,
                    unsigned long bottom);

private:
    srGERD* gerd_44_;
    unsigned long left_48_;
    unsigned long top_4c_;
    unsigned long right_50_;
    unsigned long bottom_54_;
    srColorSurface* scratch_58_;

public:
    /* Set by lockBuffer when the locked pixel format is 32-bit ARGB. */
    unsigned char argb32_5c_;

private:
    unsigned char unknown_5d_[3];
};

// FUNCTION: SURRENDER 0x100205D0
srGERD::LockSurface::LockSurface(srGERD* gerd, const srPixelConvert::PixelFormat& format)
{
    SurfaceDesc description;
    memset(&description, 0, sizeof(description));
    description.width = gerd->getWidth();
    description.height = gerd->getHeight();
    description.pitch = (format.bytes_per_pixel_minus_one + 1) * description.width;
    description.pixel_format = format;
    setSurfaceDesc(description);
    gerd_44_ = gerd;
    left_48_ = 0;
    top_4c_ = 0;
    right_50_ = 0;
    bottom_54_ = 0;
    /* One row tall and as wide as the longest scissor axis. */
    unsigned long side =
        (long)description.width < (long)description.height ? description.height : description.width;
    scratch_58_ = new srColorSurface(format, side, 1);
    argb32_5c_ = 0;
}

// FUNCTION: SURRENDER 0x1001F610
srGERD::LockSurface::~LockSurface()
{
    scratch_58_->release();
}

// FUNCTION: SURRENDER 0x1001F770
srClass* srGERD::LockSurface::vInstance()
{
    return 0;
}

// FUNCTION: SURRENDER 0x1001F750
void* srGERD::LockSurface::getDataPtr()
{
    return 0;
}

// FUNCTION: SURRENDER 0x1001F760
long srGERD::LockSurface::getDataSize()
{
    return 0;
}

// FUNCTION: SURRENDER 0x100207A0
void srGERD::LockSurface::setHLine(long y, long x0, long x1, unsigned long pixel)
{
    if (y < (long)top_4c_ || y >= (long)bottom_54_) {
        return;
    }
    if (x0 < (long)left_48_) {
        x0 = left_48_;
    }
    if (x1 > (long)right_50_) {
        x1 = right_50_;
    }
    if (x0 >= x1) {
        return;
    }
    /* Convert the pixel through the scratch surface, then hand the device a
       pointer to the converted value. */
    scratch_58_->setPixel(0, 0, pixel);
    unsigned long converted = scratch_58_->getPixelRaw(0, 0);
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 4;
    command.data_08 = &converted;
    command.x_0c = x0;
    command.y_10 = y;
    command.count_14 = x1 - x0;
    gerd_44_->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020990
void srGERD::LockSurface::getPixelRow(unsigned long* pixels, long y, long x0, long x1)
{
    if (y < (long)top_4c_ || y >= (long)bottom_54_) {
        return;
    }
    if (x0 < (long)left_48_) {
        pixels += left_48_ - x0;
        x0 = left_48_;
    }
    if (x1 > (long)right_50_) {
        x1 = right_50_;
    }
    if (x0 >= x1) {
        return;
    }
    long count = x1 - x0;
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 2;
    command.x_0c = x0;
    command.y_10 = y;
    command.count_14 = count;
    if (argb32_5c_ != 0) {
        command.data_08 = pixels;
        gerd_44_->getDD()->bufferOp(command);
        return;
    }
    command.data_08 = scratch_58_->getDataPtr();
    gerd_44_->getDD()->bufferOp(command);
    scratch_58_->getPixelRow(pixels, 0, 0, count);
}

// FUNCTION: SURRENDER 0x10020840
void srGERD::LockSurface::setPixelRow(const unsigned long* pixels, long y, long x0, long x1)
{
    if (y < (long)top_4c_ || y >= (long)bottom_54_) {
        return;
    }
    if (x0 < (long)left_48_) {
        pixels += left_48_ - x0;
        x0 = left_48_;
    }
    if (x1 > (long)right_50_) {
        x1 = right_50_;
    }
    if (x0 >= x1) {
        return;
    }
    long count = x1 - x0;
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 3;
    command.x_0c = x0;
    command.y_10 = y;
    command.count_14 = count;
    if (argb32_5c_ != 0) {
        command.data_08 = (void*)pixels;
        gerd_44_->getDD()->bufferOp(command);
        return;
    }
    scratch_58_->setPixelRow(pixels, 0, 0, count);
    command.data_08 = scratch_58_->getDataPtr();
    gerd_44_->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020A70
void srGERD::LockSurface::getPixelRowRaw(void* pixels, long y, long x0, long x1)
{
    if (y < (long)top_4c_ || y >= (long)bottom_54_) {
        return;
    }
    if (x0 < (long)left_48_) {
        /* Retail advances the raw pointer by one byte per clipped pixel. */
        pixels = (char*)pixels + (left_48_ - x0);
        x0 = left_48_;
    }
    if (x1 > (long)right_50_) {
        x1 = right_50_;
    }
    if (x0 >= x1) {
        return;
    }
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 2;
    command.data_08 = pixels;
    command.x_0c = x0;
    command.y_10 = y;
    command.count_14 = x1 - x0;
    gerd_44_->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020900
void srGERD::LockSurface::setPixelRowRaw(const void* pixels, long y, long x0, long x1)
{
    if (y < (long)top_4c_ || y >= (long)bottom_54_) {
        return;
    }
    if (x0 < (long)left_48_) {
        pixels =
            (const char*)pixels + (pixel_format_30.bytes_per_pixel_minus_one + 1) * (left_48_ - x0);
        x0 = left_48_;
    }
    if (x1 > (long)right_50_) {
        x1 = right_50_;
    }
    if (x0 >= x1) {
        return;
    }
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 3;
    command.data_08 = (void*)pixels;
    command.x_0c = x0;
    command.y_10 = y;
    command.count_14 = x1 - x0;
    gerd_44_->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020BB0
void srGERD::LockSurface::getPixelColumn(unsigned long* pixels, long x, long y0, long y1)
{
    if (x < (long)left_48_ || x >= (long)right_50_) {
        return;
    }
    if (y0 < (long)top_4c_) {
        pixels += top_4c_ - y0;
        y0 = top_4c_;
    }
    if (y1 >= (long)bottom_54_) {
        y1 = bottom_54_;
    }
    if (y0 >= y1) {
        return;
    }
    long count = y1 - y0;
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 6;
    command.x_0c = x;
    command.y_10 = y0;
    command.count_14 = count;
    if (argb32_5c_ != 0) {
        command.data_08 = pixels;
        gerd_44_->getDD()->bufferOp(command);
        return;
    }
    command.data_08 = scratch_58_->getDataPtr();
    gerd_44_->getDD()->bufferOp(command);
    scratch_58_->getPixelRow(pixels, 0, 0, count);
}

// FUNCTION: SURRENDER 0x10020AF0
void srGERD::LockSurface::setPixelColumn(const unsigned long* pixels, long x, long y0, long y1)
{
    if (x < (long)left_48_ || x >= (long)right_50_) {
        return;
    }
    if (y0 < (long)top_4c_) {
        pixels += top_4c_ - y0;
        y0 = top_4c_;
    }
    if (y1 >= (long)bottom_54_) {
        y1 = bottom_54_;
    }
    if (y0 >= y1) {
        return;
    }
    long count = y1 - y0;
    srDD::BufferCommand command;
    command.flags_00 = 0;
    command.opcode_04 = 7;
    command.x_0c = x;
    command.y_10 = y0;
    command.count_14 = count;
    if (argb32_5c_ != 0) {
        command.data_08 = (void*)pixels;
        gerd_44_->getDD()->bufferOp(command);
        return;
    }
    scratch_58_->setPixelRow(pixels, 0, 0, count);
    command.data_08 = scratch_58_->getDataPtr();
    gerd_44_->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020780
void srGERD::LockSurface::setScissor(unsigned long left, unsigned long top, unsigned long right,
                                     unsigned long bottom)
{
    left_48_ = left;
    right_50_ = right;
    top_4c_ = top;
    bottom_54_ = bottom;
}

// FUNCTION: SURRENDER 0x10020C90
srDD::e_error srGERD::_lockBuffer()
{
    if (buffer_lock_count_1b04_ == 0) {
        flushImmediateRenderers();
        getDD()->flushFrame();
        srDD::BufferCommand command;
        command.flags_00 = 0;
        command.opcode_04 = 0;
        srDD::e_error error = getDD()->bufferOp(command);
        if (error != 0) {
            return error;
        }
    }
    buffer_lock_count_1b04_ += 1;
    return static_cast<srDD::e_error>(0);
}

// FUNCTION: SURRENDER 0x10020CF0
srDD::e_error srGERD::_unlockBuffer()
{
    if (buffer_lock_count_1b04_ != 0) {
        buffer_lock_count_1b04_ -= 1;
        if (buffer_lock_count_1b04_ == 0) {
            /* Retail's command.flags_00 ends up holding the decremented lock
               count (the decrement temporary shares that slot). */
            srDD::BufferCommand command;
            command.flags_00 = buffer_lock_count_1b04_;
            command.opcode_04 = 1;
            return getDD()->bufferOp(command);
        }
    }
    return static_cast<srDD::e_error>(0);
}

// FUNCTION: SURRENDER 0x10020D30
void srGERD::clear(const srFlags<e_buffer>& buffers)
{
    if (isWindowOpen() == 0) {
        setError(static_cast<e_error>(4));
        return;
    }
    if (buffers.value != 0) {
        if ((dirty_24_ & 0x1f0) != 0) {
            applyViewStateChanges();
        }
        /* GERD buffer bits 0,1,3 map to DD bits 0,1,2; GERD bit 2 is the
           software accumulation buffer serviced by accumClear. */
        srFlags<srDD::e_buffer> device_buffers;
        device_buffers.value = (buffers.value & 1) != 0;
        if ((buffers.value & 2) != 0) {
            device_buffers.value |= 2;
        }
        if ((buffers.value & 8) != 0) {
            device_buffers.value |= 4;
        }
        if (device_buffers.value != 0) {
            getDD()->setClearValues(clear_values_1b08_);
            getDD()->clearBuffers(device_buffers);
        }
        if ((buffers.value & 4) != 0) {
            accumClear();
        }
    }
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
        memcpy(viewport.extra, &depth_min_1618_, sizeof(viewport.extra));
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
    if (((dirty_24_ & 0x800) != 0) && (1 < info_50_.max_texture_stages_28_)) {
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
    for (unsigned long stage = 0; stage < info_50_.max_texture_stages_28_; ++stage) {
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
    texture.device_2c.resident_data_68 = 0;
    texture.device_2c.resident_size_6c = 0;
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
    texture.device_2c.resident_data_68 = 0;
    texture.device_2c.width_20 = 0;
    texture.device_2c.height_24 = 0;
    texture.device_2c.first_level_28 = 0;
    texture.device_2c.last_level_2c = 0;
    texture.device_2c.format_index_30 = 0;
    texture.device_2c.parameter_34 = 0;
    texture.device_2c.resident_size_6c = 0;
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
            if (info_50_.max_texture_stages_28_ != 0) {
                Texture** slot = texture_slots_1f38_;
                unsigned long stage = 0;
                do {
                    if (*slot == texture) {
                        bound = 1;
                        break;
                    }
                    ++stage;
                    ++slot;
                } while (stage < info_50_.max_texture_stages_28_);
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
    texture->device_2c.resident_data_68 = 0;
    texture->device_2c.width_20 = 0;
    texture->device_2c.height_24 = 0;
    texture->device_2c.first_level_28 = 0;
    texture->device_2c.last_level_2c = 0;
    texture->device_2c.format_index_30 = 0;
    texture->device_2c.parameter_34 = 0;
    texture->device_2c.resident_size_6c = 0;
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
    unsigned long packed = ((((wrap_s_map_1fa4_[(state >> 0xc) & 1] & 0xfffffff3) |
                              (wrap_t_map_1fac_[(state >> 0xd) & 1] << 2))
                                 << 2 |
                             (mipmap_map_1f94_[(state >> 10) & 3] & 0xffffffc3))
                                << 2 |
                            (min_filter_map_1f80_[(state >> 7) & 7] & 0xffffff03))
                               << 2 |
                           (mag_filter_map_1f6c_[(state >> 4) & 7] & 0xfffffc0f);
    packed = (packed << 4) | (correction_map_1f5c_[state & 3] & 0xffffc00f);
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
    if (info_50_.max_texture_stages_28_ <= stage) {
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
            found->surface_data_20 != 0 && (info_50_.flags_18_ & 0x20) != 0) {
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

// FUNCTION: SURRENDER 0x10018E90
void srGERD::convertPixelFormat(srPixelConvert::PixelFormat& format,
                                const srDD::PixelFormat& device)
{
    format.red_bits = device.red_bits;
    format.green_bits = device.green_bits;
    format.blue_bits = device.blue_bits;
    format.alpha_bits = device.alpha_bits;
    format.red_shift = device.red_shift;
    format.green_shift = device.green_shift;
    format.blue_shift = device.blue_shift;
    format.alpha_shift = device.alpha_shift;
    format.conversion_class = device.conversion_class;
    format.bytes_per_pixel_minus_one = device.bytes_per_pixel_minus_one;
}

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

// FUNCTION: SURRENDER 0x10018F30
void srGERD::initTextureFormats()
{
    texture_formats_360_ = 0;
    texture_format_count_364_ = 0;
    srDD::PixelFormatList list;
    getDD()->getTextureFormats(list);
    long count = list.count;
    if (count != 0) {
        texture_formats_360_ = new srPixelConvert::PixelFormat[count];
        long i;
        for (i = 0; i < count; i++) {
            texture_formats_360_[i].flags = 0;
        }
        texture_format_count_364_ = count;
        for (i = 0; i < count; i++) {
            convertPixelFormat(texture_formats_360_[i], list.formats[i]);
        }
    }
}

// FUNCTION: SURRENDER 0x10018FE0
void srGERD::initDisplayModeList()
{
    srDD::WindowInfoList list;
    list.count = 0;
    list.entries = 0;
    getDD()->getWindowList(list);
    display_modes_368_ = 0;
    display_mode_count_36c_ = 0;
    if (list.count != 0) {
        display_modes_368_ = new unsigned long[list.count * 3];
        display_mode_count_36c_ = list.count;
        for (long i = 0; i < list.count; i++) {
            display_modes_368_[i * 3] = list.entries[i].width;
            display_modes_368_[i * 3 + 1] = list.entries[i].height;
            display_modes_368_[i * 3 + 2] = list.entries[i].depth;
        }
    }
}

// FUNCTION: SURRENDER 0x10019080
void srGERD::initDDInfo()
{
    memset(&info_50_, 0, sizeof(info_50_));
    info_50_.flags_18_ = 0;
    info_50_.hardware_id_38_ = 1;
    info_50_.texture_min_dim_2c_ = 1;
    info_50_.texture_max_aspect_34_ = 1;
    info_50_.max_texture_stages_28_ = 1;
    info_50_.renderer_batch_limit_1c_ = 0x100;
    info_50_.unknown_20_ = 0x3b808081;
    info_50_.texture_ram_24_ = 0x200000;
    info_50_.unknown_0c_ = 0x10;
    info_50_.unknown_08_ = 4;
    info_50_.texture_max_dim_30_ = 0x100;
    info_50_.unknown_10_ = 1.0f;
    info_50_.unknown_14_ = 65536.0f;
    for (long i = 0; i < 9; i++) {
        sprintf(info_50_.text_3c_[i], "Unknown");
    }
    getDD()->getInfo(info_50_);
    if (info_50_.max_texture_stages_28_ > 2) {
        info_50_.max_texture_stages_28_ = 2;
    }
}

// FUNCTION: SURRENDER 0x1001C3E0
void srGERD::initLights()
{
    ambient_light_2048_.Set(0.2f, 0.2f, 0.2f, 1.0f);
}

// FUNCTION: SURRENDER 0x10021BE0
void srGERD::initMatrices()
{
    matrix_mode_1650_ = MATRIX_MODELVIEW;
    for (long i = 0; i < 2; i++) {
        matrix_current_390_[i].vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
        matrix_current_390_[i].vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
        matrix_current_390_[i].vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
        matrix_current_390_[i].vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
        matrix_class_174c_[i] = static_cast<srMatrix4T<float>::e_type>(4);
        matrix_stacks_410_[i].depth_800 = 0;
    }
    normal_matrix_1704_.vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
    normal_matrix_1704_.vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
    normal_matrix_1704_.vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
    normal_matrix_1704_.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
    inverse_modelview_1684_.vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
    inverse_modelview_1684_.vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
    inverse_modelview_1684_.vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
    inverse_modelview_1684_.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
}

// FUNCTION: SURRENDER 0x1001CD60
void srGERD::dump(std::ostream& stream)
{
    srRuntimeClass::dump(stream);
    dump(stream, srFlags<e_info>(INFO_ALL));
}

// FUNCTION: SURRENDER 0x1001DEB0
void srGERD::dump(std::ostream& stream, const srFlags<e_info>& info)
{
    if (isContextCreated() == 0) {
        stream << "No context created" << std::endl;
        return;
    }
    if ((info.value & INFO_DEVICE) != 0) {
        stream << std::endl;
        stream << "GERD Driver/Device Information" << '\n';
        stream << "Device Name        : " << getDeviceName() << '\n';
        stream << "Device Vendor      : " << getDeviceVendor() << '\n';
        stream << "Device Platform    : " << getDevicePlatform() << '\n';
        stream << "Driver Name        : " << getDriverName() << '\n';
        stream << "Driver Vendor      : " << getDriverVendor() << '\n';
        stream << "Driver Version     : " << getDriverVersion() << '\n';
        stream << "HW Chipset         : " << getHardwareChipset() << '\n';
        stream << "HW Name            : " << getHardwareName() << '\n';
        stream << "HW Vendor          : " << getHardwareVendor() << std::endl;
    }
    if (isWindowOpen() == 0) {
        stream << "Window not open" << std::endl;
        return;
    }
    if ((info.value & INFO_DEVICE) != 0) {
        unsigned long renderers = 0;
        for (RendererEntry* entry = renderers_10_; entry != 0; entry = entry->next_04) {
            renderers++;
        }
        stream << "Renderers used     : " << renderers << std::endl;
        /* reinterpret-ok: retail streams the HWND-valued handle through
           operator<<(const void*). */
        stream << "Window handle      : " << reinterpret_cast<const void*>(getWindowHandle())
               << std::endl;
        stream << "Width              : " << getWidth() << std::endl;
        stream << "Height             : " << getHeight() << std::endl;
        stream << "Fullscreen         : " << srBoolToString(isFullScreen()) << std::endl;
        stream << "Back buffers       : ";
        e_backBuffer back_buffer = getBackBufferType();
        if (back_buffer == static_cast<e_backBuffer>(1)) {
            stream << "none (blit)" << std::endl;
        } else if (back_buffer == static_cast<e_backBuffer>(2)) {
            stream << "one (double-buffered)" << std::endl;
        } else if (back_buffer == static_cast<e_backBuffer>(3)) {
            stream << "two (triple-buffered)" << std::endl;
        }
        stream << "Swap interval      : " << swap_interval_1764_ << std::endl;
        stream << "Gamma              : {" << gamma_1758_.x << "," << gamma_1758_.y << ","
               << gamma_1758_.z << "}" << std::endl;
    }
    if ((info.value & INFO_STATISTICS) != 0) {
        Statistics statistics = statistics_19f8_;
        if (statistics.elapsed_00 > 0.1) {
            stream << std::endl;
            stream << "Seconds since reset             : " << statistics.elapsed_00 << '\n';
            stream << "Frames since reset              : " << statistics.frames_2c << '\n';
            stream << "Statistics (average per second)" << '\n';
            stream << "Frames                          : "
                   << statistics.frames_2c / statistics.elapsed_00 << '\n';
            stream << "Triangle chunks rendered        : "
                   << statistics.value_30 / statistics.elapsed_00 << '\n';
            stream << "Triangles in                    : "
                   << statistics.value_34 / statistics.elapsed_00 << '\n';
            stream << "Vertices in                     : "
                   << statistics.value_3c / statistics.elapsed_00 << '\n';
            if (statistics.value_34 != 0) {
                stream << "Input vertex/triangle ratio     : "
                       << statistics.value_3c / static_cast<float>(statistics.value_34) << '\n';
            }
            stream << "DD triangles received           : "
                   << statistics.value_20 / statistics.elapsed_00 << '\n';
            stream << "DD vertices transfered          : "
                   << statistics.value_24 / statistics.elapsed_00 << '\n';
            stream << "DD vertex indices specified     : "
                   << statistics.value_28 / statistics.elapsed_00 << '\n';
            if (statistics.sphere_tests_6c != 0) {
                stream << "Objects bounding sphere tested  : "
                       << statistics.sphere_tests_6c / statistics.elapsed_00 << std::endl;
                stream << "Bounding sphere test passed     : "
                       << statistics.sphere_visible_70 / statistics.elapsed_00 << " ("
                       << statistics.sphere_visible_70 * 100.0 / statistics.sphere_tests_6c << "%)"
                       << std::endl;
            }
            if (statistics.box_tests_74 != 0) {
                stream << "Objects bounding box tested     : "
                       << statistics.box_tests_74 / statistics.elapsed_00 << std::endl;
                stream << "Bounding box test passed        : "
                       << statistics.box_visible_78 / statistics.elapsed_00 << " ("
                       << statistics.box_visible_78 * 100.0 / statistics.box_tests_74 << "%)"
                       << std::endl;
            }
            stream << std::endl;
            stream << "Triangles sorted                : "
                   << statistics.value_38 / statistics.elapsed_00 << '\n';
            stream << "Triangles removed by clipping   : "
                   << statistics.value_64 / statistics.elapsed_00 << '\n';
            stream << "View state changes              : "
                   << statistics.view_state_applies_40 / statistics.elapsed_00 << '\n';
            stream << "Matrix changes/classifications  : "
                   << statistics.matrix_classifications_7c / statistics.elapsed_00 << '\n';
            stream << "Draw state changes              : "
                   << statistics.draw_state_applies_44 / statistics.elapsed_00 << '\n';
            stream << "Per-frame state changes         : "
                   << statistics.frame_state_count_48 / statistics.elapsed_00 << '\n';
            stream << "Texture changes                 : "
                   << statistics.texture_binds_4c / statistics.elapsed_00 << '\n';
            stream << "Palette changes                 : "
                   << statistics.palette_binds_58 / statistics.elapsed_00 << '\n';
            stream << "Texture parameter updates       : "
                   << statistics.texture_parameter_sets_50 / statistics.elapsed_00 << '\n';
            stream << "Shader changes                  : "
                   << statistics.shader_sets_5c / statistics.elapsed_00 << '\n';
            stream << std::endl;
            stream << "DD draw commands                : "
                   << statistics.draw_calls_60 / statistics.elapsed_00 << '\n';
            if ((info_50_.flags_18_ & 0x10) != 0) {
                stream << "DD pixels drawn          (M/s)  : "
                       << statistics.value_10 * 1e-06 / statistics.elapsed_00 << '\n';
                /* reinterpret-ok: the device stats mirror stores the
                   transfer counter's double bits as a dword pair. */
                stream << "DD Texture data transfer (Mb/s) : "
                       << *reinterpret_cast<const double*>(&statistics.value_08) *
                              9.5367431640625e-07 / statistics.elapsed_00
                       << '\n';
            } else {
                stream << "DD doesn't support pixel/texture statistics" << std::endl;
            }
            stream << "Function calls to DD            : "
                   << statistics.value_68 / statistics.elapsed_00 << std::endl;
        }
        if ((info.value & INFO_DEBUG_DD) != 0 && (enable_flags_20_.value & 0x20) != 0 &&
            debug_dd_44_ != 0) {
            double total = 0.0;
            srStreamPrintf(stream, "\nFunction                        Calls/sec  Time used\n");
            srStreamPrintf(stream,
                           "---------------------------------------------------------------\n");
            for (long i = 0; i < 0x2b; i++) {
                unsigned long calls = debug_dd_44_->call_counts_170[i];
                double used = debug_dd_44_->call_times_18[i] - calls * debug_dd_44_->time_scale_10;
                if (used <= 0.0) {
                    used = 0.0;
                }
                if (calls != 0) {
                    char text[36];
                    sprintf(text, "%.2f", calls / statistics.elapsed_00);
                    double percent = used / statistics.elapsed_00 * 100.0;
                    srStreamPrintf(stream, "%-32s%-10s %.3f%%\n", srDebugDD::funcName[i], text,
                                   percent);
                    total += percent;
                }
            }
            srStreamPrintf(stream, "\nTotal:                                     %.2f%%\n\n",
                           total);
        }
    }
    if ((info.value & INFO_TEXTURE_CACHE) == 0) {
        return;
    }
    stream << std::endl;
    dumpTextureCache(stream);
}

// FUNCTION: SURRENDER 0x1001EB20
void srGERD::dumpTextureCache(std::ostream& stream)
{
    SectionAccess access(state_section_18_);
    if (!texture_hash_enabled_2044_) {
        srStreamPrintf(stream, "Texture cache hibernating\n");
        return;
    }
    unsigned long count = 0;
    for (Texture* texture = texture_head_202c_; texture != 0; texture = texture->next_04) {
        count++;
    }
    srStreamPrintf(stream, "GERD Texture cache:\n\n");
    srStreamPrintf(stream, "Cached textures:          %d\n", count);
    srStreamPrintf(stream, "Hash table size           %d\n", texture_lookup_2004_.bucket_count);
    srStreamPrintf(stream, "GERD Cache memory used:   %d kB\n",
                   (texture_cache_used_2034_ + 0x3ff) >> 10);
    if (texture_cache_size_2038_ == 0) {
        srStreamPrintf(stream, "Max cache size:           infinite\n");
    } else {
        srStreamPrintf(stream, "Max cache size:           %d kB\n",
                       (texture_cache_size_2038_ + 0x3ff) >> 10);
    }
    srStreamPrintf(stream, "\n");
    srStreamPrintf(stream, "Device TMUs:              %d\n", info_50_.max_texture_stages_28_);
    if (info_50_.texture_ram_24_ == 0) {
        srStreamPrintf(stream, "Device texture RAM:       infinite\n");
    } else {
        srStreamPrintf(stream, "Device texture RAM:       %d kB\n",
                       (info_50_.texture_ram_24_ + 0x3ff) >> 10);
    }
    srStreamPrintf(stream, "Resident textures:        %d kB ",
                   (getResidentTextureMemUsed() + 0x3ff) >> 10);
    if (info_50_.texture_ram_24_ != 0) {
        srStreamPrintf(stream, " (%.2f%%)",
                       getResidentTextureMemUsed() * 100.0 / info_50_.texture_ram_24_);
    }
    srStreamPrintf(stream, "\n\n");
    srStreamPrintf(stream,
                   "#     Resolution  Format       KB    LODs  Priority   Timestamp  Resident  "
                   "FHandle     Name\n");
    srStreamPrintf(stream,
                   "-----------------------------------------------------------------------------"
                   "--------------\n");
    long index = 0;
    for (Texture* entry = texture_head_202c_; entry != 0; entry = entry->next_04) {
        char format_name[64];
        format_name[0] = '\0';
        entry->pixel_format_0c.getName(format_name);
        srStreamPrintf(stream, "%04d  %03dx%03d     %-11s  ", index, entry->device_2c.width_20,
                       entry->device_2c.height_24, format_name);
        index++;
        float kb = (entry->device_2c.size_1c + 0x3ff) * 0.0009765625f;
        if (kb >= 10.0f) {
            srStreamPrintf(stream, "%-4d  ", static_cast<long>(kb));
        } else {
            srStreamPrintf(stream, "%.1f   ", kb);
        }
        int resident =
            entry->device_2c.resident_data_68 != 0 && entry->device_2c.resident_size_6c != 0;
        srStreamPrintf(stream, "%-2d    %.4f     %08x   %-5s     %08x   ",
                       entry->device_2c.last_level_2c - entry->device_2c.first_level_28 + 1,
                       entry->device_2c.priority_14, entry->device_2c.last_use_18,
                       srBoolToString(resident), entry->id_08);
        if (entry->name_28 == 0) {
            srStreamPrintf(stream, "anon\n");
        } else {
            srStreamPrintf(stream, "%s\n", entry->name_28);
        }
    }
}

// FUNCTION: SURRENDER 0x1001EE20
void srGERD::dumpDeviceList(std::ostream& stream)
{
    for (srGERD* gerd = first; gerd != 0; gerd = gerd->next_34_) {
        srStreamPrintf(stream, "%s\n", gerd->getDeviceName());
    }
}

// FUNCTION: SURRENDER 0x10018870
srGERD* srGERD::loadDeviceWithFileName(const char* filename, unsigned long device)
{
    /* Entry-point name table indexed by the missing-function error print
       below; retail stores the six strings contiguously at 0x100993CC. */
    static const char* const entry_names[] = {"srDDGetDriverApiVersion", "srDDGetDriverName",
                                              "srDDConfigureDriver",     "srDDGetDeviceCount",
                                              "srDDGetDeviceName",       "srDDInitDevice"};
    if (filename == 0) {
        return 0;
    }
    void* library = srDynamicLibrary::load(filename);
    if (library == 0) {
        srDebugPrintf(0,
                      "srGERD::loadDeviceWithFileName() -- "
                      "srDynamicLibrary::load() failed for file '%s'\n",
                      filename);
        return 0;
    }
    srDDGetDriverApiVersionFn getDriverApiVersion = reinterpret_cast<srDDGetDriverApiVersionFn>(
        srDynamicLibrary::getFunction(library, "srDDGetDriverApiVersion"));
    srDDGetDriverNameFn getDriverName = reinterpret_cast<srDDGetDriverNameFn>(
        srDynamicLibrary::getFunction(library, "srDDGetDriverName"));
    srDDConfigureDriverFn configureDriver = reinterpret_cast<srDDConfigureDriverFn>(
        srDynamicLibrary::getFunction(library, "srDDConfigureDriver"));
    srDDGetDeviceCountFn getDeviceCount = reinterpret_cast<srDDGetDeviceCountFn>(
        srDynamicLibrary::getFunction(library, "srDDGetDeviceCount"));
    srDDGetDeviceNameFn getDeviceName = reinterpret_cast<srDDGetDeviceNameFn>(
        srDynamicLibrary::getFunction(library, "srDDGetDeviceName"));
    srDDInitDeviceFn initDevice = reinterpret_cast<srDDInitDeviceFn>(
        srDynamicLibrary::getFunction(library, "srDDInitDevice"));
    long missing = -1;
    if (getDriverApiVersion == 0) {
        missing = 0;
    } else if (getDriverName == 0) {
        missing = 1;
    } else if (configureDriver != 0 && getDeviceCount != 0 && initDevice != 0 &&
               getDeviceName != 0) {
        if (getDriverApiVersion() < SR_DD_MIN_API_VERSION) {
            srDebugPrintf(0,
                          "srGERD::loadDeviceWithFileName() -- device driver '%s' "
                          "uses old API (cannot connect)!!\n",
                          filename);
            srDynamicLibrary::free(library);
            return 0;
        }
        const char* name = getDriverName();
        if (name == 0) {
            srDebugPrintf(0,
                          "srGERD::loadDeviceWithFileName() -- device driver '%s' "
                          "uses old API (doesn't support srDDGetdriverName)!!\n",
                          filename);
            srDynamicLibrary::free(library);
            return 0;
        }
        char* key = new char[strlen(name) + 7];
        sprintf(key, "DD_%s", name);
        for (long index = 0; index < (long)strlen(key); index++) {
            key[index] = static_cast<char>(toupper(key[index]));
        }
        if (srConfig.get(key) != 0) {
            configureDriver(srConfig.get(key));
        }
        delete[] key;
        unsigned long count = getDeviceCount();
        if (device < count) {
            srDD* dd = initDevice(device);
            if (dd == 0) {
                srDebugPrintf(0,
                              "srGERD::loadDeviceWithFileName() - device "
                              "initialization failed for file '%s' (devIndex = %d)=  "
                              "-- no hardware found?\n",
                              filename);
                srDynamicLibrary::free(library);
                return 0;
            }
            const char* device_name = getDeviceName(device);
            srDebugPrintf(5,
                          "srGERD::loadDeviceWithFileName() -- DD driver '%s' "
                          "(device %s) loaded succesfully.\n",
                          filename, device_name);
            return new srGERD(dd, library, device_name);
        }
        if (count == 0) {
            srDebugPrintf(0,
                          "srGERD::loadDeviceWithFileName() -- no devices available "
                          "for driver '%s'\n",
                          filename);
        }
    } else {
        if (configureDriver == 0) {
            missing = 2;
        } else if (getDeviceCount == 0) {
            missing = 3;
        } else if (getDeviceName == 0) {
            missing = 4;
        } else {
            if (initDevice != 0) {
                srDynamicLibrary::free(library);
                return 0;
            }
            missing = 5;
        }
    }
    if (missing >= 0) {
        srDebugPrintf(0,
                      "srGERD::loadDeviceWithFileName() -- "
                      "srDynamicLibrary::getFunction('%s') failed for file '%s'  "
                      "-- not a valid Device Driver!!\n",
                      entry_names[missing], filename);
    }
    srDynamicLibrary::free(library);
    return 0;
}

// FUNCTION: SURRENDER 0x10018B50
srGERD* srGERD::loadDevice(const char* name, const char* path, unsigned long device)
{
    if (name == 0) {
        return 0;
    }
    char filename[516];
    if (path != 0) {
        sprintf(filename, "%s\\srDD_%s", path, name);
    } else {
        sprintf(filename, "srDD_%s", name);
    }
    return loadDeviceWithFileName(filename, device);
}

// FUNCTION: SURRENDER 0x10018BC0
void srGERD::loadDevices(const char* path)
{
    srStringTable libraries;
    unsigned long count = srSystem::scanLibraries(libraries, path, "srDD*");
    for (unsigned long index = 0; index < count; index++) {
        unsigned long device = 0;
        while (loadDeviceWithFileName(libraries.getString(index), device) != 0) {
            device++;
        }
    }
}

// FUNCTION: SURRENDER 0x10018DA0
srGERD* srGERD::loadDevice(srStringTable& devices, unsigned long index)
{
    const char* string = devices.getString(index);
    if (string == 0) {
        return 0;
    }
    char* filename = new char[strlen(string) + 1];
    strcpy(filename, string);
    unsigned long device = 0;
    char* open = strchr(filename, '(');
    if (open != 0) {
        *open = '\0';
        char* close = strchr(open + 1, ')');
        if (close != 0) {
            *close = '\0';
        }
        device = atoi(open + 1);
    }
    srGERD* result = loadDeviceWithFileName(filename, device);
    delete[] filename;
    return result;
}

// FUNCTION: SURRENDER 0x100191E0
srGERD::e_error srGERD::createContext(unsigned long window)
{
    deleteContext();
    if (srWindow::isWindow(window) == 0) {
        return static_cast<e_error>(6);
    }
    for (srGERD* gerd = getFirst(); gerd != 0; gerd = gerd->getNext()) {
        if (gerd->isContextCreated() != 0 && gerd->getWindowHandle() == window) {
            return static_cast<e_error>(7);
        }
    }
    if (getDD()->createContext(window) != 0) {
        return static_cast<e_error>(8);
    }
    window_374_ = window;
    initDDInfo();
    initTextureFormats();
    initDisplayModeList();
    initGlobalPalette();
    resetStatistics();
    state_flags_28_ |= 1;
    return static_cast<e_error>(0);
}

// FUNCTION: SURRENDER 0x100192A0
void srGERD::deleteContext()
{
    if ((state_flags_28_ & 1) != 0) {
        closeWindow(static_cast<e_closeHint>(0));
        closeTexCache();
        if (texture_formats_360_ != 0) {
            delete[] texture_formats_360_;
        }
        if (display_modes_368_ != 0) {
            delete[] display_modes_368_;
        }
        texture_formats_360_ = 0;
        display_modes_368_ = 0;
        texture_format_count_364_ = 0;
        display_mode_count_36c_ = 0;
        getDD()->deleteContext();
        state_flags_28_ &= ~1UL;
        window_374_ = 0;
    }
}

// FUNCTION: SURRENDER 0x10019EB0
void srGERD::deleteRenderers()
{
    SectionAccess access(renderers_section_14_);
    while (renderers_10_ != 0) {
        RendererEntry* next = renderers_10_->next_04;
        delete renderers_10_->renderer_08;
        delete renderers_10_;
        renderers_10_ = next;
    }
}

// FUNCTION: SURRENDER 0x1001F880
void srGERD::accumRelease()
{
    if (accum_buffer_1af8_ != 0) {
        srHeap.free(accum_buffer_1af8_);
        accum_buffer_1af8_ = 0;
    }
    if (accum_scratch_1afc_ != 0) {
        srHeap.free(accum_scratch_1afc_);
        accum_scratch_1afc_ = 0;
    }
}

// FUNCTION: SURRENDER 0x10020DF0
srColorSurfaceIFace* srGERD::lockBuffer()
{
    if (lock_surface_1b00_ != 0) {
        return 0;
    }
    flush();
    if (_lockBuffer() != 0) {
        return 0;
    }
    if ((dirty_24_ & 0x1f0) != 0) {
        applyViewStateChanges();
    }
    srPixelConvert::PixelFormat format;
    getPixelFormat(format);
    lock_surface_1b00_ = new LockSurface(this, format);
    lock_surface_1b00_->setScissor(scissor_1628_.left, scissor_1628_.top, scissor_1628_.right,
                                   scissor_1628_.bottom);
    if (format.conversion_class == 0 && format.bytes_per_pixel_minus_one == 3 &&
        format.red_bits == 8 && format.green_bits == 8 && format.blue_bits == 8 &&
        format.alpha_bits == 8 && format.red_shift == 0x10 && format.green_shift == 8 &&
        format.blue_shift == 0 && format.alpha_shift == 0x18) {
        lock_surface_1b00_->argb32_5c_ = 1;
    }
    return lock_surface_1b00_;
}

// FUNCTION: SURRENDER 0x10020F30
void srGERD::unlockBuffer()
{
    if (lock_surface_1b00_ != 0) {
        lock_surface_1b00_->release();
        lock_surface_1b00_ = 0;
        _unlockBuffer();
    }
}

// FUNCTION: SURRENDER 0x1001A5F0
void srGERD::closeWindow(e_closeHint hint)
{
    SectionAccess access(state_section_18_);
    if (isWindowOpen() != 0) {
        state_flags_28_ |= 0x10;
        flush();
        deleteRenderers();
        unlockBuffer();
        invalidateResidentTextures();
        invalidatePalette();
        closeTexCache();
        getDD()->closeWindow();
        resetStatistics();
        accumRelease();
        memset(&open_info_378_, 0, sizeof(open_info_378_));
        state_flags_28_ &= ~2UL;
        back_buffer_type_38c_ = 0;
        if (prev_open_38_ != 0) {
            prev_open_38_->next_open_3c_ = next_open_3c_;
        }
        if (next_open_3c_ != 0) {
            next_open_3c_->prev_open_38_ = prev_open_38_;
        }
        if (this == firstOpen) {
            firstOpen = next_open_3c_;
        }
        prev_open_38_ = 0;
        next_open_3c_ = 0;
        state_flags_28_ &= ~0x10UL;
        shader_1ff8_ = srShader();
        if (texture_iface_1ffc_[0] != 0) {
            texture_iface_1ffc_[0]->release();
            texture_iface_1ffc_[0] = 0;
        }
        if (texture_iface_1ffc_[1] != 0) {
            texture_iface_1ffc_[1]->release();
            texture_iface_1ffc_[1] = 0;
        }
        pick_vertices_2230_.release();
    }
}

// FUNCTION: SURRENDER 0x1001CD10
srGERD::e_backBuffer srGERD::getBackBufferType() const
{
    return static_cast<e_backBuffer>(back_buffer_type_38c_);
}

// FUNCTION: SURRENDER 0x1001A120
srGERD::e_error srGERD::openWindow()
{
    if ((state_flags_28_ & 1) == 0) {
        return static_cast<e_error>(9);
    }
    return openWindow(srWindow::getWidth(window_374_), srWindow::getHeight(window_374_));
}

// FUNCTION: SURRENDER 0x1001A160
srGERD::e_error srGERD::openWindow(long width, long height)
{
    if ((state_flags_28_ & 1) == 0) {
        return static_cast<e_error>(9);
    }
    if (srWindow::isWindow(window_374_) == 0) {
        return static_cast<e_error>(6);
    }
    OpenInfo info;
    info.window_width_00 = srWindow::getWidth(window_374_);
    info.window_height_04 = srWindow::getHeight(window_374_);
    info.width_08 = width;
    info.height_0c = height;
    info.display_mode_10 = -1;
    return openWindowInternal(info);
}

// FUNCTION: SURRENDER 0x1001A1F0
srGERD::e_error srGERD::openWindow(long mode)
{
    if ((state_flags_28_ & 1) == 0) {
        return static_cast<e_error>(9);
    }
    if (mode < 0) {
        return openWindow();
    }
    if (srWindow::isWindow(window_374_) == 0) {
        return static_cast<e_error>(6);
    }
    if (display_mode_count_36c_ <= mode) {
        return static_cast<e_error>(3);
    }
    unsigned long* entry = display_modes_368_ + mode * 3;
    OpenInfo info;
    info.window_width_00 = info.width_08 = (long)entry[0];
    info.window_height_04 = info.height_0c = (long)entry[1];
    info.display_mode_10 = mode;
    return openWindowInternal(info);
}

// FUNCTION: SURRENDER 0x1001A290
srGERD::e_error srGERD::openWindowInternal(const OpenInfo& info)
{
    SectionAccess access(state_section_18_);
    if ((unsigned long)info.width_08 > (unsigned long)info.window_width_00 ||
        (unsigned long)info.height_0c > (unsigned long)info.window_height_04) {
        return static_cast<e_error>(2);
    }
    if ((state_flags_28_ & 1) == 0) {
        return static_cast<e_error>(9);
    }
    closeWindow(static_cast<e_closeHint>(1));
    if (info.width_08 != 0 && info.height_0c != 0 && info.window_width_00 != 0 &&
        info.window_height_04 != 0) {
        if (srWindow::isWindow(window_374_) == 0) {
            return static_cast<e_error>(6);
        }
        if (info.display_mode_10 < -1 || display_mode_count_36c_ <= info.display_mode_10 ||
            info_50_.unknown_00_ < (unsigned long)info.width_08 ||
            info_50_.unknown_04_ < (unsigned long)info.height_0c) {
            return static_cast<e_error>(2);
        }
        memset(&open_info_378_, 0, sizeof(open_info_378_));
        srDD::OpenInfo dd_info;
        dd_info.width = (unsigned long)info.width_08;
        dd_info.height = (unsigned long)info.height_0c;
        dd_info.display_mode = info.display_mode_10;
        srDD::OpenResult result;
        result.back_buffer_type = 0;
        if (getDD()->openWindow(dd_info, result) == 0) {
            open_info_378_ = info;
            if (result.back_buffer_type == 1) {
                back_buffer_type_38c_ = 1;
            } else if (result.back_buffer_type == 2) {
                back_buffer_type_38c_ = 2;
            } else if (result.back_buffer_type == 3) {
                back_buffer_type_38c_ = 3;
            }
            prev_open_38_ = 0;
            next_open_3c_ = firstOpen;
            if (firstOpen != 0) {
                firstOpen->prev_open_38_ = this;
            }
            unsigned long flags = state_flags_28_ & ~4UL;
            firstOpen = this;
            state_flags_28_ |= 2;
            state_flags_28_ = flags | 2;
            state_flags_28_ |= 8;
            resetStatistics();
            dirty_24_ = 0xffffffff;
            initTexCache();
            if (texture_iface_1ffc_[0] != 0) {
                texture_iface_1ffc_[0]->release();
                texture_iface_1ffc_[0] = 0;
            }
            if (texture_iface_1ffc_[1] != 0) {
                texture_iface_1ffc_[1]->release();
                texture_iface_1ffc_[1] = 0;
            }
            shader_1ff8_ = srShader();
            if (info_50_.max_texture_stages_28_ != 0) {
                unsigned long stage = 0;
                do {
                    texture_parms_1f40_[stage].mipmap_bias_04 = 0.0f;
                    texture_parms_1f40_[stage].packed_00 = 0x1a1;
                    texture_parms_1f40_[stage].mipmap_bias_04 = -1234567.0f;
                    setTexture(0, stage);
                    stage++;
                } while (stage < info_50_.max_texture_stages_28_);
            }
            invalidatePalette();
            scissor_1628_.left = 0;
            scissor_1628_.top = 0;
            scissor_1628_.right = getWidth();
            scissor_1628_.bottom = getHeight();
            view_left_1638_ = 0;
            view_top_163c_ = 0;
            view_right_1640_ = getWidth();
            view_bottom_1644_ = getHeight();
            createRenderer(1);
            if ((enable_flags_20_.value & 0x40) != 0) {
                long count;
                e_backBuffer back_buffer = getBackBufferType();
                if (back_buffer == static_cast<e_backBuffer>(2)) {
                    count = 2;
                } else if (back_buffer == static_cast<e_backBuffer>(3)) {
                    count = 3;
                } else {
                    count = 1;
                }
                for (; count != 0; count--) {
                    clear(srFlags<e_buffer>(0xfffffffb));
                    flipFrame();
                }
                flush();
            }
            return static_cast<e_error>(0);
        }
    }
    return static_cast<e_error>(3);
}

// FUNCTION: SURRENDER 0x10028460
void srGERD::closeTexCache()
{
    if (texture_hash_enabled_2044_ != 0) {
        invalidateTextureCache();
        removeDeletedTextures();
        markTextureAsDeleted(*texture_default_2030_);
        deleteTexture(*texture_default_2030_);
        texture_default_2030_ = 0;
        if (texture_lookup_2004_.bucket_count != 0) {
            delete[] texture_lookup_2004_.bucket_heads;
            delete[] texture_lookup_2004_.entries;
        }
        texture_lookup_2004_.bucket_count = 0;
        texture_lookup_2004_.bucket_heads = 0;
        texture_lookup_2004_.entries = 0;
        texture_lookup_2004_.free_head = -1;
        srHashEntry<unsigned long, Texture*>* entries = new srHashEntry<unsigned long, Texture*>[4];
        int* buckets = new int[4];
        long live = 0;
        for (long i = 0; i < 4; i++) {
            entries[i].next_index = -1;
            buckets[i] = -1;
        }
        if (texture_lookup_2004_.bucket_count != 0) {
            for (long bucket = 0; bucket < (long)texture_lookup_2004_.bucket_count; bucket++) {
                long index = texture_lookup_2004_.bucket_heads[bucket];
                while (index != -1) {
                    srHashEntry<unsigned long, Texture*>* entry =
                        texture_lookup_2004_.entries + index;
                    entries[live].key = entry->key;
                    unsigned long hashed =
                        ((entry->key >> 10 & 0xc00) ^ (entry->key & 0xc00)) >> 10 ^
                        (entry->key & 3);
                    entries[live].value = entry->value;
                    entries[live].next_index = buckets[hashed];
                    buckets[hashed] = live;
                    index = entry->next_index;
                    live++;
                }
            }
            delete[] texture_lookup_2004_.bucket_heads;
            delete[] texture_lookup_2004_.entries;
        }
        if (live < 4) {
            long slot = live;
            do {
                slot++;
                entries[slot - 1].next_index = slot;
            } while (slot < 4);
        }
        entries[3].next_index = -1;
        texture_lookup_2004_.bucket_heads = buckets;
        texture_lookup_2004_.free_head = live;
        texture_lookup_2004_.entries = entries;
        texture_lookup_2004_.bucket_count = 4;
        for (unsigned long chunk = 0; chunk < texture_pool_count_2024_; chunk++) {
            if (texture_pool_201c_.capacity <= chunk) {
                texture_pool_201c_.setCapacity(texture_pool_201c_.capacity + 8 + chunk);
            }
            srHeap.free(texture_pool_201c_.data[chunk]);
        }
        texture_pool_201c_.release();
        texture_free_2018_ = 0;
        texture_pool_count_2024_ = 0;
        texture_count_2014_ = 0;
        texture_deleted_2028_ = 0;
        texture_head_202c_ = 0;
        texture_cache_used_2034_ = 0;
        texture_hash_enabled_2044_ = 0;
    }
}

// FUNCTION: SURRENDER 0x10028200
void srGERD::initTexCache()
{
    if (texture_hash_enabled_2044_ == 0) {
        if (texture_lookup_2004_.bucket_count != 0) {
            delete[] texture_lookup_2004_.bucket_heads;
            delete[] texture_lookup_2004_.entries;
        }
        texture_lookup_2004_.bucket_count = 0;
        texture_lookup_2004_.bucket_heads = 0;
        texture_lookup_2004_.entries = 0;
        texture_lookup_2004_.free_head = -1;
        srHashEntry<unsigned long, Texture*>* entries = new srHashEntry<unsigned long, Texture*>[4];
        int* buckets = new int[4];
        long live = 0;
        for (long i = 0; i < 4; i++) {
            entries[i].next_index = -1;
            buckets[i] = -1;
        }
        if (texture_lookup_2004_.bucket_count != 0) {
            for (long bucket = 0; bucket < (long)texture_lookup_2004_.bucket_count; bucket++) {
                long index = texture_lookup_2004_.bucket_heads[bucket];
                while (index != -1) {
                    srHashEntry<unsigned long, Texture*>* entry =
                        texture_lookup_2004_.entries + index;
                    entries[live].key = entry->key;
                    unsigned long hashed =
                        ((entry->key >> 10 & 0xc00) ^ (entry->key & 0xc00)) >> 10 ^
                        (entry->key & 3);
                    entries[live].value = entry->value;
                    entries[live].next_index = buckets[hashed];
                    buckets[hashed] = live;
                    index = entry->next_index;
                    live++;
                }
            }
            delete[] texture_lookup_2004_.bucket_heads;
            delete[] texture_lookup_2004_.entries;
        }
        if (live < 4) {
            long slot = live;
            do {
                slot++;
                entries[slot - 1].next_index = slot;
            } while (slot < 4);
        }
        entries[3].next_index = -1;
        texture_lookup_2004_.bucket_heads = buckets;
        texture_lookup_2004_.free_head = live;
        texture_lookup_2004_.entries = entries;
        texture_lookup_2004_.bucket_count = 4;
        texture_head_202c_ = 0;
        texture_deleted_2028_ = 0;
        texture_cache_used_2034_ = 0;
        for (unsigned long chunk = 0; chunk < texture_pool_count_2024_; chunk++) {
            if (texture_pool_201c_.capacity <= chunk) {
                texture_pool_201c_.setCapacity(texture_pool_201c_.capacity + 8 + chunk);
            }
            srHeap.free(texture_pool_201c_.data[chunk]);
        }
        texture_pool_201c_.release();
        texture_free_2018_ = 0;
        texture_pool_count_2024_ = 0;
        texture_count_2014_ = 0;
        resetCurrentTexPointers();
        texture_default_2030_ = createNewTexture(srCore.getTexture());
        texture_default_2030_->device_2c.priority_14 = 1.1f;
        texture_hash_enabled_2044_ = 1;
        invalidateTextureCache();
    }
}

// FUNCTION: SURRENDER 0x10028B80
void srGERD::evaluateTextureDimensions(srDD::Texture& device,
                                       const srTextureIFace::Dimensions& dimensions)
{
    unsigned long min_dim = info_50_.texture_min_dim_2c_;
    unsigned char reduction = 0;
    if ((dimensions.hints & 0x80) == 0) {
        reduction = (unsigned char)texture_reduction_2040_;
    }
    unsigned long width = nextTextureDimension(dimensions.width) >> (reduction & 0x1f);
    unsigned long height = nextTextureDimension(dimensions.height) >> (reduction & 0x1f);
    unsigned long device_width = min_dim;
    if (min_dim <= width && width <= info_50_.texture_max_dim_30_) {
        device_width = width;
    }
    unsigned long device_height = min_dim;
    if (min_dim <= height && height <= info_50_.texture_max_dim_30_) {
        device_height = height;
    }
    int unbalanced = 0;
    if (device_height < device_width) {
        while (info_50_.texture_max_aspect_34_ < device_width / device_height &&
               device_height < info_50_.texture_max_dim_30_) {
            device_height *= 2;
        }
        unbalanced = device_height < device_width;
    }
    if (unbalanced == 0 && device_height != device_width) {
        while (info_50_.texture_max_aspect_34_ < device_height / device_width &&
               device_width < info_50_.texture_max_dim_30_) {
            device_width *= 2;
        }
    }
    device.width_20 = device_width;
    device.height_24 = device_height;
    device.first_level_28 = 0;
    device.last_level_2c = 0;
    if (((dimensions.hints & 8) == 0 || (info_50_.flags_18_ & 0x40) != 0) &&
        info_50_.texture_min_dim_2c_ < device_width) {
        do {
            if (device_height <= info_50_.texture_min_dim_2c_) {
                return;
            }
            ++device.last_level_2c;
            device_width >>= 1;
            device_height >>= 1;
        } while (info_50_.texture_min_dim_2c_ < device_width);
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
    dimensions.compression = default_compression_1fd8_;
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
    result->device_2c.resident_data_68 = 0;
    result->device_2c.priority_14 = texture->getPriority();
    ++statistics_1a78_.textures_created_54;
    if (dimensions.palette != 0) {
        dimensions.palette->release();
    }
    return result;
}

// FUNCTION: SURRENDER 0x10017930
long srGERD::getMaxTextureWidth() const
{
    return info_50_.texture_max_dim_30_;
}

// FUNCTION: SURRENDER 0x10017940
long srGERD::getMaxTextureHeight() const
{
    return info_50_.texture_max_dim_30_;
}

// FUNCTION: SURRENDER 0x10017950
long srGERD::getMaxTextureAspectRatio() const
{
    return info_50_.texture_max_aspect_34_;
}

// FUNCTION: SURRENDER 0x10017960
void srGERD::setGlobalPalette(const srPalette& palette)
{
    SectionAccess access(state_section_18_);
    if (palette.matchPalette(global_palette_1b38_, 0x100) != 0) {
        return;
    }
    long count = palette.getPaletteSize();
    if (count > 0x100) {
        count = 0x100;
    }
    for (long i = 0; i < count; ++i) {
        global_palette_1b38_[i] = palette.getColor(i);
    }
    /* reinterpret-ok: the DD receives the palette entries as raw dwords. */
    getDD()->setGlobalPalette(reinterpret_cast<unsigned long*>(global_palette_1b38_), 0x100);
}

// FUNCTION: SURRENDER 0x10017AA0
long srGERD::getTextureReduction() const
{
    srCriticalSection* section = state_section_18_;
    section->getAccess();
    long reduction = texture_reduction_2040_;
    section->releaseAccess();
    return reduction;
}

// FUNCTION: SURRENDER 0x10017B70
void srGERD::invalidateResidentPalette(srPalette* palette)
{
    if (palette != 0 && palette == palette_1fdc_) {
        if (palette_1fdc_ != 0) {
            palette_1fdc_->release();
            palette_1fdc_ = 0;
        }
        dirty_24_ |= 0x400;
        dirty_24_ |= 0x800;
    }
}

// FUNCTION: SURRENDER 0x10017D00
int srGERD::isTextureCached(srTextureIFace* texture)
{
    SectionAccess access(state_section_18_);
    if (texture != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        long index =
            texture_lookup_2004_
                .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
        if (index != -1) {
            srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
            do {
                if (entries[index].key == handle) {
                    if (entries[index].value != 0) {
                        return 1;
                    }
                    break;
                }
                index = entries[index].next_index;
            } while (index != -1);
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10017DD0
int srGERD::isTextureResident(srTextureIFace* texture)
{
    SectionAccess access(state_section_18_);
    if (texture != 0 && isWindowOpen() != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        long index =
            texture_lookup_2004_
                .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
        if (index != -1) {
            srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
            do {
                if (entries[index].key == handle) {
                    Texture* resident = entries[index].value;
                    if (resident != 0 && resident->device_2c.resident_data_68 != 0 &&
                        resident->device_2c.resident_size_6c != 0) {
                        return 1;
                    }
                    break;
                }
                index = entries[index].next_index;
            } while (index != -1);
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10017F70
int srGERD::getTextureInfo(srTextureIFace* texture, TextureInfo& info)
{
    SectionAccess access(state_section_18_);
    if (isWindowOpen() == 0) {
        return 0;
    }
    unsigned long handle = texture->getTextureFrameHandle();
    long index = texture_lookup_2004_
                     .bucket_heads[srHashValue(handle) & (texture_lookup_2004_.bucket_count - 1)];
    if (index != -1) {
        srHashEntry<unsigned long, Texture*>* entries = texture_lookup_2004_.entries;
        while (entries[index].key != handle) {
            index = entries[index].next_index;
            if (index == -1) {
                return 0;
            }
        }
        Texture* resident = entries[index].value;
        if (resident != 0) {
            info.pixel_format_00 = resident->pixel_format_0c;
            info.width_14 = resident->device_2c.width_20;
            info.height_18 = resident->device_2c.height_24;
            info.last_level_1c = resident->device_2c.last_level_2c;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10029600
void srGERD::initGlobalPalette()
{
    float level = 0.0f;
    for (long i = 0; i < 0x100; ++i) {
        unsigned char gray = (unsigned char)(long)(level * 255.0f + 0.5f);
        global_palette_1b38_[i].blue = gray;
        global_palette_1b38_[i].green = gray;
        global_palette_1b38_[i].red = gray;
        global_palette_1b38_[i].alpha = 0xff;
        level += 0.003921569f;
    }
    /* reinterpret-ok: the DD receives the palette entries as raw dwords. */
    getDD()->setGlobalPalette(reinterpret_cast<unsigned long*>(global_palette_1b38_), 0x100);
}

// FUNCTION: SURRENDER 0x10018C70
void srGERD::scanDevices(const char* path, srStringTable& devices)
{
    srStringTable libraries;
    char entry[512];
    long count = srSystem::scanLibraries(libraries, path, "srDD*");
    for (long index = 0; index < count; ++index) {
        void* library = srDynamicLibrary::load(libraries.getString(index));
        unsigned long device = 0;
        srGERD* gerd = loadDeviceWithFileName(libraries.getString(index), device);
        while (gerd != 0) {
            sprintf(entry, "%s(%ld)", libraries.getString(index), device);
            devices.addString(entry);
            delete gerd;
            device++;
            gerd = loadDeviceWithFileName(libraries.getString(index), device);
        }
        if (library != 0) {
            srDynamicLibrary::free(library);
        }
    }
}

// FUNCTION: SURRENDER 0x10018E40
void srGERD::debugWrite(const char* text)
{
    if (text != 0 && *text != '\0') {
        srOut << text;
    }
}

// FUNCTION: SURRENDER 0x10018E60
void srGERD::releaseAll()
{
    srGERD* gerd = getFirst();
    while (gerd != 0) {
        delete gerd;
        gerd = getFirst();
    }
}

// FUNCTION: SURRENDER 0x1001AFD0
void srGERD::setHint(e_hint hint, e_hintMode mode)
{
    if (hints_370_[hint] != mode) {
        hints_370_[hint] = mode;
    }
}

// FUNCTION: SURRENDER 0x1001AFF0
const char* srGERD::getErrorString(e_error error)
{
    if (0 <= error && error < 10) {
        return errStrings[error];
    }
    return "UNKNOWN ERROR";
}

const char* srGERD::errStrings[10] = {"ERROR_NONE",
                                      "ERROR_INVALID_ENUM",
                                      "ERROR_INVALID_VALUE",
                                      "ERROR_WINDOW_OPEN_FAILED",
                                      "ERROR_WINDOW_NOT_OPEN",
                                      "ERROR_BUFFER_LOCK_FAILED",
                                      "ERROR_INVALID_WHANDLE",
                                      "ERROR_SHARED_CONTEXT",
                                      "ERROR_CONTEXT_CREATION_FAILED",
                                      "ERROR_NO_CONTEXT"};

// FUNCTION: SURRENDER 0x1001B400
long srGERD::getGERDCount()
{
    long count = 0;
    for (srGERD* gerd = first; gerd != 0; gerd = gerd->getNext()) {
        count++;
    }
    return count;
}

// FUNCTION: SURRENDER 0x1001B420
srGERD* srGERD::getGERD(unsigned long index)
{
    unsigned long current = 0;
    for (srGERD* gerd = first; gerd != 0; gerd = gerd->getNext()) {
        if (current == index) {
            return gerd;
        }
        current++;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10020DD0
int srGERD::isBufferLocked()
{
    return lock_surface_1b00_ != 0;
}

// FUNCTION: SURRENDER 0x1001C830
long srGERD::getAccumAlphaBits() const
{
    return 0x10;
}

// FUNCTION: SURRENDER 0x1001C840
long srGERD::getAccumRedBits() const
{
    return 0x10;
}

// FUNCTION: SURRENDER 0x1001C850
long srGERD::getAccumGreenBits() const
{
    return 0x10;
}

// FUNCTION: SURRENDER 0x1001C860
long srGERD::getAccumBlueBits() const
{
    return 0x10;
}

// FUNCTION: SURRENDER 0x1001C870
void srGERD::setPolygonMode(e_polygonMode mode)
{
    if (mode != (e_polygonMode)polygon_mode_1fe0_) {
        flushImmediateRenderers();
        polygon_mode_1fe0_ = mode;
        dirty_24_ |= 0x4000;
    }
}

// FUNCTION: SURRENDER 0x1001C8E0
srGERD::e_polygonMode srGERD::getPolygonMode() const
{
    return (e_polygonMode)polygon_mode_1fe0_;
}

// FUNCTION: SURRENDER 0x1001C8F0
void srGERD::getClearAccum(srVector4T<float>& color) const
{
    color = clear_values_1b08_.accum_10;
}

// FUNCTION: SURRENDER 0x1001C920
void srGERD::setClearAccum(float red, float green, float blue, float alpha)
{
    srVector4T<float> color;
    color.x = red;
    color.y = green;
    color.z = blue;
    color.w = alpha;
    setClearAccum(color);
}

// FUNCTION: SURRENDER 0x1001C960
void srGERD::setClearAccum(const srVector4T<float>& color)
{
    clear_values_1b08_.accum_10 = color;
    float* clear = &clear_values_1b08_.accum_10.x;
    if (clear[0] < -1.0f) {
        clear[0] = -1.0f;
    }
    if (clear[1] < -1.0f) {
        clear[1] = -1.0f;
    }
    if (clear[2] < -1.0f) {
        clear[2] = -1.0f;
    }
    if (clear[3] < -1.0f) {
        clear[3] = -1.0f;
    }
    if (1.0f < clear[0]) {
        clear[0] = 1.0f;
    }
    if (1.0f < clear[1]) {
        clear[1] = 1.0f;
    }
    if (1.0f < clear[2]) {
        clear[2] = 1.0f;
    }
    if (1.0f < clear[3]) {
        clear[3] = 1.0f;
    }
}

// FUNCTION: SURRENDER 0x1001CB40
void srGERD::getClearColor(srVector4T<float>& color) const
{
    color = clear_values_1b08_.color_00;
}

// FUNCTION: SURRENDER 0x1001CBE0
void srGERD::setClearStencil(unsigned long stencil)
{
    clear_values_1b08_.stencil_28 = stencil;
}

// FUNCTION: SURRENDER 0x1001CBF0
double srGERD::getClearDepth() const
{
    return clear_values_1b08_.depth_20;
}

// FUNCTION: SURRENDER 0x1001CC00
unsigned long srGERD::getClearStencil() const
{
    return clear_values_1b08_.stencil_28;
}

// FUNCTION: SURRENDER 0x1001CF00
unsigned long srGERD::getDDAPIVersion() const
{
    return driver_info_2cc_.dd_api_version_0c;
}

// FUNCTION: SURRENDER 0x1001CF60
srGERD* srGERD::getPrev() const
{
    return prev_30_;
}

// FUNCTION: SURRENDER 0x1001CF70
srGERD* srGERD::getPrevOpen() const
{
    return prev_open_38_;
}

// FUNCTION: SURRENDER 0x1001CFA0
void srGERD::extCommand(unsigned long command, void* data, unsigned long size)
{
    getDD()->extCommand(command, data, size);
}

// FUNCTION: SURRENDER 0x1001CFD0
srGERD::e_hintMode srGERD::getHint(e_hint hint) const
{
    return hints_370_[hint];
}

// FUNCTION: SURRENDER 0x1001CFE0
void srGERD::getGamma(srVector3T<float>& gamma) const
{
    gamma = gamma_1758_;
}

// FUNCTION: SURRENDER 0x1001D000
srGERD::e_antiAlias srGERD::getAntiAlias() const
{
    return antialias_1768_;
}

// FUNCTION: SURRENDER 0x1001D010
srGERD::e_error srGERD::getError()
{
    e_error error = last_error_2c_;
    setError(ERROR_NONE);
    return error;
}

// FUNCTION: SURRENDER 0x1001D0C0
int srGERD::isFlipped() const
{
    return (state_flags_28_ >> 3) & 1;
}

// FUNCTION: SURRENDER 0x1001D0F0
const char* srGERD::getApiVersion() const
{
    return driver_info_2cc_.api_name_54;
}

// FUNCTION: SURRENDER 0x1001D1B0
srGERD::e_depthBuffer srGERD::getDepthBufferType() const
{
    return (e_depthBuffer)((info_50_.flags_18_ & 0xff) >> 3 & 1);
}

// FUNCTION: SURRENDER 0x1001D1C0
srDD::e_driverID srGERD::getDriverID() const
{
    return (srDD::e_driverID)driver_info_2cc_.driver_id_10;
}

// FUNCTION: SURRENDER 0x1001D1E0
unsigned long srGERD::getSwapInterval() const
{
    return swap_interval_1764_;
}

// FUNCTION: SURRENDER 0x1001D210
void srGERD::getTextureFormat(unsigned long index, srPixelConvert::PixelFormat& format) const
{
    if (texture_format_count_364_ <= (long)index) {
        index = 0;
    }
    format = texture_formats_360_[index];
}

// FUNCTION: SURRENDER 0x1001D240
unsigned long srGERD::getTextureFormatCount() const
{
    return texture_format_count_364_;
}

// FUNCTION: SURRENDER 0x1001D250
void srGERD::getDisplayModeInfo(long index, DisplayModeInfo& info) const
{
    if (display_mode_count_36c_ <= index) {
        index = 0;
    }
    info.width_00 = display_modes_368_[index * 3];
    info.height_04 = display_modes_368_[index * 3 + 1];
    info.depth_08 = display_modes_368_[index * 3 + 2];
}

// FUNCTION: SURRENDER 0x1001D290
unsigned long srGERD::getDisplayModeCount() const
{
    return display_mode_count_36c_;
}

// FUNCTION: SURRENDER 0x1001D330
void srGERD::getDepthRange(double& minimum, double& maximum) const
{
    minimum = depth_min_1618_;
    maximum = depth_max_1620_;
}

// FUNCTION: SURRENDER 0x1001D360
void srGERD::setDepthRange(double minimum, double maximum)
{
    if (minimum <= 0.0) {
        minimum = 0.0;
    } else if (minimum >= 1.0) {
        minimum = 1.0;
    }
    depth_min_1618_ = minimum;
    if (maximum <= 0.0) {
        depth_max_1620_ = 0.0;
    } else if (maximum < 1.0) {
        depth_max_1620_ = maximum;
    } else {
        depth_max_1620_ = 1.0;
    }
    dirty_24_ |= 0x100;
}

// FUNCTION: SURRENDER 0x1001BB00
const srShader& srGERD::getShader() const
{
    return shader_1ff8_;
}

// FUNCTION: SURRENDER 0x1001BAA0
void srGERD::disable(e_enable option)
{
    if ((enable_flags_20_.value & (1UL << option)) != 0) {
        toggle(option);
    }
}

// FUNCTION: SURRENDER 0x1001BAC0
void srGERD::enable(e_enable option)
{
    if ((enable_flags_20_.value & (1UL << option)) == 0) {
        toggle(option);
    }
}

// FUNCTION: SURRENDER 0x1001BBB0
void srGERD::drawTriangle(const srVector3i& triangle)
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
        getDD()->drawElements(static_cast<srRendererDefs::e_primitive>(3), 3,
                              srRendererDefs::INDEX_ULONG, &triangle);
        statistics_1a78_.draw_calls_60++;
    }
}

// FUNCTION: SURRENDER 0x1001BB60
void srGERD::fenceVertexArrays()
{
    getDD()->fence();
}

// FUNCTION: SURRENDER 0x1001BC30
void srGERD::setDataPtr(srRendererDefs::e_vertexArray index, long components,
                        srRendererDefs::e_type type, unsigned long stride, const void* values)
{
    vertex_arrays_21c4_.components_0c[index] = components;
    vertex_arrays_21c4_.types_24[index] = type;
    vertex_arrays_21c4_.strides_3c[index] = stride;
    vertex_arrays_21c4_.arrays_54[index] = values;
    dirty_21c0_ |= 1;
}

// FUNCTION: SURRENDER 0x1001BF10
void srGERD::setDiffusePointer(long components, srRendererDefs::e_type type, unsigned long stride,
                               const void* values)
{
    setDataPtr(srRendererDefs::VERTEX_ARRAY_DIFFUSE, components, type, stride, values);
}

// FUNCTION: SURRENDER 0x1001BF50
void srGERD::setSpecularPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                                const void* values)
{
    vertex_arrays_21c4_.components_0c[2] = components;
    vertex_arrays_21c4_.types_24[2] = type;
    vertex_arrays_21c4_.strides_3c[2] = stride;
    vertex_arrays_21c4_.arrays_54[2] = values;
    dirty_21c0_ |= 1;
}

// FUNCTION: SURRENDER 0x1001BF90
void srGERD::setFogPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                           const void* values)
{
    vertex_arrays_21c4_.components_0c[3] = components;
    vertex_arrays_21c4_.types_24[3] = type;
    vertex_arrays_21c4_.strides_3c[3] = stride;
    vertex_arrays_21c4_.arrays_54[3] = values;
    dirty_21c0_ |= 1;
}

// FUNCTION: SURRENDER 0x10020550
void srGERD::initClearColors()
{
    srVector4T<float> color;
    color.Set(0.0f, 0.0f, 0.0f, 0.0f);
    setClearAccum(color);
    color.Set(0.0f, 0.0f, 0.0f, 0.0f);
    setClearColor(color);
    setClearDepth(1.0);
    setClearStencil(0);
}

// FUNCTION: SURRENDER 0x1001D2E0
void srGERD::initView()
{
    depth_min_1618_ = 0.0;
    depth_max_1620_ = 1.0;
    clip_plane_count_167c_ = 0;
    clip_mask_1674_ = 0x3f;
    clip_mode1_mask_1678_ = 0;
    cull_mode_1648_ = static_cast<e_cullMode>(0);
    winding_164c_ = static_cast<e_winding>(0);
}

// FUNCTION: SURRENDER 0x10027F60
void srGERD::initTextureParameterMatrix()
{
    correction_map_1f5c_[0] = 0;
    correction_map_1f5c_[1] = 1;
    correction_map_1f5c_[2] = 2;
    correction_map_1f5c_[3] = 1;
    mag_filter_map_1f6c_[0] = 0;
    mag_filter_map_1f6c_[1] = 1;
    mag_filter_map_1f6c_[2] = 2;
    mag_filter_map_1f6c_[3] = 3;
    mag_filter_map_1f6c_[4] = 2;
    min_filter_map_1f80_[0] = 0;
    min_filter_map_1f80_[1] = 1;
    min_filter_map_1f80_[2] = 2;
    min_filter_map_1f80_[3] = 3;
    min_filter_map_1f80_[4] = 2;
    mipmap_map_1f94_[0] = 0;
    mipmap_map_1f94_[1] = 1;
    mipmap_map_1f94_[2] = 2;
    mipmap_map_1f94_[3] = 1;
    wrap_s_map_1fa4_[0] = 0;
    wrap_s_map_1fa4_[1] = 1;
    wrap_t_map_1fac_[0] = 0;
    wrap_t_map_1fac_[1] = 1;
    default_correction_1fb4_ = static_cast<srTextureIFace::e_correction>(1);
    default_mag_filter_1fb8_ = static_cast<srTextureIFace::e_filter>(2);
    default_min_filter_1fbc_ = static_cast<srTextureIFace::e_filter>(2);
    default_mipmap_1fc0_ = static_cast<srTextureIFace::e_mipmap>(1);
    default_texture_params_1fc4_[0] = 0;
    default_texture_params_1fc4_[1] = 1;
    default_texture_params_1fc4_[2] = 2;
    default_texture_params_1fc4_[3] = 3;
    default_texture_params_1fc4_[4] = 0;
    default_compression_1fd8_ = static_cast<srTextureIFace::e_compression>(0);
}

// FUNCTION: SURRENDER 0x1001CDF0
const char* srGERD::sGetClassName()
{
    return "srGERD";
}

// FUNCTION: SURRENDER 0x1001CD90
srRegistry::ClassNode* srGERD::sGetClassNode()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x4000);
    if (node == 0) {
        node = registry->registerClass("srGERD", srRuntimeClass::sGetClassNode(), 0x4000, 1);
    }
    return node;
}

// FUNCTION: SURRENDER 0x1001CDD0
unsigned long srGERD::sGetClassID()
{
    return srCore.getRegistry()->getClassID(sGetClassNode());
}

// FUNCTION: SURRENDER 0x1001CD30
srRegistry::ClassNode* srGERD::getClassNode() const
{
    return sGetClassNode();
}

// FUNCTION: SURRENDER 0x1001CD40
const char* srGERD::getClassName() const
{
    return "srGERD";
}

// FUNCTION: SURRENDER 0x1001CD50
unsigned long srGERD::getClassID() const
{
    return sGetClassID();
}

// FUNCTION: SURRENDER 0x1001FA30
void __cdecl srGERD::accumAccum_MMX(AccumPixel* accum, const srARGB* pixels, long scale, long count)
{
    __asm {
        mov edi, accum
        mov esi, pixels
        mov ecx, count
        movd mm5, scale
        punpcklwd mm5, mm5
        punpckhdq mm5, mm5
        movd mm6, scale
        punpcklwd mm6, mm6
        punpckldq mm6, mm6
        psrlw mm6, 1
        lea edi, [edi + ecx*8]
        lea esi, [esi + ecx*4]
        neg ecx
    accumAccum_MMX_loop:
        movd mm0, dword ptr [esi + ecx*4]
        punpcklbw mm0, mm0
        psrlw mm0, 1
        movq mm1, mm0
        pmullw mm0, mm5
        pmulhw mm1, mm6
        paddw mm1, mm1
        paddw mm0, mm1
        movq mm1, qword ptr [edi + ecx*8]
        paddsw mm0, mm1
        movq qword ptr [edi + ecx*8], mm0
        inc ecx
        js accumAccum_MMX_loop
        emms
    }
}

// FUNCTION: SURRENDER 0x1001FA90
void __cdecl srGERD::accumLoad_MMX(AccumPixel* accum, const srARGB* pixels, long scale, long count)
{
    __asm {
        mov edi, accum
        mov esi, pixels
        mov ecx, count
        movd mm5, scale
        punpcklwd mm5, mm5
        punpckhdq mm5, mm5
        movd mm6, scale
        punpcklwd mm6, mm6
        punpckldq mm6, mm6
        psrlw mm6, 1
        lea edi, [edi + ecx*8]
        lea esi, [esi + ecx*4]
        neg ecx
    accumLoad_MMX_loop:
        movd mm0, dword ptr [esi + ecx*4]
        punpcklbw mm0, mm0
        psrlw mm0, 1
        movq mm1, mm0
        pmullw mm0, mm5
        pmulhw mm1, mm6
        paddw mm1, mm1
        paddw mm0, mm1
        movq qword ptr [edi + ecx*8], mm0
        inc ecx
        js accumLoad_MMX_loop
        emms
    }
}

// FUNCTION: SURRENDER 0x1001FB70
void __cdecl srGERD::accumReturn_MMX(srARGB* pixels, const AccumPixel* accum, long scale,
                                     long count)
{
    __asm {
        mov edi, accum
        mov esi, pixels
        mov ecx, count
        movd mm5, scale
        punpcklwd mm5, mm5
        punpckhdq mm5, mm5
        movd mm6, scale
        punpcklwd mm6, mm6
        punpckldq mm6, mm6
        psrlw mm6, 1
        lea edi, [edi + ecx*8]
        lea esi, [esi + ecx*4]
        neg ecx
    accumReturn_MMX_loop:
        movq mm0, qword ptr [edi + ecx*8]
        movq mm1, mm0
        pmullw mm0, mm5
        pmulhw mm1, mm6
        paddw mm1, mm1
        paddw mm0, mm1
        psraw mm0, 7
        movq mm1, mm0
        psraw mm0, 0xf
        pandn mm0, mm1
        packuswb mm0, mm0
        movd dword ptr [esi + ecx*4], mm0
        inc ecx
        js accumReturn_MMX_loop
        emms
    }
}

// FUNCTION: SURRENDER 0x1001FBD0
void srGERD::accumulate(e_accum operation, float scale)
{
    if (!isWindowOpen()) {
        return;
    }
    long width = scissor_1628_.right - scissor_1628_.left;
    long height = scissor_1628_.bottom - scissor_1628_.top;
    if (width == 0 || height == 0) {
        return;
    }
    if (accum_buffer_1af8_ == 0) {
        accumAlloc();
        if (accum_buffer_1af8_ == 0) {
            return;
        }
    }
    srColorSurfaceIFace* surface = 0;
    if ((operation == ACCUM_LOAD || operation == ACCUM_ACCUMULATE || operation == ACCUM_RETURN) &&
        (surface = lockBuffer()) == 0) {
        setError(ERROR_BUFFER_LOCK_FAILED);
        return;
    }
    AccumPixel* row = accum_buffer_1af8_ + getWidth() * scissor_1628_.top + scissor_1628_.left;
    /* reinterpret-ok: the accum row scratch is raw dword storage reused as
       an ARGB pixel row. */
    srARGB* pixels = reinterpret_cast<srARGB*>(accum_scratch_1afc_);
    if ((srCore.getTimer()->m_cpu_features & 0x800000) != 0) {
        long scale16 = (long)(scale * (operation == ACCUM_MULTIPLY ? 32767.0 : 65536.0));
        switch (operation) {
        case ACCUM_LOAD: {
            for (long y = 0; y < height; y++) {
                surface->getPixelRow(
                    reinterpret_cast<unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                __asm {
                    mov edi, row
                    mov esi, pixels
                    mov ecx, width
                    movd mm5, scale16
                    punpcklwd mm5, mm5
                    punpckhdq mm5, mm5
                    movd mm6, scale16
                    punpcklwd mm6, mm6
                    punpckldq mm6, mm6
                    psrlw mm6, 1
                    lea edi, [edi + ecx*8]
                    lea esi, [esi + ecx*4]
                    neg ecx
                accumulate_load_loop:
                    movd mm0, dword ptr [esi + ecx*4]
                    punpcklbw mm0, mm0
                    psrlw mm0, 1
                    movq mm1, mm0
                    pmullw mm0, mm5
                    pmulhw mm1, mm6
                    paddw mm1, mm1
                    paddw mm0, mm1
                    movq mm1, qword ptr [edi + ecx*8]
                    paddsw mm0, mm1
                    movq qword ptr [edi + ecx*8], mm0
                    inc ecx
                    js accumulate_load_loop
                    emms
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_ACCUMULATE: {
            for (long y = 0; y < height; y++) {
                surface->getPixelRow(
                    reinterpret_cast<unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                __asm {
                    mov edi, row
                    mov esi, pixels
                    mov ecx, width
                    movd mm5, scale16
                    punpcklwd mm5, mm5
                    punpckhdq mm5, mm5
                    movd mm6, scale16
                    punpcklwd mm6, mm6
                    punpckldq mm6, mm6
                    psrlw mm6, 1
                    lea edi, [edi + ecx*8]
                    lea esi, [esi + ecx*4]
                    neg ecx
                accumulate_accum_loop:
                    movd mm0, dword ptr [esi + ecx*4]
                    punpcklbw mm0, mm0
                    psrlw mm0, 1
                    movq mm1, mm0
                    pmullw mm0, mm5
                    pmulhw mm1, mm6
                    paddw mm1, mm1
                    paddw mm0, mm1
                    movq qword ptr [edi + ecx*8], mm0
                    inc ecx
                    js accumulate_accum_loop
                    emms
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_MULTIPLY: {
            for (long y = 0; y < height; y++) {
                __asm {
                    mov edi, row
                    mov ecx, width
                    movd mm6, scale16
                    punpcklwd mm6, mm6
                    punpckldq mm6, mm6
                    lea edi, [edi + ecx*8]
                    neg ecx
                accumulate_add_loop:
                    movq mm0, qword ptr [edi + ecx*8]
                    paddw mm0, mm6
                    movq qword ptr [edi + ecx*8], mm0
                    inc ecx
                    js accumulate_add_loop
                    emms
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_ADD: {
            for (long y = 0; y < height; y++) {
                __asm {
                    mov edi, row
                    mov ecx, width
                    movd mm5, scale16
                    punpcklwd mm5, mm5
                    punpckhdq mm5, mm5
                    movd mm6, scale16
                    punpcklwd mm6, mm6
                    punpckldq mm6, mm6
                    psrlw mm6, 1
                    lea edi, [edi + ecx*8]
                    neg ecx
                accumulate_mult_loop:
                    movq mm0, qword ptr [edi + ecx*8]
                    movq mm1, mm0
                    pmullw mm0, mm5
                    pmulhw mm1, mm6
                    paddw mm1, mm1
                    paddw mm0, mm1
                    movq qword ptr [edi + ecx*8], mm0
                    inc ecx
                    js accumulate_mult_loop
                    emms
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_RETURN: {
            for (long y = 0; y < height; y++) {
                __asm {
                    mov edi, row
                    mov esi, pixels
                    mov ecx, width
                    movd mm5, scale16
                    punpcklwd mm5, mm5
                    punpckhdq mm5, mm5
                    movd mm6, scale16
                    punpcklwd mm6, mm6
                    punpckldq mm6, mm6
                    psrlw mm6, 1
                    lea edi, [edi + ecx*8]
                    lea esi, [esi + ecx*4]
                    neg ecx
                accumulate_return_loop:
                    movq mm0, qword ptr [edi + ecx*8]
                    movq mm1, mm0
                    pmullw mm0, mm5
                    pmulhw mm1, mm6
                    paddw mm1, mm1
                    paddw mm0, mm1
                    psraw mm0, 7
                    movq mm1, mm0
                    psraw mm0, 0xf
                    pandn mm0, mm1
                    packuswb mm0, mm0
                    movd dword ptr [esi + ecx*4], mm0
                    inc ecx
                    js accumulate_return_loop
                    emms
                }
                surface->setPixelRow(
                    reinterpret_cast<const unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                row += getWidth();
            }
            break;
        }
        default:
            setError(ERROR_INVALID_ENUM);
        }
    } else {
        short addend = accumConvert(scale);
        switch (operation) {
        case ACCUM_LOAD: {
            short table[256];
            for (long i = 0; i < 256; i++) {
                table[i] = (short)(long)(i * (scale * (32767.0f / 255.0f)));
            }
            for (long y = 0; y < height; y++) {
                surface->getPixelRow(
                    reinterpret_cast<unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                for (long x = 0; x < width; x++) {
                    row[x].red_00 = row[x].red_00 + table[pixels[x].blue];
                    row[x].green_02 = row[x].green_02 + table[pixels[x].green];
                    row[x].blue_04 = row[x].blue_04 + table[pixels[x].red];
                    row[x].alpha_06 = row[x].alpha_06 + table[pixels[x].alpha];
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_ACCUMULATE: {
            short table[256];
            for (long i = 0; i < 256; i++) {
                table[i] = (short)(long)(i * (scale * (32767.0f / 255.0f)));
            }
            for (long y = 0; y < height; y++) {
                surface->getPixelRow(
                    reinterpret_cast<unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                for (long x = 0; x < width; x++) {
                    row[x].red_00 = table[pixels[x].blue];
                    row[x].green_02 = table[pixels[x].green];
                    row[x].blue_04 = table[pixels[x].red];
                    row[x].alpha_06 = table[pixels[x].alpha];
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_MULTIPLY: {
            for (long y = 0; y < height; y++) {
                for (long x = 0; x < width; x++) {
                    row[x].red_00 = row[x].red_00 + addend;
                    row[x].green_02 = row[x].green_02 + addend;
                    row[x].blue_04 = row[x].blue_04 + addend;
                    row[x].alpha_06 = row[x].alpha_06 + addend;
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_ADD: {
            for (long y = 0; y < height; y++) {
                for (long x = 0; x < width; x++) {
                    row[x].red_00 = (short)(long)(row[x].red_00 * scale);
                    row[x].green_02 = (short)(long)(row[x].green_02 * scale);
                    row[x].blue_04 = (short)(long)(row[x].blue_04 * scale);
                    row[x].alpha_06 = (short)(long)(row[x].alpha_06 * scale);
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_RETURN: {
            unsigned char table[512];
            for (long i = 0; i < 512; i++) {
                float level = i * (1.0f / 255.0f);
                if (level <= 0.0f) {
                    level = 0.0f;
                } else if (level >= 1.0f) {
                    level = 1.0f;
                }
                table[i] = (unsigned char)(long)(level * scale * 255.0f + 0.5f);
            }
            for (long y = 0; y < height; y++) {
                for (long x = 0; x < width; x++) {
                    pixels[x].blue = table[((unsigned short)row[x].red_00) >> 7];
                    pixels[x].green = table[((unsigned short)row[x].green_02) >> 7];
                    pixels[x].red = table[((unsigned short)row[x].blue_04) >> 7];
                    pixels[x].alpha = table[((unsigned short)row[x].alpha_06) >> 7];
                }
                surface->setPixelRow(
                    reinterpret_cast<const unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    scissor_1628_.top + y, scissor_1628_.left, scissor_1628_.right);
                row += getWidth();
            }
            break;
        }
        default:
            setError(ERROR_INVALID_ENUM);
        }
    }
    if (surface != 0) {
        unlockBuffer();
    }
}
