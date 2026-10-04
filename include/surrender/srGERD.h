#pragma once

#include "srArray.h"
#include "srHash.h"
#include "srStringTable.h"
#include "srTexture.h"
#include "srTypeRegistry.h"
#include "srVertexPipe.h"
#include "srMath.h"
#include "srShader.h"
#include "srFlags.h"

#include "srRendererDefs.h"
#include "srDD.h"
#include "srTriMeshPipeline.h"
#include "srColorSurfaceIFace.h"
#include "srPtr.h"
#include "srARGB.h"
class srColorSurface;
class srCriticalSection;
class srDebugDD;
class srModelInstance;
class srPalette;
class srVertexProcessor;
struct srVertexArray;

/* Retail exports private helpers and copy construction. The reconstruction
   uses class-level export; original annotation and copy spelling are unresolved. */
// VTABLE: SURRENDER 0x100766B0 srGERD
#if defined(SURRENDER_BUILD)
class __declspec(dllexport) srGERD : public srRuntimeClass {
#else
class SR_DLL_IMPORT srGERD : public srRuntimeClass {
#endif
public:
    struct Pick {
        /* Normalized pick point the caller fills: x and y are the cursor's
           viewport-space coordinates, z is the fixed 1.0 far value. */
        srVector3T<float> position_00;
        srModelInstance* selected_model;
        unsigned long value_10;
    };

    struct ClipPlanes {
        srVector4T<float> planes_000[32];
        unsigned long mask_200;
        unsigned long value_204;
    };

    /* pushEnvironment/popEnvironment record: {minimum, maximum, scale,
       inverse_scale}. Unlike the scalar environment fields this element is
       POD: the constructor emits no __ehvector_ctor over the 16-entry stack
       the way it does for every srVector4T<float> array. */
    struct Environment {
        float x;
        float y;
        float z;
        float w;
    };

    /* ortho/frustum take this packed six-double box. */
    struct Frustum {
        double left;
        double right;
        double bottom;
        double top;
        double near_plane;
        double far_plane;
    };
    class Renderer {
    public:
        struct TriInput {
            unsigned long triangle_count_00;
            unsigned long record_count;
            unsigned long vertex_count;
            const unsigned long* indices_0c;
            const srVector3i* triangles;
            const unsigned long* vertices_14;
            const srTriMeshPipeline::Pass* passes;
            int position_is_float3;
            const srMatrix4T<float>* project_clip_near;
            float sort_bias;
        };

        /* createRenderer packs this record on the stack for the ctor
           (0x10024900, size 0xe4), which stores it verbatim at
           +0xd4..+0xe0. */
        struct Parameters {
            srGERD* gerd;
            long sorted;
            long batch_limit;
            unsigned long texture_stages;
        };

        /* +0x18/+0x20: per-stage scratch the DD dispatch at 0x10026360
           repacks into interleaved {st,q} elements when the driver wants
           combined texture-coordinate+w streams. */
        struct TexCoordQ {
            srVector2T<float> st;
            float q;
        };
        static_assert(sizeof(TexCoordQ) == 0xc, "TexCoordQ_must_be_0xc");

        /* intern() (0x10024280) hashes and compares the first three words;
           the interned record additionally carries a class derived from the
           shader's DSTBLEND field (ZERO -> 0, SRC_ALPHA pair -> 1, ONE -> 2,
           SRC_COLOR pair -> 3). */
        struct TextureSetKey {
            srTextureIFace* texture0;
            srTextureIFace* texture1;
            srShader shader_08;

            bool operator==(const TextureSetKey& other) const
            {
                return texture0 == other.texture0 && texture1 == other.texture1 &&
                       shader_08.value == other.shader_08.value;
            }
            bool operator!=(const TextureSetKey& other) const
            {
                return !(*this == other);
            }
        };
        struct TextureSet {
            srTextureIFace* texture0;
            srTextureIFace* texture1;
            srShader shader_08;
            unsigned long blend;
        };
        /* +0x44: texture-set interning cache. The map's value is the index
           into sets; reset() runs the map's Clear() and empties the
           record array. */
        struct TextureSetCache {
            srHashTable<TextureSetKey, unsigned long>* map_00;
            srArray<TextureSet> sets;
            unsigned long count_0c;

            /* Retail's constructor emission allocates the map after the
               record array and count are zeroed. */
            TextureSetCache() : count_0c(0)
            {
                map_00 = new srHashTable<TextureSetKey, unsigned long>;
            }
            /* ~Renderer inlines this sequence as map->Clear(),
               sets.release(), count_0c = 0, delete map_00 followed by the
               memberwise ~sets. */
            ~TextureSetCache()
            {
                clear();
                delete map_00;
            }
            /* Renderer::reset inlines the same triple: clear the interning
               map, drop the record array, reset the count. */
            void clear()
            {
                map_00->Clear();
                sets.release();
                count_0c = 0;
            }
            unsigned long intern(const TextureSetKey& key);
        };
        /* Write pointers alloc() (0x10024460) returns for the reserved
           triangle range. */
        struct IndexWrite {
            srVector3i* triangles_00;
            unsigned long* texture_set_04;
            unsigned long* sort_key;
            unsigned long* aux;
        };
        /* +0x54: accumulated primitive work. alloc() reserves count entries
           plus 0x40 headroom across all four streams and returns the write
           pointers; reset() (0x10024620) always clears the count and only
           frees when asked. */
        struct IndexBatch {
            srArray<srVector3i> triangles_00;
            srArray<unsigned long> texture_set_08;
            srArray<unsigned long> sort_key_10;
            srArray<unsigned long> aux;
            unsigned long count_20;

            /* Retail's constructor emission calls the reserve form with 0
               on the three operator-new arrays. */
            IndexBatch() : texture_set_08(0), sort_key_10(0), aux(0), count_20(0) {}
            void alloc(IndexWrite& write, unsigned long count);
            void reset(int release);
        };
        /* +0x78: accumulated vertex streams. alloc() (0x10024680) grows all
           streams, default-fills the new range (positions {0,0,0,1}, st
           {0,0}, q 1.0, the rest zeroed/uninitialized) and bind()
           (0x10027cf0) points an srVertexArray at the reserved range. The
           +0x00/+0x08/+0x10 streams bind to diffuse/specular/eye locations
           in that order. */
        struct VertexArrays {
            srArray<srVector4T<float> > diffuse;
            srArray<srVector4T<float> > specular_08;
            srArray<srVector4T<float> > positions_10;
            srArray<srVector2T<float> > st[2];
            srArray<float> q[2];
            srArray<unsigned char> packed_38;
            /* isBatchFull compares this signed against batch_limit. */
            long count_40;
            unsigned long capacity_44;

            /* Retail's constructor emission calls the reserve form with 0
               on the vec4 streams and the packed byte array. */
            VertexArrays()
                : diffuse(0), specular_08(0), positions_10(0), packed_38(0), count_40(0),
                  capacity_44(0)
            {
            }
            void alloc(srVertexArray& arrays, unsigned long count);
            void bind(srVertexArray& arrays, unsigned long base);
        };

