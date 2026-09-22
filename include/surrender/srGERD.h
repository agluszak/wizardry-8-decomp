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
class srCriticalSection;
class srDebugDD;
class srModelInstance;
class srVertexProcessor;
struct srVertexArray;

class SR_DLL_IMPORT srGERD : public srRuntimeClass {
public:
    struct Pick {
        float value_00;
        float value_04;
        float value_08;
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

        void allocVertexArray(srVertexArray& arrays, unsigned long count);
        void render(const TriInput& input);
        /* Retail 0x100266E0: drains the renderer's queued work. */
        void flush();

    private:
        friend class srGERD;
        /* +0xd4 is the owning srGERD; +0xd8 nonzero marks a queued/deferred
           renderer that flushImmediateRenderers skips. */
        unsigned char unknown_00_[0xd4];
        srGERD* gerd_d4_;
        unsigned long deferred_d8_;
    };

    struct RendererEntry {
        unsigned long unknown_00;
        RendererEntry* next_04;
        Renderer* renderer_08;
        /* +0x0c: in-flight work count; flushImmediateRenderers yields until 0. */
        long work_count_0c;
    };

    enum e_error {};
    enum e_closeHint {};
    enum e_buffer {};
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

    static srGERD* loadDevice(srStringTable& devices, unsigned long flags);
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
    e_error openWindow();
    e_error openWindow(long mode);
    void closeWindow(e_closeHint hint);
    int isWindowOpen() const;
    unsigned long getWindowHandle() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void clear(const srFlags<e_buffer>& buffers);
    long getHeight() const;
    long getWidth() const;
    void resetStatistics();
    /* getStatistics buffer. The render probes return the double at +0x10
       through ftol; the 0x00427460 debug overlay prints the dword counters at
       +0x08/+0x0c (the TT pair), +0x20 (PO), +0x24 (VO), +0x34 (PI),
       +0x3c (VI), +0x4c (TC) and +0x68 (DD). */
    struct Statistics {
        unsigned char unknown_00[8];
        unsigned long value_08;
        unsigned long value_0c;
        double value_10;
        unsigned char unknown_18[8];
        unsigned long value_20;
        unsigned long value_24;
        unsigned char unknown_28[0xc];
        unsigned long value_34;
        unsigned char unknown_38[4];
        unsigned long value_3c;
        /* applyViewStateChanges increments this counter on every apply. */
        unsigned long value_40;
        unsigned long value_44;
        /* applyFrameStateChanges increments this counter on every apply. */
        unsigned long frame_state_count_48;
        unsigned long value_4c;
        /* Texture-parameter updates, palette binds and shader updates
           counted by setTextureParameters/changeTexture/applyDrawStateChanges. */
        unsigned long value_50;
        /* createNewTexture increments this created-texture count. */
        unsigned long value_54;
        unsigned long value_58;
        unsigned long value_5c;
        unsigned char unknown_60[8];
        unsigned long value_68;
        unsigned long value_6c;
        unsigned long value_70;
        unsigned long value_74;
        unsigned long value_78;
        unsigned long value_7c;
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
    void flipFrame();
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
    long getMaxTextureStages() const
    {
        return max_texture_stages_78_;
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
    void flushImmediateRenderers();
    void pushEnvironment();
    void popEnvironment();
    void setEnvironmentRange(float minimum, float maximum);
    void setEnvironmentScaleFactor(float scale, float inverse_scale);
    void setClipState(srFlags<srRendererDefs::e_clip> state);
    void setAntiAlias(e_antiAlias mode);
    void setTexture(srTextureIFace* texture, unsigned long layer);
    void setTextureDefaultMagFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMinFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap);
    void setTextureSubImage(srTextureIFace* texture, long mipmap, long x, long y, long width,
                            long height);
    void drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count);
    void popPick(Pick& pick);
    void pushPick(const Pick& pick);
    void toggle(e_enable option);
    void invalidateResidentTextures();
    void invalidateTextureCache();
    void invalidateTexture(srTextureIFace* texture);
    void invalidateTextureByFrameHandle(unsigned long handle);
    unsigned long getTextureCacheSize() const;
    void setTextureCacheSize(unsigned long bytes);
    void setSwapInterval(unsigned long interval);
    long getPolygonOffset() const;
    void setPolygonOffset(long offset);

    /* These ordinary methods are header-visible in Wiz8 call sites even
       though SR.DLL also exports out-of-line copies. */
    int isPickStackEmpty() const
    {
        return pick_depth_19ec_ == 0;
    }

    int isEnabled(e_enable option) const
    {
        return (enable_flags_20_.value & (1UL << option)) != 0;
    }

    void setCullMode(e_cullMode mode)
    {
        if (cull_mode_1648_ != mode) {
            cull_mode_1648_ = mode;
            dirty_24_ |= 0x2000;
        }
    }

