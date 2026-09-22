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

class SR_DLL_IMPORT srGERD : public srRuntimeClass {
public:
    struct Pick {
        /* Normalized pick point the caller fills: x and y are the cursor's
           viewport-space coordinates, z is the fixed 1.0 far value. */
        float x_00;
        float y_04;
        float z_08;
        srModelInstance* selected_model_0c;
        unsigned long value_10;
    };

    struct ClipPlanes {
        srVector4T<float> planes_000[32];
        unsigned long mask_200;
        unsigned long value_204;
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
            unsigned long record_count_04;
            unsigned long vertex_count_08;
            const unsigned long* indices_0c;
            const srVector3i* triangles_10;
            const unsigned long* vertices_14;
            const void* passes_18;
            int position_is_float3_1c;
            const srMatrix4T<float>* project_clip_near_20;
            unsigned long value_24;
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
            srVector2T<float> st_00;
            float q_08;
        };
        static_assert(sizeof(TexCoordQ) == 0xc, "TexCoordQ_must_be_0xc");

        /* intern() (0x10024280) hashes and compares the first three words;
           the interned record additionally carries a class derived from the
           shader's DSTBLEND field (ZERO -> 0, SRC_ALPHA pair -> 1, ONE -> 2,
           SRC_COLOR pair -> 3). */
        struct TextureSetKey {
            srTextureIFace* texture0_00;
            srTextureIFace* texture1_04;
            srShader shader_08;
        };
        struct TextureSet {
            srTextureIFace* texture0_00;
            srTextureIFace* texture1_04;
            srShader shader_08;
            unsigned long blend_0c;
        };
        /* +0x44: texture-set interning cache. The map's value is the index
           into sets_04_; reset() runs the map's Clear() and empties the
           record array. */
        struct TextureSetCache {
            srHashTable<TextureSetKey, unsigned long>* map_00;
            srArray<TextureSet> sets_04;
            unsigned long count_0c;