        Renderer(const Parameters& parameters);
        void allocVertexArray(srVertexArray& arrays, unsigned long count);
        /* intern the pass's {texture0,texture1,shader} key into
           texture_set, folding per-vertex texture/shader table transitions
           into the output ids. */
        void assignTextureSets(unsigned long* texture_set, const unsigned long* indices,
                               unsigned long count, const srTriMeshPipeline::Pass* pass);
        /* Retail 0x10024DE0: while a vertex range is reserved
           (first_vertex != -1), give count accumulated vertices back. */
        void rewindVertexArray(unsigned long count);
        /* expands/dedups the input triangles into the index and
           vertex batches, per record. */
        void expandTriangles(const TriInput& input, int sorted);
        /* transforms the reserved position range by the input's
           matrix in 0x80-vertex chunks (choosing ortho/perspective/generic by
           the matrix's zero pattern), derives per-vertex clip flags, and
           replicates the chunk across the remaining records. */
        void transformVertices(const TriInput& input, unsigned char* clip_flags);
        void render(const TriInput& input);
        /* the accumulated batch count passed the limit. Only
           immediate (non-sorted) renderers report full. */
        int isBatchFull() const;
        /* submit the accumulated batch through the DD. */
        void submit();
        /* the immediate (sorted == 0) and
           sorted draw paths over the accumulated index batch. */
        void drawImmediate();
        void drawSorted();
        /* point the draw state at texture set `index`,
           updating each of texture0/texture1/shader only on change. */
        void bindTextureSet(unsigned long index);
        /* repack the per-stage stq scratch streams and program
           the DD vertex arrays for the bound batch. */
        void programVertexArrays(srVertexArray* arrays, unsigned long count);
        /* discard accumulated state; nonzero also releases
           the backing arrays. */
        void reset(int release_buffers);
        /* resetStatistics zeroes the +0x28 stat block; getStatistics copies
           its seven counters out for srGERD::getStatistics. */
        void resetStatistics();
        void getStatistics(unsigned long* statistics);

        /* The checked-free srHeapBuffer family, not srArray: ~Renderer
           null-checks before freeing these streams. bytes grows by 1-byte
           elements, dwords and remap by 4-byte elements (the ensure
           emissions at 0x100271D0/0x10027280 multiply by the element size). */
        srHeapBuffer<unsigned char> bytes;
        srHeapBuffer<unsigned long> dwords;
        /* render()'s per-corner dedup scratch (six slots per triangle). */
        srHeapBuffer<unsigned long> remap;
        srHeapBuffer<TexCoordQ> stq[2];
        /* memset for 0x1c bytes in the ctor; submit() (0x100266E0) bumps
           [4] per call and accumulates the vertex count into [5] and the
           index-batch count into [6]. */
        unsigned long statistics[7];
        TextureSetCache texture_sets;
        IndexBatch indices;
        VertexArrays vertices;
        /* allocVertexArray() snapshots the vertex count here so render() can
           offset indices into the reserved range. */
        long first_vertex;
        /* Bound draw state, refreshed per texture set in the immediate and
           sorted paths. shader re-defaults in the ctor body. */
        srTextureIFace* texture0;
        srTextureIFace* texture1;
        srShader shader;
        unsigned long clip_state;
        srGERD* gerd;
        /* lockRenderer matches this against the sorted-mode enable bit;
           flushSort flushes entries where it is 1, flushImmediateRenderers
           where it is 0. */
        long sorted;
        long batch_limit;
        unsigned long texture_stages;
    };
    static_assert(sizeof(Renderer) == 0xe4, "srGERD_Renderer_must_be_0xe4");
    static_assert(sizeof(Renderer::Parameters) == 0x10, "srGERD_Renderer_Parameters_must_be_0x10");
    static_assert(sizeof(Renderer::TextureSetKey) == 0x0c,
                  "srGERD_Renderer_TextureSetKey_must_be_0x0c");
    static_assert(sizeof(Renderer::IndexWrite) == 0x10, "srGERD_Renderer_IndexWrite_must_be_0x10");

    /* errStrings literal order at 0x100993A4. */
    enum e_error {
        ERROR_NONE = 0,
        ERROR_INVALID_ENUM = 1,
        ERROR_INVALID_VALUE = 2,
        ERROR_WINDOW_OPEN_FAILED = 3,
        ERROR_WINDOW_NOT_OPEN = 4,
        ERROR_BUFFER_LOCK_FAILED = 5,
        ERROR_INVALID_WHANDLE = 6,
        ERROR_SHARED_CONTEXT = 7,
        ERROR_CONTEXT_CREATION_FAILED = 8,
        ERROR_NO_CONTEXT = 9
    };
    enum e_closeHint {};
    /* clear() maps COLOR/DEPTH/STENCIL onto srDD::e_buffer and services
       ACCUM itself through accumClear. */
    enum e_buffer { BUFFER_COLOR = 1, BUFFER_DEPTH = 2, BUFFER_ACCUM = 4, BUFFER_STENCIL = 8 };
    /* dump prints none (blit)/one (double)/two (triple) for values 1/2/3. */
    enum e_backBuffer { BACKBUFFER_NONE = 1, BACKBUFFER_ONE = 2, BACKBUFFER_TWO = 3 };
    /* applyRenderState translates each value to the same srDD mode. */
    enum e_polygonMode { POLYGON_POINT = 0, POLYGON_LINE = 1, POLYGON_FILL = 2 };
    /* Wizardry uses 0 immediately before model-view loads and 1 immediately
       before identity+ortho. OpenGL srDD talks GL_MODELVIEW (0x1700) and
       GL_PROJECTION (0x1701) for those two stacks. */
    enum e_matrixMode { MATRIX_MODELVIEW = 0, MATRIX_PROJECTION = 1 };
    enum e_antiAlias { ANTIALIAS_NONE = 0 };
    /* srClipPlane::process passes its clip_type_ through unchanged; Wizardry
       always writes 0. */
    enum e_clipMode { CLIPMODE_POSITIONAL_0 = 0 };
    /* applyDrawStateChanges hands 0 to srDD as back-face culling and 1 as
       front-face culling (each flipped by the winding) and 2 as srDD::CULL_NONE;
       the constructor defaults to 0. */
    enum e_cullMode { CULL_BACK = 0, CULL_FRONT = 1, CULL_NONE = 2 };
    /* toggle XORs 1<<option into +0x20. Option 0 also dirties dirty_24 bit 0
       (Wizardry render-option 5). Option 1 selects sorted rendering. Option 4 is
       SetRendererAutoFlipEnabled. Option 5 wraps/unwraps srDebugDD. GERD dump
       has no enable-name table. */
    /* The constructor sets bits 4 and 6 on enable_flags; ~srGERD toggles
       bit 5, which openWindow's comment identifies as the debug-DD wrap. */
    enum e_enable {
        ENABLE_POSITIONAL_0 = 0,
        ENABLE_SORTED_RENDERING = 1,
        ENABLE_AUTO_FLIP = 4,
        ENABLE_DEBUG_DD = 5,
        ENABLE_CLEAR_ON_OPEN = 6
    };
    enum e_winding { WINDING_POSITIONAL_0 = 0, WINDING_POSITIONAL_1 = 1 };
    enum e_visibility { VISIBILITY_OUTSIDE = 0 };
    /* dump(stream, flags) section selectors: bit 0 driver/device info plus
       the window block, bit 1 the texture cache, bit 3 the statistics
       snapshot, bit 5 the srDebugDD call profile. dump(stream) passes 0x3f. */
    enum e_info {
        INFO_DEVICE = 0x1,
        INFO_TEXTURE_CACHE = 0x2,
        INFO_STATISTICS = 0x8,
        INFO_DEBUG_DD = 0x20,
        INFO_ALL = 0x3f
    };
    enum e_hint {};
    enum e_hintMode {};
    /* accumulate() switch evidence: 0 loads the frame buffer into the accum
       buffer, 1 accumulates, 4 returns the accum buffer to the frame
       buffer; 2/3 multiply/add through the accum path. */
    enum e_accum {
        ACCUM_LOAD = 0,
        ACCUM_ACCUMULATE = 1,
        ACCUM_MULTIPLY = 2,
        ACCUM_ADD = 3,
        ACCUM_RETURN = 4
    };
    enum e_depthBuffer {};
    /* getDisplayModeInfo output triple. */
    struct DisplayModeInfo {
        long width_00;
        long height_04;
        long depth_08;
    };
    /* getTextureInfo output: the device pixel format plus the device's
       width/height and last mip level. */
    struct TextureInfo {
        srPixelConvert::PixelFormat pixel_format;
        unsigned long width_14;
        unsigned long height_18;
        unsigned long last_level;
    };
    /* accumulate()'s signed 16-bit accum-buffer pixel. */
    struct AccumPixel {
        short red_00;
        short green_02;
        short blue_04;
        short alpha_06;
    };

