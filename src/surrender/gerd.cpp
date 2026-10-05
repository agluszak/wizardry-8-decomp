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

// FUNCTION: SURRENDER 0x1001EEA0
srGERD::TexturePool::TexturePool() : count(0), free(0), pool_count(0) {}

// FUNCTION: SURRENDER 0x1001F0B0
void srGERD::TexturePool::release()
{
    for (unsigned long index = 0; index < pool_count; ++index) {
        srHeap.free(chunks[index]);
    }
    chunks.release();
    free = 0;
    pool_count = 0;
    count = 0;
}

/* The pool operations are expanded in allocTexture/deleteTexture. Their
   names are descriptive; retail establishes the chunk and free-list behavior. */
srGERD::Texture* srGERD::TexturePool::allocate()
{
    if (free == 0) {
        unsigned long chunk_count = count;
        if (chunk_count < 2) {
            chunk_count = 1;
        } else if (0xff < (long)chunk_count) {
            chunk_count = 0x100;
        }
        Texture* chunk = static_cast<Texture*>(srHeap.allocate(chunk_count * sizeof(Texture)));
        free = chunk;
        chunks[pool_count++] = chunk;
        Texture* link = chunk;
        for (unsigned long index = chunk_count; index != 0; --index) {
            link->prev = link + 1;
            ++link;
        }
        chunk[chunk_count - 1].prev = 0;
    }
    Texture* texture = free;
    free = texture->prev;
    ++count;
    return texture;
}

void srGERD::TexturePool::release(Texture* texture)
{
    if (texture == 0) {
        return;
    }
    --count;
    texture->prev = free;
    free = texture;
    if (count == 0) {
        release();
    }
}

// FUNCTION: SURRENDER 0x10019A00 SYMBOL
// ??0MatrixStack@srGERD@@QAE@XZ
srGERD::MatrixStack::MatrixStack() : depth(0) {}

// FUNCTION: SURRENDER 0x10019320
srGERD::srGERD(srDD* device, void* module, const char* device_name)
    : dirty(0), state_flags(0), vertex_arrays_dirty(0)
{
    owner_thread = srThread::getHandle();
    this->device.dd = device;
    this->device.module = module;
    this->device.window = 0;
    this->device.back_buffer_type = static_cast<e_backBuffer>(0);
    this->device.debug_dd = 0;
    this->device.real_dd = 0;
    vertex_arrays_dirty = 0xffffffff;
    vertex_arrays.clip.value = srRendererDefs::FRUSTUM_CLIP_MASK;
    vertex_arrays.count = 0;
    vertex_arrays.mask.value = 0;
    for (unsigned long index = 0; index < 6; ++index) {
        vertex_arrays.components[index] = 0;
        vertex_arrays.types[index] = srRendererDefs::TYPE_FLOAT;
        vertex_arrays.strides[index] = 0;
        vertex_arrays.arrays[index] = 0;
    }
    srZeroMemory(&state, 0x13c8);
    srZeroMemory(&accum_buffer, 0x440);
    srZeroMemory(&texture_slots, 0xb0);
    srZeroMemory(&this->device.info, 0x27c);
    srZeroMemory(&this->device.texture_formats, 8);
    srZeroMemory(&this->device.display_modes, 8);
    srZeroMemory(&this->device.driver_info, 0x94);
    if (device_name != 0) {
        strncpy(this->device.info.text[0], device_name, 0x3f);
    }
    polygon_mode = POLYGON_FILL;
    polygon_offset = 0;
    state_section = new srCriticalSection;
    pick.pick_key = 0;
    exclusion_mask = 0;
    enable_flags.value = 0;
    enable_flags.set(ENABLE_AUTO_FLIP, 1);
    enable_flags.set(ENABLE_CLEAR_ON_OPEN, 1);
    state_flags = 0;
    dirty = 0xffffffff;
    setError(ERROR_NONE);
    display.gamma = 1.0f;
    display.swap_interval = 1;
    display.antialias = ANTIALIAS_NONE;
    fog_color = 0.0f;
    shader = srShader();
    texture_iface[0] = 0;
    texture_iface[1] = 0;
    enable_flags.set(ENABLE_POSITIONAL_0, 1);
    this->device.hints[0] = static_cast<e_hintMode>(0);
    initTextureParameterMatrix();
    this->device.driver_info.api_version = 0x128;
    this->device.driver_info.dd_api_version = 0;
    this->device.driver_info.debug_write = debugWrite;
    this->device.driver_info.flags = 0;
    if ((srCore.getTimer()->m_cpu_features & (1UL << srTimer::CPU_FEATURE_MMX)) != 0) {
        this->device.driver_info.flags = 1;
    }
    getDD()->getDriverInfo(this->device.driver_info);
    texture_hash_enabled = false;
    texture_cache_size = 0;
    texture_default = 0;
    texture_reduction = 0;
    initClearColors();
    initLights();
    initMatrices();
    initView();
    srDD::e_driverID driver_id = getDriverID();
    srGERD* node = getFirst();
    srGERD* previous = 0;
    if (node != 0) {
        do {
            if (driver_id <= node->getDriverID()) {
                break;
            }
            previous = node;
            node = node->getNext();
        } while (node != 0);
    }
    if (previous != 0) {
        prev = previous;
        next = previous->getNext();
    } else {
        prev = 0;
        next = getFirst();
    }
    if (prev != 0) {
        prev->next = this;
    } else {
        first = this;
    }
    if (next != 0) {
        next->prev = this;
    }
    prev_open = 0;
    next_open = 0;
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    setName(this->device.driver_info.name);
    renderers = 0;
    renderers_section = new srCriticalSection;
}

// FUNCTION: SURRENDER 0x10019F40
srGERD::~srGERD()
{
    deleteRenderers();
    if (isEnabled(ENABLE_DEBUG_DD)) {
        toggle(ENABLE_DEBUG_DD);
    }
    deleteContext();
    delete device.dd;
    device.dd = 0;
    if (device.module != 0) {
        srDynamicLibrary::free(device.module);
    }
    if (prev != 0) {
        prev->next = next;
    }
    if (next != 0) {
        next->prev = prev;
    }
    if (this == first) {
        first = next;
    }
    prev = 0;
    next = 0;
    srCriticalSection* section = renderers_section;
    if (section != 0) {
        section->getAccess();
        section->releaseAccess();
        delete section;
    }
    renderers_section = 0;
    section = state_section;
    if (section != 0) {
        section->getAccess();
        section->releaseAccess();
        delete section;
    }
    state_section = 0;
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x1001B010
srGERD& srGERD::operator=(const srGERD& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10017920
srDD* srGERD::getDD() const
{
    statistics.device_calls++;
    return device.dd;
}

// FUNCTION: SURRENDER 0x10017A50
void srGERD::setTextureReduction(long reduction)
{
    srCriticalSection* section = state_section;
    section->getAccess();
    if (reduction < 0) {
        texture_reduction = 0;
        section->releaseAccess();
        return;
    }
    if (reduction > 7) {
        reduction = 7;
    }
    texture_reduction = reduction;
    section->releaseAccess();
}

// FUNCTION: SURRENDER 0x10017AC0
void srGERD::setTexture(srTextureIFace* texture, unsigned long layer)
{
    srCriticalSectionAccess access(state_section);
    if (layer < device.info.max_texture_stages && texture_iface[layer] != texture) {
        texture_iface[layer] = texture;
        changeTexture(texture, layer, 0);
        dirty |= 1 << (layer + DIRTY_TEXTURE_SHIFT);
    }
}

// FUNCTION: SURRENDER 0x10017F30
unsigned long srGERD::getTextureCacheSize() const
{
    srCriticalSection* section = state_section;
    section->getAccess();
    unsigned long size = texture_cache_size;
    section->releaseAccess();
    return size;
}

// FUNCTION: SURRENDER 0x10017F50
unsigned long srGERD::getTextureCacheUsed() const
{
    srCriticalSection* section = state_section;
    section->getAccess();
    unsigned long used = texture_cache_used;
    section->releaseAccess();
    return used;
}

// FUNCTION: SURRENDER 0x1001AA60
long srGERD::getDisplayMode(unsigned long width, unsigned long height, unsigned long depth) const
{
    if ((state_flags & STATE_CONTEXT_CREATED) != 0) {
        for (long i = 0; i < device.display_mode_count; i++) {
            const srDD::WindowInfo& mode = device.display_modes[i];
            if (mode.width == width && mode.height == height && mode.depth == depth) {
                return i;
            }
        }
    }
    return -1;
}

// FUNCTION: SURRENDER 0x1001AAF0
void srGERD::getStatistics(Statistics& statistics)
{
    srCriticalSectionAccess access(renderers_section);
    this->statistics.input_triangles = 0;
    this->statistics.input_vertices = 0;
    this->statistics.triangle_chunks = 0;
    this->statistics.sorted_triangles = 0;
    this->statistics.clipped_triangles = 0;
    for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
        Renderer* renderer = entry->renderer;
        if (renderer != 0) {
            unsigned long renderer_stats[7];
            renderer->getStatistics(renderer_stats);
            this->statistics.input_triangles += renderer_stats[1];
            this->statistics.clipped_triangles += renderer_stats[2];
            this->statistics.input_vertices += renderer_stats[3];
            this->statistics.triangle_chunks += renderer_stats[0];
            if (renderer->sorted == 1) {
                this->statistics.sorted_triangles += renderer_stats[6];
            }
        }
    }
    srDD::Statistics device;
    memset(&device, 0, sizeof(device));
    getDD()->getStatistics(device);
    this->statistics.texture_transfer_low = device.value_00;
    this->statistics.texture_transfer_high = device.value_04;
    this->statistics.pixels_drawn = device.pixels_drawn;
    this->statistics.value_18 = device.value_10;
    this->statistics.value_1c = device.value_14;
    this->statistics.device_triangles = device.triangles_received;
    this->statistics.device_vertices = device.vertices_transferred;
    this->statistics.device_vertex_indices = device.vertex_indices;
    statistics = this->statistics;
    statistics.elapsed =
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - statistics.elapsed;
}

// FUNCTION: SURRENDER 0x1001ACD0
void srGERD::resetStatistics()
{
    memset(&statistics, 0, sizeof(statistics));
    getDD()->resetStatistics();
    srCriticalSectionAccess access(renderers_section);
    for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
        while (entry->busy != 0) {
            srThread::yield(0);
        }
        entry->renderer->resetStatistics();
    }
    statistics.elapsed = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
    frame_statistics = statistics;
}

// FUNCTION: SURRENDER 0x1001ADD0
void srGERD::toggle(e_enable option)
{
    enable_flags.value ^= 1UL << option;
    if (option == ENABLE_POSITIONAL_0) {
        dirty |= DIRTY_FRAME_ENABLE;
        return;
    }
    if (option == ENABLE_DEBUG_DD) {
        if ((enable_flags.value & (1UL << ENABLE_DEBUG_DD)) == 0) {
            device.dd = device.real_dd;
            if (device.debug_dd != 0) {
                delete device.debug_dd;
            }
            device.real_dd = 0;
            device.debug_dd = 0;
            return;
        }
        device.real_dd = device.dd;
        device.debug_dd = new srDebugDD(device.real_dd);
        device.dd = device.debug_dd;
    }
}

