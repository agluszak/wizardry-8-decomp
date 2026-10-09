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
        srVector3T<float> position;
        srModelInstance* selected_model;
        w8_ulong triangle_index;
    };

    struct ClipPlanes {
        srVector4T<float> planes[32];
        w8_ulong mask;
        w8_ulong mode1_mask;
    };

    /* pushEnvironment/popEnvironment record: {minimum, maximum, scale, inverse_scale}. */
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
            w8_ulong triangle_count;
            w8_ulong record_count;
            w8_ulong vertex_count;
            const w8_ulong* indices;
            const srVector3i* triangles;
            const w8_ulong* vertices;
            const srTriMeshPipeline::Pass* passes;
            int direct_vertex_indices;
            const srMatrix4T<float>* project_clip_near;
            float sort_bias;
        };

        struct Parameters {
            srGERD* gerd;
            w8_long sorted;
            w8_long batch_limit;
            w8_ulong texture_stages;
        };

        /* Per-stage {st,q} scratch for drivers that want combined texture-coordinate and w streams. */
        struct TexCoordQ {
            srVector2T<float> st;
            float q;
        };
        static_assert(sizeof(TexCoordQ) == 0xc, "TexCoordQ_must_be_0xc");

        /* The interned record additionally carries a blend class derived from the shader's DSTBLEND
           field (ZERO -> 0, SRC_ALPHA pair -> 1, ONE -> 2, SRC_COLOR pair -> 3). */
        struct TextureSetKey {
            srTextureIFace* texture0;
            srTextureIFace* texture1;
            srShader shader;

            bool operator==(const TextureSetKey& other) const
            {
                return texture0 == other.texture0 && texture1 == other.texture1 &&
                       shader.value == other.shader.value;
            }
            bool operator!=(const TextureSetKey& other) const
            {
                return !(*this == other);
            }
        };
        struct TextureSet {
            srTextureIFace* texture0;
            srTextureIFace* texture1;
            srShader shader;
            w8_ulong blend;
        };
        /* Texture-set interning cache; the map's value is the index into sets. */
        struct TextureSetCache {
            srHashTable<TextureSetKey, w8_ulong>* map;
            srArray<TextureSet> sets;
            w8_ulong count;

            TextureSetCache() : count(0)
            {
                map = new srHashTable<TextureSetKey, w8_ulong>;
            }
            ~TextureSetCache()
            {
                clear();
                delete map;
            }
            void clear()
            {
                map->Clear();
                sets.release();
                count = 0;
            }
            w8_ulong intern(const TextureSetKey& key);
        };
        /* Write pointers alloc() returns for the reserved triangle range. */
        struct IndexWrite {
            srVector3i* triangles;
            w8_ulong* texture_set;
            w8_ulong* sort_key;
            w8_ulong* aux;
        };
        /* Accumulated primitive work. alloc() reserves count entries plus 0x40 headroom across all
           four streams; reset() always clears the count and only frees when asked. */
        struct IndexBatch {
            srArray<srVector3i> triangles;
            srArray<w8_ulong> texture_set;
            srArray<w8_ulong> sort_key;
            srArray<w8_ulong> aux;
            w8_ulong count;

            IndexBatch() : texture_set(0), sort_key(0), aux(0), count(0) {}
            void alloc(IndexWrite& write, w8_ulong count);
            void reset(int release);
        };
        /* Accumulated vertex streams. alloc() grows all streams and default-fills the new range
           (positions {0,0,0,1}, st {0,0}, q 1.0); bind() points an srVertexArray at the reserved
           range. */
        struct VertexArrays {
            srArray<srVector4T<float> > diffuse;
            srArray<srVector4T<float> > specular;
            srArray<srVector4T<float> > positions;
            srArray<srVector2T<float> > st[2];
            srArray<float> q[2];
            srArray<unsigned char> attributes;
            /* isBatchFull compares this signed against batch_limit. */
            w8_long count;
            w8_ulong capacity;

            VertexArrays()
                : diffuse(0), specular(0), positions(0), attributes(0), count(0), capacity(0)
            {
            }
            void alloc(srVertexArray& arrays, w8_ulong count);
            void bind(srVertexArray& arrays, w8_ulong base);
            void reset(int release);
        };

        Renderer(const Parameters& parameters);
        void allocVertexArray(srVertexArray& arrays, w8_ulong count);
        /* intern the pass's {texture0,texture1,shader} key into
           texture_set, folding per-vertex texture/shader table transitions
           into the output ids. */
        void assignTextureSets(w8_ulong* texture_set, const w8_ulong* indices, w8_ulong count,
                               const srTriMeshPipeline::Pass* pass);
        /* While a vertex range is reserved (first_vertex != -1), give count accumulated vertices
           back. */
        void rewindVertexArray(w8_ulong count);
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
        void bindTextureSet(w8_ulong index);
        /* repack the per-stage stq scratch streams and program
           the DD vertex arrays for the bound batch. */
        void programVertexArrays(srVertexArray* arrays, w8_ulong count);
        /* discard accumulated state; nonzero also releases
           the backing arrays. */
        void reset(int release_buffers);
        /* resetStatistics zeroes statistics; getStatistics copies its seven counters out. */
        void resetStatistics();
        void getStatistics(w8_ulong* statistics);

        /* Checked-free heap buffers: ~Renderer null-checks before freeing them. */
        srHeapBuffer<unsigned char> bytes;
        srHeapBuffer<w8_ulong> dwords;
        /* render()'s per-corner dedup scratch (six slots per triangle). */
        srHeapBuffer<w8_ulong> remap;
        srHeapBuffer<TexCoordQ> stq[2];
        /* submit() bumps [4] per call and accumulates the vertex count into [5] and the index-batch
           count into [6]. */
        w8_ulong statistics[7];
        TextureSetCache texture_sets;
        IndexBatch indices;
        VertexArrays vertices;
        /* allocVertexArray() snapshots the vertex count here so render() can
           offset indices into the reserved range. */
        w8_long first_vertex;
        /* Bound draw state, refreshed per texture set. */
        srTextureIFace* texture0;
        srTextureIFace* texture1;
        srShader shader;
        w8_ulong clip_state;
        srGERD* gerd;
        /* lockRenderer matches this against the sorted-mode enable bit;
           flushSort flushes entries where it is 1, flushImmediateRenderers
           where it is 0. */
        w8_long sorted;
        w8_long batch_limit;
        w8_ulong texture_stages;
    };
    W8_ABI_ASSERT(sizeof(Renderer) == 0xe4, "srGERD_Renderer_must_be_0xe4");
    W8_ABI_ASSERT(sizeof(Renderer::Parameters) == 0x10, "srGERD_Renderer_Parameters_must_be_0x10");
    W8_ABI_ASSERT(sizeof(Renderer::TextureSetKey) == 0x0c,
                  "srGERD_Renderer_TextureSetKey_must_be_0x0c");
    W8_ABI_ASSERT(sizeof(Renderer::IndexWrite) == 0x10, "srGERD_Renderer_IndexWrite_must_be_0x10");

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
    /* 0 selects the model-view stack and 1 the projection stack. */
    enum e_matrixMode { MATRIX_MODELVIEW = 0, MATRIX_PROJECTION = 1 };
    enum e_antiAlias { ANTIALIAS_NONE = 0 };
    enum e_clipMode { CLIPMODE_POSITIONAL_0 = 0 };
    /* 0 culls back faces and 1 front faces (each flipped by the winding); 2 disables culling. */
    enum e_cullMode { CULL_BACK = 0, CULL_FRONT = 1, CULL_NONE = 2 };
    /* toggle XORs 1<<option into enable_flags. Option 1 selects sorted rendering; option 5
       wraps/unwraps srDebugDD. The constructor sets options 4 and 6. */
    enum e_enable {
        ENABLE_POSITIONAL_0 = 0,
        ENABLE_SORTED_RENDERING = 1,
        ENABLE_REVERSE_NORMALS = 3,
        ENABLE_AUTO_FLIP = 4,
        ENABLE_DEBUG_DD = 5,
        ENABLE_CLEAR_ON_OPEN = 6
    };
    enum e_winding { WINDING_POSITIONAL_0 = 0, WINDING_POSITIONAL_1 = 1 };
    enum e_visibility { VISIBILITY_OUTSIDE = 0 };
    /* dump(stream, flags) section selectors: bit 0 driver/device info plus the window block, bit 1
       the texture cache, bit 3 the statistics snapshot, bit 5 the srDebugDD call profile.
       dump(stream) passes 0x3f. */
    enum e_info {
        INFO_DEVICE = 0x1,
        INFO_TEXTURE_CACHE = 0x2,
        INFO_STATISTICS = 0x8,
        INFO_DEBUG_DD = 0x20,
        INFO_ALL = 0x3f
    };
    enum e_hint {};
    enum e_hintMode {};
    /* 0 loads the frame buffer into the accum buffer, 1 accumulates, 4 returns the accum buffer to
       the frame buffer; 2/3 multiply/add through the accum path. */
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
        w8_long width;
        w8_long height;
        w8_long depth;
    };
    /* getTextureInfo output: the device pixel format plus the device's
       width/height and last mip level. */
    struct TextureInfo {
        srPixelConvert::PixelFormat pixel_format;
        w8_ulong width;
        w8_ulong height;
        w8_ulong last_level;
    };
    /* accumulate()'s signed 16-bit accum-buffer pixel. */
    struct AccumPixel {
        short red;
        short green;
        short blue;
        short alpha;
    };

    static const char* sGetClassName();
    static srRegistry::ClassNode* sGetClassNode();

    srGERD(srDD* device, void* module, const char* device_name);
    virtual ~srGERD() override;

    virtual const char* getClassName() const override;
    virtual w8_ulong getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual void dump(std::ostream& stream) override;
    void dump(std::ostream& stream, const srFlags<e_info>& info);
    static srGERD* loadDevice(srStringTable& devices, w8_ulong index);
    static srGERD* loadDevice(const char* name, const char* path, w8_ulong device);
    static srGERD* loadDeviceWithFileName(const char* filename, w8_ulong device);
    static srGERD* getFirst();
    srGERD* getNext() const;
    /* Open-device list used by srTexture::invalidateFrameHandle. */
    static srGERD* getFirstOpen();
    srGERD* getNextOpen() const;
    e_error createContext(w8_ulong_ptr window);
    int isContextCreated() const;
    void deleteContext();
    w8_long getDisplayMode(w8_ulong width, w8_ulong height, w8_ulong depth) const;
    /* openWindowInternal parameter block: the current client size and the
       requested backbuffer size plus the display-mode index (-1 windowed). */
    struct OpenInfo {
        w8_long window_width;
        w8_long window_height;
        w8_long width;
        w8_long height;
        w8_long display_mode;
    };
    /* Number of back buffers in the swap chain (1, 2 or 3). */
    e_error openWindow();
    e_error openWindow(w8_long width, w8_long height);
    e_error openWindow(w8_long mode);
    void closeWindow(e_closeHint hint);
    e_backBuffer getBackBufferType() const;
    int isWindowOpen() const;
    int isFullScreen() const;
    w8_ulong_ptr getWindowHandle() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void flushImmediateRenderers();
    void clear(const srFlags<e_buffer>& buffers);
    w8_long getHeight() const;
    w8_long getWidth() const;
    void resetStatistics();
    struct Statistics {
        /* Epoch written by resetStatistics; getStatistics returns the
           seconds elapsed since then. */
        double elapsed;
        /* Device texture bytes transferred. */
        double texture_transfer;
        double pixels_drawn;
        w8_ulong value_18;
        w8_ulong value_1c;
        w8_ulong device_triangles;
        w8_ulong device_vertices;
        w8_ulong device_vertex_indices;
        /* Frames presented: flipFrame increments once per call. */
        w8_ulong frames;
        w8_ulong triangle_chunks;
        w8_ulong input_triangles;
        /* getStatistics accumulates this only for sorted renderers. */
        w8_ulong sorted_triangles;
        w8_ulong input_vertices;
        /* applyViewStateChanges increments this counter on every apply. */
        w8_ulong view_state_applies;
        /* applyDrawStateChanges increments this counter on every apply. */
        w8_ulong draw_state_applies;
        /* applyFrameStateChanges increments this counter on every apply. */
        w8_ulong frame_state_count;
        /* Texture binds counted by changeTexture after the stage's bound
           texture actually changes; the debug overlay prints it as "TC". */
        w8_ulong texture_binds;
        /* Texture-parameter updates counted by setTextureParameters. */
        w8_ulong texture_parameter_sets;
        /* createNewTexture increments this created-texture count. */
        w8_ulong textures_created;
        /* Palette binds counted when a changed texture carries a new palette. */
        w8_ulong palette_binds;
        /* setShader calls counted by applyDrawStateChanges. */
        w8_ulong shader_sets;
        /* drawArrays/drawElements increment this draw-call count. */
        w8_ulong draw_calls;
        w8_ulong clipped_triangles;
        w8_ulong device_calls;
        /* testBoundingSphere call count / visible-result count. */
        w8_ulong sphere_tests;
        w8_ulong sphere_visible;
        /* testBoundingBox call count / visible-result count. */
        w8_ulong box_tests;
        w8_ulong box_visible;
        /* classifyMatrix call count. */
        w8_ulong matrix_classifications;
    };
    void getStatistics(Statistics& statistics);
    w8_ulong getTextureCacheUsed() const;
    w8_ulong getResidentTextureMemUsed() const;
    void setClearColor(const srVector4T<float>& color);
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(double depth);
    void setAmbientLight(float red, float green, float blue, float alpha);
    void setAmbientLight(const srVector4T<float>& light);
    void setAmbientLight(const srVector3T<float>& light);
    void setFogColor(const srVector3T<float>& color);
    void setFogColor(const srVector4T<float>& color);
    void setScissor(w8_ulong x, w8_ulong y, w8_ulong width, w8_ulong height);
    /* Dirty-rectangle pair handed to flipFrame in {x,y,width,height} form;
       GERD converts each to srDD::Scissor left/top/right/bottom. */
    struct Rectangle {
        w8_long x, y, width, height;
    };
    void flipFrame();
    void flipFrame(const Rectangle* first, const Rectangle* second, w8_ulong count);
    void setTextureReduction(w8_long reduction);
    void setViewPort(w8_ulong x, w8_ulong y, w8_ulong width, w8_ulong height);
    void matrixMode(e_matrixMode mode);
    e_matrixMode getMatrixMode() const;
    void getMatrix(srMatrix4T<float>& matrix);
    void getMatrix(srMatrix4T<double>& matrix);
    void getMatrix(e_matrixMode mode, srMatrix4T<float>& matrix);
    void getMatrix(e_matrixMode mode, srMatrix4T<double>& matrix);
    void getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                           const srVector3T<float>& object_center, float object_radius);
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

    float getMaxModelViewScale();
    e_cullMode getCullMode() const;
    e_winding getWinding() const;
    void setWinding(e_winding winding);
    Renderer* lockRenderer();
    void unlockRenderer(Renderer* renderer, int submit);
    srColorSurfaceIFace* lockBuffer();
    void unlockBuffer();
    w8_ulong getVertexProcessorCount() const;
    void getVertexProcessors(srVertexProcessor** processors) const;
    void getAmbientLight(srVector4T<float>& light);
    void getFogColor(srVector4T<float>& color) const;
    void getEnvironmentRange(float& minimum, float& maximum) const;
    void getEnvironmentScaleFactor(float& scale, float& inverse_scale);
    w8_ulong getExclusionMask() const;
    void setExclusionMask(w8_ulong mask);
    // FUNCTION: SURRENDER 0x1001BB70 SYMBOL
    // RECOMP: ?getMaxTextureStages@srGERD@@QBEJXZ
    w8_long getMaxTextureStages() const
    {
        return device.info.max_texture_stages;
    }
    // FUNCTION: SURRENDER 0x1001CF10 SYMBOL
    // RECOMP: ?getMaxPickStackDepth@srGERD@@QBEJXZ
    w8_long getMaxPickStackDepth() const
    {
        return 0x20;
    }
    // FUNCTION: SURRENDER 0x1001CF20 SYMBOL
    // RECOMP: ?getMaxModelviewStackDepth@srGERD@@QBEJXZ
    w8_long getMaxModelviewStackDepth() const
    {
        return 0x20;
    }
    // FUNCTION: SURRENDER 0x1001CF30 SYMBOL
    // RECOMP: ?getMaxProjectionStackDepth@srGERD@@QBEJXZ
    w8_long getMaxProjectionStackDepth() const
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
    void setPickKey(w8_ulong_ptr key);
    w8_ulong_ptr getPickKey() const;
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
    void getScissor(w8_ulong& x, w8_ulong& y, w8_ulong& width, w8_ulong& height) const;
    void getViewPort(w8_ulong& x, w8_ulong& y, w8_ulong& width, w8_ulong& height) const;
    void pushEnvironment();
    void popEnvironment();
    void setEnvironmentRange(float minimum, float maximum);
    void setEnvironmentScaleFactor(float scale, float inverse_scale);
    void setClipState(srFlags<srRendererDefs::e_clip> state);
    void setAntiAlias(e_antiAlias mode);
    void setTexture(srTextureIFace* texture, w8_ulong layer);
    void setTextureDefaultCorrection(srTextureIFace::e_correction correction);
    void setTextureDefaultCompression(srTextureIFace::e_compression compression);
    void setTextureDefaultMagFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMinFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap);
    void setTextureSubImage(srTextureIFace* texture, w8_long mipmap, w8_long x, w8_long y,
                            w8_long width, w8_long height);
    void drawArrays(srRendererDefs::e_primitive primitive, w8_long first, w8_ulong count);
    void drawElements(srRendererDefs::e_primitive primitive, w8_ulong count,
                      srRendererDefs::e_indexType type, const void* indices);
    void popPick(Pick& pick);
    void pushPick(const Pick& pick);
    void toggle(e_enable option);
    void invalidateResidentTextures();
    void invalidateResidentTexture(srTextureIFace* texture);
    void invalidateTextureCache();
    void invalidateTexture(srTextureIFace* texture);
    void invalidateTextureByFrameHandle(w8_ulong handle);
    w8_ulong getTextureCacheSize() const;
    void setTextureCacheSize(w8_ulong bytes);
    void setSwapInterval(w8_ulong interval);
    w8_long getPolygonOffset() const;
    void setPolygonOffset(w8_long offset);

    int isBufferLocked();
    srGERD* getPrev() const;
    srGERD* getPrevOpen() const;
    static w8_long getGERDCount();
    static srGERD* getGERD(w8_ulong index);
    /* Scan provider libraries for devices. */
    static void loadDevices(const char* path);
    static void scanDevices(const char* path, srStringTable& devices);
    /* Releases every GERD on the global list. */
    static void releaseAll();
    const char* getErrorString(e_error error);
    e_error getError();
    int isFlipped() const;
    const char* getApiVersion() const;
    /* device.info.text[0..8] accessors. */
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
    w8_ulong getSwapInterval() const;
    void getTextureFormat(w8_ulong index, srPixelConvert::PixelFormat& format) const;
    w8_ulong getTextureFormatCount() const;
    void getDisplayModeInfo(w8_long index, DisplayModeInfo& info) const;
    w8_ulong getDisplayModeCount() const;
    e_hintMode getHint(e_hint hint) const;
    void setHint(e_hint hint, e_hintMode mode);
    void getGamma(srVector3T<float>& gamma) const;
    e_antiAlias getAntiAlias() const;
    void extCommand(w8_ulong command, void* data, w8_ulong size);
    void disable(e_enable option);
    void enable(e_enable option);
    srShader getShader() const;
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
    void setClearStencil(w8_ulong stencil);
    w8_ulong getClearStencil() const;
    w8_long getAccumAlphaBits() const;
    w8_long getAccumRedBits() const;
    w8_long getAccumGreenBits() const;
    w8_long getAccumBlueBits() const;
    void accumulate(e_accum operation, float scale);
    int isTextureCached(srTextureIFace* texture) const;
    int isTextureResident(srTextureIFace* texture) const;
    int getTextureInfo(srTextureIFace* texture, TextureInfo& info);
    srTextureIFace::e_compression getTextureDefaultCompression() const;
    srTextureIFace::e_correction getTextureDefaultCorrection() const;
    srTextureIFace::e_filter getTextureDefaultMagFilter() const;
    srTextureIFace::e_filter getTextureDefaultMinFilter() const;
    srTextureIFace::e_mipmap getTextureDefaultMipmap() const;
    w8_long getTextureReduction() const;
    void invalidateResidentPalette(srPalette* palette);
    void setGlobalPalette(const srPalette& palette);
    w8_long getMaxTextureWidth() const;
    w8_long getMaxTextureHeight() const;
    w8_long getMaxTextureAspectRatio() const;
    static w8_ulong sGetClassID();
    static void dumpDeviceList(std::ostream& stream);

    // FUNCTION: SURRENDER 0x1001BB80 SYMBOL
    // RECOMP: ?isPickStackEmpty@srGERD@@QBEHXZ
    int isPickStackEmpty() const
    {
        return pick.pick_depth == 0;
    }

    // FUNCTION: SURRENDER 0x1001BAE0 SYMBOL
    // RECOMP: ?isEnabled@srGERD@@QBEHW4e_enable@1@@Z
    int isEnabled(e_enable option) const
    {
        return (enable_flags.value & (1UL << option)) != 0;
    }

    // FUNCTION: SURRENDER 0x1001BB90 SYMBOL
    // RECOMP: ?setCullMode@srGERD@@QAEXW4e_cullMode@1@@Z
    void setCullMode(e_cullMode mode)
    {
        if (state.cull_mode != mode) {
            state.cull_mode = mode;
            dirty |= DIRTY_CULLING;
        }
    }

    // FUNCTION: SURRENDER 0x1001BB40 SYMBOL
    // RECOMP: ?setShader@srGERD@@QAEXABVsrShader@@@Z
    void setShader(const srShader& shader)
    {
        if (this->shader.value != shader.value) {
            this->shader = shader;
            dirty |= DIRTY_SHADER;
        }
    }

    // FUNCTION: SURRENDER 0x1001BEF0 SYMBOL
    // RECOMP: ?setVertexArrayMask@srGERD@@QAEXV?$srFlags@W4e_vertexArray@srRendererDefs@@@@@Z
    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask)
    {
        vertex_arrays.mask = mask;
        vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
    }

    // FUNCTION: SURRENDER 0x1001BEE0 SYMBOL
    // RECOMP: ?getVertexArrayMask@srGERD@@QBE?AV?$srFlags@W4e_vertexArray@srRendererDefs@@@@XZ
    srFlags<srRendererDefs::e_vertexArray> getVertexArrayMask() const
    {
        return vertex_arrays.mask;
    }

    void setDiffusePointer(w8_long components, srRendererDefs::e_type type, w8_ulong stride,
                           const void* values);
    void setSpecularPointer(w8_long components, srRendererDefs::e_type type, w8_ulong stride,
                            const void* values);
    void setFogPointer(w8_long components, srRendererDefs::e_type type, w8_ulong stride,
                       const void* values);

    // FUNCTION: SURRENDER 0x1001BFD0 SYMBOL
    // RECOMP: ?setTexCoordPointer@srGERD@@QAEXJW4e_type@srRendererDefs@@KPBXK@Z
    void setTexCoordPointer(w8_long components, srRendererDefs::e_type type, w8_ulong stride,
                            const void* values, w8_ulong layer)
    {
        w8_ulong index = layer + 4;
        vertex_arrays.components[index] = components;
        vertex_arrays.types[index] = type;
        vertex_arrays.strides[index] = stride;
        vertex_arrays.arrays[index] = values;
        vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
    }

    // FUNCTION: SURRENDER 0x1001BE90 SYMBOL
    // RECOMP: ?setVertexPointer@srGERD@@QAEXJW4e_type@srRendererDefs@@KPBXJ@Z
    void setVertexPointer(w8_long primitive, srRendererDefs::e_type type, w8_ulong stride,
                          const void* values, w8_long count)
    {
        vertex_arrays.count = count < 0 ? 0 : count;
        vertex_arrays.components[0] = primitive;
        vertex_arrays.types[0] = type;
        vertex_arrays.strides[0] = stride;
        vertex_arrays.arrays[0] = values;
        vertex_arrays_dirty |= DIRTY_VERTEX_ARRAY_INFO;
    }