    void setShader(const srShader& shader)
    {
        if (shader_1ff8_.value != shader.value) {
            shader_1ff8_ = shader;
            dirty_24_ |= 0x1000;
        }
    }

    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask)
    {
        vertex_array_mask_21c4_ = mask;
        dirty_21c0_ |= 1;
    }

    void setTexCoordPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                            const void* values, unsigned long layer)
    {
        unsigned long index = layer + 4;
        array_components_21d0_[index] = components;
        array_types_21e8_[index] = type;
        array_strides_2200_[index] = stride;
        arrays_2218_[index] = values;
        dirty_21c0_ |= 1;
    }

    void setVertexPointer(long primitive, srRendererDefs::e_type type, unsigned long stride,
                          const void* values, long count)
    {
        vertex_count_21c8_ = count < 0 ? 0 : count;
        array_components_21d0_[0] = primitive;
        array_types_21e8_[0] = type;
        array_strides_2200_[0] = stride;
        arrays_2218_[0] = values;
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

    srGERD& operator=(const srGERD& other);
    srDD* getDD() const;
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
    unsigned long getTextureBytesNeeded(const Texture& texture) const;
    Texture* findLowestPriority();
    void invalidateTexture(Texture& texture);
    void markTextureAsDeleted(Texture& texture);
    static void convertPixelFormat(srDD::PixelFormat& device,
                                   const srPixelConvert::PixelFormat& format);
    class LockSurface;

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
    unsigned char unknown_30_[4];
    srGERD* next_34_;
    unsigned char unknown_38_[4];
    srGERD* next_open_3c_;
    srDD* dd_40_;
    srDebugDD* debug_dd_44_;
    srDD* real_dd_48_;
    unsigned char unknown_4c_[0x1c];
    /* changeTexture tests bit 5 to release resident surface data after a
       texture-stage swap. */
    unsigned long flags_68_;
    unsigned char unknown_6c_[0xc];
    long max_texture_stages_78_;
    /* Texture-dimension clamps applied by evaluateTextureDimensions. */
    unsigned long texture_min_dim_7c_;
    unsigned long texture_max_dim_80_;
    unsigned long texture_max_aspect_84_;
    unsigned char unknown_88_[0x2d8];
    srPixelConvert::PixelFormat* texture_formats_360_;
    long texture_format_count_364_;
    unsigned long* display_modes_368_;
    long display_mode_count_36c_;
    unsigned char unknown_370_[4];
    unsigned long window_374_;
    unsigned char unknown_378_[8];
    long width_380_;
    long height_384_;
    unsigned char unknown_388_[8];
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
    /* Extra viewport parameters copied into srDD::ViewPort by
       applyViewStateChanges. */
    unsigned long viewport_extra_1618_[4];
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
    unsigned char unknown_19f4_[0x84];
    Statistics statistics_1a78_;
    unsigned char unknown_1af8_[8];
    LockSurface* lock_surface_1b00_;
    unsigned char unknown_1b04_[4];
    srVector4T<float> clear_color_1b08_;
    unsigned char unknown_1b18_[0x10];
    double clear_depth_1b28_;
    unsigned char unknown_1b30_[0x408];
    /* Bound Texture per stage, swapped by changeTexture. */
    Texture* texture_slots_1f38_[2];
    /* Per-stage packed device parameters written by setTextureParameters. */
    srDD::TexParms texture_parms_1f40_[2];
    /* Device palette record handed to bindPalette/deletePalette. */
    srDD::Palette palette_1f50_;
    /* setTextureParameters packs the stage parameters through these filter
       tables, indexed by bits of the texture's packed state. */
    unsigned long wrap_map_1f5c_[4];
    unsigned long mag_filter_map_1f6c_[4];
    unsigned long mag_filter_param_1f7c_;
    unsigned long min_filter_map_1f80_[4];
    unsigned long min_filter_param_1f90_;
    /* The fourth entry doubles as the current mipmap parameter written by
       setTextureDefaultMipmap. */
    unsigned long mipmap_map_1f94_[4];
    unsigned long correction_map_1fa4_[2];
    unsigned long detail_map_1fac_[2];
    unsigned char unknown_1fb4_[4];
    srTextureIFace::e_filter default_mag_filter_1fb8_;
    srTextureIFace::e_filter default_min_filter_1fbc_;
    srTextureIFace::e_mipmap default_mipmap_1fc0_;
    /* Per-type default device parameters; evaluateTexturePixelFormat copies
       entry [Dimensions::parameter_index] into srDD::Texture::parameter_34. */
    unsigned long default_texture_params_1fc4_[5];
    /* Default Dimensions::parameter_index for newly created textures. */
    unsigned long default_parameter_index_1fd8_;
    /* Palette currently bound to the device. */
    srPalette* palette_1fdc_;
    /* srDD::e_polygonMode value; the empty enum cannot be a field type. */
    unsigned long polygon_mode_1fe0_;
    long polygon_offset_1fe4_;
    srVector4T<float> fog_color_1fe8_;
    srShader shader_1ff8_;
    /* Texture interfaces requested through setTexture for stages 0/1. */
    srTextureIFace* texture_iface_1ffc_[2];
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
    srFlags<srRendererDefs::e_vertexArray> vertex_array_mask_21c4_;
    unsigned long vertex_count_21c8_;
    srFlags<srRendererDefs::e_clip> clip_state_21cc_;
    long array_components_21d0_[6];
    srRendererDefs::e_type array_types_21e8_[6];
    unsigned long array_strides_2200_[6];
    const void* arrays_2218_[6];
    unsigned char unknown_2230_[8];
};

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
static_assert(sizeof(srGERD::ClipPlanes) == 0x208, "srGERD_ClipPlanes_must_be_0x208");
static_assert(sizeof(srGERD::Renderer::TriInput) == 0x28, "srGERD_Renderer_TriInput_must_be_0x28");