            /* Retail's constructor emission allocates the map after the
               record array and count are zeroed. */
            TextureSetCache() : count_0c(0)
            {
                map_00 = new srHashTable<TextureSetKey, unsigned long>;
            }
            /* ~Renderer inlines this sequence as map->Clear(),
               sets_04.release(), count_0c = 0, delete map_00 followed by the
               memberwise ~sets_04. */
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
                sets_04.release();
                count_0c = 0;
            }
            unsigned long intern(const TextureSetKey& key);
        };
        /* Write pointers alloc() (0x10024460) returns for the reserved
           triangle range. */
        struct IndexWrite {
            srVector3i* triangles_00;
            unsigned long* texture_set_04;
            unsigned long* sort_key_08;
            unsigned long* aux_0c;
        };
        /* +0x54: accumulated primitive work. alloc() reserves count entries
           plus 0x40 headroom across all four streams and returns the write
           pointers; reset() (0x10024620) always clears the count and only
           frees when asked. */
        struct IndexBatch {
            srHeapArray<srVector3i> triangles_00;
            srArray<unsigned long> texture_set_08;
            srArray<unsigned long> sort_key_10;
            srArray<unsigned long> aux_18;
            unsigned long count_20;

            /* Retail's constructor emission calls the reserve form with 0
               on the three operator-new arrays. */
            IndexBatch() : texture_set_08(0), sort_key_10(0), aux_18(0), count_20(0) {}
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
            srHeapArray<srVector4T<float> > diffuse_00;
            srHeapArray<srVector4T<float> > specular_08;
            srHeapArray<srVector4T<float> > positions_10;
            srHeapArray<srVector2T<float> > st_18[2];
            srArray<float> q_28[2];
            srArray<unsigned char> packed_38;
            /* isBatchFull compares this signed against batch_limit_dc_. */
            long count_40;
            unsigned long capacity_44;

            /* Retail's constructor emission calls the reserve form with 0
               on the vec4 streams and the packed byte array. */
            VertexArrays()
                : diffuse_00(0), specular_08(0), positions_10(0), packed_38(0), count_40(0),
                  capacity_44(0)
            {
            }
            void alloc(srVertexArray& arrays, unsigned long count);
            void bind(srVertexArray& arrays, unsigned long base);
        };

        Renderer(const Parameters& parameters);
        void allocVertexArray(srVertexArray& arrays, unsigned long count);
        /* FUN_10024E30: intern the pass's {texture0,texture1,shader} key into
           texture_set, folding per-vertex texture/shader table transitions
           into the output ids. */
        void assignTextureSets(unsigned long* texture_set, const unsigned long* indices,
                               unsigned long count, const srTriMeshPipeline::Pass* pass);
        /* Retail 0x10024DE0: while a vertex range is reserved
           (first_vertex_c0_ != -1), give count accumulated vertices back. */
        void rewindVertexArray(unsigned long count);
        /* FUN_10025260: expands/dedups the input triangles into the index and
           vertex batches, per record. */
        void expandTriangles(const TriInput& input, int sorted);
        /* FUN_100259D0: transforms the reserved position range by the input's
           matrix in 0x80-vertex chunks (choosing ortho/perspective/generic by
           the matrix's zero pattern), derives per-vertex clip flags, and
           replicates the chunk across the remaining records. */
        void transformVertices(const TriInput& input, unsigned char* clip_flags);
        void render(const TriInput& input);
        /* FUN_10024db0: the accumulated batch count passed the limit. Only
           immediate (non-sorted) renderers report full. */
        int isBatchFull() const;
        /* FUN_100266e0: submit the accumulated batch through the DD. */
        void submit();
        /* FUN_10025d50/0x10025F40: the immediate (sorted_d8_ == 0) and
           sorted draw paths over the accumulated index batch. */
        void drawImmediate();
        void drawSorted();
        /* FUN_10027ed0: point the draw state at texture set `index`,
           updating each of texture0/texture1/shader only on change. */
        void bindTextureSet(unsigned long index);
        /* FUN_10026360: repack the per-stage stq scratch streams and program
           the DD vertex arrays for the bound batch. */
        void programVertexArrays(srVertexArray* arrays, unsigned long count);
        /* FUN_100268a0: discard accumulated state; nonzero also releases
           the backing arrays. */
        void reset(int release_buffers);
        /* resetStatistics zeroes the +0x28 stat block; getStatistics copies
           its seven counters out for srGERD::getStatistics. */
        void resetStatistics();
        void getStatistics(unsigned long* statistics);

        /* The checked-free srHeapBuffer family, not srHeapArray: ~Renderer
           null-checks before freeing these streams. bytes_00_ grows by 1-byte
           elements, dwords_08_ and remap_10_ by 4-byte elements (the ensure
           emissions at 0x100271D0/0x10027280 multiply by the element size). */
        srHeapBuffer<unsigned char> bytes_00_;
        srHeapBuffer<unsigned long> dwords_08_;
        /* render()'s per-corner dedup scratch (six slots per triangle). */
        srHeapBuffer<unsigned long> remap_10_;
        srHeapBuffer<TexCoordQ> stq_18_[2];
        /* memset for 0x1c bytes in the ctor; submit() (0x100266E0) bumps
           [4] per call and accumulates the vertex count into [5] and the
           index-batch count into [6]. */
        unsigned long statistics_28_[7];
        TextureSetCache texture_sets_44_;
        IndexBatch indices_54_;
        VertexArrays vertices_78_;
        /* allocVertexArray() snapshots the vertex count here so render() can
           offset indices into the reserved range. */
        long first_vertex_c0_;
        /* Bound draw state, refreshed per texture set in the immediate and
           sorted paths. shader_cc_ re-defaults in the ctor body. */
        srTextureIFace* texture0_c4_;
        srTextureIFace* texture1_c8_;
        srShader shader_cc_;
        unsigned long clip_state_d0_;
        srGERD* gerd_d4_;
        /* lockRenderer matches this against the sorted-mode enable bit;
           flushSort flushes entries where it is 1, flushImmediateRenderers
           where it is 0. */
        long sorted_d8_;
        long batch_limit_dc_;
        unsigned long texture_stages_e0_;
    };
    static_assert(sizeof(Renderer) == 0xe4, "srGERD_Renderer_must_be_0xe4");

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
    enum e_buffer {};
    /* dump prints none (blit)/one (double)/two (triple) for values 1/2/3. */
    enum e_backBuffer { BACKBUFFER_NONE = 1, BACKBUFFER_ONE = 2, BACKBUFFER_TWO = 3 };
    enum e_polygonMode {};
    /* Wizardry uses 0 immediately before model-view loads and 1 immediately
       before identity+ortho. OpenGL srDD talks GL_MODELVIEW (0x1700) and
       GL_PROJECTION (0x1701) for those two stacks. */
    enum e_matrixMode { MATRIX_MODELVIEW = 0, MATRIX_PROJECTION = 1 };
    enum e_antiAlias { ANTIALIAS_NONE = 0 };
    /* srClipPlane::process passes its clip_type_ through unchanged; Wizardry
       always writes 0. */
    enum e_clipMode { CLIPMODE_POSITIONAL_0 = 0 };
    /* OpenGL: 0 disables GL_CULL_FACE, 1 enables + GL_BACK, 2 enables + GL_FRONT.
       DirectX7: D3DCULL_NONE / D3DCULL_CCW / D3DCULL_CW. */
    enum e_cullMode { CULL_NONE = 0, CULL_BACK = 1, CULL_FRONT = 2 };
    /* toggle XORs 1<<option into +0x20. Option 0 also dirties dirty_24 bit 0
       (Wizardry render-option 5). Option 1 is the particle path. Option 4 is
       SetRendererOption4Enabled. Option 5 wraps/unwraps srDebugDD. GERD dump
       has no enable-name table. */
    enum e_enable { ENABLE_POSITIONAL_0 = 0, ENABLE_POSITIONAL_1 = 1, ENABLE_POSITIONAL_4 = 4 };
    enum e_winding { WINDING_POSITIONAL_0 = 0, WINDING_POSITIONAL_1 = 1 };
    enum e_visibility { VISIBILITY_POSITIONAL_0 = 0 };
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
        srPixelConvert::PixelFormat pixel_format_00;
        unsigned long width_14;
        unsigned long height_18;
        unsigned long last_level_1c;
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
    srGERD(const srGERD& other);
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
        long window_width_00;
        long window_height_04;
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
        double elapsed_00;
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
        unsigned long frames_2c;
        unsigned long value_30;
        unsigned long value_34;
        /* getStatistics accumulates this only for sorted renderers. */
        unsigned long value_38;
        unsigned long value_3c;
        /* applyViewStateChanges increments this counter on every apply. */
        unsigned long view_state_applies_40;
        /* applyDrawStateChanges increments this counter on every apply. */
        unsigned long draw_state_applies_44;
        /* applyFrameStateChanges increments this counter on every apply. */
        unsigned long frame_state_count_48;
        /* Texture binds counted by changeTexture after the stage's bound
           texture actually changes; the debug overlay prints it as "TC". */
        unsigned long texture_binds_4c;
        /* Texture-parameter updates counted by setTextureParameters. */
        unsigned long texture_parameter_sets_50;
        /* createNewTexture increments this created-texture count. */
        unsigned long textures_created_54;
        /* Palette binds counted when a changed texture carries a new palette. */
        unsigned long palette_binds_58;
        /* setShader calls counted by applyDrawStateChanges. */
        unsigned long shader_sets_5c;
        /* drawArrays/drawElements increment this draw-call count. */
        unsigned long draw_calls_60;
        unsigned long value_64;
        unsigned long value_68;
        /* testBoundingSphere call count / visible-result count. */
        unsigned long sphere_tests_6c;
        unsigned long sphere_visible_70;
        /* testBoundingBox call count / visible-result count. */
        unsigned long box_tests_74;
        unsigned long box_visible_78;
        /* classifyMatrix call count. */
        unsigned long matrix_classifications_7c;
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
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    long getMaxTextureStages() const
    {
        return info_50_.max_texture_stages_28_;
    }