// FUNCTION: SURRENDER 0x1001AE60
void srGERD::popEnable()
{
    unsigned long flags = 0;
    if (environment_state.enable_depth != 0) {
        environment_state.enable_depth -= 1;
        flags = environment_state.enable_stack[environment_state.enable_depth]
                    .value;
    }
    if (flags != enable_flags.value) {
        for (e_enable option = static_cast<e_enable>(0); static_cast<int>(option) < 7;
             option = static_cast<e_enable>(static_cast<int>(option) + 1)) {
            unsigned long bit = 1UL << option;
            if (((flags & bit) != 0) != ((enable_flags.value & bit) != 0)) {
                toggle(option);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1001AEC0
void srGERD::setSwapInterval(unsigned long interval)
{
    if (interval != display.swap_interval) {
        display.swap_interval = interval;
        dirty |= DIRTY_SWAP_INTERVAL;
    }
}

// FUNCTION: SURRENDER 0x1001AEE0
void srGERD::setGamma(const srVector3T<float>& gamma)
{
    srVector3T<float> adjusted = gamma;
    if (adjusted.x < 0.0f) {
        adjusted.x = 0.0f;
    }
    if (adjusted.y < 0.0f) {
        adjusted.y = 0.0f;
    }
    if (adjusted.z < 0.0f) {
        adjusted.z = 0.0f;
    }
    if (!(adjusted == display.gamma)) {
        display.gamma = adjusted;
        dirty |= DIRTY_GAMMA;
    }
}

// FUNCTION: SURRENDER 0x1001AFB0
void srGERD::setAntiAlias(e_antiAlias mode)
{
    if (mode != display.antialias) {
        display.antialias = mode;
        dirty |= DIRTY_ANTIALIAS;
    }
}

// FUNCTION: SURRENDER 0x1001BE70
void srGERD::setClipState(srFlags<srRendererDefs::e_clip> state)
{
    if (state.value != vertex_arrays.clip.value) {
        vertex_arrays.clip = state;
        vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
    }
}

// FUNCTION: SURRENDER 0x1001C380
srGERD::e_winding srGERD::getWinding() const
{
    return state.winding;
}

// FUNCTION: SURRENDER 0x1001C390
void srGERD::setWinding(e_winding winding)
{
    if (state.winding != winding) {
        state.winding = winding;
        dirty |= DIRTY_CULLING;
    }
}

// FUNCTION: SURRENDER 0x1001C3B0
void srGERD::setAmbientLight(const srVector4T<float>& light)
{
    ambient_light = light;
}

// FUNCTION: SURRENDER 0x1001C410
void srGERD::getAmbientLight(srVector4T<float>& light)
{
    light = ambient_light;
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
    return state.cull_mode;
}

// FUNCTION: SURRENDER 0x1001C4F0
void srGERD::getEnvironmentRange(float& minimum, float& maximum) const
{
    minimum = environment.x;
    maximum = environment.y;
}

// FUNCTION: SURRENDER 0x1001C5A0
void srGERD::getEnvironmentScaleFactor(float& scale, float& inverse_scale)
{
    scale = environment.z;
    inverse_scale = environment.w;
}

// FUNCTION: SURRENDER 0x1001C8A0
void srGERD::setPolygonOffset(long offset)
{
    if (offset != polygon_offset) {
        flushImmediateRenderers();
        polygon_offset = offset;
        dirty |= DIRTY_POLYGON_OFFSET;
    }
}

// FUNCTION: SURRENDER 0x1001C8D0
long srGERD::getPolygonOffset() const
{
    return polygon_offset;
}

// FUNCTION: SURRENDER 0x1001CA30
void srGERD::setClearColor(const srVector4T<float>& color)
{
    clear_state.clear_values.color = color;
    float* clear = &clear_state.clear_values.color.x;
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
        clear_state.clear_values.depth = 0.0;
        return;
    }
    if (depth >= 1.0) {
        clear_state.clear_values.depth = 1.0;
        return;
    }
    clear_state.clear_values.depth = depth;
}

// FUNCTION: SURRENDER 0x1001CC10
void srGERD::drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count)
{
    if ((enable_flags.value & 4) == 0) {
        checkViewStateChanges();
        checkDrawStateChanges();
        if ((vertex_arrays_dirty & DIRTY_VERTEX_ARRAY_INFO) != 0) {
            getDD()->setVertexArrayInfo(&vertex_arrays);
            vertex_arrays_dirty &= ~DIRTY_VERTEX_ARRAY_INFO;
        }
        getDD()->drawArrays(primitive, first, count);
        statistics.draw_calls++;
    }
}

// FUNCTION: SURRENDER 0x1001CC90
void srGERD::drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                          srRendererDefs::e_indexType type, const void* indices)
{
    if ((enable_flags.value & 4) == 0) {
        checkViewStateChanges();
        checkDrawStateChanges();
        if ((vertex_arrays_dirty & DIRTY_VERTEX_ARRAY_INFO) != 0) {
            getDD()->setVertexArrayInfo(&vertex_arrays);
            vertex_arrays_dirty &= ~DIRTY_VERTEX_ARRAY_INFO;
        }
        getDD()->drawElements(primitive, count, type, indices);
        statistics.draw_calls++;
    }
}

// FUNCTION: SURRENDER 0x1001CEC0
unsigned long srGERD::getVertexProcessorCount() const
{
    return vertex_processors.count;
}

// FUNCTION: SURRENDER 0x1001CED0
void srGERD::getVertexProcessors(srVertexProcessor** processors) const
{
    if (processors != 0) {
        unsigned long count = vertex_processors.count;
        for (unsigned long i = 0; i < count; i++) {
            processors[i] = vertex_processors.data[i];
        }
    }
}

// FUNCTION: SURRENDER 0x1001CFC0
void srGERD::setError(e_error error)
{
    last_error = error;
}

// FUNCTION: SURRENDER 0x1001CF80
srGERD* srGERD::getNext() const
{
    return next;
}

// FUNCTION: SURRENDER 0x1001CF50
srGERD* srGERD::getFirstOpen()
{
    return firstOpen;
}

// FUNCTION: SURRENDER 0x1001CF90
srGERD* srGERD::getNextOpen() const
{
    return next_open;
}

// FUNCTION: SURRENDER 0x10018330
void srGERD::invalidateTexture(srTextureIFace* texture)
{
    srCriticalSection* section = state_section;
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
    if (candidate != 0 && candidate != texture_default) {
        markTextureAsDeleted(texture);
    }
}

// FUNCTION: SURRENDER 0x10028050
void srGERD::invalidateResidentTexture(Texture& texture)
{
    if (texture.device.resident_data != 0) {
        if (texture.surface_data == 0 || srThread::getHandle() != owner_thread) {
            invalidateTexture(texture);
        } else {
            getDD()->deleteTexture(texture.device);
            texture.device.resident_data = 0;
            texture.device.resident_size = 0;
            for (unsigned long stage = 0; stage < device.info.max_texture_stages;
                 ++stage) {
                if (texture_slots[stage] == &texture) {
                    texture_slots[stage] = 0;
                }
            }
        }
        dirty |= DIRTY_TEXTURE0;
        dirty |= DIRTY_TEXTURE1;
    }
}

// FUNCTION: SURRENDER 0x100280F0
void srGERD::resetCurrentTexPointers()
{
    for (unsigned long stage = 0; stage < device.info.max_texture_stages; ++stage) {
        texture_iface[stage] = 0;
        texture_slots[stage] = 0;
    }
    dirty |= DIRTY_TEXTURE0;
    dirty |= DIRTY_TEXTURE1;
}

// FUNCTION: SURRENDER 0x10028150
void srGERD::markTextureAsDeleted(Texture& texture)
{
    if (texture.device.deleted == 0) {
        if (texture.prev != 0) {
            texture.prev->next = texture.next;
        }
        if (texture.next != 0) {
            texture.next->prev = texture.prev;
        }
        if (&texture == texture_head) {
            texture_head = texture.next;
        }
        texture.prev = 0;
        Texture* previous = texture_deleted;
        texture.next = previous;
        if (previous != 0) {
            previous->prev = &texture;
        }
        texture_deleted = &texture;
        texture.device.deleted = 1;
    }
}

// FUNCTION: SURRENDER 0x10018390
void srGERD::invalidateTextureByFrameHandle(unsigned long handle)
{
    srCriticalSectionAccess access(state_section);
    if (handle != 0 && texture_hash_enabled) {
        Texture* texture = texture_lookup.Lookup(&handle);
        if (texture != 0) {
            invalidateTexture(*texture);
        }
    }
}

// FUNCTION: SURRENDER 0x10017BC0
void srGERD::invalidateResidentTextures()
{
    flushImmediateRenderers();
    srCriticalSectionAccess access(state_section);
    Texture* texture = texture_head;
    while (texture != 0) {
        Texture* next = texture->next;
        invalidateResidentTexture(*texture);
        texture = next;
    }
    resetCurrentTexPointers();
}

// FUNCTION: SURRENDER 0x10017C40
void srGERD::invalidateResidentTexture(srTextureIFace* texture)
{
    flushImmediateRenderers();
    srCriticalSectionAccess access(state_section);
    if (texture != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        Texture* found = texture_lookup.Lookup(&handle);
        if (found != 0) {
            invalidateResidentTexture(*found);
        }
    }
}

// FUNCTION: SURRENDER 0x10017EC0
void srGERD::setTextureCacheSize(unsigned long bytes)
{
    srCriticalSectionAccess access(state_section);
    texture_cache_size = bytes;
    if (bytes != 0 && bytes < texture_cache_used) {
        releaseTextureMemory(texture_cache_used - bytes);
    }
}

// FUNCTION: SURRENDER 0x100180A0
void srGERD::setTextureSubImage(srTextureIFace* texture, long mipmap, long x, long y, long width,
                                long height)
{
    srCriticalSectionAccess access(state_section);
    if (isWindowOpen() == 0) {
        return;
    }
    unsigned long handle = texture->getTextureFrameHandle();
    Texture* resident = texture_lookup.Lookup(&handle);
    if (resident != 0 && resident->device.first_level <= (unsigned long)mipmap &&
        (unsigned long)mipmap <= resident->device.last_level) {
        srTextureIFace::PartialRequest request;
        request.width = resident->device.width;
        request.height = resident->device.height;
        request.mipmap_level = mipmap;
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
            (unsigned long)((char)mipmap - (char)resident->device.first_level);
        unsigned long level_width = request.width >> (shift & 0x1f);
        if (level_width == 0) {
            level_width = 1;
        }
        unsigned long level_height = request.height >> (shift & 0x1f);
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
            static_cast<unsigned long>(resident->pixel_format.pixel_size) + 1;
        unsigned long pitch = bytes_per_pixel * level_width;
        if (resident->surface_data == 0) {
            void* staging =
                srCore.getGlobalRecycler()->allocate(bytes_per_pixel * level_height * level_width);
            request.destination = new srColorSurface(resident->pixel_format, staging,
                                                     level_width, level_height, pitch);
            texture->getMipmapLevelPartial(request);
            request.destination->release();
            resident->device.levels[mipmap] = staging;
            getDD()->texSubImage(resident->device, mipmap, request.destination_x,
                                 request.destination_y, request.source_right,
                                 request.source_bottom);
            resident->device.levels[mipmap] = 0;
            srCore.getGlobalRecycler()->free(staging);
        } else {
            request.destination =
                new srColorSurface(resident->pixel_format, resident->device.levels[mipmap],
                                   level_width, level_height, pitch);
            texture->getMipmapLevelPartial(request);
            request.destination->release();
            getDD()->texSubImage(resident->device, mipmap, request.destination_x,
                                 request.destination_y, request.source_right,
                                 request.source_bottom);
        }
    }
}

// FUNCTION: SURRENDER 0x10018480
void srGERD::invalidateTextureCache()
{
    srCriticalSectionAccess access(state_section);
    if (texture_hash_enabled) {
        Texture* texture = texture_head;
        while (texture != 0) {
            Texture* next = texture->next;
            invalidateTexture(*texture);
            texture = next;
        }
        resetCurrentTexPointers();
        texture_sequence = 0;
    }
}

// FUNCTION: SURRENDER 0x10018500
unsigned long srGERD::getResidentTextureMemUsed() const
{
    srCriticalSection* section = state_section;
    section->getAccess();
    unsigned long used = 0;
    for (Texture* texture = texture_head; texture != 0; texture = texture->next) {
        if (texture->device.resident_data != 0 && texture->device.resident_size > 0) {
            used += texture->device.resident_size;
        }
    }
    section->releaseAccess();
    return used;
}

// FUNCTION: SURRENDER 0x10023550
void srGERD::matrixMode(e_matrixMode mode)
{
    state.matrix_mode = mode;
}

// FUNCTION: SURRENDER 0x100235B0
void srGERD::pushMatrix()
{
    MatrixStack& stack = state.matrix_stacks[state.matrix_mode];
    if (stack.depth < 0x20) {
        stack.stack[stack.depth] =
            state.matrix_current[state.matrix_mode];
        stack.depth++;
    }
}

// FUNCTION: SURRENDER 0x10023560
void srGERD::popMatrix()
{
    srMatrix4T<float>* matrix = &state.matrix_current[state.matrix_mode];
    MatrixStack& stack = state.matrix_stacks[state.matrix_mode];
    if (stack.depth != 0) {
        stack.depth--;
        *matrix = stack.stack[stack.depth];
    }
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x100236D0
void srGERD::setMatrixDirty()
{
    if (state.matrix_mode == MATRIX_PROJECTION) {
        dirty |= DIRTY_CLIP_PLANES;
    }
    dirty |= 1 << (state.matrix_mode + DIRTY_MATRIX_SHIFT);
}

// FUNCTION: SURRENDER 0x1001D2D0
void srGERD::checkFrameStateChanges()
{
    if ((dirty & DIRTY_FRAME_STATE) != 0) {
        applyFrameStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001B450
void srGERD::applyFrameStateChanges()
{
    srDD::Update update;
    update.gamma = display.gamma;
    update.enabled = static_cast<unsigned long>(static_cast<char>(enable_flags.value) & 1);
    update.swap_interval = display.swap_interval;
    update.antialias = display.antialias;
    update.flags = 0;
    update.value_10 = 1.0f;
    if ((dirty & DIRTY_ANTIALIAS) != 0) {
        update.flags |= srDD::Update::UPDATE_ANTIALIAS;
    }
    if ((dirty & DIRTY_FRAME_ENABLE) != 0) {
        update.flags |= srDD::Update::UPDATE_ENABLE;
    }
    if ((dirty & DIRTY_GAMMA) != 0) {
        update.flags |= srDD::Update::UPDATE_GAMMA;
    }
    if ((dirty & DIRTY_SWAP_INTERVAL) != 0) {
        update.flags |= srDD::Update::UPDATE_SWAP_INTERVAL;
    }
    getDD()->update(update);
    dirty &= ~DIRTY_FRAME_STATE;
    statistics.frame_state_count++;
}

// FUNCTION: SURRENDER 0x10019A40
void srGERD::flushRenderers()
{
    if (srThread::getHandle() == owner_thread) {
        flushImmediateRenderers();
        flushSort();
        srCriticalSectionAccess access(renderers_section);
        for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
            while (entry->busy != 0) {
                srThread::yield(0);
            }
            entry->renderer->reset(0);
        }
    }
}

// FUNCTION: SURRENDER 0x10019AD0
void srGERD::flushSort()
{
    if (srThread::getHandle() == owner_thread) {
        srCriticalSectionAccess access(renderers_section);
        for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
            if (entry->renderer->sorted == 1) {
                while (entry->busy != 0) {
                    srThread::yield(0);
                }
                entry->renderer->submit();
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10019B60
void srGERD::flushImmediateRenderers()
{
    if (srThread::getHandle() == owner_thread) {
        srCriticalSectionAccess access(renderers_section);
        for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
            if (entry->renderer->sorted == 0) {
                while (entry->busy != 0) {
                    srThread::yield(0);
                }
                entry->renderer->submit();
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10019BF0
srGERD::RendererEntry* srGERD::createRenderer(int sorted)
{
    srCriticalSectionAccess access(renderers_section);
    Renderer::Parameters parameters;
    parameters.gerd = this;
    parameters.sorted = sorted != 0;
    parameters.batch_limit = device.info.renderer_batch_limit;
    parameters.texture_stages = device.info.max_texture_stages;
    RendererEntry* entry = new RendererEntry;
    entry->renderer = new Renderer(parameters);
    entry->busy = 0;
    entry->prev = 0;
    entry->next = renderers;
    if (renderers != 0) {
        renderers->prev = entry;
    }
    renderers = entry;
    return entry;
}

// FUNCTION: SURRENDER 0x10019CC0
srGERD::Renderer* srGERD::lockRenderer()
{
    int sorted = 0;
    if ((enable_flags.value & (1UL << ENABLE_SORTED_RENDERING)) != 0) {
        sorted = 1;
    }
    for (;;) {
        srCriticalSectionAccess access(renderers_section);
        flushNonBusyRenderers();
        for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
            if (entry->busy == 0 && entry->renderer->sorted == sorted) {
                return _lockRenderer(entry);
            }
        }
        if (sorted == 0) {
            return _lockRenderer(createRenderer((enable_flags.value >> 1) & 1));
        }
    }
}

// FUNCTION: SURRENDER 0x10019D70
srGERD::Renderer* srGERD::_lockRenderer(RendererEntry* entry)
{
    entry->busy = 1;
    return entry->renderer;
}

// FUNCTION: SURRENDER 0x10019D90
void srGERD::unlockRenderer(Renderer* renderer, int submit)
{
    srCriticalSectionAccess access(renderers_section);
    for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
        if (entry->renderer == renderer) {
            entry->busy = 0;
            if (srThread::getHandle() == owner_thread &&
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
    if (srThread::getHandle() == owner_thread) {
        srCriticalSectionAccess access(renderers_section);
        for (RendererEntry* entry = renderers; entry != 0; entry = entry->next) {
            if (entry->busy == 0 && entry->renderer->isBatchFull() != 0) {
                entry->renderer->submit();
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1001A790
srGERD::e_error srGERD::beginFrame()
{
    if ((state_flags & STATE_CONTEXT_CREATED) == 0) {
        return ERROR_NO_CONTEXT;
    }
    if (isWindowOpen() == 0) {
        return ERROR_WINDOW_NOT_OPEN;
    }
    if (srWindow::isWindow(device.window) == 0) {
        return ERROR_INVALID_WHANDLE;
    }
    if ((state_flags & STATE_FRAME_STARTED) == 0) {
        if ((enable_flags.value & (1UL << ENABLE_AUTO_FLIP)) != 0 && (state_flags & STATE_FRAME_FLIPPED) == 0) {
            flipFrame();
        }
        checkFrameStateChanges();
        getDD()->beginFrame();
        state_flags |= STATE_FRAME_STARTED;
        state_flags &= ~STATE_FRAME_FLIPPED;
    }
    return ERROR_NONE;
}

// FUNCTION: SURRENDER 0x1001A810
void srGERD::endFrame()
{
    if (isWindowOpen() == 0) {
        setError(ERROR_WINDOW_NOT_OPEN);
        return;
    }
    if ((state_flags & STATE_FRAME_STARTED) != 0) {
        flushRenderers();
        getDD()->endFrame();
        state_flags &= ~STATE_FRAME_STARTED;
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
        setError(ERROR_WINDOW_NOT_OPEN);
        return;
    }
    if ((state_flags & STATE_FRAME_FLIPPED) != 0) {
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
    statistics.frames += 1;
    state_flags |= STATE_FRAME_FLIPPED;
    getStatistics(frame_statistics);
}

// FUNCTION: SURRENDER 0x1001AAB0
void srGERD::flush()
{
    if (isWindowOpen() == 0) {
        setError(ERROR_WINDOW_NOT_OPEN);
        return;
    }
    checkAllStateChanges();
    flushImmediateRenderers();
    getDD()->flushFrame();
}

// FUNCTION: SURRENDER 0x1001CD20
int srGERD::isContextCreated() const
{
    return state_flags & STATE_CONTEXT_CREATED;
}

// FUNCTION: SURRENDER 0x1001D020
long srGERD::getWidth() const
{
    return device.open_info.width;
}

// FUNCTION: SURRENDER 0x1001D0A0
long srGERD::getHeight() const
{
    return device.open_info.height;
}

// FUNCTION: SURRENDER 0x1001D0B0
unsigned long srGERD::getWindowHandle() const
{
    return device.window;
}

// FUNCTION: SURRENDER 0x1001D0D0
int srGERD::isWindowOpen() const
{
    return (state_flags >> 1) & 1;
}

// FUNCTION: SURRENDER 0x1001D0E0
int srGERD::isFullScreen() const
{
    return device.open_info.display_mode >= 0;
}

// FUNCTION: SURRENDER 0x1001D030
void srGERD::getPixelFormat(srPixelConvert::PixelFormat& format) const
{
    if (isWindowOpen() != 0) {
        /* Default 8888 format the device getBufferPixelFormat refines. */
        srDD::PixelFormat device = {
            8, 0x10, 8, 8, 8, 0, 8, 0x18, srPixelConvert::COLOR_RGB, srPixelConvert::PIXEL_SIZE_32};
        getDD()->getBufferPixelFormat(device);
        convertPixelFormat(format, device);
    }
}

// FUNCTION: SURRENDER 0x1001D100
const char* srGERD::getDeviceName() const
{
    return device.info.text[0];
}

// FUNCTION: SURRENDER 0x1001D110
const char* srGERD::getDeviceVendor() const
{
    return device.info.text[1];
}

// FUNCTION: SURRENDER 0x1001D120
const char* srGERD::getDevicePlatform() const
{
    return device.info.text[2];
}

// FUNCTION: SURRENDER 0x1001D130
const char* srGERD::getDriverName() const
{
    const char* name = device.info.text[3];
    if (isContextCreated() == 0) {
        name = device.driver_info.name;
    }
    return name;
}

// FUNCTION: SURRENDER 0x1001D150
const char* srGERD::getDriverVendor() const
{
    return device.info.text[4];
}

// FUNCTION: SURRENDER 0x1001D160
const char* srGERD::getDriverVersion() const
{
    return device.info.text[5];
}

// FUNCTION: SURRENDER 0x1001D170
const char* srGERD::getHardwareChipset() const
{
    return device.info.text[6];
}

// FUNCTION: SURRENDER 0x1001D180
const char* srGERD::getHardwareName() const
{
    return device.info.text[7];
}

// FUNCTION: SURRENDER 0x1001D190
const char* srGERD::getHardwareVendor() const
{
    return device.info.text[8];
}

// FUNCTION: SURRENDER 0x1001D1A0
srGERD* srGERD::getFirst()
{
    return first;
}

// FUNCTION: SURRENDER 0x1001D1D0
srDD::e_hardwareID srGERD::getHardwareID() const
{
    return static_cast<srDD::e_hardwareID>(device.info.hardware_id);
}

// FUNCTION: SURRENDER 0x1001D1F0
void srGERD::pushEnable()
{
    unsigned long depth = environment_state.enable_depth;
    if (depth < 0x10) {
        environment_state.enable_depth = depth + 1;
        environment_state.enable_stack[depth] = enable_flags;
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
    state.view_top = y;
    state.view_left = x;
    state.view_bottom = y + height;
    state.view_right = x + width;
    if (state.view_left >= (unsigned long)getWidth()) {
        state.view_left = getWidth();
    }
    if (state.view_top >= (unsigned long)getHeight()) {
        state.view_top = getHeight();
    }
    if (state.view_right >= (unsigned long)getWidth()) {
        state.view_right = getWidth();
    }
    if (state.view_bottom >= (unsigned long)getHeight()) {
        state.view_bottom = getHeight();
    }
    dirty |= DIRTY_VIEWPORT;
}

// FUNCTION: SURRENDER 0x1001D630
void srGERD::performPickTest(const PickInput& input)
{
    unsigned long depth = this->pick.pick_depth;
    if (depth == 0) {
        return;
    }
    srVector4T<float>* vertices = pick_vertices.ensure(input.vertex_count);
    for (unsigned long index = 0; index < input.vertex_count; index++) {
        float inv_w = 1.0f / input.positions[index].w;
        vertices[index].w = inv_w < 0.0f ? -1.0f : 1.0f;
        inv_w = fabs(inv_w);
        vertices[index].x = input.positions[index].x * inv_w;
        vertices[index].y = input.positions[index].y * inv_w;
        vertices[index].z = input.positions[index].z * inv_w;
    }
    Pick* pick = this->pick.pick_stack;
    do {
        float pick_x = pick->position.x;
        float pick_y = pick->position.y;
        for (unsigned long index = 0; index < input.triangle_count; index++) {
            unsigned long triangle_index = input.indices[index];
            const srVector3i& triangle = input.triangles[triangle_index];
            const srVector4T<float>* corner0 = &vertices[input.vertices[triangle.x]];
            const srVector4T<float>* corner1 = &vertices[input.vertices[triangle.y]];
            const srVector4T<float>* corner2 = &vertices[input.vertices[triangle.z]];
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
            if (hit >= -1.0f && hit < pick->position.z) {
                pick->position.z = hit;
                /* reinterpret-ok: the public pick key arrives as ulong bits
                   naming the selected model instance. */
                pick->selected_model = reinterpret_cast<srModelInstance*>(this->pick.pick_key);
                pick->triangle_index = triangle_index;
            }
        }
        pick++;
        depth--;
    } while (depth != 0);
}

// FUNCTION: SURRENDER 0x1001DAA0
void srGERD::pushPick(const Pick& pick)
{
    if (this->pick.pick_depth < 0x20) {
        this->pick.pick_stack[this->pick.pick_depth] = pick;
        this->pick.pick_depth += 1;
    }
}

// FUNCTION: SURRENDER 0x1001DAE0
void srGERD::popPick(Pick& pick)
{
    if (this->pick.pick_depth != 0) {
        this->pick.pick_depth -= 1;
        pick = this->pick.pick_stack[this->pick.pick_depth];
    }
}

// FUNCTION: SURRENDER 0x1001DB10
void srGERD::setPickKey(unsigned long key)
{
    pick.pick_key = key;
}

// FUNCTION: SURRENDER 0x1001EE50
void srGERD::setExclusionMask(unsigned long mask)
{
    exclusion_mask = mask;
}

// FUNCTION: SURRENDER 0x1001EE60
unsigned long srGERD::getExclusionMask() const
{
    return exclusion_mask;
}

// FUNCTION: SURRENDER 0x100293E0
void srGERD::resetTexture()
{
    dirty |= DIRTY_TEXTURE0;
    dirty |= DIRTY_TEXTURE1;
}

// FUNCTION: SURRENDER 0x100213A0
srMatrix4T<float>::e_scaleType srGERD::getModelViewScaleType()
{
    checkViewStateChanges();
    return state.modelview_scale_type;
}

// FUNCTION: SURRENDER 0x100213C0
void srGERD::getNormalMatrix(srMatrix4T<float>& matrix)
{
    checkViewStateChanges();
    if ((enable_flags.value & (1UL << ENABLE_REVERSE_NORMALS)) != 0) {
        srVector4T<float> negated;
        negated.Set(-state.normal_matrix.vectors[0].x,
                    -state.normal_matrix.vectors[0].y,
                    -state.normal_matrix.vectors[0].z,
                    -state.normal_matrix.vectors[0].w);
        matrix.vectors[0] = negated;
        negated.Set(-state.normal_matrix.vectors[1].x,
                    -state.normal_matrix.vectors[1].y,
                    -state.normal_matrix.vectors[1].z,
                    -state.normal_matrix.vectors[1].w);
        matrix.vectors[1] = negated;
        negated.Set(-state.normal_matrix.vectors[2].x,
                    -state.normal_matrix.vectors[2].y,
                    -state.normal_matrix.vectors[2].z,
                    -state.normal_matrix.vectors[2].w);
        matrix.vectors[2] = negated;
        matrix.vectors[3] = state.normal_matrix.vectors[3];
        return;
    }
    matrix = state.normal_matrix;
}

// FUNCTION: SURRENDER 0x100214F0
void srGERD::getProjectClipNearMatrix(srMatrix4T<float>& matrix)
{
    checkViewStateChanges();
    matrix = state.project_clip_near;
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
    srMatrix4T<float>& matrix = state.matrix_current[state.matrix_mode];
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
        srVector3T<double> unit_axis;
        if (length_sq != 1.0) {
            double inverse = 1.0 / sqrt(length_sq);
            unit_axis = srVector3T<double>(axis.x * inverse, axis.y * inverse, axis.z * inverse);
        } else {
            unit_axis = axis;
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
        srMatrix4T<float>& matrix = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& matrix = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& matrix = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& matrix = state.matrix_current[state.matrix_mode];
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
    checkViewStateChanges();
    matrix = state.matrix_current[mode];
}

// FUNCTION: SURRENDER 0x10023510
void srGERD::getInverseModelViewMatrix(srMatrix4T<float>& matrix)
{
    checkViewStateChanges();
    matrix = state.inverse_modelview;
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
    return static_cast<short>(srFloatToInt(value * 32767.0f));
}

// FUNCTION: SURRENDER 0x1001F7F0
void srGERD::accumAlloc()
{
    if (getWidth() != 0) {
        if (getHeight() != 0) {
            accum_buffer =
                static_cast<AccumPixel*>(srHeap.allocate(getWidth() * getHeight() * 8));
            unsigned long* scratch = static_cast<unsigned long*>(srHeap.allocate(getWidth() * 4));
            if (scratch != 0) {
                accum_scratch = scratch;
                accumClear();
                return;
            }
            accum_scratch = 0;
            accumClear();
        }
    }
}

// FUNCTION: SURRENDER 0x1001F8D0
void srGERD::accumClear()
{
    if (accum_buffer == 0) {
        accumAlloc();
    }
    if (accum_buffer == 0) {
        return;
    }
    short first = accumConvert(clear_state.clear_values.accum.x);
    short second = accumConvert(clear_state.clear_values.accum.y);
    short third = accumConvert(clear_state.clear_values.accum.z);
    short fourth = accumConvert(clear_state.clear_values.accum.w);
    unsigned long left = state.scissor.left;
    unsigned long top = state.scissor.top;
    unsigned long width = state.scissor.right - left;
    unsigned long height = state.scissor.bottom - top;
    /* reinterpret-ok: 16-bit accumulation pixels are filled as dword lanes. */
    SRDWORD* row = reinterpret_cast<SRDWORD*>(accum_buffer) + (getWidth() * top + left) * 2;
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

// VTABLE: SURRENDER 0x10076728
// class srClassSupport<srGERD::LockSurface, srColorSurfaceIFace, 0, 12561>

// VTABLE: SURRENDER 0x100767F8
// class srGERD::LockSurface

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
    srGERD* gerd;
    unsigned long left;
    unsigned long top;
    unsigned long right;
    unsigned long bottom;
    srColorSurface* scratch;

public:
    /* Set by lockBuffer when the locked pixel format is 32-bit ARGB. */
    unsigned char argb32;

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
    description.pitch = (format.pixel_size + 1) * description.width;
    description.pixel_format = format;
    setSurfaceDesc(description);
    this->gerd = gerd;
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;
    /* One row tall and as wide as the longest scissor axis. */
    unsigned long side =
        (long)description.width < (long)description.height ? description.height : description.width;
    scratch = new srColorSurface(format, side, 1);
    argb32 = 0;
}

// FUNCTION: SURRENDER 0x1001F610
srGERD::LockSurface::~LockSurface()
{
    scratch->release();
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
    if (y < (long)top || y >= (long)bottom) {
        return;
    }
    if (x0 < (long)left) {
        x0 = left;
    }
    if (x1 > (long)right) {
        x1 = right;
    }
    if (x0 >= x1) {
        return;
    }
    /* Convert the pixel through the scratch surface, then hand the device a
       pointer to the converted value. */
    scratch->setPixel(0, 0, pixel);
    unsigned long converted = scratch->getPixelRaw(0, 0);
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 4;
    command.data = &converted;
    command.x = x0;
    command.y = y;
    command.count = x1 - x0;
    gerd->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020990
void srGERD::LockSurface::getPixelRow(unsigned long* pixels, long y, long x0, long x1)
{
    if (y < (long)top || y >= (long)bottom) {
        return;
    }
    if (x0 < (long)left) {
        pixels += left - x0;
        x0 = left;
    }
    if (x1 > (long)right) {
        x1 = right;
    }
    if (x0 >= x1) {
        return;
    }
    long count = x1 - x0;
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 2;
    command.x = x0;
    command.y = y;
    command.count = count;
    if (argb32 != 0) {
        command.data = pixels;
        gerd->getDD()->bufferOp(command);
        return;
    }
    command.data = scratch->getDataPtr();
    gerd->getDD()->bufferOp(command);
    scratch->getPixelRow(pixels, 0, 0, count);
}

// FUNCTION: SURRENDER 0x10020840
void srGERD::LockSurface::setPixelRow(const unsigned long* pixels, long y, long x0, long x1)
{
    if (y < (long)top || y >= (long)bottom) {
        return;
    }
    if (x0 < (long)left) {
        pixels += left - x0;
        x0 = left;
    }
    if (x1 > (long)right) {
        x1 = right;
    }
    if (x0 >= x1) {
        return;
    }
    long count = x1 - x0;
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 3;
    command.x = x0;
    command.y = y;
    command.count = count;
    if (argb32 != 0) {
        command.data = (void*)pixels;
        gerd->getDD()->bufferOp(command);
        return;
    }
    scratch->setPixelRow(pixels, 0, 0, count);
    command.data = scratch->getDataPtr();
    gerd->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020A70
void srGERD::LockSurface::getPixelRowRaw(void* pixels, long y, long x0, long x1)
{
    if (y < (long)top || y >= (long)bottom) {
        return;
    }
    if (x0 < (long)left) {
        /* Retail advances the raw pointer by one byte per clipped pixel. */
        pixels = (char*)pixels + (left - x0);
        x0 = left;
    }
    if (x1 > (long)right) {
        x1 = right;
    }
    if (x0 >= x1) {
        return;
    }
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 2;
    command.data = pixels;
    command.x = x0;
    command.y = y;
    command.count = x1 - x0;
    gerd->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020900
void srGERD::LockSurface::setPixelRowRaw(const void* pixels, long y, long x0, long x1)
{
    if (y < (long)top || y >= (long)bottom) {
        return;
    }
    if (x0 < (long)left) {
        pixels = static_cast<const char*>(pixels) + (pixel_format.pixel_size + 1) * (left - x0);
        x0 = left;
    }
    if (x1 > (long)right) {
        x1 = right;
    }
    if (x0 >= x1) {
        return;
    }
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 3;
    command.data = (void*)pixels;
    command.x = x0;
    command.y = y;
    command.count = x1 - x0;
    gerd->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020BB0
void srGERD::LockSurface::getPixelColumn(unsigned long* pixels, long x, long y0, long y1)
{
    if (x < (long)left || x >= (long)right) {
        return;
    }
    if (y0 < (long)top) {
        pixels += top - y0;
        y0 = top;
    }
    if (y1 >= (long)bottom) {
        y1 = bottom;
    }
    if (y0 >= y1) {
        return;
    }
    long count = y1 - y0;
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 6;
    command.x = x;
    command.y = y0;
    command.count = count;
    if (argb32 != 0) {
        command.data = pixels;
        gerd->getDD()->bufferOp(command);
        return;
    }
    command.data = scratch->getDataPtr();
    gerd->getDD()->bufferOp(command);
    scratch->getPixelRow(pixels, 0, 0, count);
}

// FUNCTION: SURRENDER 0x10020AF0
void srGERD::LockSurface::setPixelColumn(const unsigned long* pixels, long x, long y0, long y1)
{
    if (x < (long)left || x >= (long)right) {
        return;
    }
    if (y0 < (long)top) {
        pixels += top - y0;
        y0 = top;
    }
    if (y1 >= (long)bottom) {
        y1 = bottom;
    }
    if (y0 >= y1) {
        return;
    }
    long count = y1 - y0;
    srDD::BufferCommand command;
    command.flags = 0;
    command.opcode = 7;
    command.x = x;
    command.y = y0;
    command.count = count;
    if (argb32 != 0) {
        command.data = (void*)pixels;
        gerd->getDD()->bufferOp(command);
        return;
    }
    scratch->setPixelRow(pixels, 0, 0, count);
    command.data = scratch->getDataPtr();
    gerd->getDD()->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10020780
void srGERD::LockSurface::setScissor(unsigned long left, unsigned long top, unsigned long right,
                                     unsigned long bottom)
{
    this->left = left;
    this->right = right;
    this->top = top;
    this->bottom = bottom;
}

// FUNCTION: SURRENDER 0x10020C90
srDD::e_error srGERD::_lockBuffer()
{
    if (buffer_lock_count == 0) {
        flushImmediateRenderers();
        getDD()->flushFrame();
        srDD::BufferCommand command;
        command.flags = 0;
        command.opcode = 0;
        srDD::e_error error = getDD()->bufferOp(command);
        if (error != 0) {
            return error;
        }
    }
    buffer_lock_count += 1;
    return srDD::ERROR_NONE;
}

// FUNCTION: SURRENDER 0x10020CF0
srDD::e_error srGERD::_unlockBuffer()
{
    if (buffer_lock_count != 0) {
        buffer_lock_count -= 1;
        if (buffer_lock_count == 0) {
            /* Retail's command.flags ends up holding the decremented lock
               count (the decrement temporary shares that slot). */
            srDD::BufferCommand command;
            command.flags = buffer_lock_count;
            command.opcode = 1;
            return getDD()->bufferOp(command);
        }
    }
    return srDD::ERROR_NONE;
}

// FUNCTION: SURRENDER 0x10020D30
void srGERD::clear(const srFlags<e_buffer>& buffers)
{
    if (isWindowOpen() == 0) {
        setError(ERROR_WINDOW_NOT_OPEN);
        return;
    }
    if (buffers.value != 0) {
        checkViewStateChanges();
        /* GERD buffer bits 0,1,3 map to DD bits 0,1,2; GERD bit 2 is the
           software accumulation buffer serviced by accumClear. */
        srFlags<srDD::e_buffer> device_buffers;
        device_buffers.value = (buffers.value & BUFFER_COLOR) != 0;
        if ((buffers.value & BUFFER_DEPTH) != 0) {
            device_buffers.value |= srDD::BUFFER_DEPTH;
        }
        if ((buffers.value & BUFFER_STENCIL) != 0) {
            device_buffers.value |= srDD::BUFFER_STENCIL;
        }
        if (device_buffers.value != 0) {
            getDD()->setClearValues(clear_state.clear_values);
            getDD()->clearBuffers(device_buffers);
        }
        if ((buffers.value & BUFFER_ACCUM) != 0) {
            accumClear();
        }
    }
}

// FUNCTION: SURRENDER 0x1001BC70
void srGERD::applyViewStateChanges()
{
    unsigned long dirty = this->dirty;
    statistics.view_state_applies += 1;
    if ((dirty & DIRTY_MODELVIEW) != 0) {
        classifyMatrix(MATRIX_MODELVIEW);
        dirty = this->dirty & ~DIRTY_MODELVIEW;
        this->dirty = dirty;
        if (dirty == 0) {
            return;
        }
    }
    if ((dirty & (DIRTY_VIEWPORT | DIRTY_DEPTH_RANGE)) != 0) {
        srDD::ViewPort viewport;
        viewport.x = state.view_left;
        viewport.y = state.view_top;
        viewport.width = state.view_right - viewport.x;
        viewport.height = state.view_bottom - viewport.y;
        memcpy(viewport.extra, &state.depth_min, sizeof(viewport.extra));
        if ((state_flags & STATE_CLOSING_WINDOW) == 0) {
            getDD()->setViewPort(viewport);
        }
    }
    if ((this->dirty & DIRTY_PROJECTION) != 0) {
        classifyMatrix(MATRIX_PROJECTION);
        state.project_clip_near = state.matrix_current[MATRIX_PROJECTION];
        srMatrix4T<float>& projection = state.matrix_current[MATRIX_PROJECTION];
        if (((projection.vectors[3].x != 0.0f) || (projection.vectors[3].y != 0.0f) ||
             (projection.vectors[3].z != 0.0f)) &&
            ((projection.vectors[3].w + projection.vectors[2].w != 0.0f) &&
             (fabs((projection.vectors[3].z + projection.vectors[2].z) /
                   (projection.vectors[3].w + projection.vectors[2].w)) != 1.0))) {
            float scale = (float)fabs((projection.vectors[3].z + projection.vectors[2].z) /
                                      (projection.vectors[3].w + projection.vectors[2].w));
            state.project_clip_near.vectors[0].x *= scale;
            state.project_clip_near.vectors[0].y *= scale;
            state.project_clip_near.vectors[0].z *= scale;
            state.project_clip_near.vectors[0].w *= scale;
            state.project_clip_near.vectors[1].x *= scale;
            state.project_clip_near.vectors[1].y *= scale;
            state.project_clip_near.vectors[1].z *= scale;
            state.project_clip_near.vectors[1].w *= scale;
            state.project_clip_near.vectors[2].x *= scale;
            state.project_clip_near.vectors[2].y *= scale;
            state.project_clip_near.vectors[2].z *= scale;
            state.project_clip_near.vectors[2].w *= scale;
            state.project_clip_near.vectors[3].x *= scale;
            state.project_clip_near.vectors[3].y *= scale;
            state.project_clip_near.vectors[3].z *= scale;
            state.project_clip_near.vectors[3].w *= scale;
        }
        if ((state_flags & STATE_CLOSING_WINDOW) == 0) {
            getDD()->setProjectionMatrix(state.project_clip_near,
                                         state.matrix_class[MATRIX_PROJECTION]);
        }
    }
    if ((this->dirty & DIRTY_SCISSOR) != 0) {
        recalcScissor();
    }
    this->dirty &= ~DIRTY_VIEW_STATE;
}

// FUNCTION: SURRENDER 0x1001D580
void srGERD::setScissor(unsigned long x, unsigned long y, unsigned long width, unsigned long height)
{
    state.scissor.left = x;
    state.scissor.right = x + width;
    state.scissor.top = y;
    state.scissor.bottom = y + height;
    if (state.scissor.left >= (unsigned long)getWidth()) {
        state.scissor.left = getWidth();
    }
    if (state.scissor.top >= (unsigned long)getHeight()) {
        state.scissor.top = getHeight();
    }
    if (state.scissor.right >= (unsigned long)getWidth()) {
        state.scissor.right = getWidth();
    }
    if (state.scissor.bottom >= (unsigned long)getHeight()) {
        state.scissor.bottom = getHeight();
    }
    recalcScissor();
    dirty |= DIRTY_SCISSOR;
}

// FUNCTION: SURRENDER 0x100204C0
void srGERD::recalcScissor()
{
    if (state.scissor.left == 0 &&
        state.scissor.right == (unsigned long)getWidth() &&
        state.scissor.top == 0 &&
        state.scissor.bottom == (unsigned long)getHeight()) {
        state.scissor_flags |= 2;
    } else {
        state.scissor_flags &= ~2UL;
    }
    getDD()->setScissor(state.scissor);
    if (lock_surface != 0) {
        lock_surface->setScissor(state.scissor.left, state.scissor.top,
                                       state.scissor.right,
                                       state.scissor.bottom);
    }
}

// FUNCTION: SURRENDER 0x100215A0
void srGERD::classifyMatrix(e_matrixMode mode)
{
    statistics.matrix_classifications += 1;
    if (mode == MATRIX_MODELVIEW) {
        srMatrix4T<float>* modelview = &state.matrix_current[MATRIX_MODELVIEW];
        state.matrix_class[MATRIX_MODELVIEW] = srMatrix4T<float>::TYPE_GENERAL;
        srMatrix4T<float>* inverse = &state.inverse_modelview;
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
                state.modelview_scale_type = srMatrix4T<float>::SCALE_TYPE_UNIFORM;
                state.max_modelview_scale = (float)sqrt(length0);
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
                float scale = state.max_modelview_scale;
                state.normal_matrix.vectors[0].x = scale * inverse->vectors[0].x;
                state.normal_matrix.vectors[0].y = scale * inverse->vectors[1].x;
                state.normal_matrix.vectors[0].z = scale * inverse->vectors[2].x;
                state.normal_matrix.vectors[1].x = scale * inverse->vectors[0].y;
                state.normal_matrix.vectors[1].y = scale * inverse->vectors[1].y;
                state.normal_matrix.vectors[1].z = scale * inverse->vectors[2].y;
                state.normal_matrix.vectors[2].x = scale * inverse->vectors[0].z;
                state.normal_matrix.vectors[2].y = scale * inverse->vectors[1].z;
                state.normal_matrix.vectors[2].z = scale * inverse->vectors[2].z;
            } else {
                state.modelview_scale_type = srMatrix4T<float>::SCALE_TYPE_UNIT;
                state.max_modelview_scale = 1.0f;
                inverse->vectors[0].x = modelview->vectors[0].x;
                state.normal_matrix.vectors[0].x = modelview->vectors[0].x;
                inverse->vectors[1].x = modelview->vectors[0].y;
                state.normal_matrix.vectors[0].y = modelview->vectors[0].y;
                inverse->vectors[2].x = modelview->vectors[0].z;
                state.normal_matrix.vectors[0].z = modelview->vectors[0].z;
                inverse->vectors[0].y = modelview->vectors[1].x;
                state.normal_matrix.vectors[1].x = modelview->vectors[1].x;
                inverse->vectors[1].y = modelview->vectors[1].y;
                state.normal_matrix.vectors[1].y = modelview->vectors[1].y;
                inverse->vectors[2].y = modelview->vectors[1].z;
                state.normal_matrix.vectors[1].z = modelview->vectors[1].z;
                inverse->vectors[0].z = modelview->vectors[2].x;
                state.normal_matrix.vectors[2].x = modelview->vectors[2].x;
                inverse->vectors[1].z = modelview->vectors[2].y;
                state.normal_matrix.vectors[2].y = modelview->vectors[2].y;
                inverse->vectors[2].z = modelview->vectors[2].z;
                state.normal_matrix.vectors[2].z = modelview->vectors[2].z;
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
        state.modelview_scale_type = srMatrix4T<float>::SCALE_TYPE_NON_UNIFORM;
        state.max_modelview_scale = (float)sqrt(length0);
        inverse->Inverse(*modelview);
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
        adjugate.Inverse(normalized);
        state.normal_matrix.vectors[0].x = adjugate.vectors[0].x;
        state.normal_matrix.vectors[0].y = adjugate.vectors[1].x;
        state.normal_matrix.vectors[0].z = adjugate.vectors[2].x;
        state.normal_matrix.vectors[1].x = adjugate.vectors[0].y;
        state.normal_matrix.vectors[1].y = adjugate.vectors[1].y;
        state.normal_matrix.vectors[1].z = adjugate.vectors[2].y;
        state.normal_matrix.vectors[2].x = adjugate.vectors[0].z;
        state.normal_matrix.vectors[2].y = adjugate.vectors[1].z;
        state.normal_matrix.vectors[2].z = adjugate.vectors[2].z;
        return;
    }
    srMatrix4T<float>& matrix = state.matrix_current[mode];
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
    srMatrix4T<float>::e_type type;
    if (mask == 0x7bde && matrix.vectors[0].x == 1.0f && matrix.vectors[1].y == 1.0f &&
        matrix.vectors[2].z == 1.0f && matrix.vectors[3].w == 1.0f) {
        type = srMatrix4T<float>::TYPE_IDENTITY;
    } else if ((mask & 0xb39a) == 0xb39a) {
        type = srMatrix4T<float>::TYPE_PERSPECTIVE;
    } else if ((mask & 0x7356) == 0x7356) {
        type = srMatrix4T<float>::TYPE_ORTHOGRAPHIC;
    } else if ((mask & 0x7000) == 0x7000 && matrix.vectors[3].w == 1.0f) {
        type = srMatrix4T<float>::TYPE_AFFINE;
    } else {
        type = srMatrix4T<float>::TYPE_GENERAL;
    }
    state.matrix_class[mode] = type;
}

// FUNCTION: SURRENDER 0x1001CF40
void srGERD::assertContext() const {}

// FUNCTION: SURRENDER 0x1001BB10
void srGERD::checkViewStateChanges()
{
    if ((dirty & DIRTY_VIEW_STATE) != 0) {
        applyViewStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001BB30
void srGERD::checkClipPlaneChanges()
{
    if ((dirty & DIRTY_CLIP_PLANES) != 0) {
        applyClipPlaneChanges();
    }
}

// FUNCTION: SURRENDER 0x1001BB20
void srGERD::checkDrawStateChanges()
{
    if ((dirty & DIRTY_DRAW_STATE) != 0) {
        applyDrawStateChanges();
    }
}

// FUNCTION: SURRENDER 0x1001D2A0
void srGERD::checkAllStateChanges()
{
    if (dirty != 0) {
        checkDrawStateChanges();
        checkFrameStateChanges();
        checkViewStateChanges();
    }
}

// FUNCTION: SURRENDER 0x10023540
srGERD::e_matrixMode srGERD::getMatrixMode() const
{
    return state.matrix_mode;
}

// FUNCTION: SURRENDER 0x10023350
void srGERD::getMatrix(srMatrix4T<float>& matrix)
{
    checkViewStateChanges();
    matrix = state.matrix_current[state.matrix_mode];
}

// FUNCTION: SURRENDER 0x10023390
void srGERD::getMatrix(e_matrixMode mode, srMatrix4T<double>& matrix)
{
    checkViewStateChanges();
    const srMatrix4T<float>& current = state.matrix_current[mode];
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
    checkViewStateChanges();
    const srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    checkViewStateChanges();
    return state.max_modelview_scale;
}

// FUNCTION: SURRENDER 0x10021D20
void srGERD::pushMultMatrix(const srMatrix4x3T<float>& matrix)
{
    pushMatrix();
    srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    state.matrix_current[state.matrix_mode] = converted;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022B20
void srGERD::loadMatrix(const srMatrix4T<float>& matrix)
{
    state.matrix_current[state.matrix_mode] = matrix;
    setMatrixDirty();
}

// FUNCTION: SURRENDER 0x10022B50
void srGERD::loadMatrix(const srMatrix3T<double>& matrix)
{
    srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
    current.Set(matrix, srVector3T<float>(0.0f, 0.0f, 0.0f));
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
        srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
        srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
        srMatrix4T<float>& current = state.matrix_current[state.matrix_mode];
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
    const srMatrix4T<float>& projection = state.matrix_current[MATRIX_PROJECTION];
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
    state.clip_planes[0].Set(slope, 0.0f, near_left, 0.0f);
    state.clip_planes[1].Set(
        -slope, 0.0f,
        -(((projection.vectors[0].w - 1.0f) * projection.vectors[3].w) / bottom +
          ((projection.vectors[0].z + 1.0f) * projection.vectors[3].z) /
              (1.0f - projection.vectors[0].z)) *
            near_left,
        0.0f);
    state.clip_planes[2].Set(
        0.0f, -slope,
        -(((projection.vectors[1].w - 1.0f) * projection.vectors[3].w) / top +
          ((projection.vectors[1].z + 1.0f) * projection.vectors[3].z) /
              (1.0f - projection.vectors[1].z)) *
            near_top,
        0.0f);
    state.clip_planes[3].Set(0.0f, slope, near_top, 0.0f);
    state.clip_planes[4].Set(0.0f, 0.0f, -1.0f, slope);
    state.clip_planes[5].Set(0.0f, 0.0f, 1.0f,
                                        (projection.vectors[2].w - projection.vectors[3].w) /
                                            (projection.vectors[2].z - projection.vectors[3].z));
    for (int plane = 0; plane != 4; ++plane) {
        float length =
            sqrt(state.clip_planes[plane].z * state.clip_planes[plane].z +
                 state.clip_planes[plane].y * state.clip_planes[plane].y +
                 state.clip_planes[plane].w * state.clip_planes[plane].w +
                 state.clip_planes[plane].x * state.clip_planes[plane].x);
        if (length != 0.0f) {
            float inverse_length = 1.0f / length;
            state.clip_planes[plane].x *= inverse_length;
            state.clip_planes[plane].y *= inverse_length;
            state.clip_planes[plane].z *= inverse_length;
            state.clip_planes[plane].w *= inverse_length;
        }
    }
    dirty &= ~DIRTY_CLIP_PLANES;
}

// FUNCTION: SURRENDER 0x1001D4E0
void srGERD::getScissor(unsigned long& x, unsigned long& y, unsigned long& width,
                        unsigned long& height) const
{
    x = state.scissor.left;
    y = state.scissor.top;
    width = state.scissor.right - state.scissor.left;
    height = state.scissor.bottom - state.scissor.top;
}

// FUNCTION: SURRENDER 0x1001D530
void srGERD::getViewPort(unsigned long& x, unsigned long& y, unsigned long& width,
                         unsigned long& height) const
{
    x = state.view_left;
    y = state.view_top;
    width = state.view_right - state.view_left;
    height = state.view_bottom - state.view_top;
}

// FUNCTION: SURRENDER 0x1001C010
void srGERD::pushClipPlane(const srVector4T<float>& plane, e_clipMode mode)
{
    if ((unsigned long)state.clip_plane_count < 0x1a) {
        checkClipPlaneChanges();
        checkViewStateChanges();
        unsigned long bit = 1UL << (state.clip_plane_count + 6);
        float inverse_length =
            1.0 / sqrt(plane.z * plane.z + plane.x * plane.x + plane.y * plane.y);
        float x = plane.x * inverse_length;
        float y = plane.y * inverse_length;
        float z = plane.z * inverse_length;
        float w = inverse_length * plane.w;
        srVector4T<float>& eye_plane =
            state.clip_planes[state.clip_plane_count + 6];
        eye_plane.x = x * state.inverse_modelview.vectors[0].x +
                      y * state.inverse_modelview.vectors[1].x +
                      z * state.inverse_modelview.vectors[2].x +
                      w * state.inverse_modelview.vectors[3].x;
        eye_plane.y = x * state.inverse_modelview.vectors[0].y +
                      y * state.inverse_modelview.vectors[1].y +
                      z * state.inverse_modelview.vectors[2].y +
                      w * state.inverse_modelview.vectors[3].y;
        eye_plane.z = x * state.inverse_modelview.vectors[0].z +
                      y * state.inverse_modelview.vectors[1].z +
                      z * state.inverse_modelview.vectors[2].z +
                      w * state.inverse_modelview.vectors[3].z;
        eye_plane.w = x * state.inverse_modelview.vectors[0].w +
                      y * state.inverse_modelview.vectors[1].w +
                      z * state.inverse_modelview.vectors[2].w +
                      w * state.inverse_modelview.vectors[3].w;
        state.clip_modes[state.clip_plane_count] =
            static_cast<unsigned char>(mode);
        state.clip_mask |= bit;
        if (mode == 1) {
            state.clip_mode1_mask |= bit;
        } else {
            state.clip_mode1_mask &= ~bit;
        }
    }
    // Retail increments even when the plane limit skips insertion.
    state.clip_plane_count += 1;
}

// FUNCTION: SURRENDER 0x1001C1F0
void srGERD::popClipPlane()
{
    if (state.clip_plane_count != 0) {
        state.clip_plane_count -= 1;
        unsigned long mask = ~(1UL << (state.clip_plane_count + 6));
        state.clip_mask &= mask;
        state.clip_mode1_mask &= mask;
    }
}

// FUNCTION: SURRENDER 0x1001C230
void srGERD::getClipPlanes(ClipPlanes& planes)
{
    checkClipPlaneChanges();
    for (int plane = 0; plane != 6; ++plane) {
        planes.planes[plane] = state.clip_planes[plane];
    }
    if ((state.clip_mask & 0xffffffc0) != 0) {
        for (unsigned long index = 6; index < 0x20; ++index) {
            if ((state.clip_mask & (1UL << index)) != 0) {
                planes.planes[index] = state.clip_planes[index];
            }
        }
    }
    planes.mask = state.clip_mask;
    planes.mode1_mask = state.clip_mode1_mask;
}

// FUNCTION: SURRENDER 0x1001C5C0
void srGERD::pushEnvironment()
{
    if (environment_state.environment_depth < 0x10) {
        environment_state
            .environment_stack[environment_state.environment_depth] =
            environment;
        environment_state.environment_depth += 1;
    }
}

// FUNCTION: SURRENDER 0x1001C600
void srGERD::popEnvironment()
{
    if (environment_state.environment_depth != 0) {
        environment_state.environment_depth -= 1;
        environment =
            environment_state
                .environment_stack[environment_state.environment_depth];
    }
}

// FUNCTION: SURRENDER 0x1001C4D0
void srGERD::setEnvironmentRange(float minimum, float maximum)
{
    environment.x = minimum;
    environment.y = maximum;
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
    environment.z = scale;
    if (0.0f < inverse_scale) {
        if (inverse_scale < 1.0f) {
            environment.w = inverse_scale;
            return;
        }
        environment.w = 1.0f;
        return;
    }
    environment.w = 0.0f;
}

// FUNCTION: SURRENDER 0x1001CE00
void srGERD::pushVertexProcessor(srVertexProcessor& processor)
{
    unsigned long count = vertex_processors.count;
    vertex_processors[count] = &processor;
    vertex_processors.count += 1;
}

// FUNCTION: SURRENDER 0x1001CEA0
void srGERD::popVertexProcessor()
{
    if (vertex_processors.count > 0) {
        vertex_processors.count--;
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
    if (clamped.x != fog_color.x || clamped.y != fog_color.y || clamped.z != fog_color.z ||
        clamped.w != fog_color.w) {
        flushImmediateRenderers();
        fog_color = clamped;
        dirty |= DIRTY_FOG_COLOR;
    }
}

// FUNCTION: SURRENDER 0x1001C680
void srGERD::getFogColor(srVector4T<float>& color) const
{
    color = fog_color;
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
    return pick.pick_key;
}

// FUNCTION: SURRENDER 0x1001DB30
srGERD::e_visibility srGERD::testBoundingSphere(const srVector3T<float>& center, float radius)
{
    statistics.sphere_tests++;
    checkViewStateChanges();
    checkClipPlaneChanges();
    const srMatrix4T<float>& modelview = state.matrix_current[MATRIX_MODELVIEW];
    float eye_z = modelview.vectors[2].z * center.z + modelview.vectors[2].y * center.y +
                  modelview.vectors[2].x * center.x + modelview.vectors[2].w;
    float negative_radius = -(radius * state.max_modelview_scale);
    if (eye_z * state.clip_planes[4].z + state.clip_planes[4].w <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    if (eye_z * state.clip_planes[5].z + state.clip_planes[5].w <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    float eye_x = modelview.vectors[0].x * center.x + modelview.vectors[0].z * center.z +
                  modelview.vectors[0].y * center.y + modelview.vectors[0].w;
    if (eye_x * state.clip_planes[0].x + eye_z * state.clip_planes[0].z <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    if (eye_z * state.clip_planes[1].z + eye_x * state.clip_planes[1].x <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    float eye_y = modelview.vectors[1].z * center.z + modelview.vectors[1].y * center.y +
                  modelview.vectors[1].x * center.x + modelview.vectors[1].w;
    if (eye_z * state.clip_planes[2].z + eye_y * state.clip_planes[2].y <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    if (eye_z * state.clip_planes[3].z + eye_y * state.clip_planes[3].y <=
        negative_radius) {
        return VISIBILITY_OUTSIDE;
    }
    unsigned long remaining = state.clip_mask & 0xffffffc0;
    if (remaining != 0) {
        for (unsigned long index = 6; index < 0x20; ++index) {
            if (remaining == 0) {
                break;
            }
            unsigned long bit = 1UL << index;
            if ((remaining & bit) != 0) {
                const srVector4T<float>& plane = state.clip_planes[index];
                if (eye_z * plane.z + eye_x * plane.x + eye_y * plane.y + plane.w <=
                    negative_radius) {
                    return VISIBILITY_OUTSIDE;
                }
                remaining &= ~bit;
            }
        }
    }
    statistics.sphere_visible++;
    return static_cast<e_visibility>(1);
}

// FUNCTION: SURRENDER 0x1001DD60
srGERD::e_visibility srGERD::testBoundingBox(const srVector3T<float>& minimum,
                                             const srVector3T<float>& maximum)
{
    statistics.box_tests++;
    checkViewStateChanges();
    srMatrix4T<float> combined = state.matrix_current[MATRIX_PROJECTION];
    combined.MultiplyBy(state.matrix_current[MATRIX_MODELVIEW]);
    if (srVectorProcessor::vp->_srTestBoundingBox(combined, minimum, maximum) != 0) {
        statistics.box_visible++;
        return static_cast<e_visibility>(1);
    }
    return VISIBILITY_OUTSIDE;
}

// FUNCTION: SURRENDER 0x100235F0
srVector4T<float> srGERD::getEyeSpaceLocation(const srVector3T<float>& object_location)
{
    checkViewStateChanges();
    return state.matrix_current[MATRIX_MODELVIEW].Transform(object_location);
}

// FUNCTION: SURRENDER 0x1001DDF0
void srGERD::getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                               const srVector3T<float>& object_center, float object_radius)
{
    checkViewStateChanges();
    const srMatrix4T<float>& modelview = state.matrix_current[MATRIX_MODELVIEW];
    center.x = modelview.vectors[0].x * object_center.x + modelview.vectors[0].y * object_center.y +
               modelview.vectors[0].z * object_center.z + modelview.vectors[0].w;
    center.z = modelview.vectors[2].y * object_center.y + modelview.vectors[2].z * object_center.z +
               modelview.vectors[2].x * object_center.x + modelview.vectors[2].w;
    center.y = modelview.vectors[1].y * object_center.y + modelview.vectors[1].z * object_center.z +
               modelview.vectors[1].x * object_center.x + modelview.vectors[1].w;
    radius = object_radius * state.max_modelview_scale;
}

// FUNCTION: SURRENDER 0x1001B570
void srGERD::applyDrawStateChanges()
{
    srCriticalSectionAccess access(state_section);
    if ((dirty & DIRTY_FOG_COLOR) != 0) {
        getDD()->setFogColor(fog_color);
    }
    if ((dirty & DIRTY_SHADER) != 0) {
        getDD()->setShader(shader);
        statistics.shader_sets++;
    }
    removeDeletedTextures();
    if ((dirty & DIRTY_TEXTURE0) != 0) {
        changeTexture(texture_iface[0], 0, 1);
    }
    if (((dirty & DIRTY_TEXTURE1) != 0) && (1 < device.info.max_texture_stages)) {
        changeTexture(texture_iface[1], 1, 1);
    }
    if ((dirty & DIRTY_CULLING) != 0) {
        switch (state.cull_mode) {
        case CULL_BACK:
            getDD()->setCullMode(state.winding != 0 ? srDD::CULL_FRONT : srDD::CULL_BACK);
            break;
        case CULL_FRONT:
            getDD()->setCullMode(state.winding == 0 ? srDD::CULL_FRONT : srDD::CULL_BACK);
            break;
        case CULL_NONE:
            getDD()->setCullMode(srDD::CULL_NONE);
            break;
        }
    }
    if ((dirty & DIRTY_POLYGON_MODE) != 0) {
        switch (polygon_mode) {
        case POLYGON_POINT:
            getDD()->setPolygonMode(srDD::POLYGON_POINT);
            break;
        case POLYGON_LINE:
            getDD()->setPolygonMode(srDD::POLYGON_LINE);
            break;
        case POLYGON_FILL:
            getDD()->setPolygonMode(srDD::POLYGON_FILL);
            break;
        }
    }
    if ((dirty & DIRTY_POLYGON_OFFSET) != 0) {
        getDD()->setPolygonOffset(polygon_offset);
    }
    dirty &= ~DIRTY_DRAW_STATE;
    statistics.draw_state_applies++;
}

// FUNCTION: SURRENDER 0x100281B0
void srGERD::removeDeletedTextures()
{
    Texture* texture = texture_deleted;
    while (texture != 0) {
        Texture* next = texture->next;
        deleteTexture(*texture);
        texture = next;
    }
}

// FUNCTION: SURRENDER 0x100286B0
void srGERD::releaseTextureSurfaceData(Texture& texture)
{
    for (long index = 0; index < 12; ++index) {
        texture.device.levels[index] = 0;
    }
    if (texture.surface_data != 0) {
        srHeap.free(texture.surface_data);
        texture.surface_data = 0;
    }
    texture_cache_used -= texture.device.size;
    texture.device.size = 0;
}

// FUNCTION: SURRENDER 0x100286F0
void srGERD::deleteTexture(Texture& texture)
{
    for (unsigned long stage = 0; stage < device.info.max_texture_stages; ++stage) {
        if (&texture == texture_slots[stage]) {
            texture_slots[stage] = 0;
            dirty |= 1UL << (stage + DIRTY_TEXTURE_SHIFT);
        }
    }
    if (texture.prev != 0) {
        texture.prev->next = texture.next;
    }
    if (texture.next != 0) {
        texture.next->prev = texture.prev;
    }
    if (&texture == texture_deleted) {
        texture_deleted = texture.next;
    }
    texture_lookup.Remove(&texture.id);
    getDD()->deleteTexture(texture.device);
    if (texture.name != 0) {
        srHeap.free(texture.name);
        texture.name = 0;
    }
    texture.device.resident_data = 0;
    texture.device.resident_size = 0;
    texture.prev = 0;
    texture.next = 0;
    texture.device.deleted = 0;
    releaseTextureSurfaceData(texture);
    texture.palette = 0;
    texture.id = 0;
    texture.device.reset();
    texture_pool.release(&texture);
}

// FUNCTION: SURRENDER 0x10028910
srGERD::Texture* srGERD::findLowestPriority()
{
    Texture* texture = texture_head;
    Texture* found = 0;
    unsigned long last_use = texture_sequence + 1;
    float lowest = 1.01f;
    if (texture == 0) {
        return 0;
    }
    do {
        if (texture->device.priority < lowest ||
            (texture->device.priority == lowest &&
             texture->device.last_use < last_use)) {
            int bound = 0;
            if (device.info.max_texture_stages != 0) {
                Texture** slot = texture_slots;
                unsigned long stage = 0;
                do {
                    if (*slot == texture) {
                        bound = 1;
                        break;
                    }
                    ++stage;
                    ++slot;
                } while (stage < device.info.max_texture_stages);
            }
            if (bound == 0) {
                lowest = texture->device.priority;
                last_use = texture->device.last_use;
                found = texture;
            }
        }
        texture = texture->next;
        if (texture == 0) {
            return found;
        }
    } while (true);
}

// FUNCTION: SURRENDER 0x10028990
srGERD::Texture* srGERD::allocTexture(unsigned long id)
{
    Texture* texture = texture_pool.allocate();
    /* Retail zeroes 0xa4 bytes; the final unknown dword is untouched. */
    memset(texture, 0, 0xa4);
    texture->id = id;
    texture->palette = 0;
    texture_lookup.Insert(&texture->id, &texture);
    texture->next = texture_head;
    texture->prev = 0;
    if (texture_head != 0) {
        texture_head->prev = texture;
    }
    texture_head = texture;
    texture->device.reset();
    texture->name = 0;
    texture->device.resident = 0;
    texture->device.deleted = 0;
    return texture;
}

// FUNCTION: SURRENDER 0x10029390
void srGERD::invalidatePalette()
{
    if (palette != 0) {
        getDD()->deletePalette(device_palette);
        palette = 0;
        device_palette.data = 0;
        device_palette.size = 0;
    }
}

// FUNCTION: SURRENDER 0x10028FB0
void srGERD::setTextureParameters(unsigned long stage, const srTextureIFace::Parameters& parameters)
{
    unsigned long state = parameters.packed_state;
    float bias = parameters.mipmap_bias;
    unsigned long packed =
        ((((texture_state.wrap_s_map[(state >> srTextureIFace::Parameters::WRAP_S_SHIFT) & 1] &
            0xfffffff3) |
           (texture_state.wrap_t_map[(state >> srTextureIFace::Parameters::WRAP_T_SHIFT) & 1] << 2))
              << 2 |
          (texture_state.mipmap_map[(state >> srTextureIFace::Parameters::MIPMAP_SHIFT) & 3] &
           0xffffffc3))
             << 2 |
         (texture_state
              .min_filter_map[(state >> srTextureIFace::Parameters::MIN_FILTER_SHIFT) & 7] &
          0xffffff03))
            << 2 |
        (texture_state.mag_filter_map[(state >> srTextureIFace::Parameters::MAG_FILTER_SHIFT) & 7] &
         0xfffffc0f);
    packed = (packed << 4) |
             (texture_state.correction_map[state & srTextureIFace::Parameters::CORRECTION_MASK] &
              0xffffc00f);
    srDD::TexParms* parms = &texture_parms[stage];
    if (packed == parms->packed && bias == parms->mipmap_bias) {
        return;
    }
    parms->packed = packed;
    parms->mipmap_bias = bias;
    ++statistics.texture_parameter_sets;
    getDD()->setTextureParameters(stage, *parms);
}

// FUNCTION: SURRENDER 0x10029400
void srGERD::changeTexture(srTextureIFace* texture, unsigned long stage, int apply_parms)
{
    if ((state_flags & STATE_CLOSING_WINDOW) != 0) {
        return;
    }
    if (device.info.max_texture_stages <= stage) {
        return;
    }
    Texture* found;
    if (texture == 0) {
        if (texture_slots[stage] == texture_default) {
            return;
        }
    } else {
        unsigned long id = texture->getTextureFrameHandle();
        if (id != 0) {
            int slot = texture_lookup.FindNextEntry(&id, -1);
            if (slot != -1) {
                found = texture_lookup.entries[slot].value;
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
    found = texture_default;
    texture = srCore.getTexture();
bound:
    ++texture_sequence;
    found->device.last_use = texture_sequence;
    if (apply_parms == 0) {
        return;
    }
    if (found->palette != 0 && found->palette != this->palette) {
        ++statistics.palette_binds;
        invalidatePalette();
        srPalette* palette = found->palette;
        device_palette.flags = 0;
        device_palette.data = palette->getPaletteDataPtr();
        unsigned long size = palette->getPaletteSize();
        device_palette.size = size;
        if (0x100 < size) {
            device_palette.size = 0x100;
        }
        getDD()->bindPalette(device_palette);
        this->palette = palette;
    }
    Texture* previous = texture_slots[stage];
    texture_slots[stage] = found;
    if (previous != found) {
        getDD()->bindTexture(stage, found->device);
        if (found != texture_default && found->device.resident == 0 && found->surface_data != 0 &&
            (device.info.flags & srDD::Info::RELEASE_SURFACE_AFTER_BIND) != 0) {
            releaseTextureSurfaceData(*found);
        }
        ++statistics.texture_binds;
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
    format.color_model = device.color_model;
    format.pixel_size = device.pixel_size;
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
    device.color_model = format.color_model;
    device.pixel_size = format.pixel_size;
}

// FUNCTION: SURRENDER 0x10018F30
void srGERD::initTextureFormats()
{
    device.texture_formats = 0;
    device.texture_format_count = 0;
    srDD::PixelFormatList list;
    getDD()->getTextureFormats(list);
    long count = list.count;
    if (count != 0) {
        device.texture_formats = new srPixelConvert::PixelFormat[count];
        long i;
        for (i = 0; i < count; i++) {
            device.texture_formats[i].fourcc = 0;
        }
        device.texture_format_count = count;
        for (i = 0; i < count; i++) {
            convertPixelFormat(device.texture_formats[i], list.formats[i]);
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
    device.display_modes = 0;
    device.display_mode_count = 0;
    if (list.count != 0) {
        device.display_modes = new srDD::WindowInfo[list.count];
        device.display_mode_count = list.count;
        for (long i = 0; i < list.count; i++) {
            device.display_modes[i].width = list.entries[i].width;
            device.display_modes[i].height = list.entries[i].height;
            device.display_modes[i].depth = list.entries[i].depth;
        }
    }
}

// FUNCTION: SURRENDER 0x10019080
void srGERD::initDDInfo()
{
    memset(&device.info, 0, sizeof(device.info));
    device.info.flags = 0;
    device.info.hardware_id = 1;
    device.info.texture_min_dim = 1;
    device.info.texture_max_aspect = 1;
    device.info.max_texture_stages = 1;
    device.info.renderer_batch_limit = 0x100;
    device.info.unknown_20_ = 0x3b808081;
    device.info.texture_ram = 0x200000;
    device.info.unknown_0c_ = 0x10;
    device.info.unknown_08_ = 4;
    device.info.texture_max_dim = 0x100;
    device.info.unknown_10_ = 1.0f;
    device.info.unknown_14_ = 65536.0f;
    for (long i = 0; i < 9; i++) {
        sprintf(device.info.text[i], "Unknown");
    }
    getDD()->getInfo(device.info);
    if (device.info.max_texture_stages > 2) {
        device.info.max_texture_stages = 2;
    }
}

// FUNCTION: SURRENDER 0x1001C3E0
void srGERD::initLights()
{
    ambient_light.Set(0.2f, 0.2f, 0.2f, 1.0f);
}

// FUNCTION: SURRENDER 0x10021BE0
void srGERD::initMatrices()
{
    state.matrix_mode = MATRIX_MODELVIEW;
    for (long i = 0; i < 2; i++) {
        state.matrix_current[i].vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
        state.matrix_current[i].vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
        state.matrix_current[i].vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
        state.matrix_current[i].vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
        state.matrix_class[i] = srMatrix4T<float>::TYPE_IDENTITY;
        state.matrix_stacks[i].depth = 0;
    }
    state.normal_matrix.vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
    state.normal_matrix.vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
    state.normal_matrix.vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
    state.normal_matrix.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
    state.inverse_modelview.vectors[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
    state.inverse_modelview.vectors[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
    state.inverse_modelview.vectors[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
    state.inverse_modelview.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
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
        for (RendererEntry* entry = this->renderers; entry != 0; entry = entry->next) {
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
        stream << "Swap interval      : " << display.swap_interval << std::endl;
        stream << "Gamma              : " << '{' << display.gamma.x << ','
               << display.gamma.y << ',' << display.gamma.z << '}' << std::endl;
    }
    if ((info.value & INFO_STATISTICS) != 0) {
        Statistics statistics = frame_statistics;
        if (statistics.elapsed > 0.1) {
            stream << std::endl;
            stream << "Seconds since reset             : " << statistics.elapsed << '\n';
            stream << "Frames since reset              : " << statistics.frames << '\n';
            stream << "Statistics (average per second)" << '\n';
            stream << "Frames                          : "
                   << statistics.frames / statistics.elapsed << '\n';
            stream << "Triangle chunks rendered        : "
                   << statistics.triangle_chunks / statistics.elapsed << '\n';
            stream << "Triangles in                    : "
                   << statistics.input_triangles / statistics.elapsed << '\n';
            stream << "Vertices in                     : "
                   << statistics.input_vertices / statistics.elapsed << '\n';
            if (statistics.input_triangles != 0) {
                stream << "Input vertex/triangle ratio     : "
                       << statistics.input_vertices / static_cast<float>(statistics.input_triangles)
                       << '\n';
            }
            stream << "DD triangles received           : "
                   << statistics.device_triangles / statistics.elapsed << '\n';
            stream << "DD vertices transfered          : "
                   << statistics.device_vertices / statistics.elapsed << '\n';
            stream << "DD vertex indices specified     : "
                   << statistics.device_vertex_indices / statistics.elapsed << '\n';
            if (statistics.sphere_tests != 0) {
                stream << "Objects bounding sphere tested  : "
                       << statistics.sphere_tests / statistics.elapsed << std::endl;
                stream << "Bounding sphere test passed     : "
                       << statistics.sphere_visible / statistics.elapsed << " ("
                       << statistics.sphere_visible * 100.0 / statistics.sphere_tests << "%)"
                       << std::endl;
            }
            if (statistics.box_tests != 0) {
                stream << "Objects bounding box tested     : "
                       << statistics.box_tests / statistics.elapsed << std::endl;
                stream << "Bounding box test passed        : "
                       << statistics.box_visible / statistics.elapsed << " ("
                       << statistics.box_visible * 100.0 / statistics.box_tests << "%)"
                       << std::endl;
            }
            stream << std::endl;
            stream << "Triangles sorted                : "
                   << statistics.sorted_triangles / statistics.elapsed << '\n';
            stream << "Triangles removed by clipping   : "
                   << statistics.clipped_triangles / statistics.elapsed << '\n';
            stream << "View state changes              : "
                   << statistics.view_state_applies / statistics.elapsed << '\n';
            stream << "Matrix changes/classifications  : "
                   << statistics.matrix_classifications / statistics.elapsed << '\n';
            stream << "Draw state changes              : "
                   << statistics.draw_state_applies / statistics.elapsed << '\n';
            stream << "Per-frame state changes         : "
                   << statistics.frame_state_count / statistics.elapsed << '\n';
            stream << "Texture changes                 : "
                   << statistics.texture_binds / statistics.elapsed << '\n';
            stream << "Palette changes                 : "
                   << statistics.palette_binds / statistics.elapsed << '\n';
            stream << "Texture parameter updates       : "
                   << statistics.texture_parameter_sets / statistics.elapsed << '\n';
            stream << "Shader changes                  : "
                   << statistics.shader_sets / statistics.elapsed << '\n';
            stream << std::endl;
            stream << "DD draw commands                : "
                   << statistics.draw_calls / statistics.elapsed << '\n';
            if ((device.info.flags & srDD::Info::PIXEL_TEXTURE_STATISTICS) != 0) {
                stream << "DD pixels drawn          (M/s)  : "
                       << statistics.pixels_drawn * 1e-06 / statistics.elapsed << '\n';
                /* reinterpret-ok: the device stats mirror stores the
                   transfer counter's double bits as a dword pair. */
                stream << "DD Texture data transfer (Mb/s) : "
                       << *reinterpret_cast<const double*>(&statistics.texture_transfer_low) *
                              9.5367431640625e-07 / statistics.elapsed
                       << '\n';
            } else {
                stream << "DD doesn't support pixel/texture statistics" << std::endl;
            }
            stream << "Function calls to DD            : "
                   << statistics.device_calls / statistics.elapsed << std::endl;
        }
        if ((info.value & INFO_DEBUG_DD) != 0 && (enable_flags.value & (1UL << ENABLE_DEBUG_DD)) != 0 &&
            device.debug_dd != 0) {
            double total = 0.0;
            srStreamPrintf(stream, "\nFunction                        Calls/sec  Time used\n");
            srStreamPrintf(stream,
                           "---------------------------------------------------------------\n");
            for (long i = 0; i < 0x2b; i++) {
                unsigned long calls = device.debug_dd->call_counts[i];
                double used = device.debug_dd->call_times[i] -
                              calls * device.debug_dd->time_scale;
                if (used <= 0.0) {
                    used = 0.0;
                }
                if (calls != 0) {
                    char text[36];
                    sprintf(text, "%.2f", calls / statistics.elapsed);
                    double percent = used / statistics.elapsed * 100.0;
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
    srCriticalSectionAccess access(state_section);
    if (!texture_hash_enabled) {
        srStreamPrintf(stream, "Texture cache hibernating\n");
        return;
    }
    unsigned long count = 0;
    for (Texture* texture = texture_head; texture != 0; texture = texture->next) {
        count++;
    }
    srStreamPrintf(stream, "GERD Texture cache:\n\n");
    srStreamPrintf(stream, "Cached textures:          %d\n", count);
    srStreamPrintf(stream, "Hash table size           %d\n", texture_lookup.bucket_count);
    srStreamPrintf(stream, "GERD Cache memory used:   %d kB\n",
                   (texture_cache_used + 0x3ff) >> 10);
    if (texture_cache_size == 0) {
        srStreamPrintf(stream, "Max cache size:           infinite\n");
    } else {
        srStreamPrintf(stream, "Max cache size:           %d kB\n",
                       (texture_cache_size + 0x3ff) >> 10);
    }
    srStreamPrintf(stream, "\n");
    srStreamPrintf(stream, "Device TMUs:              %d\n",
                   device.info.max_texture_stages);
    if (device.info.texture_ram == 0) {
        srStreamPrintf(stream, "Device texture RAM:       infinite\n");
    } else {
        srStreamPrintf(stream, "Device texture RAM:       %d kB\n",
                       (device.info.texture_ram + 0x3ff) >> 10);
    }
    srStreamPrintf(stream, "Resident textures:        %d kB ",
                   (getResidentTextureMemUsed() + 0x3ff) >> 10);
    if (device.info.texture_ram != 0) {
        srStreamPrintf(stream, " (%.2f%%)",
                       getResidentTextureMemUsed() * 100.0 / device.info.texture_ram);
    }
    srStreamPrintf(stream, "\n\n");
    srStreamPrintf(stream,
                   "#     Resolution  Format       KB    LODs  Priority   Timestamp  Resident  "
                   "FHandle     Name\n");
    srStreamPrintf(stream,
                   "-----------------------------------------------------------------------------"
                   "--------------\n");
    long index = 0;
    for (Texture* entry = texture_head; entry != 0; entry = entry->next) {
        char format_name[64];
        format_name[0] = '\0';
        entry->pixel_format.getName(format_name);
        srStreamPrintf(stream, "%04d  %03dx%03d     %-11s  ", index, entry->device.width,
                       entry->device.height, format_name);
        index++;
        float kb = (entry->device.size + 0x3ff) * 0.0009765625f;
        if (kb >= 10.0f) {
            srStreamPrintf(stream, "%-4d  ", static_cast<long>(kb));
        } else {
            srStreamPrintf(stream, "%.1f   ", kb);
        }
        int resident =
            entry->device.resident_data != 0 && entry->device.resident_size != 0;
        srStreamPrintf(stream, "%-2d    %.4f     %08x   %-5s     %08x   ",
                       entry->device.last_level - entry->device.first_level + 1,
                       entry->device.priority, entry->device.last_use,
                       srBoolToString(resident), entry->id);
        if (entry->name == 0) {
            srStreamPrintf(stream, "anon\n");
        } else {
            srStreamPrintf(stream, "%s\n", entry->name);
        }
    }
}

// FUNCTION: SURRENDER 0x1001EE20
void srGERD::dumpDeviceList(std::ostream& stream)
{
    for (srGERD* gerd = first; gerd != 0; gerd = gerd->next) {
        srStreamPrintf(stream, "%s\n", gerd->getDeviceName());
    }
}

// FUNCTION: SURRENDER 0x10018870
srGERD* srGERD::loadDeviceWithFileName(const char* filename, unsigned long device)
{
    /* Retail reads the six entry-point names from the same contiguous table
       for lookup and the missing-function error print (0x100993CC). */
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
        srDynamicLibrary::getFunction(library, entry_names[0]));
    srDDGetDriverNameFn getDriverName = reinterpret_cast<srDDGetDriverNameFn>(
        srDynamicLibrary::getFunction(library, entry_names[1]));
    srDDConfigureDriverFn configureDriver = reinterpret_cast<srDDConfigureDriverFn>(
        srDynamicLibrary::getFunction(library, entry_names[2]));
    srDDGetDeviceCountFn getDeviceCount = reinterpret_cast<srDDGetDeviceCountFn>(
        srDynamicLibrary::getFunction(library, entry_names[3]));
    srDDGetDeviceNameFn getDeviceName = reinterpret_cast<srDDGetDeviceNameFn>(
        srDynamicLibrary::getFunction(library, entry_names[4]));
    srDDInitDeviceFn initDevice =
        reinterpret_cast<srDDInitDeviceFn>(srDynamicLibrary::getFunction(library, entry_names[5]));
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
        char* key = new char[strlen(name) + 8];
        sprintf(key, "DD_%s", name);
        long key_length = static_cast<long>(strlen(key));
        for (long index = 0; index < key_length; index++) {
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
            srDebugPrintf(5,
                          "srGERD::loadDeviceWithFileName() -- DD driver '%s' "
                          "(device %s) loaded succesfully.\n",
                          filename, getDeviceName(device));
            return new srGERD(dd, library, getDeviceName(device));
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
        return ERROR_INVALID_WHANDLE;
    }
    for (srGERD* gerd = getFirst(); gerd != 0; gerd = gerd->getNext()) {
        if (gerd->isContextCreated() != 0 && gerd->getWindowHandle() == window) {
            return ERROR_SHARED_CONTEXT;
        }
    }
    if (getDD()->createContext(window) != 0) {
        return ERROR_CONTEXT_CREATION_FAILED;
    }
    device.window = window;
    initDDInfo();
    initTextureFormats();
    initDisplayModeList();
    initGlobalPalette();
    resetStatistics();
    state_flags |= STATE_CONTEXT_CREATED;
    return ERROR_NONE;
}

// FUNCTION: SURRENDER 0x100192A0
void srGERD::deleteContext()
{
    if ((state_flags & STATE_CONTEXT_CREATED) != 0) {
        closeWindow(static_cast<e_closeHint>(0));
        closeTexCache();
        if (device.texture_formats != 0) {
            delete[] device.texture_formats;
        }
        if (device.display_modes != 0) {
            delete[] device.display_modes;
        }
        device.texture_formats = 0;
        device.display_modes = 0;
        device.texture_format_count = 0;
        device.display_mode_count = 0;
        getDD()->deleteContext();
        state_flags &= ~STATE_CONTEXT_CREATED;
        device.window = 0;
    }
}

// FUNCTION: SURRENDER 0x10019EB0
void srGERD::deleteRenderers()
{
    srCriticalSectionAccess access(renderers_section);
    while (renderers != 0) {
        RendererEntry* next = renderers->next;
        delete renderers->renderer;
        delete renderers;
        renderers = next;
    }
}

// FUNCTION: SURRENDER 0x1001F880
void srGERD::accumRelease()
{
    if (accum_buffer != 0) {
        srHeap.free(accum_buffer);
        accum_buffer = 0;
    }
    if (accum_scratch != 0) {
        srHeap.free(accum_scratch);
        accum_scratch = 0;
    }
}

// FUNCTION: SURRENDER 0x10020DF0
srColorSurfaceIFace* srGERD::lockBuffer()
{
    if (lock_surface != 0) {
        return 0;
    }
    flush();
    if (_lockBuffer() != 0) {
        return 0;
    }
    checkViewStateChanges();
    srPixelConvert::PixelFormat format;
    getPixelFormat(format);
    lock_surface = new LockSurface(this, format);
    lock_surface->setScissor(state.scissor.left, state.scissor.top, state.scissor.right,
                             state.scissor.bottom);
    if (format.color_model == srPixelConvert::COLOR_RGB &&
        format.pixel_size == srPixelConvert::PIXEL_SIZE_32 && format.red_bits == 8 &&
        format.green_bits == 8 && format.blue_bits == 8 && format.alpha_bits == 8 &&
        format.red_shift == 0x10 && format.green_shift == 8 && format.blue_shift == 0 &&
        format.alpha_shift == 0x18) {
        lock_surface->argb32 = 1;
    }
    return lock_surface;
}

// FUNCTION: SURRENDER 0x10020F30
void srGERD::unlockBuffer()
{
    if (lock_surface != 0) {
        lock_surface->release();
        lock_surface = 0;
        _unlockBuffer();
    }
}

// FUNCTION: SURRENDER 0x1001A5F0
void srGERD::closeWindow(e_closeHint hint)
{
    srCriticalSectionAccess access(state_section);
    if (isWindowOpen() != 0) {
        state_flags |= STATE_CLOSING_WINDOW;
        flush();
        deleteRenderers();
        unlockBuffer();
        invalidateResidentTextures();
        invalidatePalette();
        closeTexCache();
        getDD()->closeWindow();
        resetStatistics();
        accumRelease();
        memset(&device.open_info, 0, sizeof(device.open_info));
        state_flags &= ~STATE_WINDOW_OPEN;
        device.back_buffer_type = static_cast<e_backBuffer>(0);
        if (prev_open != 0) {
            prev_open->next_open = next_open;
        }
        if (next_open != 0) {
            next_open->prev_open = prev_open;
        }
        if (this == firstOpen) {
            firstOpen = next_open;
        }
        prev_open = 0;
        next_open = 0;
        state_flags &= ~STATE_CLOSING_WINDOW;
        shader = srShader();
        texture_iface[0] = 0;
        texture_iface[1] = 0;
        pick_vertices.release();
    }
}

// FUNCTION: SURRENDER 0x1001CD10
srGERD::e_backBuffer srGERD::getBackBufferType() const
{
    return device.back_buffer_type;
}

// FUNCTION: SURRENDER 0x1001A120
srGERD::e_error srGERD::openWindow()
{
    if ((state_flags & STATE_CONTEXT_CREATED) == 0) {
        return ERROR_NO_CONTEXT;
    }
    return openWindow(srWindow::getWidth(device.window), srWindow::getHeight(device.window));
}

// FUNCTION: SURRENDER 0x1001A160
srGERD::e_error srGERD::openWindow(long width, long height)
{
    if ((state_flags & STATE_CONTEXT_CREATED) == 0) {
        return ERROR_NO_CONTEXT;
    }
    if (srWindow::isWindow(device.window) == 0) {
        return ERROR_INVALID_WHANDLE;
    }
    OpenInfo info;
    info.window_width = srWindow::getWidth(device.window);
    info.window_height = srWindow::getHeight(device.window);
    info.width = width;
    info.height = height;
    info.display_mode = -1;
    return openWindowInternal(info);
}

// FUNCTION: SURRENDER 0x1001A1F0
srGERD::e_error srGERD::openWindow(long mode)
{
    if ((state_flags & STATE_CONTEXT_CREATED) == 0) {
        return ERROR_NO_CONTEXT;
    }
    if (mode < 0) {
        return openWindow();
    }
    if (srWindow::isWindow(device.window) == 0) {
        return ERROR_INVALID_WHANDLE;
    }
    if (device.display_mode_count <= mode) {
        return ERROR_WINDOW_OPEN_FAILED;
    }
    const srDD::WindowInfo& entry = device.display_modes[mode];
    OpenInfo info;
    info.window_width = info.width = (long)entry.width;
    info.window_height = info.height = (long)entry.height;
    info.display_mode = mode;
    return openWindowInternal(info);
}

// FUNCTION: SURRENDER 0x1001A290
srGERD::e_error srGERD::openWindowInternal(const OpenInfo& info)
{
    srCriticalSectionAccess access(state_section);
    if ((unsigned long)info.width > (unsigned long)info.window_width ||
        (unsigned long)info.height > (unsigned long)info.window_height) {
        return ERROR_INVALID_VALUE;
    }
    if ((state_flags & STATE_CONTEXT_CREATED) == 0) {
        return ERROR_NO_CONTEXT;
    }
    closeWindow(static_cast<e_closeHint>(1));
    if (info.width != 0 && info.height != 0 && info.window_width != 0 &&
        info.window_height != 0) {
        if (srWindow::isWindow(device.window) == 0) {
            return ERROR_INVALID_WHANDLE;
        }
        if (info.display_mode < -1 || device.display_mode_count <= info.display_mode ||
            device.info.max_back_buffer_width < (unsigned long)info.width ||
            device.info.max_back_buffer_height < (unsigned long)info.height) {
            return ERROR_INVALID_VALUE;
        }
        memset(&device.open_info, 0, sizeof(device.open_info));
        srDD::OpenInfo dd_info;
        dd_info.width = (unsigned long)info.width;
        dd_info.height = (unsigned long)info.height;
        dd_info.display_mode = info.display_mode;
        srDD::OpenResult result;
        result.back_buffer_type = 0;
        if (getDD()->openWindow(dd_info, result) == 0) {
            device.open_info = info;
            if (result.back_buffer_type == 1) {
                device.back_buffer_type = BACKBUFFER_NONE;
            } else if (result.back_buffer_type == 2) {
                device.back_buffer_type = BACKBUFFER_ONE;
            } else if (result.back_buffer_type == 3) {
                device.back_buffer_type = BACKBUFFER_TWO;
            }
            prev_open = 0;
            next_open = firstOpen;
            if (firstOpen != 0) {
                firstOpen->prev_open = this;
            }
            unsigned long flags = state_flags & ~4UL;
            firstOpen = this;
            state_flags |= STATE_WINDOW_OPEN;
            state_flags = flags | STATE_WINDOW_OPEN;
            state_flags |= STATE_FRAME_FLIPPED;
            resetStatistics();
            dirty = 0xffffffff;
            initTexCache();
            texture_iface[0] = 0;
            texture_iface[1] = 0;
            shader = srShader();
            if (device.info.max_texture_stages != 0) {
                unsigned long stage = 0;
                do {
                    texture_parms[stage].mipmap_bias = 0.0f;
                    texture_parms[stage].packed = 0x1a1;
                    texture_parms[stage].mipmap_bias = -1234567.0f;
                    setTexture(0, stage);
                    stage++;
                } while (stage < device.info.max_texture_stages);
            }
            invalidatePalette();
            state.scissor.left = 0;
            state.scissor.top = 0;
            state.scissor.right = getWidth();
            state.scissor.bottom = getHeight();
            state.view_left = 0;
            state.view_top = 0;
            state.view_right = getWidth();
            state.view_bottom = getHeight();
            createRenderer(1);
            if ((enable_flags.value & (1UL << ENABLE_CLEAR_ON_OPEN)) != 0) {
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
                    clear(srFlags<e_buffer>(~static_cast<unsigned long>(BUFFER_ACCUM)));
                    flipFrame();
                }
                flush();
            }
            return ERROR_NONE;
        }
    }
    return ERROR_WINDOW_OPEN_FAILED;
}

// FUNCTION: SURRENDER 0x10028460
void srGERD::closeTexCache()
{
    if (texture_hash_enabled != 0) {
        invalidateTextureCache();
        removeDeletedTextures();
        markTextureAsDeleted(*texture_default);
        deleteTexture(*texture_default);
        texture_default = 0;
        texture_lookup.Clear();
        texture_pool.release();
        texture_deleted = 0;
        texture_head = 0;
        texture_cache_used = 0;
        texture_hash_enabled = 0;
    }
}

// FUNCTION: SURRENDER 0x10028200
void srGERD::initTexCache()
{
    if (texture_hash_enabled == 0) {
        texture_lookup.Clear();
        texture_head = 0;
        texture_deleted = 0;
        texture_cache_used = 0;
        texture_pool.release();
        resetCurrentTexPointers();
        texture_default = createNewTexture(srCore.getTexture());
        texture_default->device.priority = 1.1f;
        texture_hash_enabled = 1;
        invalidateTextureCache();
    }
}

// FUNCTION: SURRENDER 0x10028B80
void srGERD::evaluateTextureDimensions(srDD::Texture& device,
                                       const srTextureIFace::Dimensions& dimensions)
{
    unsigned long min_dim = this->device.info.texture_min_dim;
    unsigned char reduction = 0;
    if ((dimensions.hints & (1UL << srTextureIFace::HINT_NO_REDUCTION)) == 0) {
        reduction = (unsigned char)texture_reduction;
    }
    unsigned long width = nextTextureDimension(dimensions.width) >> (reduction & 0x1f);
    unsigned long height = nextTextureDimension(dimensions.height) >> (reduction & 0x1f);
    unsigned long device_width = min_dim;
    if (min_dim <= width && width <= this->device.info.texture_max_dim) {
        device_width = width;
    }
    unsigned long device_height = min_dim;
    if (min_dim <= height && height <= this->device.info.texture_max_dim) {
        device_height = height;
    }
    int unbalanced = 0;
    if (device_height < device_width) {
        while (this->device.info.texture_max_aspect < device_width / device_height &&
               device_height < this->device.info.texture_max_dim) {
            device_height *= 2;
        }
        unbalanced = device_height < device_width;
    }
    if (unbalanced == 0 && device_height != device_width) {
        while (this->device.info.texture_max_aspect < device_height / device_width &&
               device_width < this->device.info.texture_max_dim) {
            device_width *= 2;
        }
    }
    device.width = device_width;
    device.height = device_height;
    device.first_level = 0;
    device.last_level = 0;
    if (((dimensions.hints & (1UL << srTextureIFace::HINT_NO_MIPMAPS)) == 0 ||
         (this->device.info.flags & 0x40) != 0) &&
        this->device.info.texture_min_dim < device_width) {
        do {
            if (device_height <= this->device.info.texture_min_dim) {
                return;
            }
            ++device.last_level;
            device_width >>= 1;
            device_height >>= 1;
        } while (this->device.info.texture_min_dim < device_width);
    }
}

// FUNCTION: SURRENDER 0x10028D20
void srGERD::evaluateTexturePixelFormat(Texture& texture,
                                        const srTextureIFace::Dimensions& dimensions)
{
    unsigned long flags = dimensions.hints;
    srPixelConvert::PixelFormat format = dimensions.format;
    if ((flags & (1UL << srTextureIFace::HINT_INTENSITY)) != 0) {
        srPixelConvert::mapPixelFormat(format.alpha_bits != 0 ? srPixelConvert::SURFACE_AL88
                                                              : srPixelConvert::SURFACE_L8,
                                       format);
    }
    if ((dimensions.hints & (1UL << srTextureIFace::HINT_ALPHA_ONLY)) != 0) {
        srPixelConvert::mapPixelFormat(srPixelConvert::SURFACE_A8, format);
    }
    flags = dimensions.hints;
    if ((flags & (1UL << srTextureIFace::HINT_NO_ALPHA)) == 0) {
        if ((flags & (1UL << srTextureIFace::HINT_ONE_BIT_ALPHA)) != 0) {
            format.alpha_bits = 1;
        }
    } else {
        format.alpha_bits = 0;
    }
    if ((flags & (1UL << srTextureIFace::HINT_POSITIONAL_6)) != 0) {
        texture.device.flags |= 1;
    }
    texture.device.resident = (dimensions.hints >> srTextureIFace::HINT_RESIDENT) & 1;
    convertPixelFormat(texture.device.format, format);
    unsigned long index =
        format.match(device.texture_formats, (unsigned long)device.texture_format_count);
    texture.device.format_index = index;
    texture.pixel_format = device.texture_formats[index];
    if (texture.pixel_format.color_model == srPixelConvert::COLOR_INDEXED) {
        texture.palette = dimensions.palette;
    } else {
        texture.palette = 0;
    }
    texture.device.parameter =
        texture_state.default_texture_params[dimensions.compression];
}

// FUNCTION: SURRENDER 0x10028E60
void srGERD::releaseTextureMemory(long bytes)
{
    if (0 < bytes) {
        Texture* texture = findLowestPriority();
        while (texture != 0) {
            bytes -= (long)texture->device.size;
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
unsigned long srGERD::getTextureBytesNeeded(Texture& texture) const
{
    unsigned long width = texture.device.width;
    unsigned long height = texture.device.height;
    unsigned long bytes = 0;
    if (texture.device.first_level <= texture.device.last_level) {
        long count = (long)(texture.device.last_level - texture.device.first_level) + 1;
        do {
            bytes += height * width * (texture.pixel_format.pixel_size + 1);
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
    if (texture_cache_size != 0 &&
        texture_cache_size < size + texture_cache_used) {
        float priority = texture.device.priority;
        texture.device.priority = 1.2f;
        releaseTextureMemory((long)(size - texture_cache_size + texture_cache_used));
        texture.device.priority = priority;
    }
    texture.surface_data = srHeap.allocate(size);
    texture.device.size = size;
    texture_cache_used += size;
    long bytes_per_pixel = texture.pixel_format.pixel_size;
    unsigned char* data = (unsigned char*)texture.surface_data;
    unsigned long width = texture.device.width;
    unsigned long height = texture.device.height;
    unsigned long level = texture.device.first_level;
    if (level <= texture.device.last_level) {
        void** levels = &texture.device.levels[level];
        do {
            *levels = data;
            data += height * width * (unsigned long)(bytes_per_pixel + 1);
            width >>= 1;
            height >>= 1;
            ++level;
            ++levels;
        } while (level <= texture.device.last_level);
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
    srPixelConvert::mapPixelFormat(srPixelConvert::SURFACE_ARGB4444, dimensions.format);
    Texture* result = allocTexture(texture->getTextureFrameHandle());
    const char* name = texture->getName();
    if (name == 0 || *name == 0) {
        result->name = 0;
    } else {
        char* copy = (char*)srHeap.allocate(strlen(name) + 1);
        result->name = copy;
        strcpy(copy, name);
    }
    dimensions.compression = texture_state.default_compression;
    dimensions.width = 1;
    dimensions.height = 1;
    dimensions.palette = 0;
    dimensions.format = *device.texture_formats;
    dimensions.filter = 0;
    dimensions.hints = 0;
    texture->getDimensions(dimensions);
    evaluateTextureDimensions(result->device, dimensions);
    evaluateTexturePixelFormat(*result, dimensions);
    allocTextureData(*result);
    srTextureIFace::MultiRequest request;
    request.mipmap_level = (long)result->device.first_level;
    request.last_level = result->device.last_level;
    unsigned long width = result->device.width;
    unsigned long height = result->device.height;
    long level = request.mipmap_level;
    for (; level <= (long)request.last_level; ++level) {
        srColorSurface* surface =
            new srColorSurface(result->pixel_format, result->device.levels[level], width, height,
                               (unsigned long)(result->pixel_format.pixel_size + 1) * width);
        request.destinations[level] = surface;
        if (result->palette != 0) {
            surface->setPalette(result->palette);
        }
        surface->setFilter(dimensions.filter);
        width >>= 1;
        height >>= 1;
    }
    texture->getMipmapData(request);
    for (level = request.mipmap_level; level <= (long)request.last_level; ++level) {
        request.destinations[level]->release();
    }
    result->device.resident_data = 0;
    result->device.priority = texture->getPriority();
    ++statistics.textures_created;
    return result;
}

// FUNCTION: SURRENDER 0x10017930
long srGERD::getMaxTextureWidth() const
{
    return device.info.texture_max_dim;
}

// FUNCTION: SURRENDER 0x10017940
long srGERD::getMaxTextureHeight() const
{
    return device.info.texture_max_dim;
}

// FUNCTION: SURRENDER 0x10017950
long srGERD::getMaxTextureAspectRatio() const
{
    return device.info.texture_max_aspect;
}

// FUNCTION: SURRENDER 0x10017960
void srGERD::setGlobalPalette(const srPalette& palette)
{
    srCriticalSectionAccess access(state_section);
    if (palette.matchPalette(global_palette, 0x100) != 0) {
        return;
    }
    unsigned long count = palette.getPaletteSize();
    if (count > 0x100) {
        count = 0x100;
    }
    for (unsigned long i = 0; i < count; ++i) {
        global_palette[i] = palette.getColor(i);
    }
    /* reinterpret-ok: the DD receives the palette entries as raw dwords. */
    getDD()->setGlobalPalette(reinterpret_cast<unsigned long*>(global_palette), count);
}

// FUNCTION: SURRENDER 0x10017AA0
long srGERD::getTextureReduction() const
{
    srCriticalSection* section = state_section;
    section->getAccess();
    long reduction = texture_reduction;
    section->releaseAccess();
    return reduction;
}

// FUNCTION: SURRENDER 0x10017B70
void srGERD::invalidateResidentPalette(srPalette* palette)
{
    if (palette != 0 && palette == this->palette) {
        this->palette = 0;
        dirty |= DIRTY_TEXTURE0;
        dirty |= DIRTY_TEXTURE1;
    }
}

// FUNCTION: SURRENDER 0x10017D00
int srGERD::isTextureCached(srTextureIFace* texture) const
{
    srCriticalSectionAccess access(state_section);
    if (texture != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        return texture_lookup.Lookup(&handle) != 0;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10017DD0
int srGERD::isTextureResident(srTextureIFace* texture) const
{
    srCriticalSectionAccess access(state_section);
    if (texture != 0 && isWindowOpen() != 0) {
        unsigned long handle = texture->getTextureFrameHandle();
        Texture* resident = texture_lookup.Lookup(&handle);
        if (resident != 0 && resident->device.resident_data != 0 &&
            resident->device.resident_size != 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10017F70
int srGERD::getTextureInfo(srTextureIFace* texture, TextureInfo& info)
{
    srCriticalSectionAccess access(state_section);
    if (isWindowOpen() == 0) {
        return 0;
    }
    unsigned long handle = texture->getTextureFrameHandle();
    Texture* resident = texture_lookup.Lookup(&handle);
    if (resident != 0) {
        info.pixel_format = resident->pixel_format;
        info.width = resident->device.width;
        info.height = resident->device.height;
        info.last_level = resident->device.last_level;
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10029600
void srGERD::initGlobalPalette()
{
    float level = 0.0f;
    for (long i = 0; i < 0x100; ++i) {
        unsigned char gray = (unsigned char)srFloatToInt(level * 255.0);
        global_palette[i].blue = gray;
        global_palette[i].green = gray;
        global_palette[i].red = gray;
        global_palette[i].alpha = 0xff;
        level += 0.003921569f;
    }
    /* reinterpret-ok: the DD receives the palette entries as raw dwords. */
    getDD()->setGlobalPalette(reinterpret_cast<unsigned long*>(global_palette), 0x100);
}

// FUNCTION: SURRENDER 0x10018C70
void srGERD::scanDevices(const char* path, srStringTable& devices)
{
    srStringTable libraries;
    char entry[512];
    unsigned long count = srSystem::scanLibraries(libraries, path, "srDD*");
    for (unsigned long index = 0; index < count; ++index) {
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
    while (getFirst() != 0) {
        delete getFirst();
    }
}

// FUNCTION: SURRENDER 0x1001AFD0
void srGERD::setHint(e_hint hint, e_hintMode mode)
{
    if (device.hints[hint] != mode) {
        device.hints[hint] = mode;
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

// GLOBAL: SURRENDER 0x100993A4
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
    return lock_surface != 0;
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
    if (mode != polygon_mode) {
        flushImmediateRenderers();
        polygon_mode = mode;
        dirty |= DIRTY_POLYGON_MODE;
    }
}

// FUNCTION: SURRENDER 0x1001C8E0
srGERD::e_polygonMode srGERD::getPolygonMode() const
{
    return polygon_mode;
}

// FUNCTION: SURRENDER 0x1001C8F0
void srGERD::getClearAccum(srVector4T<float>& color) const
{
    color = clear_state.clear_values.accum;
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
    clear_state.clear_values.accum = color;
    float* clear = &clear_state.clear_values.accum.x;
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
    color = clear_state.clear_values.color;
}

// FUNCTION: SURRENDER 0x1001CBE0
void srGERD::setClearStencil(unsigned long stencil)
{
    clear_state.clear_values.stencil = stencil;
}

// FUNCTION: SURRENDER 0x1001CBF0
double srGERD::getClearDepth() const
{
    return clear_state.clear_values.depth;
}

// FUNCTION: SURRENDER 0x1001CC00
unsigned long srGERD::getClearStencil() const
{
    return clear_state.clear_values.stencil;
}

// FUNCTION: SURRENDER 0x1001CF00
unsigned long srGERD::getDDAPIVersion() const
{
    return device.driver_info.dd_api_version;
}

// FUNCTION: SURRENDER 0x1001CF60
srGERD* srGERD::getPrev() const
{
    return prev;
}

// FUNCTION: SURRENDER 0x1001CF70
srGERD* srGERD::getPrevOpen() const
{
    return prev_open;
}

// FUNCTION: SURRENDER 0x1001CFA0
void srGERD::extCommand(unsigned long command, void* data, unsigned long size)
{
    getDD()->extCommand(command, data, size);
}

// FUNCTION: SURRENDER 0x1001CFD0
srGERD::e_hintMode srGERD::getHint(e_hint hint) const
{
    return device.hints[hint];
}

// FUNCTION: SURRENDER 0x1001CFE0
void srGERD::getGamma(srVector3T<float>& gamma) const
{
    gamma = display.gamma;
}

// FUNCTION: SURRENDER 0x1001D000
srGERD::e_antiAlias srGERD::getAntiAlias() const
{
    return display.antialias;
}

// FUNCTION: SURRENDER 0x1001D010
srGERD::e_error srGERD::getError()
{
    e_error error = last_error;
    setError(ERROR_NONE);
    return error;
}

// FUNCTION: SURRENDER 0x1001D0C0
int srGERD::isFlipped() const
{
    return (state_flags >> 3) & 1;
}

// FUNCTION: SURRENDER 0x1001D0F0
const char* srGERD::getApiVersion() const
{
    return device.driver_info.api_name;
}

// FUNCTION: SURRENDER 0x1001D1B0
srGERD::e_depthBuffer srGERD::getDepthBufferType() const
{
    return (e_depthBuffer)((device.info.flags & 0xff) >> 3 & 1);
}

// FUNCTION: SURRENDER 0x1001D1C0
srDD::e_driverID srGERD::getDriverID() const
{
    return (srDD::e_driverID)device.driver_info.driver_id;
}

// FUNCTION: SURRENDER 0x1001D1E0
unsigned long srGERD::getSwapInterval() const
{
    return display.swap_interval;
}

// FUNCTION: SURRENDER 0x1001D210
void srGERD::getTextureFormat(unsigned long index, srPixelConvert::PixelFormat& format) const
{
    if (device.texture_format_count <= (long)index) {
        index = 0;
    }
    format = device.texture_formats[index];
}

// FUNCTION: SURRENDER 0x1001D240
unsigned long srGERD::getTextureFormatCount() const
{
    return device.texture_format_count;
}

// FUNCTION: SURRENDER 0x1001D250
void srGERD::getDisplayModeInfo(long index, DisplayModeInfo& info) const
{
    if (device.display_mode_count <= index) {
        index = 0;
    }
    info.width = device.display_modes[index].width;
    info.height = device.display_modes[index].height;
    info.depth = device.display_modes[index].depth;
}

// FUNCTION: SURRENDER 0x1001D290
unsigned long srGERD::getDisplayModeCount() const
{
    return device.display_mode_count;
}

// FUNCTION: SURRENDER 0x1001D330
void srGERD::getDepthRange(double& minimum, double& maximum) const
{
    minimum = state.depth_min;
    maximum = state.depth_max;
}

// FUNCTION: SURRENDER 0x1001D360
void srGERD::setDepthRange(double minimum, double maximum)
{
    if (minimum <= 0.0) {
        minimum = 0.0;
    } else if (minimum >= 1.0) {
        minimum = 1.0;
    }
    state.depth_min = minimum;
    if (maximum > 0.0) {
        if (maximum < 1.0) {
            state.depth_max = maximum;
        } else {
            state.depth_max = 1.0;
        }
    } else {
        state.depth_max = 0.0;
    }
    dirty |= DIRTY_DEPTH_RANGE;
}

// FUNCTION: SURRENDER 0x1001BB00
srShader srGERD::getShader() const
{
    return shader;
}

// FUNCTION: SURRENDER 0x1001BAA0
void srGERD::disable(e_enable option)
{
    if ((enable_flags.value & (1UL << option)) != 0) {
        toggle(option);
    }
}

// FUNCTION: SURRENDER 0x1001BAC0
void srGERD::enable(e_enable option)
{
    if ((enable_flags.value & (1UL << option)) == 0) {
        toggle(option);
    }
}

// FUNCTION: SURRENDER 0x1001BBB0
void srGERD::drawTriangle(const srVector3i& triangle)
{
    drawElements(static_cast<srRendererDefs::e_primitive>(3), 3, srRendererDefs::INDEX_ULONG,
                 &triangle);
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
    vertex_arrays.components[index] = components;
    vertex_arrays.types[index] = type;
    vertex_arrays.strides[index] = stride;
    vertex_arrays.arrays[index] = values;
    vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
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
    vertex_arrays.components[2] = components;
    vertex_arrays.types[2] = type;
    vertex_arrays.strides[2] = stride;
    vertex_arrays.arrays[2] = values;
    vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
}

// FUNCTION: SURRENDER 0x1001BF90
void srGERD::setFogPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                           const void* values)
{
    vertex_arrays.components[3] = components;
    vertex_arrays.types[3] = type;
    vertex_arrays.strides[3] = stride;
    vertex_arrays.arrays[3] = values;
    vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
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
    state.depth_min = 0.0;
    state.depth_max = 1.0;
    state.clip_plane_count = 0;
    state.clip_mask = srRendererDefs::FRUSTUM_CLIP_MASK;
    state.clip_mode1_mask = 0;
    state.cull_mode = CULL_BACK;
    state.winding = static_cast<e_winding>(0);
}

// FUNCTION: SURRENDER 0x10027F60
void srGERD::initTextureParameterMatrix()
{
    texture_state.correction_map[0] = 0;
    texture_state.correction_map[1] = 1;
    texture_state.correction_map[2] = 2;
    texture_state.correction_map[3] = 1;
    texture_state.mag_filter_map[0] = 0;
    texture_state.mag_filter_map[1] = 1;
    texture_state.mag_filter_map[2] = 2;
    texture_state.mag_filter_map[3] = 3;
    texture_state.mag_filter_map[4] = 2;
    texture_state.min_filter_map[0] = 0;
    texture_state.min_filter_map[1] = 1;
    texture_state.min_filter_map[2] = 2;
    texture_state.min_filter_map[3] = 3;
    texture_state.min_filter_map[4] = 2;
    texture_state.mipmap_map[0] = 0;
    texture_state.mipmap_map[1] = 1;
    texture_state.mipmap_map[2] = 2;
    texture_state.mipmap_map[3] = 1;
    texture_state.wrap_s_map[0] = 0;
    texture_state.wrap_s_map[1] = 1;
    texture_state.wrap_t_map[0] = 0;
    texture_state.wrap_t_map[1] = 1;
    texture_state.default_correction = static_cast<srTextureIFace::e_correction>(1);
    texture_state.default_mag_filter = static_cast<srTextureIFace::e_filter>(2);
    texture_state.default_min_filter = static_cast<srTextureIFace::e_filter>(2);
    texture_state.default_mipmap = static_cast<srTextureIFace::e_mipmap>(1);
    texture_state.default_texture_params[0] = 0;
    texture_state.default_texture_params[1] = 1;
    texture_state.default_texture_params[2] = 2;
    texture_state.default_texture_params[3] = 3;
    texture_state.default_texture_params[4] = 0;
    texture_state.default_compression = static_cast<srTextureIFace::e_compression>(0);
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

// FUNCTION: SURRENDER 0x1001FAF0
void __cdecl srGERD::accumAdd_MMX(AccumPixel* accum, long value, long count)
{
    __asm {
        mov edi, accum
        mov ecx, count
        movd mm6, value
        punpcklwd mm6, mm6
        punpckldq mm6, mm6
        lea edi, [edi + ecx*8]
        neg ecx
    accumAdd_MMX_loop:
        movq mm0, qword ptr [edi + ecx*8]
        paddw mm0, mm6
        movq qword ptr [edi + ecx*8], mm0
        inc ecx
        js accumAdd_MMX_loop
        emms
    }
}

// FUNCTION: SURRENDER 0x1001FB20
void __cdecl srGERD::accumMult_MMX(AccumPixel* accum, long value, long count)
{
    __asm {
        mov edi, accum
        mov ecx, count
        movd mm5, value
        punpcklwd mm5, mm5
        punpckhdq mm5, mm5
        movd mm6, value
        punpcklwd mm6, mm6
        punpckldq mm6, mm6
        psrlw mm6, 1
        lea edi, [edi + ecx*8]
        neg ecx
    accumMult_MMX_loop:
        movq mm0, qword ptr [edi + ecx*8]
        movq mm1, mm0
        pmullw mm0, mm5
        pmulhw mm1, mm6
        paddw mm1, mm1
        paddw mm0, mm1
        movq qword ptr [edi + ecx*8], mm0
        inc ecx
        js accumMult_MMX_loop
        emms
    }
}

// FUNCTION: SURRENDER 0x1001FBD0
void srGERD::accumulate(e_accum operation, float scale)
{
    if (!isWindowOpen()) {
        return;
    }
    long width = state.scissor.right - state.scissor.left;
    long height = state.scissor.bottom - state.scissor.top;
    if (width == 0 || height == 0) {
        return;
    }
    if (accum_buffer == 0) {
        accumAlloc();
        if (accum_buffer == 0) {
            return;
        }
    }
    srColorSurfaceIFace* surface = 0;
    if ((operation == ACCUM_LOAD || operation == ACCUM_ACCUMULATE || operation == ACCUM_RETURN) &&
        (surface = lockBuffer()) == 0) {
        setError(ERROR_BUFFER_LOCK_FAILED);
        return;
    }
    AccumPixel* row = accum_buffer + getWidth() * state.scissor.top +
                      state.scissor.left;
    /* reinterpret-ok: the accum row scratch is raw dword storage reused as
       an ARGB pixel row. */
    srARGB* pixels = reinterpret_cast<srARGB*>(accum_scratch);
    if ((srCore.getTimer()->m_cpu_features & (1UL << srTimer::CPU_FEATURE_MMX)) != 0) {
        long scale16 = (long)(scale * (operation == ACCUM_MULTIPLY ? 32767.0 : 65536.0));
        switch (operation) {
        case ACCUM_LOAD: {
            for (long y = 0; y < height; y++) {
                surface->getPixelRow(
                    reinterpret_cast<unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
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
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
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
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
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
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
                for (long x = 0; x < width; x++) {
                    row[x].red = row[x].red + table[pixels[x].blue];
                    row[x].green = row[x].green + table[pixels[x].green];
                    row[x].blue = row[x].blue + table[pixels[x].red];
                    row[x].alpha = row[x].alpha + table[pixels[x].alpha];
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
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
                for (long x = 0; x < width; x++) {
                    row[x].red = table[pixels[x].blue];
                    row[x].green = table[pixels[x].green];
                    row[x].blue = table[pixels[x].red];
                    row[x].alpha = table[pixels[x].alpha];
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_MULTIPLY: {
            for (long y = 0; y < height; y++) {
                for (long x = 0; x < width; x++) {
                    row[x].red = row[x].red + addend;
                    row[x].green = row[x].green + addend;
                    row[x].blue = row[x].blue + addend;
                    row[x].alpha = row[x].alpha + addend;
                }
                row += getWidth();
            }
            break;
        }
        case ACCUM_ADD: {
            for (long y = 0; y < height; y++) {
                for (long x = 0; x < width; x++) {
                    row[x].red = (short)srFloatToInt(row[x].red * scale);
                    row[x].green = (short)srFloatToInt(row[x].green * scale);
                    row[x].blue = (short)srFloatToInt(row[x].blue * scale);
                    row[x].alpha = (short)srFloatToInt(row[x].alpha * scale);
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
                    pixels[x].blue = table[((unsigned short)row[x].red) >> 7];
                    pixels[x].green = table[((unsigned short)row[x].green) >> 7];
                    pixels[x].red = table[((unsigned short)row[x].blue) >> 7];
                    pixels[x].alpha = table[((unsigned short)row[x].alpha) >> 7];
                }
                surface->setPixelRow(
                    reinterpret_cast<const unsigned long*>(
                        pixels), // reinterpret-ok: ARGB row buffer through the dword pixel-row ABI
                    state.scissor.top + y, state.scissor.left,
                    state.scissor.right);
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