    /* Not in the consumer import table and no client emission exists in
       retail Wiz8 (no "srGERD" literal): the consumer never references it,
       so the inherited class-wide import decoration is unobservable. */
    static const char* sGetClassName();
    static srRegistry::ClassNode* sGetClassNode();

    srGERD(srDD* device, void* module, const char* device_name);
    virtual ~srGERD() override;

    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual void dump(std::ostream& stream) override;
    void dump(std::ostream& stream, const srFlags<e_info>& info);
    static srGERD* loadDevice(srStringTable& devices, unsigned long index);
    static srGERD* loadDevice(const char* name, const char* path, unsigned long device);
    static srGERD* loadDeviceWithFileName(const char* filename, unsigned long device);
    static srGERD* getFirst();
    srGERD* getNext() const;
    /* Open-device list used by srTexture::invalidateFrameHandle; the links
       live in the object (getNextOpen reads +0x3c). */
    static srGERD* getFirstOpen();
    srGERD* getNextOpen() const;
    e_error createContext(unsigned long window);
    int isContextCreated() const;
    void deleteContext();
    long getDisplayMode(unsigned long width, unsigned long height, unsigned long depth) const;
    /* openWindowInternal parameter block: the current client size and the
       requested backbuffer size plus the display-mode index (-1 windowed). */
    struct OpenInfo {
        long window_width;
        long window_height;
        long width_08;
        long height_0c;
        long display_mode_10;
    };
    /* Number of back buffers in the swap chain; openWindowInternal stores
       the device result at +0x38c (1, 2 or 3). */
    e_error openWindow();
    e_error openWindow(long width, long height);
    e_error openWindow(long mode);
    void closeWindow(e_closeHint hint);
    e_backBuffer getBackBufferType() const;
    int isWindowOpen() const;
    int isFullScreen() const;
    unsigned long getWindowHandle() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void flushImmediateRenderers();
    void clear(const srFlags<e_buffer>& buffers);
    long getHeight() const;
    long getWidth() const;
    void resetStatistics();
    /* getStatistics buffer. The render probes return the double at +0x10
       through ftol; the 0x00427460 debug overlay prints the dword counters at
       +0x08/+0x0c (the TT pair halves), +0x20 (PO), +0x24 (VO), +0x34 (PI),
       +0x3c (VI), +0x4c (TC) and +0x68 (DD). */
    struct Statistics {
        /* Epoch written by resetStatistics; getStatistics returns the
           seconds elapsed since then. */
        double elapsed;
        /* Device texture byte count mirrored from srDD::Statistics +0x00 as
           two dwords; dump reinterprets the pair as a double for the
           "DD Texture data transfer (Mb/s)" line. */
        unsigned long value_08;
        unsigned long value_0c;
        double value_10;
        unsigned long value_18;
        unsigned long value_1c;
        unsigned long value_20;
        unsigned long value_24;
        unsigned long value_28;
        /* Frames presented: flipFrame increments once per call. */
        unsigned long frames;
        unsigned long value_30;
        unsigned long value_34;
        /* getStatistics accumulates this only for sorted renderers. */
        unsigned long value_38;
        unsigned long value_3c;
        /* applyViewStateChanges increments this counter on every apply. */
        unsigned long view_state_applies;
        /* applyDrawStateChanges increments this counter on every apply. */
        unsigned long draw_state_applies;
        /* applyFrameStateChanges increments this counter on every apply. */
        unsigned long frame_state_count;
        /* Texture binds counted by changeTexture after the stage's bound
           texture actually changes; the debug overlay prints it as "TC". */
        unsigned long texture_binds;
        /* Texture-parameter updates counted by setTextureParameters. */
        unsigned long texture_parameter_sets;
        /* createNewTexture increments this created-texture count. */
        unsigned long textures_created;
        /* Palette binds counted when a changed texture carries a new palette. */
        unsigned long palette_binds;
        /* setShader calls counted by applyDrawStateChanges. */
        unsigned long shader_sets;
        /* drawArrays/drawElements increment this draw-call count. */
        unsigned long draw_calls;
        unsigned long value_64;
        unsigned long value_68;
        /* testBoundingSphere call count / visible-result count. */
        unsigned long sphere_tests;
        unsigned long sphere_visible;
        /* testBoundingBox call count / visible-result count. */
        unsigned long box_tests;
        unsigned long box_visible;
        /* classifyMatrix call count. */
        unsigned long matrix_classifications;
    };
    void getStatistics(Statistics& statistics);
    unsigned long getTextureCacheUsed() const;
    unsigned long getResidentTextureMemUsed() const;
    void setClearColor(const srVector4T<float>& color);
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(double depth);
    void setAmbientLight(float red, float green, float blue, float alpha);
    void setAmbientLight(const srVector4T<float>& light);
    /* Retail 0x1001C440: the scene's process installs its v3 ambient through
       this overload. */
    void setAmbientLight(const srVector3T<float>& light);
    void setFogColor(const srVector3T<float>& color);
    /* Retail 0x1001C6B0: the scene restores the saved v4 fog color through
       this overload. */
    void setFogColor(const srVector4T<float>& color);
    void setScissor(unsigned long x, unsigned long y, unsigned long width, unsigned long height);
    /* Dirty-rectangle pair handed to flipFrame in {x,y,width,height} form;
       GERD converts each to srDD::Scissor left/top/right/bottom. */
    struct Rectangle {
        long x, y, width, height;
    };
    void flipFrame();
    void flipFrame(const Rectangle* first, const Rectangle* second, unsigned long count);
    void setTextureReduction(long reduction);
    void setViewPort(unsigned long x, unsigned long y, unsigned long width, unsigned long height);
    void matrixMode(e_matrixMode mode);
    e_matrixMode getMatrixMode() const;
    void getMatrix(srMatrix4T<float>& matrix);
    void getMatrix(srMatrix4T<double>& matrix);
    void getMatrix(e_matrixMode mode, srMatrix4T<float>& matrix);
    void getMatrix(e_matrixMode mode, srMatrix4T<double>& matrix);
    void getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                           const srVector3T<float>& object_center, float object_radius);
    /* srIlluminator::process stores the eye-space camera position for the
       current model view through this export. */
    srVector4T<float> getEyeSpaceLocation(const srVector3T<float>& object_location);
    void pushVertexProcessor(srVertexProcessor& processor);
    void popVertexProcessor();
    void getInverseModelViewMatrix(srMatrix4T<float>& matrix);
    void getClipPlanes(ClipPlanes& planes);
    void pushClipPlane(const srVector4T<float>& plane, e_clipMode mode);
    void popClipPlane();
    void getProjectClipNearMatrix(srMatrix4T<float>& matrix);
    void getNormalMatrix(srMatrix4T<float>& matrix);
    srMatrix4T<float>::e_scaleType getModelViewScaleType();