// FUNCTION: SURRENDER 0x1001CF10 SYMBOL
// ?getMaxPickStackDepth@srGERD@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    long getMaxPickStackDepth() const
    {
        return 0x20;
    }
// FUNCTION: SURRENDER 0x1001CF20 SYMBOL
// ?getMaxModelviewStackDepth@srGERD@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    long getMaxModelviewStackDepth() const
    {
        return 0x20;
    }
// FUNCTION: SURRENDER 0x1001CF30 SYMBOL
// ?getMaxProjectionStackDepth@srGERD@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
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
    /* info_50_.text_3c_[0..8] accessors: initDDInfo seeds the nine 0x40-byte
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
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int isTextureCached(srTextureIFace* texture) const;
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
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
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int isPickStackEmpty() const
    {
        return pick_depth_19ec_ == 0;
    }

// FUNCTION: SURRENDER 0x1001BAE0 SYMBOL
// ?isEnabled@srGERD@@QBEHW4e_enable@1@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int isEnabled(e_enable option) const
    {
        return (enable_flags_20_.value & (1UL << option)) != 0;
    }

// FUNCTION: SURRENDER 0x1001BB90 SYMBOL
// ?setCullMode@srGERD@@QAEXW4e_cullMode@1@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setCullMode(e_cullMode mode)
    {
        if (cull_mode_1648_ != mode) {
            cull_mode_1648_ = mode;
            dirty_24_ |= 0x2000;
        }
    }

/* Header inline that also emits the standalone retail 0x1001BB40 copy;
       drawSorted calls the emission while drawImmediate inlines it. */