private:
    /* Pooled device-texture record. allocTexture links chunks through the first word and keys the
       hash by the texture-interface id; the embedded srDD::Texture is handed to the device. */
    struct Texture {
        Texture* prev;
        Texture* next;
        w8_ulong id;
        /* evaluateTexturePixelFormat copies the matched device format here. */
        srPixelConvert::PixelFormat pixel_format;
        void* surface_data;
        srPtr<srPalette> palette;
        char* name;
        srDD::Texture device;
        w8_ulong unknown_a4;
    };
    W8_ABI_ASSERT(sizeof(Texture) == 0xa8, "srGERD_Texture_must_be_0xa8");

    /* Pick batch view: indices selects triangles out of the caller's stream, vertices remaps each
       corner to a position index, positions is the renderer's vec4 stream base. */
    struct PickInput {
        const w8_ulong* indices;
        const srVector3i* triangles;
        w8_ulong triangle_count;
        const w8_ulong* vertices;
        const srVector4T<float>* positions;
        w8_ulong vertex_count;
    };

    srGERD& operator=(const srGERD& other);
    srDD* getDD() const;

    static void debugWrite(const char* text);
    void fenceVertexArrays();
    void dumpTextureCache(std::ostream& stream);
    void setDataPtr(srRendererDefs::e_vertexArray index, w8_long components,
                    srRendererDefs::e_type type, w8_ulong stride, const void* values);

    friend class Renderer;
    /* For each queued Pick, w-normalize the batch's positions into pick_vertices and run the
       edge-function triangle test against the pick ray. */
    void performPickTest(const PickInput& input);
    void deleteRenderers();
    /* getErrorString table: the ten e_error names followed by no terminator;
       out-of-range errors report "UNKNOWN ERROR". */
    static const char* errStrings[10];

    struct TextureEntry {
        w8_long next;
        w8_ulong handle;
        Texture* texture;
    };
    struct RendererEntry {
        RendererEntry* prev;
        RendererEntry* next;
        Renderer* renderer;
        w8_long busy;
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
    void changeTexture(srTextureIFace* texture, w8_ulong stage, int apply_parms);
    Texture* createNewTexture(srTextureIFace* texture);
    Texture* allocTexture(w8_ulong id);
    void allocTextureData(Texture& texture);
    void deleteTexture(Texture& texture);
    void removeDeletedTextures();
    void invalidatePalette();
    void releaseTextureSurfaceData(Texture& texture);
    void setTextureParameters(w8_ulong stage, const srTextureIFace::Parameters& parameters);
    void evaluateTextureDimensions(srDD::Texture& device,
                                   const srTextureIFace::Dimensions& dimensions);
    void evaluateTexturePixelFormat(Texture& texture, const srTextureIFace::Dimensions& dimensions);
    void releaseTextureMemory(w8_long bytes);
    w8_ulong getTextureBytesNeeded(Texture& texture) const;
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
    w8_ulong getDDAPIVersion() const;
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
    /* MMX row kernels for accumulate(). */
    static void __cdecl accumAccum_MMX(AccumPixel* accum, const srARGB* pixels, w8_long scale,
                                       w8_long count);
    static void __cdecl accumLoad_MMX(AccumPixel* accum, const srARGB* pixels, w8_long scale,
                                      w8_long count);
    static void __cdecl accumReturn_MMX(srARGB* pixels, const AccumPixel* accum, w8_long scale,
                                        w8_long count);
    static void __cdecl accumAdd_MMX(AccumPixel* accum, w8_long value, w8_long count);
    static void __cdecl accumMult_MMX(AccumPixel* accum, w8_long value, w8_long count);
    static void convertPixelFormat(srDD::PixelFormat& device,
                                   const srPixelConvert::PixelFormat& format);
    static void convertPixelFormat(srPixelConvert::PixelFormat& format,
                                   const srDD::PixelFormat& device);
    RendererEntry* createRenderer(int sorted);
    void flushNonBusyRenderers();
    void flushSort();
    Renderer* _lockRenderer(RendererEntry* entry);
    class LockSurface;
    friend class LockSurface;

    /* Texture pool: live-texture count, free-list head and the chunk-pointer array. */
    struct TexturePool {
        TexturePool();

        ~TexturePool()
        {
            release();
        }

        /* Frees every chunk, releases the chunk array and zeroes the record. */
        void release();
        Texture* allocate();
        void release(Texture* texture);

        w8_ulong count;
        Texture* free;
        srArray<Texture*> chunks;
        w8_ulong pool_count;
    };
    friend struct TexturePool;

    /* The registered srVertexProcessor pointers plus the live count. */
    struct VertexProcessors : public srArray<srVertexProcessor*> {
        VertexProcessors() : count(0)
        {
            release();
        }

        w8_ulong count;
    };

    static srGERD* first;
    static srGERD* firstOpen;

    struct MatrixStack {
        MatrixStack();

        srMatrix4T<float> stack[32];
        w8_ulong depth;
    };

    struct Device {
        srDD* dd;
        srDebugDD* debug_dd;
        srDD* real_dd;
        /* Dynamic-library handle the constructor stores and ~srGERD passes
           to srDynamicLibrary::free. */
        void* module;
        srDD::Info info;
        /* getDriverInfo target; getDDAPIVersion/getDriverID/getDriverName
           (pre-context) and getApiVersion read its trailing fields. */
        srDD::DriverInfo driver_info;
        srPixelConvert::PixelFormat* texture_formats;
        w8_long texture_format_count;
        srDD::WindowInfo* display_modes;
        w8_long display_mode_count;
        /* setHint/getHint index this by e_hint. */
        e_hintMode hints[1];
        w8_ulong_ptr window;
        /* openWindowInternal fills this record: windowed dims, backbuffer dims, then the
           display-mode index (-1 when windowed). */
        OpenInfo open_info;
        /* e_backBuffer result of srDD::openWindow; getBackBufferType reads
           it. */
        e_backBuffer back_buffer_type;
    };

    struct State {
        State() : scissor_flags(0) {}

        /* Per-mode current matrices; pushMatrix indexes by mode. */
        srMatrix4T<float> matrix_current[2];
        /* Per-mode 32-deep matrix stacks. */
        MatrixStack matrix_stacks[2];
        /* Entries [0..5] are the eye-space frustum planes and [6..31] the user planes pushed by
           pushClipPlane (mask bits 6..31 of clip_mask). */
        srVector4T<float> clip_planes[32];
        /* Depth range forwarded into srDD::ViewPort by applyViewStateChanges;
           initView resets it to [0.0, 1.0] and setDepthRange clamps it. */
        double depth_min;
        double depth_max;
        srDD::Scissor scissor;
        w8_ulong view_left;
        w8_ulong view_top;
        w8_ulong view_right;
        w8_ulong view_bottom;
        e_cullMode cull_mode;
        e_winding winding;
        e_matrixMode matrix_mode;
        unsigned char unknown_12c4_[6];
        /* Per-user-plane e_clipMode bytes written by pushClipPlane. */
        unsigned char clip_modes[26];
        /* Plane-enable mask: bits 0..5 frustum, bits 6..31 user planes. */
        w8_ulong clip_mask;
        /* Subset of clip_mask carrying mode-1 user planes. */
        w8_ulong clip_mode1_mask;
        w8_long clip_plane_count;
        /* Bit 1: recalcScissor marks the scissor as the full view. */
        w8_ulong scissor_flags;
        srMatrix4T<float> inverse_modelview;
        srMatrix4T<float> project_clip_near;
        srMatrix4T<float> normal_matrix;
        srMatrix4T<float>::e_scaleType modelview_scale_type;
        float max_modelview_scale;
        /* classifyMatrix writes the per-mode projection-shape class here. */
        srMatrix4T<float>::e_type matrix_class[2];
        unsigned char unknown_13c4_[4];
    };

    struct Display {
        srVector3T<float> gamma;
        w8_ulong swap_interval;
        e_antiAlias antialias;
    };

    struct PickState {
        PickState() : pick_depth(0) {}

        Pick pick_stack[32];
        w8_ulong pick_depth;
        w8_ulong_ptr pick_key;
    };

    struct ClearState {
        srDD::ClearValues clear_values;
        unsigned char unknown_2c_[4];
    };

    /* setTextureParameters indexes these maps from the packed texture state; filter selector 4 is a
       valid index in both filter maps. */
    struct TextureState {
        w8_ulong correction_map[4];
        w8_ulong mag_filter_map[5];
        w8_ulong min_filter_map[5];
        /* The fourth entry doubles as the current mipmap parameter written
           by setTextureDefaultMipmap. */
        w8_ulong mipmap_map[4];
        w8_ulong wrap_s_map[2];
        w8_ulong wrap_t_map[2];
        srTextureIFace::e_correction default_correction;
        srTextureIFace::e_filter default_mag_filter;
        srTextureIFace::e_filter default_min_filter;
        srTextureIFace::e_mipmap default_mipmap;
        /* Per-type default device parameters. Entry [4] doubles as the current compression
           parameter written by setTextureDefaultCompression. */
        w8_ulong default_texture_params[5];
        srTextureIFace::e_compression default_compression;
    };

    /* pushEnvironment/popEnvironment stack plus the enable-flag stack. */
    struct EnvironmentState {
        EnvironmentState() : environment_depth(0), enable_depth(0) {}

        Environment environment_stack[16];
        w8_ulong environment_depth;
        srFlags<e_enable> enable_stack[16];
        w8_ulong enable_depth;
    };

    unsigned char unknown_0c_[4];
    RendererEntry* renderers;
    srCriticalSection* renderers_section;
    srCriticalSection* state_section;
    w8_ulong owner_thread;
    srFlags<e_enable> enable_flags;
    enum {
        DIRTY_FRAME_ENABLE = 0x1UL,
        DIRTY_GAMMA = 0x2UL,
        DIRTY_SWAP_INTERVAL = 0x4UL,
        DIRTY_ANTIALIAS = 0x8UL,
        DIRTY_SCISSOR = 0x10UL,
        DIRTY_MATRIX_SHIFT = 5,
        DIRTY_MODELVIEW = 0x20UL,
        DIRTY_PROJECTION = 0x40UL,
        DIRTY_VIEWPORT = 0x80UL,
        DIRTY_DEPTH_RANGE = 0x100UL,
        DIRTY_FOG_COLOR = 0x200UL,
        DIRTY_TEXTURE_SHIFT = 10,
        DIRTY_TEXTURE0 = 0x400UL,
        DIRTY_TEXTURE1 = 0x800UL,
        DIRTY_SHADER = 0x1000UL,
        DIRTY_CULLING = 0x2000UL,
        DIRTY_POLYGON_MODE = 0x4000UL,
        DIRTY_POLYGON_OFFSET = 0x8000UL,
        DIRTY_CLIP_PLANES = 0x10000UL,
        DIRTY_FRAME_STATE =
            DIRTY_FRAME_ENABLE | DIRTY_GAMMA | DIRTY_SWAP_INTERVAL | DIRTY_ANTIALIAS,
        DIRTY_VIEW_STATE =
            DIRTY_SCISSOR | DIRTY_MODELVIEW | DIRTY_PROJECTION | DIRTY_VIEWPORT | DIRTY_DEPTH_RANGE,
        DIRTY_DRAW_STATE = DIRTY_FOG_COLOR | DIRTY_TEXTURE0 | DIRTY_TEXTURE1 | DIRTY_SHADER |
                           DIRTY_CULLING | DIRTY_POLYGON_MODE | DIRTY_POLYGON_OFFSET
    };
    w8_ulong dirty;
    enum {
        STATE_CONTEXT_CREATED = 0x01u,
        STATE_WINDOW_OPEN = 0x02u,
        STATE_FRAME_STARTED = 0x04u,
        STATE_FRAME_FLIPPED = 0x08u,
        STATE_CLOSING_WINDOW = 0x10u
    };
    w8_ulong state_flags;
    e_error last_error;
    /* getPrev reads this list link; first is the global head. */
    srGERD* prev;
    srGERD* next;
    /* Open-GERD list links; closeWindow splices via prev->next_open and
       next->prev_open. */
    srGERD* prev_open;
    srGERD* next_open;
    Device device;
    State state;
    Display display;
    PickState pick;
    unsigned char unknown_19f4_[4];
    /* Snapshot getStatistics refreshes on every flipFrame; dump prints it. */
    Statistics frame_statistics;
    /* getDD counts each device access, so the live counters are mutable. */
    mutable Statistics statistics;
    /* Accumulation buffer: width*height pixels plus a width*4 scratch block. */
    AccumPixel* accum_buffer;
    w8_ulong* accum_scratch;
    LockSurface* lock_surface;
    /* Buffer-lock nesting depth; _lockBuffer only locks the device on the
       first entry and _unlockBuffer unlocks when this returns to zero. */
    w8_long buffer_lock_count;
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
    TextureState texture_state;
    srPtr<srPalette> palette;
    e_polygonMode polygon_mode;
    w8_long polygon_offset;
    srVector4T<float> fog_color;
    srShader shader;
    /* Texture interfaces requested through setTexture for stages 0/1. */
    srPtr<srTextureIFace> texture_iface[2];
    /* Live textures keyed by the texture interface's frame handle. */
    srHashTable<w8_ulong, Texture*> texture_lookup;
    TexturePool texture_pool;
    Texture* texture_deleted;
    Texture* texture_head;
    Texture* texture_default;
    w8_ulong texture_cache_used;
    w8_ulong texture_cache_size;
    w8_ulong texture_sequence;
    w8_long texture_reduction;
    bool texture_hash_enabled;
    unsigned char unknown_2045_[3];
    srVector4T<float> ambient_light;
    Environment environment;
    EnvironmentState environment_state;
    VertexProcessors vertex_processors;
    w8_ulong exclusion_mask;
    enum { DIRTY_VERTEX_ARRAY_INFO = 0x01u };
    w8_ulong vertex_arrays_dirty;
    srRendererDefs::VertexArrayInfo vertex_arrays;
    /* performPickTest's w-normalized {x,y,z,sign(w)} scratch per vertex;
       released by closeWindow. */
    srHeapBuffer<srVector4T<float> > pick_vertices;
};

// FUNCTION: SURRENDER 0x10027BF0 SYMBOL
// RECOMP: ?srHashValue@@YAIABUTextureSetKey@Renderer@srGERD@@@Z
inline unsigned int srHashValue(const srGERD::Renderer::TextureSetKey& key)
{
    // reinterpret-ok: the hash mixes the stored interface addresses.
    return ((key.shader.value >> 10 ^
             static_cast<w8_ulong>(reinterpret_cast<w8_ulong_ptr>(key.texture1))) >>
                1 ^
            // reinterpret-ok: as above.
            static_cast<w8_ulong>(reinterpret_cast<w8_ulong_ptr>(key.texture0))) >>
               5 ^
           key.shader.value;
}

W8_ABI_ASSERT(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
W8_ABI_ASSERT(sizeof(srGERD::ClipPlanes) == 0x208, "srGERD_ClipPlanes_must_be_0x208");
W8_ABI_ASSERT(sizeof(srGERD::Renderer::TriInput) == 0x28, "srGERD_Renderer_TriInput_must_be_0x28");