    /* Retail 0x10021380. */
    float getMaxModelViewScale();
    e_cullMode getCullMode() const;
    e_winding getWinding() const;
    void setWinding(e_winding winding);
    Renderer* lockRenderer();
    void unlockRenderer(Renderer* renderer, int submit);
    srColorSurfaceIFace* lockBuffer();
    void unlockBuffer();
    unsigned long getVertexProcessorCount() const;
    void getVertexProcessors(srVertexProcessor** processors) const;
    void getAmbientLight(srVector4T<float>& light);
    /* Retail 0x1001C680: the scene saves the current v4 fog color through
       this overload. */
    void getFogColor(srVector4T<float>& color) const;
    void getEnvironmentRange(float& minimum, float& maximum) const;
    void getEnvironmentScaleFactor(float& scale, float& inverse_scale);
    unsigned long getExclusionMask() const;
    void setExclusionMask(unsigned long mask);
    /* The pipeline's single-stage mask branch inlines this exported getter. */
    // FUNCTION: SURRENDER 0x1001BB70 SYMBOL
    // ?getMaxTextureStages@srGERD@@QBEJXZ
    long getMaxTextureStages() const
    {
        return device.info.max_texture_stages;
    }
    // FUNCTION: SURRENDER 0x1001CF10 SYMBOL
    // ?getMaxPickStackDepth@srGERD@@QBEJXZ
    long getMaxPickStackDepth() const
    {
        return 0x20;
    }
    // FUNCTION: SURRENDER 0x1001CF20 SYMBOL
    // ?getMaxModelviewStackDepth@srGERD@@QBEJXZ
    long getMaxModelviewStackDepth() const
    {
        return 0x20;
    }
    // FUNCTION: SURRENDER 0x1001CF30 SYMBOL
    // ?getMaxProjectionStackDepth@srGERD@@QBEJXZ
    long getMaxProjectionStackDepth() const
    {
        return 0x20;
    }
    void pushMatrix();
    void pushMultMatrix(const srMatrix4x3T<float>& matrix);
    void popMatrix();
    void pushEnable();
    void popEnable();
    void loadIdentity();
    void multMatrix(const srMatrix4T<float>& matrix);
    void multMatrix(const srMatrix4T<double>& matrix);
    void perspective(double fov_y, double aspect, double near_plane, double far_plane);
    void rotate(double angle, const srVector3T<float>& axis);
    void rotate(double angle, const srVector3T<double>& axis);
    void rotate(double angle, double x, double y, double z);
    void scale(double x, double y, double z);
    void scale(const srVector3T<float>& factors);
    void scale(const srVector3T<double>& factors);
    void scale(double factor);
    void translate(const srVector3T<float>& offset);
    void translate(const srVector3T<double>& offset);
    void translate(double x, double y, double z);
    e_visibility testBoundingSphere(const srVector3T<float>& center, float radius);
    e_visibility testBoundingBox(const srVector3T<float>& minimum,
                                 const srVector3T<float>& maximum);
    void setPickKey(unsigned long key);
    /* Retail 0x1001DB20. */
    unsigned long getPickKey() const;
    void ortho(double left, double right, double bottom, double top, double near_plane,
               double far_plane);
    void ortho(const Frustum& frustum);
    void frustum(double left, double right, double bottom, double top, double near_plane,
                 double far_plane);
    void frustum(const Frustum& frustum);
    void loadMatrix(const srMatrix4T<double>& matrix);
    void loadMatrix(const srMatrix4T<float>& matrix);
    void loadMatrix(const srMatrix3T<double>& matrix);
    void loadMatrix(const srMatrix3T<float>& matrix);
    void getScissor(unsigned long& x, unsigned long& y, unsigned long& width,
                    unsigned long& height) const;
    void getViewPort(unsigned long& x, unsigned long& y, unsigned long& width,
                     unsigned long& height) const;
    void pushEnvironment();
    void popEnvironment();
    void setEnvironmentRange(float minimum, float maximum);
    void setEnvironmentScaleFactor(float scale, float inverse_scale);
    void setClipState(srFlags<srRendererDefs::e_clip> state);
    void setAntiAlias(e_antiAlias mode);
    void setTexture(srTextureIFace* texture, unsigned long layer);
    void setTextureDefaultCorrection(srTextureIFace::e_correction correction);
    void setTextureDefaultCompression(srTextureIFace::e_compression compression);
    void setTextureDefaultMagFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMinFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap);
    void setTextureSubImage(srTextureIFace* texture, long mipmap, long x, long y, long width,
                            long height);
    void drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count);
    void drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                      srRendererDefs::e_indexType type, const void* indices);
    void popPick(Pick& pick);
    void pushPick(const Pick& pick);
    void toggle(e_enable option);
    void invalidateResidentTextures();
    void invalidateResidentTexture(srTextureIFace* texture);
    void invalidateTextureCache();
    void invalidateTexture(srTextureIFace* texture);
    void invalidateTextureByFrameHandle(unsigned long handle);
    unsigned long getTextureCacheSize() const;
    void setTextureCacheSize(unsigned long bytes);
    void setSwapInterval(unsigned long interval);
    long getPolygonOffset() const;
    void setPolygonOffset(long offset);

    /* Retail 0x10020DD0. */
    int isBufferLocked();
    /* Retail 0x1001CF60/0x1001CF70: the +0x30/+0x38 list links. */
    srGERD* getPrev() const;
    srGERD* getPrevOpen() const;
    /* Retail 0x1001B400/0x1001B420: walks the global device list. */
    static long getGERDCount();
    static srGERD* getGERD(unsigned long index);
    /* Retail 0x10018BC0/0x10018C70: scan provider libraries for devices. */
    static void loadDevices(const char* path);
    static void scanDevices(const char* path, srStringTable& devices);
    /* Retail 0x10018E60: releases every GERD on the global list. */
    static void releaseAll();
    /* Retail 0x1001AFF0: static error-string table lookup. */
    const char* getErrorString(e_error error);
    e_error getError();
    int isFlipped() const;
    const char* getApiVersion() const;
    /* device.info.text[0..8] accessors: initDDInfo seeds the nine 0x40-byte
       identity strings, getInfo's driver fills them. */
    const char* getDeviceName() const;
    const char* getDeviceVendor() const;
    const char* getDevicePlatform() const;
    const char* getDriverName() const;
    const char* getDriverVendor() const;
    const char* getDriverVersion() const;
    const char* getHardwareChipset() const;
    const char* getHardwareName() const;
    const char* getHardwareVendor() const;
    srDD::e_driverID getDriverID() const;
    srDD::e_hardwareID getHardwareID() const;
    e_depthBuffer getDepthBufferType() const;
    unsigned long getSwapInterval() const;
    void getTextureFormat(unsigned long index, srPixelConvert::PixelFormat& format) const;
    unsigned long getTextureFormatCount() const;
    void getDisplayModeInfo(long index, DisplayModeInfo& info) const;
    unsigned long getDisplayModeCount() const;
    e_hintMode getHint(e_hint hint) const;
    void setHint(e_hint hint, e_hintMode mode);
    void getGamma(srVector3T<float>& gamma) const;
    e_antiAlias getAntiAlias() const;
    void extCommand(unsigned long command, void* data, unsigned long size);
    void disable(e_enable option);
    void enable(e_enable option);
    srShader getShader() const;
    /* Retail 0x1001BBB0: submits one triangle through drawElements. */
    void drawTriangle(const srVector3i& triangle);
    void setDepthRange(double minimum, double maximum);
    void getDepthRange(double& minimum, double& maximum) const;
    void setPolygonMode(e_polygonMode mode);
    e_polygonMode getPolygonMode() const;
    void getClearColor(srVector4T<float>& color) const;
    void getClearAccum(srVector4T<float>& color) const;
    void setClearAccum(const srVector4T<float>& color);
    void setClearAccum(float red, float green, float blue, float alpha);
    double getClearDepth() const;
    void setClearStencil(unsigned long stencil);
    unsigned long getClearStencil() const;
    long getAccumAlphaBits() const;
    long getAccumRedBits() const;
    long getAccumGreenBits() const;
    long getAccumBlueBits() const;
    void accumulate(e_accum operation, float scale);
    int isTextureCached(srTextureIFace* texture) const;
    int isTextureResident(srTextureIFace* texture) const;
    int getTextureInfo(srTextureIFace* texture, TextureInfo& info);
    srTextureIFace::e_compression getTextureDefaultCompression() const;
    srTextureIFace::e_correction getTextureDefaultCorrection() const;
    srTextureIFace::e_filter getTextureDefaultMagFilter() const;
    srTextureIFace::e_filter getTextureDefaultMinFilter() const;
    srTextureIFace::e_mipmap getTextureDefaultMipmap() const;
    long getTextureReduction() const;
    void invalidateResidentPalette(srPalette* palette);
    void setGlobalPalette(const srPalette& palette);
    long getMaxTextureWidth() const;
    long getMaxTextureHeight() const;
    long getMaxTextureAspectRatio() const;
    static unsigned long sGetClassID();
    static void dumpDeviceList(std::ostream& stream);

    /* These ordinary methods are header-visible in Wiz8 call sites even
       though SR.DLL also exports out-of-line copies. */
    // FUNCTION: SURRENDER 0x1001BB80 SYMBOL
    // ?isPickStackEmpty@srGERD@@QBEHXZ
    int isPickStackEmpty() const
    {
        return pick.pick_depth == 0;
    }

    // FUNCTION: SURRENDER 0x1001BAE0 SYMBOL
    // ?isEnabled@srGERD@@QBEHW4e_enable@1@@Z
    int isEnabled(e_enable option) const
    {
        return (enable_flags.value & (1UL << option)) != 0;
    }

    // FUNCTION: SURRENDER 0x1001BB90 SYMBOL
    // ?setCullMode@srGERD@@QAEXW4e_cullMode@1@@Z
    void setCullMode(e_cullMode mode)
    {
        if (state.cull_mode != mode) {
            state.cull_mode = mode;
            dirty |= 0x2000;
        }
    }

    /* Header inline that also emits the standalone retail 0x1001BB40 copy;
       drawSorted calls the emission while drawImmediate inlines it. */
    // FUNCTION: SURRENDER 0x1001BB40 SYMBOL
    // ?setShader@srGERD@@QAEXABVsrShader@@@Z
    void setShader(const srShader& shader)
    {
        if (this->shader.value != shader.value) {
            this->shader = shader;
            dirty |= 0x1000;
        }
    }

    // FUNCTION: SURRENDER 0x1001BEF0 SYMBOL
    // ?setVertexArrayMask@srGERD@@QAEXV?$srFlags@W4e_vertexArray@srRendererDefs@@@@@Z
    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask)
    {
        vertex_arrays.mask = mask;
        vertex_arrays_dirty |= 1;
    }

    // FUNCTION: SURRENDER 0x1001BEE0 SYMBOL
    // ?getVertexArrayMask@srGERD@@QBE?AV?$srFlags@W4e_vertexArray@srRendererDefs@@@@XZ
    srFlags<srRendererDefs::e_vertexArray> getVertexArrayMask() const
    {
        return vertex_arrays.mask;
    }

    /* Retail emits each specialized setter as its own export writing the
       indexed slot directly; setDataPtr is the general entry point the
       renderer's bindVertexArrays calls. */
    void setDiffusePointer(long components, srRendererDefs::e_type type, unsigned long stride,
                           const void* values);
    void setSpecularPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                            const void* values);
    void setFogPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                       const void* values);

    // FUNCTION: SURRENDER 0x1001BFD0 SYMBOL
    // ?setTexCoordPointer@srGERD@@QAEXJW4e_type@srRendererDefs@@KPBXK@Z
    void setTexCoordPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                            const void* values, unsigned long layer)
    {
        unsigned long index = layer + 4;
        vertex_arrays.components[index] = components;
        vertex_arrays.types[index] = type;
        vertex_arrays.strides[index] = stride;
        vertex_arrays.arrays[index] = values;
        vertex_arrays_dirty |= 1;
    }

    // FUNCTION: SURRENDER 0x1001BE90 SYMBOL
    // ?setVertexPointer@srGERD@@QAEXJW4e_type@srRendererDefs@@KPBXJ@Z
    void setVertexPointer(long primitive, srRendererDefs::e_type type, unsigned long stride,
                          const void* values, long count)
    {
        vertex_arrays.count = count < 0 ? 0 : count;
        vertex_arrays.components[0] = primitive;
        vertex_arrays.types[0] = type;
        vertex_arrays.strides[0] = stride;
        vertex_arrays.arrays[0] = values;
        vertex_arrays_dirty |= 1;
    }