// FUNCTION: SURRENDER 0x1001BB40 SYMBOL
// ?setShader@srGERD@@QAEXABVsrShader@@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setShader(const srShader& shader)
    {
        if (shader_1ff8_.value != shader.value) {
            shader_1ff8_ = shader;
            dirty_24_ |= 0x1000;
        }
    }

// FUNCTION: SURRENDER 0x1001BEF0 SYMBOL
// ?setVertexArrayMask@srGERD@@QAEXV?$srFlags@W4e_vertexArray@srRendererDefs@@@@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask)
    {
        vertex_arrays_21c4_.mask_00 = mask;
        dirty_21c0_ |= 1;
    }

// FUNCTION: SURRENDER 0x1001BEE0 SYMBOL
// ?getVertexArrayMask@srGERD@@QBE?AV?$srFlags@W4e_vertexArray@srRendererDefs@@@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srFlags<srRendererDefs::e_vertexArray> getVertexArrayMask() const
    {
        return vertex_arrays_21c4_.mask_00;
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
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setTexCoordPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                            const void* values, unsigned long layer)
    {
        unsigned long index = layer + 4;
        vertex_arrays_21c4_.components_0c[index] = components;
        vertex_arrays_21c4_.types_24[index] = type;
        vertex_arrays_21c4_.strides_3c[index] = stride;
        vertex_arrays_21c4_.arrays_54[index] = values;
        dirty_21c0_ |= 1;
    }

// FUNCTION: SURRENDER 0x1001BE90 SYMBOL
// ?setVertexPointer@srGERD@@QAEXJW4e_type@srRendererDefs@@KPBXJ@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setVertexPointer(long primitive, srRendererDefs::e_type type, unsigned long stride,
                          const void* values, long count)
    {
        vertex_arrays_21c4_.count_04 = count < 0 ? 0 : count;
        vertex_arrays_21c4_.components_0c[0] = primitive;
        vertex_arrays_21c4_.types_24[0] = type;
        vertex_arrays_21c4_.strides_3c[0] = stride;
        vertex_arrays_21c4_.arrays_54[0] = values;
        dirty_21c0_ |= 1;
    }

private:
    /* Pooled device-texture record, 0xa8 bytes. allocTexture links chunks
       through +0x00, keeps live/deleted lists in {prev_00, next_04} and the
       texture-interface id at +0x08 as the hash key. The embedded
       srDD::Texture at +0x2c is handed to the device. */
    struct Texture {
        Texture* prev_00;
        Texture* next_04;
        unsigned long id_08;
        /* evaluateTexturePixelFormat copies the matched device format here. */
        srPixelConvert::PixelFormat pixel_format_0c;
        void* surface_data_20;
        srPalette* palette_24;
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
        const srVector3i* triangles_04;
        unsigned long triangle_count_08;
        const unsigned long* vertices_0c;
        const srVector4T<float>* positions_10;
        unsigned long vertex_count_14;
    };

    srGERD& operator=(const srGERD& other);
    /* Renderer::submit and LockSurface's pixel transfers reach getDD; VC6
       does not give nested classes enclosing-member access. */
    srDD* getDD() const;

    /* The members marked dllexport below are retail exports no recovered
       caller references; without the annotation the linker garbage-collects
       the emissions and the provider exports dangle. Retail mangles them
       private. */
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    static void debugWrite(const char* text);
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void fenceVertexArrays();
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void dumpTextureCache(std::ostream& stream);
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setDataPtr(srRendererDefs::e_vertexArray index, long components,
                    srRendererDefs::e_type type, unsigned long stride, const void* values);

    /* Renderer member functions reach GERD's array-state fields directly;
       VC6 extended enclosing-class access to nested members, clang-cl
       needs the explicit grant. */
    friend class Renderer;
    /* Retail 0x1001D630: for each queued Pick, w-normalize the batch's
       positions into pick_vertices_2230_ and run the edge-function
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
        RendererEntry* prev_00;
        RendererEntry* next_04;
        Renderer* renderer_08;
        long busy_0c;
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
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
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

    static srGERD* first;
    static srGERD* firstOpen;

    struct MatrixStack {
        srMatrix4T<float> stack_00[32];
        unsigned long depth_800;
    };

    unsigned char unknown_0c_[4];
    RendererEntry* renderers_10_;
    srCriticalSection* renderers_section_14_;
    srCriticalSection* state_section_18_;
    unsigned long owner_thread_1c_;
    srFlags<e_enable> enable_flags_20_;
    unsigned long dirty_24_;
    unsigned long state_flags_28_;
    e_error last_error_2c_;
    /* getPrev reads this list link; first is the global head. */
    srGERD* prev_30_;
    srGERD* next_34_;
    /* Open-GERD list links; closeWindow splices via prev->next_open_3c_ and
       next->prev_open_38_. */
    srGERD* prev_open_38_;
    srGERD* next_open_3c_;
    srDD* dd_40_;
    srDebugDD* debug_dd_44_;
    srDD* real_dd_48_;
    unsigned char unknown_4c_[4];
    /* Device info record handed to srDD::getInfo by initDDInfo; GERD reads
       the staging/clamp fields out of it. openWindowInternal clamps the
       requested back-buffer against its maximums. */
    srDD::Info info_50_;
    /* getDriverInfo target; getDDAPIVersion/getDriverID/getDriverName
       (pre-context) and getApiVersion read its trailing fields. */
    srDD::DriverInfo driver_info_2cc_;
    srPixelConvert::PixelFormat* texture_formats_360_;
    long texture_format_count_364_;
    unsigned long* display_modes_368_;
    long display_mode_count_36c_;
    /* setHint/getHint index this by e_hint. */
    e_hintMode hints_370_[1];
    unsigned long window_374_;
    /* openWindowInternal memsets then struct-copies the OpenInfo record
       verbatim: windowed dims, backbuffer dims, then the display-mode index.
       isFullScreen tests display_mode_10 against -1, and openWindow leaves
       it -1 for the windowed path. */
    OpenInfo open_info_378_;
    /* e_backBuffer result of srDD::openWindow; getBackBufferType reads it. */
    unsigned long back_buffer_type_38c_;
    /* Per-mode current matrices at 0x390; pushMatrix indexes by mode. */
    srMatrix4T<float> matrix_current_390_[2];
    /* Per-mode 32-deep matrix stacks; each block ends with its depth counter
       (0x410 and 0xC14, stride 0x804). */
    MatrixStack matrix_stacks_410_[2];
    /* Six eye-space frustum planes maintained by applyClipPlaneChanges. */
    srVector4T<float> frustum_planes_1418_[6];
    /* User clip planes pushed by pushClipPlane; mask bits 6..31 of
       clip_mask_1674_. */
    srVector4T<float> user_clip_planes_1478_[26];
    /* Depth range forwarded into srDD::ViewPort by applyViewStateChanges;
       initView resets it to [0.0, 1.0] and setDepthRange clamps it. */
    double depth_min_1618_;
    double depth_max_1620_;
    srDD::Scissor scissor_1628_;
    unsigned long view_left_1638_;
    unsigned long view_top_163c_;
    unsigned long view_right_1640_;
    unsigned long view_bottom_1644_;
    e_cullMode cull_mode_1648_;
    e_winding winding_164c_;
    e_matrixMode matrix_mode_1650_;
    unsigned char unknown_1654_[6];
    /* Per-user-plane e_clipMode bytes written by pushClipPlane. */
    unsigned char clip_modes_165a_[26];
    /* Plane-enable mask: bits 0..5 frustum, bits 6..31 user planes. */
    unsigned long clip_mask_1674_;
    /* Subset of clip_mask_1674_ carrying mode-1 user planes. */
    unsigned long clip_mode1_mask_1678_;
    long clip_plane_count_167c_;
    /* Bit 1: recalcScissor marks the scissor as the full view. */
    unsigned long scissor_flags_1680_;
    srMatrix4T<float> inverse_modelview_1684_;
    srMatrix4T<float> project_clip_near_16c4_;
    srMatrix4T<float> normal_matrix_1704_;
    srMatrix4T<float>::e_scaleType modelview_scale_type_1744_;
    float max_modelview_scale_1748_;
    /* classifyMatrix writes the per-mode projection-shape class here; the
       projection class at +0x1750 feeds srDD::setProjectionMatrix. */
    srMatrix4T<float>::e_type matrix_class_174c_[2];
    unsigned char unknown_1754_[4];
    srVector3T<float> gamma_1758_;
    unsigned long swap_interval_1764_;
    e_antiAlias antialias_1768_;
    Pick pick_stack_176c_[32];
    unsigned long pick_depth_19ec_; /* 0x19ec */
    unsigned long pick_key_19f0_;
    unsigned char unknown_19f4_[4];
    /* Snapshot getStatistics refreshes on every flipFrame; dump prints it. */
    Statistics statistics_19f8_;
    /* getDD counts each device access at value_68 (the overlay's "DD" row),
       so the live counters are writable from const. */
    mutable Statistics statistics_1a78_;
    /* accumAlloc sizes this width*height*8 accumulation pixel buffer plus a
       width*4 scratch block; accumClear fills it from clear_values_1b08_'s
       accum color over the current scissor. */
    AccumPixel* accum_buffer_1af8_;
    unsigned long* accum_scratch_1afc_;
    LockSurface* lock_surface_1b00_;
    /* Buffer-lock nesting depth; _lockBuffer only locks the device on the
       first entry and _unlockBuffer unlocks when this returns to zero. */
    long buffer_lock_count_1b04_;
    /* srDD::ClearValues record passed straight to setClearValues. */
    srDD::ClearValues clear_values_1b08_;
    unsigned char unknown_1b34_[4];
    /* Grayscale ramp built by initGlobalPalette and handed to
       srDD::setGlobalPalette; matchPalette compares it as srARGB. */
    srARGB global_palette_1b38_[0x100];
    /* Bound Texture per stage, swapped by changeTexture. */
    Texture* texture_slots_1f38_[2];
    /* Per-stage packed device parameters written by setTextureParameters. */
    srDD::TexParms texture_parms_1f40_[2];
    /* Device palette record handed to bindPalette/deletePalette. */
    srDD::Palette palette_1f50_;
    /* setTextureParameters indexes these maps from the packed texture state.
       Filter selector 4 is a valid index in both filter maps. */
    unsigned long correction_map_1f5c_[4];
    unsigned long mag_filter_map_1f6c_[5];
    unsigned long min_filter_map_1f80_[5];
    /* The fourth entry doubles as the current mipmap parameter written by
       setTextureDefaultMipmap. */
    unsigned long mipmap_map_1f94_[4];
    unsigned long wrap_s_map_1fa4_[2];
    unsigned long wrap_t_map_1fac_[2];
    srTextureIFace::e_correction default_correction_1fb4_;
    srTextureIFace::e_filter default_mag_filter_1fb8_;
    srTextureIFace::e_filter default_min_filter_1fbc_;
    srTextureIFace::e_mipmap default_mipmap_1fc0_;
    /* Per-type default device parameters; evaluateTexturePixelFormat copies
       entry [Dimensions::parameter_index] into srDD::Texture::parameter_34.
       Entry [4] doubles as the current compression parameter written by
       setTextureDefaultCompression. */
    unsigned long default_texture_params_1fc4_[5];
    /* Default Dimensions::compression for newly created textures;
       setTextureDefaultCompression indexes default_texture_params_1fc4_
       with it. */
    srTextureIFace::e_compression default_compression_1fd8_;
    /* Palette currently bound to the device. */
    srPalette* palette_1fdc_;
    /* srDD::e_polygonMode value; the empty enum cannot be a field type. */
    unsigned long polygon_mode_1fe0_;
    long polygon_offset_1fe4_;
    srVector4T<float> fog_color_1fe8_;
    srShader shader_1ff8_;
    /* Texture interfaces requested through setTexture for stages 0/1; the
       srPtr array is proven by ~srGERD's __ehvec_dtor over two releasing
       elements. */
    srPtr<srTextureIFace> texture_iface_1ffc_[2];
    /* Live textures keyed by the texture interface's frame handle. */
    srHashTable<unsigned long, Texture*> texture_lookup_2004_;
    unsigned long texture_count_2014_;
    Texture* texture_free_2018_;
    /* Chunk pointers backing the 0xa8-byte Texture pool. */
    srArray<Texture*> texture_pool_201c_;
    unsigned long texture_pool_count_2024_;
    Texture* texture_deleted_2028_;
    Texture* texture_head_202c_;
    Texture* texture_default_2030_;
    unsigned long texture_cache_used_2034_;
    unsigned long texture_cache_size_2038_;
    unsigned long texture_sequence_203c_;
    long texture_reduction_2040_;
    bool texture_hash_enabled_2044_;
    unsigned char unknown_2045_[3];
    srVector4T<float> ambient_light_2048_;
    srVector4T<float> environment_2058_;
    /* pushEnvironment/popEnvironment stack of {min, max, scale, inv_scale}. */
    srVector4T<float> environment_stack_2068_[16];
    unsigned long environment_depth_2168_;
    unsigned long enable_stack_216c_[16];
    unsigned long enable_depth_21ac_;
    srArray<srVertexProcessor*> vertex_processors_21b0_;
    unsigned long vertex_processor_count_21b8_;
    unsigned long exclusion_mask_21bc_;
    unsigned long dirty_21c0_;
    srRendererDefs::VertexArrayInfo vertex_arrays_21c4_;
    /* performPickTest's w-normalized {x,y,z,sign(w)} scratch per vertex;
       released by closeWindow. */
    srHeapBuffer<srVector4T<float> > pick_vertices_2230_;
};

/* Retail 0x10027BF0: the three-word texture-set key hash; the interning
   cache inlines it for the lookup probe and calls this emission when
   inserting. */
// FUNCTION: SURRENDER 0x10027BF0 SYMBOL
// ?srHashValue@@YAIABUTextureSetKey@Renderer@srGERD@@@Z
inline unsigned int srHashValue(const srGERD::Renderer::TextureSetKey& key)
{
    // reinterpret-ok: the hash mixes the stored interface addresses.
    return ((key.shader_08.value >> 10 ^ reinterpret_cast<unsigned long>(key.texture1_04)) >> 1 ^
            // reinterpret-ok: as above.
            reinterpret_cast<unsigned long>(key.texture0_00)) >>
               5 ^
           key.shader_08.value;
}

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
static_assert(sizeof(srGERD::ClipPlanes) == 0x208, "srGERD_ClipPlanes_must_be_0x208");
static_assert(sizeof(srGERD::Renderer::TriInput) == 0x28, "srGERD_Renderer_TriInput_must_be_0x28");