private:
    /* Pooled device-texture record, 0xa8 bytes. allocTexture links chunks
       through +0x00, keeps live/deleted lists in {prev, next_04} and the
       texture-interface id at +0x08 as the hash key. The embedded
       srDD::Texture at +0x2c is handed to the device. */
    struct Texture {
        Texture* prev;
        Texture* next_04;
        unsigned long id_08;
        /* evaluateTexturePixelFormat copies the matched device format here. */
        srPixelConvert::PixelFormat pixel_format;
        void* surface_data;
        srPtr<srPalette> palette_24;
        char* name_28;
        srDD::Texture device_2c;
        unsigned long unknown_a4;
    };
    static_assert(sizeof(Texture) == 0xa8, "srGERD_Texture_must_be_0xa8");

    /* Renderer::render's pick path hands this batch view to
       performPickTest: indices selects triangles out of the caller's
       srVector3i stream, vertices remaps each corner to a position index,
       positions is the renderer's vec4 stream base. */
    struct PickInput {
        const unsigned long* indices_00;
        const srVector3i* triangles;
        unsigned long triangle_count;
        const unsigned long* vertices;
        const srVector4T<float>* positions_10;
        unsigned long vertex_count;
    };

    /* Retail's exported operator= is a private no-op returning *this; the
       class-level dllexport emits it even though nothing calls it. */
    srGERD& operator=(const srGERD& other);
    /* Renderer::submit and LockSurface's pixel transfers reach getDD; VC6
       does not give nested classes enclosing-member access. */
    srDD* getDD() const;

    /* The members marked dllexport below are retail exports no recovered
       caller references; without the annotation the linker garbage-collects
       the emissions and the provider exports dangle. Retail mangles them
       private. */
    static void debugWrite(const char* text);
    void fenceVertexArrays();
    void dumpTextureCache(std::ostream& stream);
    void setDataPtr(srRendererDefs::e_vertexArray index, long components,
                    srRendererDefs::e_type type, unsigned long stride, const void* values);

    /* Renderer member functions reach GERD's array-state fields directly;
       VC6 extended enclosing-class access to nested members, clang-cl
       needs the explicit grant. */
    friend class Renderer;
    /* Retail 0x1001D630: for each queued Pick, w-normalize the batch's
       positions into pick_vertices and run the edge-function
       triangle test against the pick ray. */
    void performPickTest(const PickInput& input);
    void deleteRenderers();
    /* getErrorString table: the ten e_error names followed by no terminator;
       out-of-range errors report "UNKNOWN ERROR". */
    static const char* errStrings[10];

    /* Handle-hash chain node: {next, handle, texture} at stride 0xc, proven
       by invalidateTextureByFrameHandle's walk. */
    struct TextureEntry {
        long next_00;
        unsigned long handle_04;
        Texture* texture_08;
    };
    /* Doubly-linked renderer list node proven by createRenderer's prepend
       and the lock/flush walks; +0x0c is the busy flag _lockRenderer
       raises. */
    struct RendererEntry {
        RendererEntry* prev;
        RendererEntry* next_04;
        Renderer* renderer_08;
        long busy;
    };
    void setError(e_error error);
    void resetTexture();
    void assertContext() const;
    void setMatrixDirty();
    void classifyMatrix(e_matrixMode mode);
    void applyViewStateChanges();
    void applyClipPlaneChanges();
    void applyDrawStateChanges();
    void applyFrameStateChanges();
    void checkViewStateChanges();
    void checkClipPlaneChanges();
    void checkDrawStateChanges();
    void checkFrameStateChanges();
    void checkAllStateChanges();
    void recalcScissor();
    void changeTexture(srTextureIFace* texture, unsigned long stage, int apply_parms);
    Texture* createNewTexture(srTextureIFace* texture);
    Texture* allocTexture(unsigned long id);
    void allocTextureData(Texture& texture);
    void deleteTexture(Texture& texture);
    void removeDeletedTextures();
    void invalidatePalette();
    void releaseTextureSurfaceData(Texture& texture);
    void setTextureParameters(unsigned long stage, const srTextureIFace::Parameters& parameters);
    void evaluateTextureDimensions(srDD::Texture& device,
                                   const srTextureIFace::Dimensions& dimensions);
    void evaluateTexturePixelFormat(Texture& texture, const srTextureIFace::Dimensions& dimensions);
    void releaseTextureMemory(long bytes);
    unsigned long getTextureBytesNeeded(Texture& texture) const;
    Texture* findLowestPriority();
    void invalidateTexture(Texture& texture);
    void invalidateResidentTexture(Texture& texture);
    void resetCurrentTexPointers();
    void markTextureAsDeleted(Texture& texture);
    void initTexCache();
    void closeTexCache();
    void initDDInfo();
    void initTextureFormats();
    void initDisplayModeList();
    void initGlobalPalette();
    /* Retail 0x1001CF00: +0x2d8 of the device info block. */
    unsigned long getDDAPIVersion() const;
    void initClearColors();
    void initView();
    void initLights();
    void initMatrices();
    void initTextureParameterMatrix();
    void deleteDeletedRenderers();
    e_error openWindowInternal(const OpenInfo& info);
    srDD::e_error _lockBuffer();
    srDD::e_error _unlockBuffer();
    void getPixelFormat(srPixelConvert::PixelFormat& format) const;
    void accumAlloc();
    short accumConvert(float value) const;
    void accumClear();
    void accumRelease();
    /* MMX row kernels for accumulate(); retail inlines the same instruction
       sequences inside accumulate itself. */
    static void __cdecl accumAccum_MMX(AccumPixel* accum, const srARGB* pixels, long scale,
                                       long count);
    static void __cdecl accumLoad_MMX(AccumPixel* accum, const srARGB* pixels, long scale,
                                      long count);
    static void __cdecl accumReturn_MMX(srARGB* pixels, const AccumPixel* accum, long scale,
                                        long count);
    static void __cdecl accumAdd_MMX(AccumPixel* accum, long value, long count);
    static void __cdecl accumMult_MMX(AccumPixel* accum, long value, long count);
    static void convertPixelFormat(srDD::PixelFormat& device,
                                   const srPixelConvert::PixelFormat& format);
    static void convertPixelFormat(srPixelConvert::PixelFormat& format,
                                   const srDD::PixelFormat& device);
    RendererEntry* createRenderer(int sorted);
    void flushNonBusyRenderers();
    void flushSort();
    Renderer* _lockRenderer(RendererEntry* entry);
    /* lockBuffer's 0x60-byte locked-back-buffer surface, defined in
       gerd.cpp; LockSurface's pixel transfers reach the private getDD. */
    class LockSurface;
    friend class LockSurface;

    /* The five-word texture pool at +0x2014: live-texture count, free-list
       head, the chunk-pointer array and the chunk count. srGERD's destructor
       calls release() at body level and deleteTexture reaches it when the live
       count drains; its embedded array still gets the memberwise teardown. */
    struct TexturePool {
        /* Retail's constructor helper (0x1001EEA0) is the out-of-line
           emission; it zeroes all five words (the embedded srArray
           contributes the middle two). */
        TexturePool();

        /* ~srGERD's member teardown calls this before the embedded array's
           memberwise ~srArray, so the destructor body is release(). */
        ~TexturePool()
        {
            release();
        }

        /* Frees every chunk through srHeap, releases the chunk array and
           zeroes the record; the indexed free goes through srArray's
           growing operator[] the way retail inlines it. */
        void release();

        unsigned long count_00;
        Texture* free_04;
        srArray<Texture*> chunks;
        unsigned long pool_count;
    };

    /* +0x21b0: the registered srVertexProcessor pointers plus the live count
       share one constructed record — the ctor helper zeroes all three words
       then calls release(), so the count lives inside a derived array. */
    struct VertexProcessors : public srArray<srVertexProcessor*> {
        VertexProcessors() : count_08(0)
        {
            release();
        }

        unsigned long count_08;
    };

    static srGERD* first;
    static srGERD* firstOpen;

    struct MatrixStack {
        /* The constructor emits __ehvector_ctor over state.matrix_stacks with
           this block's constructor (0x10019A00) as the element callback: it
           is a real function, not an inlined member init. */
        MatrixStack();

        srMatrix4T<float> stack[32];
        unsigned long depth_800;
    };

    /* +0x40 device record: the copy body contains a 0xD4-dword rep movsd.
       That alone does not establish the original member declaration boundaries.
       Info's and DriverInfo's declared ctors produce the constructor's
       +0x68/+0x2D0 init stores inside the inlined block construction. */
    struct Device {
        srDD* dd;
        srDebugDD* debug_dd;
        srDD* real_dd;
        /* Dynamic-library handle the constructor stores and ~srGERD passes
           to srDynamicLibrary::free. */
        void* module;
        /* Device info record handed to srDD::getInfo by initDDInfo; GERD
           reads the staging/clamp fields out of it. openWindowInternal
           clamps the requested back-buffer against its maximums. */
        srDD::Info info;
        /* getDriverInfo target; getDDAPIVersion/getDriverID/getDriverName
           (pre-context) and getApiVersion read its trailing fields. */
        srDD::DriverInfo driver_info;
        srPixelConvert::PixelFormat* texture_formats;
        long texture_format_count;
        unsigned long* display_modes;
        long display_mode_count;
        /* setHint/getHint index this by e_hint. */
        e_hintMode hints[1];
        unsigned long window;
        /* openWindowInternal memsets then struct-copies the OpenInfo record
           verbatim: windowed dims, backbuffer dims, then the display-mode
           index. isFullScreen tests display_mode_10 against -1, and
           openWindow leaves it -1 for the windowed path. */
        OpenInfo open_info;
        /* e_backBuffer result of srDD::openWindow; getBackBufferType reads
           it. */
        e_backBuffer back_buffer_type;
    };

    /* +0x390 render-state record: the copy emits one 0x4F2-dword
       rep movsd over the block and the constructor zeroes it wholesale
       through srZeroMemory(&state, 0x13C8). scissor_flags is the
       only ctor-initialised scalar; its store lands between the clip-plane
       and inverse-modelview __ehvector_ctor calls. */
    struct State {
        State() : scissor_flags(0) {}

        /* Per-mode current matrices; pushMatrix indexes by mode. */
        srMatrix4T<float> matrix_current[2];
        /* Per-mode 32-deep matrix stacks; each block ends with its depth
           counter (stride 0x804). */
        MatrixStack matrix_stacks[2];
        /* The constructor emits a single __ehvector_ctor over 32 srVector4T
           elements: entries [0..5] are the eye-space frustum planes
           maintained by applyClipPlaneChanges and [6..31] the user planes
           pushed by pushClipPlane (mask bits 6..31 of clip_mask). */
        srVector4T<float> clip_planes[32];
        /* Depth range forwarded into srDD::ViewPort by applyViewStateChanges;
           initView resets it to [0.0, 1.0] and setDepthRange clamps it. */
        double depth_min;
        double depth_max;
        srDD::Scissor scissor;
        unsigned long view_left;
        unsigned long view_top;
        unsigned long view_right;
        unsigned long view_bottom;
        e_cullMode cull_mode;
        e_winding winding;
        e_matrixMode matrix_mode;
        unsigned char unknown_12c4_[6];
        /* Per-user-plane e_clipMode bytes written by pushClipPlane. */
        unsigned char clip_modes[26];
        /* Plane-enable mask: bits 0..5 frustum, bits 6..31 user planes. */
        unsigned long clip_mask;
        /* Subset of clip_mask carrying mode-1 user planes. */
        unsigned long clip_mode1_mask;
        long clip_plane_count;
        /* Bit 1: recalcScissor marks the scissor as the full view. */
        unsigned long scissor_flags;
        srMatrix4T<float> inverse_modelview;
        srMatrix4T<float> project_clip_near;
        srMatrix4T<float> normal_matrix;
        srMatrix4T<float>::e_scaleType modelview_scale_type;
        float max_modelview_scale;
        /* classifyMatrix writes the per-mode projection-shape class here;
           the projection class at +0x13C0 feeds srDD::setProjectionMatrix. */
        srMatrix4T<float>::e_type matrix_class[2];
        unsigned char unknown_13c4_[4];
    };

    /* +0x1758 presentation record: the copy emits a 5-dword rep
       movsd over the block. */
    struct Display {
        srVector3T<float> gamma;
        unsigned long swap_interval;
        e_antiAlias antialias;
    };

    /* +0x176C pick record: the copy emits a 0xA2-dword rep movsd
       over the block; pick_depth is the ctor-initialised scalar whose
       store lands right after the pick-stack __ehvector_ctor. */
    struct PickState {
        PickState() : pick_depth(0) {}

        Pick pick_stack[32];
        unsigned long pick_depth;
        unsigned long pick_key;
    };

    /* +0x1B08 clear record: the copy emits a 0xC-dword rep movsd
       over the block. */
    struct ClearState {
        srDD::ClearValues clear_values;
        unsigned char unknown_2c_[4];
    };

    /* +0x1F5C texture-state record: the copy emits a 0x20-dword
       rep movsd over the block. setTextureParameters indexes these maps
       from the packed texture state; filter selector 4 is a valid index in
       both filter maps. */
    struct TextureState {
        unsigned long correction_map[4];
        unsigned long mag_filter_map[5];
        unsigned long min_filter_map[5];
        /* The fourth entry doubles as the current mipmap parameter written
           by setTextureDefaultMipmap. */
        unsigned long mipmap_map[4];
        unsigned long wrap_s_map[2];
        unsigned long wrap_t_map[2];
        srTextureIFace::e_correction default_correction;
        srTextureIFace::e_filter default_mag_filter;
        srTextureIFace::e_filter default_min_filter;
        srTextureIFace::e_mipmap default_mipmap;
        /* Per-type default device parameters; evaluateTexturePixelFormat
           copies entry [Dimensions::parameter_index] into
           srDD::Texture::parameter. Entry [4] doubles as the current
           compression parameter written by setTextureDefaultCompression. */
        unsigned long default_texture_params[5];
        /* Default Dimensions::compression for newly created textures;
           setTextureDefaultCompression indexes default_texture_params
           with it. */
        srTextureIFace::e_compression default_compression;
    };

    /* +0x2068 environment/enable record: the copy emits a
       0x52-dword rep movsd over the block. pushEnvironment/popEnvironment
       stack {min, max, scale, inv_scale} POD elements — the constructor
       emits no ehctor over that array — while enable_stack gets the
       sixteen-element srFlags __ehvector_ctor (element ctor 0x1001EF50). */
    struct EnvironmentState {
        EnvironmentState() : environment_depth(0), enable_depth(0) {}

        Environment environment_stack[16];
        unsigned long environment_depth;
        srFlags<e_enable> enable_stack[16];
        unsigned long enable_depth;
    };

    unsigned char unknown_0c_[4];
    RendererEntry* renderers;
    srCriticalSection* renderers_section;
    srCriticalSection* state_section;
    unsigned long owner_thread;
    srFlags<e_enable> enable_flags;
    unsigned long dirty;
    unsigned long state_flags;
    e_error last_error;
    /* getPrev reads this list link; first is the global head. */
    srGERD* prev;
    srGERD* next;
    /* Open-GERD list links; closeWindow splices via prev->next_open and
       next->prev_open. */
    srGERD* prev_open;
    srGERD* next_open;
    /* Device record; retail's copy constructor emits a single
       0xD4-dword rep movsd over the whole member. */
    Device device;
    /* Render-state record; retail copies it as one 0x4F2-dword rep movsd
       and the constructor zeroes it wholesale. */
    State state;
    /* Presentation record; retail copies it as a 5-dword rep movsd. */
    Display display;
    /* Pick record; retail copies it as a 0xA2-dword rep movsd. */
    PickState pick;
    unsigned char unknown_19f4_[4];
    /* Snapshot getStatistics refreshes on every flipFrame; dump prints it. */
    Statistics frame_statistics;
    /* getDD counts each device access at value_68 (the overlay's "DD" row),
       so the live counters are writable from const. */
    mutable Statistics statistics;
    /* accumAlloc sizes this width*height*8 accumulation pixel buffer plus a
       width*4 scratch block; accumClear fills it from clear_state.clear_values's
       accum color over the current scissor. */
    AccumPixel* accum_buffer;
    unsigned long* accum_scratch;
    LockSurface* lock_surface;
    /* Buffer-lock nesting depth; _lockBuffer only locks the device on the
       first entry and _unlockBuffer unlocks when this returns to zero. */
    long buffer_lock_count;
    /* Clear record; retail's copy constructor emits a 0xC-dword
       rep movsd over the whole member. */
    ClearState clear_state;
    /* Grayscale ramp built by initGlobalPalette and handed to
       srDD::setGlobalPalette; matchPalette compares it as srARGB. */
    srARGB global_palette[0x100];
    /* Bound Texture per stage, swapped by changeTexture. */
    Texture* texture_slots[2];
    /* Per-stage packed device parameters written by setTextureParameters. */
    srDD::TexParms texture_parms[2];
    /* Device palette record handed to bindPalette/deletePalette. */
    srDD::Palette device_palette;
    /* Texture-parameter map record; retail's copy constructor
       emits a 0x20-dword rep movsd over the whole member. */
    TextureState texture_state;
    /* Palette currently bound to the device; srGERD's destructor runs the
       releasing srPtr teardown at the member-teardown level after
       texture_iface; the assigning sites inline operator='s addref/release
       handoff. */
    srPtr<srPalette> palette;
    e_polygonMode polygon_mode;
    long polygon_offset;
    srVector4T<float> fog_color;
    srShader shader;
    /* Texture interfaces requested through setTexture for stages 0/1; the
       srPtr array is proven by ~srGERD's __ehvec_dtor over two releasing
       elements. */
    srPtr<srTextureIFace> texture_iface[2];
    /* Live textures keyed by the texture interface's frame handle. */
    srHashTable<unsigned long, Texture*> texture_lookup;
    /* Chunk pointers backing the 0xa8-byte Texture pool; ~srGERD calls its
       release() before the memberwise teardown reaches chunks. */
    TexturePool texture_pool;
    Texture* texture_deleted;
    Texture* texture_head;
    Texture* texture_default;
    unsigned long texture_cache_used;
    unsigned long texture_cache_size;
    unsigned long texture_sequence;
    long texture_reduction;
    bool texture_hash_enabled;
    unsigned char unknown_2045_[3];
    srVector4T<float> ambient_light;
    Environment environment;
    /* Environment/enable record; retail's copy constructor emits a
       0x52-dword rep movsd over the whole member. */
    EnvironmentState environment_state;
    VertexProcessors vertex_processors;
    unsigned long exclusion_mask;
    unsigned long vertex_arrays_dirty;
    srRendererDefs::VertexArrayInfo vertex_arrays;
    /* performPickTest's w-normalized {x,y,z,sign(w)} scratch per vertex;
       released by closeWindow. */
    srHeapBuffer<srVector4T<float> > pick_vertices;
};

/* The copy body is consistent with memberwise copying; no in-DLL call site is
   identified in the reviewed evidence. */

/* Retail 0x10027BF0: the three-word texture-set key hash; the interning
   cache inlines it for the lookup probe and calls this emission when
   inserting. */
// FUNCTION: SURRENDER 0x10027BF0 SYMBOL
// ?srHashValue@@YAIABUTextureSetKey@Renderer@srGERD@@@Z
inline unsigned int srHashValue(const srGERD::Renderer::TextureSetKey& key)
{
    // reinterpret-ok: the hash mixes the stored interface addresses.
    return ((key.shader_08.value >> 10 ^ reinterpret_cast<unsigned long>(key.texture1)) >> 1 ^
            // reinterpret-ok: as above.
            reinterpret_cast<unsigned long>(key.texture0)) >>
               5 ^
           key.shader_08.value;
}

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
static_assert(sizeof(srGERD::ClipPlanes) == 0x208, "srGERD_ClipPlanes_must_be_0x208");
static_assert(sizeof(srGERD::Renderer::TriInput) == 0x28, "srGERD_Renderer_TriInput_must_be_0x28");
